;;;; bench/scout/engines/april.lisp - April (APL compiled to Common Lisp) side of the scout matrix.
;;;;
;;;; Run:  april-sbcl bench/scout/engines/april.lisp <op> <N> <runs> <warmup>
;;;;       where april-sbcl = sbcl --core <image with quicklisp + april loaded> --script
;;;; Protocol and data model: bench/scout/SCOUT_SPEC.md
;;;;
;;;; April compiles APL (Dyalog dialect) into Lisp at macro-expansion time, so each
;;;; kernel below is compiled once, when this file is loaded; the timed calls run
;;;; compiled code.  Same idioms as the Dyalog adapter: {⍵[⍋⍵]} sort, dyadic ⍳ / ∊,
;;;; ∪, +.× dot, the key operator ⌸ for group-by, n-wise reduction for the moving
;;;; windows (padded with the identity so the first windows are partial).
;;;; ⎕IO is 0.  Core tier only (no table layer).
;;;;
;;;; Single thread: April renders arrays on an lparallel kernel sized from the CPU
;;;; count (varray::*workers-count*); both are pinned to 1 worker here.
;;;;
;;;; Scale (observed, April 1.0.0 @0001af6): dyadic ⍳ and ∊, monadic ∪ and ⌸ grow
;;;; as O(n*k) here (join_inner, 1M probes into 1000 keys, takes ~36 s), and the
;;;; masked sum (X>50)/... takes ~9 s at N = 100k.  Every op validates, but
;;;; arith_mask, find, member, distinct_100k, group_10k, group_100k and mavg_256
;;;; cannot finish 2+5 passes at N = 10M inside the runner's 900 s timeout.

(setf *read-default-float-format* 'double-float)
(setf varray::*workers-count* 1)
(when lparallel:*kernel* (lparallel:end-kernel :wait t))
(setf lparallel:*kernel* (lparallel:make-kernel 1 :name "april-language-kernel"))

(defparameter *args* (last sb-ext:*posix-argv* 4))
(defparameter *op* (first *args*))
(defparameter *n* (parse-integer (second *args*)))
(defparameter *runs* (parse-integer (third *args*)))
(defparameter *warm* (parse-integer (fourth *args*)))

(defmacro apl (code) `(april:april (with (:state :index-origin 0)) ,code))

;; ---- base data (outside the timed region)
(april:april (with (:state :index-origin 0 :in ((n *n*))))
  "N←n ⋄ H←1048573|262147×⍳N ⋄ A←1000|H ⋄ B←997|H ⋄ X←A ⋄ Y←B ⋄ CHK←(+/A)+3×+/B ⋄ 0")
(apl "KR←1048573|7919×⍳1000 ⋄ VR←2×⍳1000 ⋄ MJ←1000000 ⋄ 0")
(apl "SFA←{m←≢⍵ ⋄ (+/⍵[0,(⌊m÷4),(⌊m÷2),(⌊3×m÷4),m-1])+1E9×+/(1↓⍵)<¯1↓⍵} ⋄ 0")
(apl "DSUM←{(1E6×≢⍵)++/⍵} ⋄ 0")
(apl "GRP←{r←⍺{⍺,+/⍵}⌸⍵ ⋄ +/(1+251|r[;0])×r[;1]} ⋄ 0")

(defun op= (s) (string= *op* s))

;; ---- per-op setup + kernel
(defparameter *kern*
  (cond ((op= "sum_f")          (lambda () (apl "+/X")))
        ((op= "max_f")          (lambda () (apl "⌈/X")))
        ((op= "scan_f")         (lambda () (apl "S←+\\X ⋄ S[0]+S[⌊N÷2]+S[N-1]")))
        ((op= "dot")            (lambda () (apl "X+.×Y")))
        ((op= "sum_i")          (lambda () (apl "+/A")))
        ((op= "arith_mask")     (lambda () (apl "+/(X>50)/Y+2.5×X")))
        ((op= "sort_f")         (lambda () (apl "SFA X[⍋X]")))
        ((op= "sort_presorted") (apl "P←X[⍋X] ⋄ 0")
                                (lambda () (apl "SFA P[⍋P]")))
        ((op= "grade_i")        (apl "GK←1000⌊N ⋄ 0")
                                (lambda () (apl "+/GK↑⍋A")))
        ((op= "find")           (apl "PR←KR[1000|H] ⋄ 0")
                                (lambda () (apl "+/KR⍳PR")))
        ((op= "member")         (lambda () (apl "+/H∊KR")))
        ((op= "distinct")       (lambda () (apl "DSUM ∪A")))
        ((op= "distinct_100k")  (apl "G5←100000|H ⋄ 0")
                                (lambda () (apl "DSUM ∪G5")))
        ((op= "group_10")       (apl "G←10|H ⋄ 0")     (lambda () (apl "G GRP X")))
        ((op= "group_100")      (apl "G←100|H ⋄ 0")    (lambda () (apl "G GRP X")))
        ((op= "group_10k")      (apl "G←10000|H ⋄ 0")  (lambda () (apl "G GRP X")))
        ((op= "group_100k")     (apl "G←100000|H ⋄ 0") (lambda () (apl "G GRP X")))
        ((op= "join_inner")     (apl "HJ←1048573|262147×⍳MJ ⋄ KL←KR[1000|HJ] ⋄ VL←1000|HJ ⋄ 0")
                                (lambda () (apl "+/VL×VR[KR⍳KL]")))
        ((op= "msum_16")        (lambda () (apl "+/16+/(15⍴0),X")))
        ((op= "mavg_256")       (lambda () (apl "+/(256+/(255⍴0),X)÷256⌊1+⍳N")))
        ((op= "mmax_64")        (lambda () (apl "+/64⌈/(63⍴⌈/⍬),X")))
        (t nil)))

;; ---- warm-up, timed runs, median
;; get-internal-real-time is the coarse (jiffy, ~4 ms) clock here, so read
;; CLOCK_MONOTONIC directly.
(defun now-ms ()
  (multiple-value-bind (s ns) (sb-unix:clock-gettime sb-unix:clock-monotonic)
    (+ (* 1000.0d0 s) (/ ns 1.0d6))))

(if (null *kern*)
    (format t "SKIP ~a~%" *op*)
    (let ((ans 0) (ts nil))
      (dotimes (i *warm*) (setf ans (funcall *kern*)))
      (dotimes (i *runs*)
        (let ((t0 (now-ms)))
          (setf ans (funcall *kern*))
          (push (- (now-ms) t0) ts)))
      (setf ts (sort ts #'<))
      (format t "BENCH   ~a~%" *op*)
      (format t "CHECK   ~a~%" (apl "CHK"))
      (format t "ANSWER  ~a~%" (prin1-to-string (coerce ans 'double-float)))
      (format t "TIME_MS ~,6f~%" (nth (floor *runs* 2) ts))))
(finish-output)
