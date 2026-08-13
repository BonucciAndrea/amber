/ ============================================================================
/ std.k  -  Amber standard extensions.  Loaded after fin.k.
/   * vectorised moving / window aggregates   (msum mavg mdev … O(n) prefix)
/   * parse / eval / serialize                 (parse eval reval ser deser)
/   * casting helpers                          (cast + typed constructors)
/   * peach                                    (parallel-each; see note)
/   * ts                                       (time an expression; \ts in the REPL)
/   * a little linear algebra                  (mmu dot)
/ ============================================================================

/ ---- moving / window aggregates -------------------------------------------
/ kdb semantics: a growing window for the first w-1 points, then a fixed w.
/ .
/ All of msum/mavg/mvar/mdev/mmin/mmax now run as SINGLE-PASS O(n) C kernels
/ (`mw, src/v.c).  msum/mavg/mvar/mdev carry a running difference
/     new_sum = old_sum + incoming - outgoing
/ so they touch each element twice and materialise nothing; mmin/mmax use a
/ monotonic index deque, which makes them O(n) *independent of the window* --
/ the K definitions below were O(n*w) AND built one index vector plus one slice
/ per element.  Nulls (0n) are treated as absent rather than poisoning every
/ later element, which is what kdb does; on null-free input the results are
/ identical to the K definitions.
/ .
/ Each wrapper keeps its original K definition as mXXXK and falls back to it
/ whenever the kernel declines the input (unsupported type, w<1, empty vector).
/ `mw signals that by returning the generic empty list (), hence the `A type
/ test -- every value the kernel can actually produce is a flat F/I vector.
mcount:{[w;x]w&1+!#x}                                    / window element count
msumK:{[w;x]s:sums 0.0+x;s-(w#0.0),(-w)_s}               / moving sum   (prefix diff)
mavgK:{[w;x]msumK[w;x]%mcount[w;x]}                      / moving average
mvarK:{[w;x]m:mavgK[w;x];(mavgK[w;x*x])-m*m}             / moving variance (population)
mminK:{[w;x]{[w;x;j]&/x@(0|1+j-w)+!(1+j)&w}[w;x]'!#x}    / moving min  (window)
mmaxK:{[w;x]{[w;x;j]|/x@(0|1+j-w)+!(1+j)&w}[w;x]'!#x}    / moving max  (window)
msum:{[w;x]$[`A~@r:`mw(0;w;x);msumK[w;x];r]}             / O(n) running difference
mavg:{[w;x]$[`A~@r:`mw(1;w;x);mavgK[w;x];r]}
mvar:{[w;x]$[`A~@r:`mw(2;w;x);mvarK[w;x];r]}
mdev:{[w;x]$[`A~@r:`mw(3;w;x);sqrt mvarK[w;x];r]}        / moving standard deviation
mmin:{[w;x]$[`A~@r:`mw(4;w;x);mminK[w;x];r]}             / O(n) monotonic deque
mmax:{[w;x]$[`A~@r:`mw(5;w;x);mmaxK[w;x];r]}
mprd:{[w;x]exp msum[w;log 0.0+x]}                        / moving product
/ moving weighted / correlation helpers
dot:{+/x*y}                                              / vector dot product
mmu:{[x;y]yc:+y;{[yc;r]{[r;c]+/r*c}[r]'yc}[yc]'x}        / matrix multiply  x mmu y

/ ---- parse / eval / serialize ---------------------------------------------
parse:{`p x}                                             / string -> parse tree (for inspection)
eval:{. x}                                               / eval a string  ->  value
reval:{. x}                                              / eval a string, no protection (alias)
ser:{`k x}                                               / serialize a value -> portable text
deser:{. x}                                              / text -> value  (inverse of ser)
protect:{.[{(1b;. x)};,x;{(0b;x)}]}                      / (ok;result) | (0b;error) — like .Q.trp

/ ---- casting --------------------------------------------------------------
/ q's $ already casts:  `i$x `f$x `c$x `$x  ; these are named wrappers.
long:{`i$x}
int:{`i$x}
float:{`f$x}
char:{`c$x}
sym:{`$x}
bool:{0<x}
cast:{[t;x]$[t~`long;`i$x;t~`int;`i$x;t~`float;`f$x;t~`char;`c$x;t~`sym;`$x;t~`bool;0<x;`$$[`C=@x;x;$x]]}

/ ---- peach : parallel each ------------------------------------------------
/ peach[f;y] applies f to each item of y across a PERSISTENT pool of POSIX
/ worker threads (src/peachpool.c), sized by $AMBER_THREADS (default: online
/ CPU count). It is transparent: the result is identical to serial f'y for
/ every datatype, list and table. Native builds run it truly in parallel;
/ the wasm build has no threads and evaluates sequentially. Also exposed as
/ .Q.fc (parallel-on-cut) in sys.k.
peach:{`pe(x;y)}                                         / parallel each: thread-pool workers (C, src/i.c peachC)

/ ---- timing ---------------------------------------------------------------
/ ts"expr"  ->  evaluates expr, prints elapsed ms, returns the result.
/ The REPL also accepts  \ts expr  (see repl.k).
ts:{[e]t0:`t[];r:. e;`0:"ts: ",($0.001*`t[]-t0)," ms";r}
