NB. bench/scout/engines/j.ijs - J side of the scout matrix (core tier).
NB. Run:  jconsole bench/scout/engines/j.ijs <op> <N> <runs> <warmup>
NB. Protocol and data model: bench/scout/SCOUT_SPEC.md
NB.
NB. Core tier: no table layer, so the table ops report SKIP.  mmax_64 has no
NB. O(n) J formulation and is left out rather than timed against native
NB. monotonic-deque kernels at a different complexity class.
NB. Globals are UPPERCASE: J binds x and y as a verb's implicit arguments.
NB. Control words (if. select. for.) are only legal inside an explicit
NB. definition, so the dispatch and the timing loop live in verbs.

(9!:11) 17                          NB. full round-trip float printing

args =: 2 }. ARGV
OP   =: > 0 { args
N    =: ". > 1 { args
RUNS =: ". > 2 { args
WARM =: ". > 3 { args

I   =: i. N
H   =: 1048573 | 262147 * I
A   =: 1000 | H
B   =: 997 | H
X   =: 0.0 + A
Y   =: 0.0 + B
CHK =: (+/ A) + 3 * +/ B

sfa =: 3 : 0
 s =. y
 n =. # s
 o =. (0{s) + ((<. n%4){s) + ((<. n%2){s) + ((<. 3*n%4){s) + ({: s)
 o + 1e9 * +/ (}. s) < (}: s)
)

grp =: 4 : 0
 k =. ~. x
 +/ (1 + 251 | k) * x +//. y
)

msumw =: 4 : 0
 s =. +/\ y
 s - (x $ 0.0) , (- x) }. s
)

mavgw =: 4 : 0
 (x msumw y) % x <. >: i. # y
)

KR =: 1048573 | 7919 * i. 1000
VR =: 2.0 * i. 1000
MJ =: 1000000

setup =: 3 : 0
 F =: 0
 if.     OP -: 'sum_f'          do. F =: 3 : '+/ X'
 elseif. OP -: 'max_f'          do. F =: 3 : '>./ X'
 elseif. OP -: 'dot'            do. F =: 3 : '+/ X * Y'
 elseif. OP -: 'sum_i'          do. F =: 3 : '0.0 + +/ A'
 elseif. OP -: 'arith_mask'     do. F =: 3 : '+/ (X > 50) # (Y + 2.5 * X)'
 elseif. OP -: 'sort_f'         do. F =: 3 : 'sfa /:~ X'
 elseif. OP -: 'sort_presorted' do. P =: /:~ X
                                    F =: 3 : 'sfa /:~ P'
 elseif. OP -: 'grade_i'        do. GK =: 1000 <. N
                                    F =: 3 : '0.0 + +/ GK {. /: A'
 elseif. OP -: 'find'           do. PR =: KR {~ 1000 | H
                                    F =: 3 : '0.0 + +/ KR i. PR'
 elseif. OP -: 'member'         do. F =: 3 : '0.0 + +/ H e. KR'
 elseif. OP -: 'distinct'       do. F =: 3 : '(1e6 * # ~. A) + +/ ~. A'
 elseif. OP -: 'distinct_100k'  do. G5 =: 100000 | H
                                    F =: 3 : '(1e6 * # ~. G5) + +/ ~. G5'
 elseif. OP -: 'group_10'       do. G =: 10 | H
                                    F =: 3 : 'G grp X'
 elseif. OP -: 'group_100'      do. G =: 100 | H
                                    F =: 3 : 'G grp X'
 elseif. OP -: 'group_10k'      do. G =: 10000 | H
                                    F =: 3 : 'G grp X'
 elseif. OP -: 'group_100k'     do. G =: 100000 | H
                                    F =: 3 : 'G grp X'
 elseif. OP -: 'join_inner'     do. HJ =: 1048573 | 262147 * i. MJ
                                    KL =: KR {~ 1000 | HJ
                                    VL =: 0.0 + 1000 | HJ
                                    F  =: 3 : '+/ VL * VR {~ KR i. KL'
 elseif. OP -: 'msum_16'        do. F =: 3 : '+/ 16 msumw X'
 elseif. OP -: 'mavg_256'       do. F =: 3 : '+/ 256 mavgw X'
 end.
 0
)

main =: 3 : 0
 setup 0
 NB. `-:` needs nouns, and a matched branch has rebound F to a VERB, so ask
 NB. the name class instead: 4!:0 gives 3 for a verb, 0 for a noun.
 if. 3 ~: 4!:0 < 'F' do.
   echo 'SKIP ' , OP
   return.
 end.
 ANS =: 0.0
 if. WARM > 0 do.
   for_w. i. WARM do. ANS =: F 0 end.
 end.
 TS =: 0 $ 0.0
 for_r. i. RUNS do.
   t0 =. 6!:1 ''
   ANS =: F 0
   TS =: TS , 1000 * (6!:1 '') - t0
 end.
 echo 'BENCH   ' , OP
 echo 'CHECK   ' , ": CHK
 echo 'ANSWER  ' , ": 0.0 + ANS
 echo 'TIME_MS ' , ": (<. RUNS % 2) { /:~ TS
 0
)

main 0
exit ''
