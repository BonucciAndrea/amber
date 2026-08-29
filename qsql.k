/ ============================================================================
/ qsql.k  -  Amber qSQL template syntax.  Loaded after std.k.
/   sel "select … [by …] from … [where …]"    exq "exec … from … [where …]"
/   upd "update … [by …] from … [where …]"     del "delete [cols] from … [where …]"
/ Column names in the expressions are rewritten to  x`col  and run on the
/ existing qwhere / qby / qselect engine, so  wavg[sz;px]  just works.
/ Functional form:  qexec[t; where-mask|(); by-cols|(); name!func-dict|()]
/ ============================================================================

/ ---- parsing helpers ------------------------------------------------------
qsplit:{[s;kw]i:ss[s;kw];$[#i;(trim(*i)#s;trim((*i)+#kw)_s;1b);(trim s;"";0b)]}   / split on first kw
/ split on kw, also matching kw with its leading space stripped at position 0 --
/ needed for `select by sym from t`, where the by-clause IS the whole
/ select-part and so has no space in front of it to match " by " against.
qsplit0:{[s;kw]$[(s~*i:qsplit[s;kw])&((#kw)-1)<#s;$[(1_kw)~(-1+#kw)#s;("";trim(-1+#kw)_s;1b);i];i]}
/ last-of-each-column aggregation, used for a `by` with no explicit projection
qlastf:{[c]{[c;x]last x c}[c]}
qcut:{[s]d:sums(s="[")-s="]";b:0,&(s=",")&d=0;trim'{$[#x;$[","=*x;1_x;x];x]}'b _ s}  / comma-split at depth 0
qname:{[s]d:sums(s="[")-s="]";i:&(s=":")&d=0;$[#i;(trim(*i)#s;trim(1+*i)_s);("";trim s)]}  / name:expr
qword:{((x>64)&x<91)|((x>96)&x<123)|((x>47)&x<58)|x=95}                            / word-char mask
qsimple:{(0<#x)&&/qword 0+x}                                                       / is a bare identifier
qidsub:{[cols;s]$[0=#s;s;,/{[cols;t;f]$[f&in[`$t;cols];"(x`",t,")";t]}[cols]'[bnd _ s;(qword 0+s)bnd:&differ qword 0+s]]}
qfn:{[cols;e]. "{[x]",qidsub[cols;e],"}"}                                          / expr string -> 1-arg fn
qmask:{[t;w]$[0=#w;(#t)#1b;&/{[t;c]qfn[cols t;c]t}[t]'qcut w]}                      / where -> boolean mask
/ qdec: drop a trailing K comment from a query string.
/ A "/" that STARTS a token -- at the beginning of the string, or preceded by a
/ space or a tab -- and is not inside a "…" literal begins a comment.  The qSQL
/ parsers never applied that rule, so a comment written after a query
/   select from t where px>150      / filter rows
/ was carried into the where-clause and compiled INTO the generated lambda,
/   {[x](x`px)>150      / filter rows}
/ which is one line, so the closing "}" was itself commented out and the whole
/ query died with E0105.  Stripping here fixes it for the bare form and the
/ sel"…" / exq"…" / upd"…" / del"…" string forms alike.  The adverb "/" always
/ follows its verb with no space, so it is never mistaken for a comment.
qdec:{[s]$[0=#s;s;[q:(s="\"")&~0,-1_s="\\";ins:1=2!0,-1_sums q;ws:1,-1_(s=" ")|s="\t";i:&(s="/")&ws&~ins;$[#i;trim(*i)#s;s]]]}
/ qnorm: what every qSQL entry point runs on its argument before parsing it --
/ strip each line's comment, then fold the lines into one.  The clause splitters
/ look for " from " / " where " / " by " WITH their spaces, so a query broken
/ across lines for readability (a Grafana panel query, a query sent to amberd)
/ never matched and fell over on the first fragment.  Comments are removed line
/ by line first, so a comment can only ever eat the rest of its own line.
qnorm:{[s]trim(" "/qdec'"\n"\s)}
qnm:{[C;i;it]p:qname it;$[#p 0;`$p 0;qsimple p 1;`$p 1;[r:qrefs[C;p 1];$[1=#r;*r;`$"x",$i]]]}   / item -> column name
/ by-clause item -> the name its group column gets.  Explicit "name:expr" wins;
/ a bare column keeps its own name; an expression over exactly one column takes
/ that column's name (so `by time:1m xbar time` and `by minbar[1;time]` both
/ produce a `time` column, as q does); anything wider falls back to g0, g1, …
qbyn:{[C;i;it]p:qname it;$[#p 0;`$p 0;qsimple p 1;`$p 1;[r:qrefs[C;p 1];$[1=#r;*r;`$"g",$i]]]}
/ qbyx: resolve a by-clause into (table; group-column-names).
/ qbc used to turn every by item into a SYMBOL of its own source text, so
/ `by time:minbar[1;time]` grouped on a column literally called
/ "time:minbar[1;time]" -- which no table has.  Grouping on a missing column
/ collapsed every row into one group with a null key, so the query returned a
/ single wrong row instead of one row per bucket, and returned it silently.
/ Here each by item is COMPUTED with the same qfn machinery the select-list
/ uses and appended to the table under the name qbyn gives it; grouping then
/ runs on real columns.  A bare column name computes to itself, so the common
/ `by sym` path is unchanged.
qbyx:{[t;byp]C:cols t;it:qcut byp;ex:{(qname x)1}'it;nm:qbyn[C]'[!#it;it];v:{[t;C;e]qfn[C;e]t}[t;C]'ex;(+((+t),nm!v);nm)}
qnull:{$[in[@x;`I`i];0N;in[@x;`F`f];0n;in[@x;`S`s];`;in[@x;`C`c];" ";0N]}
/ qrefs: the column names an expression actually mentions.  Same tokenisation
/ qidsub uses to REWRITE identifiers into x`col, so the set returned here is
/ exactly the set of columns the compiled aggregate can reach.
qrefs:{[C;s]$[0=#s;0#`;[m:qword 0+s;bnd:&differ m;tk:bnd _ s;fl:m bnd;sy:`$'tk;?sy@&fl&{[C;y]in[y;C]}[C]'sy]]}
/ qproj: narrow a table to `cs` before a group-by.  qby materialises one
/ sub-table per group (atr), which indexes EVERY column it is handed -- on a
/ wide table that copied dozens of columns per group that no aggregate ever
/ read.  Projecting first makes that cost scale with columns USED, not columns
/ present.  Only ever narrows, so it cannot change a result.
qproj:{[t;cs]+(?cs)#+t}


/ ---- select ---------------------------------------------------------------
sel:{[s]s:qnorm s;s:$[(6#s)~"select";trim 6_s;s];r:qsplit[s;" where "];wh:$[r 2;r 1;""];h:trim r 0;r2:$[(5#h)~"from ";("";trim 5_h;1b);qsplit[h;" from "]];r3:qsplit0[r2 0;" by "];selp:r3 0;u:. r2 1;t:qwhere[u;qmask[u;wh]];C:cols u;$[(selp~,"*")|0=#selp;$[r3 2;[r:qbyx[t;r3 1];nk:except[cols r 0;r 1];qby[r 0;r 1;nk!qlastf'nk]];t];[it:qcut selp;d:(qnm[C]'[!#it;it])!{[C;e]qfn[C;(qname e)1]}[C]'it;$[r3 2;[r:qbyx[t;r3 1];qby[qproj[r 0;(r 1),(,/qrefs[cols r 0]'{(qname x)1}'it)];r 1;d]];qselect[t;d]]]]}

/ ---- exec  (return one column / expression, not a table) ------------------
exq:{[s]s:qnorm s;s:$[(4#s)~"exec";trim 4_s;s];r:qsplit[s;" where "];wh:$[r 2;r 1;""];r2:qsplit[r 0;" from "];u:. r2 1;t:qwhere[u;qmask[u;wh]];(qfn[cols u;(qname trim r2 0)1])t}

/ ---- update  (add / replace columns; optional where and by) ---------------
qasg:{[t;idx;nm;v]@[$[in[nm;cols t];(+t)nm;(#t)#qnull v];idx;:;v]}
qasgby:{[t;nm;f;gi]{[t;f;c;iv]@[c;iv;:;f atr[t;iv]]}[t;f]/[$[in[nm;cols t];(+t)nm;(#t)#qnull f atr[t;*gi]];gi]}
upd:{[s]s:qnorm s;s:$[(6#s)~"update";trim 6_s;s];r:qsplit[s;" where "];wh:$[r 2;r 1;""];r2:qsplit[r 0;" from "];r3:qsplit[r2 0;" by "];t:. r2 1;C:cols t;it:qcut r3 0;nm:qnm[C]'[!#it;it];fns:{[C;e]qfn[C;(qname e)1]}[C]'it;$[r3 2;[gi:value(=rows[{(x 1)#+x 0}qbyx[t;r3 1]]);+((+t),nm!{[t;gi;n;f]qasgby[t;n;f;gi]}[t;gi]'[nm;fns])];[m:qmask[t;wh];idx:&m;sub:qwhere[t;m];+((+t),nm!{[t;idx;sub;n;f]qasg[t;idx;n;f sub]}[t;idx;sub]'[nm;fns])]]}

/ ---- delete  (rows via where, or named columns) ---------------------------
del:{[s]s:qnorm s;s:$[(6#s)~"delete";trim 6_s;s];b:$[(5#s)~"from ";("";trim 5_s);2#qsplit[s;" from "]];colp:trim b 0;rw:qsplit[b 1;" where "];t:. trim rw 0;wh:$[rw 2;rw 1;""];$[#colp;+(except[cols t;`$'{(qname x)1}'qcut colp])#+t;qwhere[t;~qmask[t;wh]]]}

/ ---- functional form ------------------------------------------------------
qexec:{[t;w;b;d]t:$[0=#w;t;qwhere[t;w]];$[0=#b;$[0=#d;t;qselect[t;d]];qby[t;b,();d]]}

/ ---- bare qSQL rewriter ----------------------------------------------------
/ Lets you type   select/exec/update/delete … from … [where …] [by …]   with no
/ sel"…" wrapper.  qrw rewrites such an expression into the matching string call
/ (sel/exq/upd/del); the REPL (repl.k, line1) runs every input line through it.
/ Ordinary expressions - and an explicit sel"…" - are returned unchanged.
qkws:("select";"exec";"update";"delete")
qkfn:("sel";"exq";"upd";"del")
qpre:" ([{;:,=<>+-*%!&|^~?@."
qbig:1000000
qesc:{,/{$[x="\\";"\\\\";$[x="\"";"\\\"";,x]]}'x}                                 / escape \ and " for a "…" literal
qsmask:{[t]q:(t="\"")&~0,-1_t="\\";1=2!0,-1_sums q}                              / 1b where inside a "…" literal
qendx:{[t;p]b:p_t;n:#b;op:(b="(")|(b="[")|b="{";cl:(b=")")|(b="]")|b="}";dep:sums op-cl;dB:0,-1_dep;ns:(dep<0)|(b=";")&dB=0;p+min((!n)*ns)+n*~ns}  / end of the expr starting at p
qokp:{[t;sm;kw;p]((p=0)|(qpre?t[p-1])<#qpre)&(((p+#kw)<#t)&" "=t[p+#kw])&~sm p}   / is a valid keyword start at p
qmin1:{[t;sm;kw]$[(#t)<#kw;qbig;[ps:ss[t;kw];$[0=#ps;qbig;[ok:qokp[t;sm;kw]'ps;v:ps+qbig*~ok;min v]]]]}  / earliest valid start of kw
qfind:{[t]sm:qsmask t;mins:qmin1[t;sm]'qkws;best:min mins;$[~best<qbig;();(best;mins?best)]}             / (pos;keyword-index) or ()
/ qml: fold a MULTI-LINE qSQL statement onto one line before the rewriter looks
/ at it.  qrw runs on one REPL input line, which never contains a newline -- but
/ the same rewriter is what an amberd client or a Grafana panel hands a whole
/ query to, and those are written across several lines for readability.  Such a
/ query has no " from " for qokp/qsplit to find and used to fail on its first
/ fragment ("undefined variable `select`").  Only text that both spans lines and
/ mentions a qSQL keyword is folded, so ordinary input is untouched.
qml:{[s]$[(|/s="\n")&|/{0<#ss[x;y]}[s]'qkws;qnorm s;s]}
qrw:{[s]s:qml s;$[2>#s;s;[r:qfind s;$[0=#r;s;[p:r 0;ki:r 1;e:qendx[s;p];body:(e-p)#p_s;$[0<#ss[body;" from "];(p#s),qkfn[ki],"\"",(qesc body),"\"",qrw e_s;[L:p+#qkws ki;(L#s),qrw L_s]]]]]]}
/ qtry / qrwf: the SAME rewrite applied to a WHOLE .k file so bare qSQL works in
/ scripts, not just at the prompt -- the C loader bsl() calls qrwf before eval.
/ Line-by-line (exactly as the REPL rewrites each typed line); a line qrw can't
/ handle is left verbatim via the `ERR sentinel (never a real qrw result, which
/ is always a char vector) so a rewrite failure can never corrupt a script.
qtry:{[s]r:.[qrw;,s;{`ERR}];$[`ERR~r;s;r]}
qrwf:{$[|/{0<#ss[x;y]}[x]'qkws;sv["\n";qtry'vs["\n";x]];x]}
