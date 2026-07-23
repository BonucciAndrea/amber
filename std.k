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
/ msum/mavg/mvar/mdev/mcount are O(n) (prefix sums); mmin/mmax are O(n*w).
mcount:{[w;x]w&1+!#x}                                    / window element count
msum:{[w;x]s:sums 0.0+x;s-(w#0.0),(-w)_s}               / moving sum   (prefix diff)
mavg:{[w;x]msum[w;x]%mcount[w;x]}                        / moving average
mprd:{[w;x]exp msum[w;log 0.0+x]}                        / moving product
mvar:{[w;x]m:mavg[w;x];(mavg[w;x*x])-m*m}               / moving variance (population)
mdev:{[w;x]sqrt mvar[w;x]}                               / moving standard deviation
mmin:{[w;x]{[w;x;j]&/x@(0|1+j-w)+!(1+j)&w}[w;x]'!#x}     / moving min  (window)
mmax:{[w;x]{[w;x;j]|/x@(0|1+j-w)+!(1+j)&w}[w;x]'!#x}     / moving max  (window)
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
/ NOTE: this build is single-threaded, so peach evaluates sequentially with
/ each's semantics. Written as its own verb so code using peach is portable
/ to a threaded build; also exposed as .Q.fc (parallel-on-cut) in sys.k.
peach:{x'y}

/ ---- timing ---------------------------------------------------------------
/ ts"expr"  ->  evaluates expr, prints elapsed ms, returns the result.
/ The REPL also accepts  \ts expr  (see repl.k).
ts:{[e]t0:`t[];r:. e;`0:"ts: ",($0.001*`t[]-t0)," ms";r}
