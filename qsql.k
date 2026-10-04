/ ============================================================================
/ qsql.k  -  Amber qSQL template syntax.  Loaded after std.k.
/   sel "select … [by …] from … [where …]"    exq "exec … from … [where …]"
/   upd "update … [by …] from … [where …]"     del "delete [cols] from … [where …]"
/ Column names in the expressions are rewritten to  x`col  and run on the
/ existing qwhere / qby / qselect engine, so  wavg[sz;px]  just works.
/ Functional form:  qexec[t; where-mask|(); by-cols|(); name!func-dict|()]
/ ============================================================================

/ ---- parsing helpers ------------------------------------------------------
qsplit:{[s;kw]$[0=#s;:(s;"";0b);];i:ss[s;kw];m:~qsmask s;d:+\(m*in[s;"([{"])-m*in[s;")]}"];i:i@&0=d i;$[#i;(trim(*i)#s;trim((*i)+#kw)_s;1b);(trim s;"";0b)]}   / split on the first kw outside brackets and strings (a nested select keeps its own where)
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
qidsub:{[cols;s]$[0=#s;s;,/{[cols;t;f]$[f&in[`$t;cols];"(x`",t,")";f&(t~,"i");"(!#x)";t]}[cols]'[bnd _ s;(qword 0+s)bnd:&differ qword 0+s]]}   / a bare i is the row number, as q's virtual column
qfn:{[cols;e]. "{[x]",qidsub[cols;e],"}"}                                          / expr string -> 1-arg fn
/ NB the all-true mask is built as ~(#t)#0b, NOT (#t)#1b. A take of a scalar is kept in
/ Amber's compact constant form, and the fused C group-aggregate (`gagg, src/o.c) rejects a
/ mask in that form: it returns an empty result, qaggf reads that as "cannot fuse" and the
/ whole query silently falls back to the per-group path. So `select .. by ..` with NO where
/ clause never fused, while the same query WITH a where clause did. ~(#t)#0b is materialised,
/ ~-identical to (#t)#1b, and fuses. (Verified: gagg with (#t)#1b -> 0 rows; with ~(#t)#0b,
/ K=K or 0<1+!#t -> the correct rows.)
qmask:{[t;w]$[0=#w;~(#t)#0b;1=#c:qcut w;qfn[cols t;*c]t;&/qloc[cols t]'1_c;&/{[t;c]qfn[cols t;c]t}[t]'c;qcas[t;c]]}   / Where -> boolean mask
/ Several constraints cascade, as q's do: each one sees only the rows the earlier ones kept (and only the columns it
/ names), so  where sym=`a, n=max n  is the largest n among the a's. They were all run on the whole table (digest #39).
/ A constraint that only looks at its own row (columns, literals, data globals, comparisons, in/within/like...) gives
/ the same answer either way, so when every later one is like that the masks are just and-ed, at the old speed.
qcas:{[t;c]C:cols t;i:qcw[t;C;!#t;*c];i:{[t;C;i;e]qcw[t;C;i;e]}[t;C]/[i;1_c];@[~(#t)#1b;i;:;1b]}
qcw:{[t;C;i;e]r:qrefs[C;e];v:qfn[C;e]$[#r;+r!(+t)[r]@\:i;t i];$[isat v;$[v;i;0#i];i@&v]}   / The rows of i that e keeps
qlw:`in`within`like`not`null`abs`neg`floor`ceiling`signum`string`lower`upper`mod`div`xbar
qlwd:{[C;w]s:`$w;c:*w;$[in[s;C]|in[s;qlw];1;(~(c<"0")|c>"9")&~|/w="_";1;[v:.[{.x};,s;{`qlfail}];$[`qlfail~v;0;~|/"opqruvwx"=*$@v]]]}   / A word that keeps a row to itself
qblank:{[e]s:e;q:qsmask s;s:@[s;&q|s="\"";:;" "];w:qword 0+s;rid:+\~~':w;bt:&s="`";p:1+bt;p:p@&p<#s;p:p@&w p;s:@[s;bt;:;" "];@[s;&w&in[rid;rid p];:;" "]}   / strings and `symbols blanked
qloc:{[C;e]s:qblank e;w:qword 0+s;b:&~~':w;tk:b _ s;ok:&/1,qlwd[C]'tk@&w b;oi:&~w|s=" ";o:s oi;ok&:&/1,in[o;"=<>~+-*%&|$,.()"];nsi:&~s=" ";mp:oi@&in[o;"*|&,<>=+"];ok&&/1,{[s;w;nsi;p]j:nsi?p;$[j=0;0;[q:nsi j-1;c:s q;$[w q;1;c=")";1;(|/c="<>")&|/(s p)="=>";1;0]]]}[s;w;nsi]'mp}   / No aggregate, adverb or monadic first/where/count
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
/ Item -> column name, as q: an explicit name; else the first name in the expression, following a function to its
/ first argument and into brackets (sum px*sz is px, f[a;b] is a, (v+w)*2 is v); a number or literal first is x.
/ qnms numbers the repeats as q does: v v1 v2, x x1 (they were two columns called v, or x0 x1, digest #56)
qnm:{[C;i;it]p:qname it;$[#p 0;`$p 0;qcn[C;p 1]]}
qnms:{[C;it]n:qnm[C]'[!#it;it];{[n;i]k:+/n[i]=i#n;$[k;`$($n i),$k;n i]}[n]'!#n}
qcn:{[C;e]e:trim e;$[0=#e;`err"parse";"("=*e;qcn[C;qpin[e;"(";")"]];(2<#e)&("`"=*e)&("#"=e 2)&in[e 1;"supg"];qcn[C;3_e];"`"=*e;`x;"\""=*e;`x;~qword 0+*e;$[("-"=*e)&(1<#e)&~(e 1)<"0";$[(e 1)>"9";qcn[C;1_e];`x];qcn[C;1_e]];qcnw[C;e]]}
qcnw:{[C;e]w:qword 0+e;n:*(&~w),#e;t:n#e;r:trim n_e;c:*t;$[in[`$t;C];`$t;~(c<"0")|c>"9";`x;t~,"i";`x;~qfnw t;`$t;0=#r;`$t;"["=*r;qcn[C;*qscut qpin[r;"[";"]"]];qcn[C;r]]}
qfnw:{v:.[{.x};,`$x;{`qlfail}];$[`qlfail~v;0;|/"opqruvwx"=*$@v]}   / Does the word name a function
qpin:{[e;o;c]d:+\(e=o)-e=c;j:*(&d=0),#e;1_j#e}   / The text inside the bracket e starts with
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
/ 2.1: a select-list item of the form `agg col` with agg one of qsimp and col a
/ real column is a SIMPLE aggregate; a by-query whose every item is simple is
/ answered by qaggf through the fused C group-aggregate (`gagg) -- one pass per
/ aggregate over the raw columns, no per-group sub-tables, no lambda per group.
/ Integer results are narrowed with `i$ so column types match the generic path.
qsimp:`sum`avg`min`max`count`first`last
/ 2.1+: a THREE-word item of the form `w wavg c` / `w wsum c`, with w and c both real
/ columns, is also fused -- qaggf answers it with two `gagg sum passes (over w*c and over w)
/ and one divide, instead of dropping the whole query onto the per-group path. Without this,
/ one weighted average in the select list de-optimised every other aggregate beside it.
qwagg:`wavg`wsum
qsagg:{[C;e]w:" "vs trim e;$[2=#w;$[(in[`$w 0;qsimp])&in[`$w 1;C];(`$w 0;`$w 1);()];3=#w;$[(in[`$w 1;qwagg])&(in[`$w 0;C])&in[`$w 2;C];(`$w 1;`$w 0;`$w 2);()];()]}
/ the by-branch of sel, split out so sel itself stays under the bytecode limit
/ the by-branch of sel, split out so sel itself stays under the bytecode limit.
/ u is the UNFILTERED table and m the where-mask: the fused path hands the mask
/ to `gagg and never builds the filtered table; the generic path filters first.
qselby:{[u;m;byp;it;d]r:qbyx[u;byp];sg:qsagg[cols r 0]'{(qname x)1}'it;fa:$[&/{0<#x}'sg;qaggf[r 0;r 1;(!d)!sg;m];()];$[#fa;qks fa;[t:qwhere[r 0;m];qby[qproj[t;(r 1),(,/qrefs[cols t]'{(qname x)1}'it)];r 1;d]]]}
/ gag1: one select-list item -> one fused result triple (head;values;first-row-indices).
/ (agg;col)        -> a single `gagg pass, as before.
/ (wavg|wsum;w;c)  -> two `gagg sum passes over the same keys and mask, so the group order
/                     matches, then divide for wavg. Returns () if either pass declines,
/                     which keeps the existing "cannot fuse -> fall back" contract.
gag1:{[d;k;m;p]$[2=#p;`gagg(p 0;k;d p 1;m);[wv:d p 1;a:`gagg(`sum;k;wv*d p 2;m);$[(p 0)~`wsum;a;$[0=#a;();[b:`gagg(`sum;k;wv;m);$[0=#b;();(a 0;(a 1)%b 1;a 2)]]]]]]}
qaggf:{[t;b;sg;m]d:+t;bc:d b;k:$[1=#b;*bc;qcard[bc]<qEXACT;qenc bc;()];$[0=#k;();[rs:gag1[d;k;m]'sg;$[&/value[{0<#x}'rs];[fr:(*value[rs])2;(+b!bc@\:fr)!+{v:x 1;$[`I=@v;`i$v;v]}'rs];()]]]}


/ ---- select[...] : q's sorted and limited select ---------------------------
/   select[5] from t          the first 5 rows       select[-5] from t   the last 5
/   select[>px] from t        sorted by px, descending
/   select[<sym] from t       ... ascending
/   select[5;>px] from t      sorted, THEN limited -- q's order, not the reverse
/   select[<sym;>px] from t   several keys: the FIRST listed is the primary one
/ A sort key may be any expression over the columns, not only a column name; a
/ bare column name goes through xasc/xdesc so the result keeps the `s attribute
/ where that applies.  On a keyed (by-clause) result the spec is applied to the
/ unkeyed rows and the key is put back, which is what q does.
/ qbrk: peel a leading "[...]" off the select-part -> (spec-text; rest).
/ (the char tests are "["=*s, not "["~1#s : a char ATOM never matches a
/ one-element char VECTOR, so the ~ form is false for every input)
qbrk:{[s]$[0=#s;("";s);~"["=*s;("";s);[d:sums(s="[")-s="]";c:&d=0;$[0=#c;("";s);[i:*c;(trim 1_i#s;trim(1+i)_s)]]]]}
qsdir:{[e]$[0=#e;();"<"=*e;(0;trim 1_e);">"=*e;(1;trim 1_e);()]}     / () = not a sort item
qsone:{[r;p]C:cols r;e:p 1;$[in[`$e;C];$[p 0;xdesc[`$e;r];xasc[`$e;r]];[v:qfn[C;e]r;r@$[p 0;>v;<v]]]}
/ (the parameter is sk, not ss: `ss` is one of the names the parser treats as
/ an INFIX verb, and it does that from the name alone -- so `1_ss` inside a
/ lambda whose parameter is called ss parses ss as the string-search verb and
/ silently builds a projection instead of dropping.  See tests/test_infix.k.)
qsapp:{[r;sk]$[0=#sk;r;qsone[qsapp[r;1_sk];*sk]]}                   / first listed = primary
/ the bracket spec is SEMICOLON-separated (q: select[5;>px]), not comma-separated
/ like a select list, so it needs its own depth-aware splitter.
qscut:{[s]d:sums(s="[")-s="]";b:0,&(s=";")&d=0;trim'{$[#x;$[";"=*x;1_x;x];x]}'b _ s}
qlim1:{[r;sp]it:qscut sp;ds:qsdir'it;sk:ds@&{0<#x}'ds;nn:it@&{0=#x}'ds;r:qsapp[r;sk];$[0=#nn;r;(. *nn)#r]}
qlim:{[r;sp]$[0=#sp;r;iskeyed r;[k:keys r;xkey[k;qlim1[unkey r;sp]]];qlim1[r;sp]]}

/ ---- select ---------------------------------------------------------------
/ qflt: the rows this query selects.  With no where clause at all there is
/ nothing to select, so the table is handed back as-is rather than copied
/ through t@&m -- an all-true mask still made qwhere rebuild every column of
/ every row, which for `select from t` is a full copy of the table to answer a
/ query that asked for the table.
qflt:{[u;m;hw]$[hw;qwhere[u;m];u]}
/ qselall: `select from t` / `select * from t`, with or without a by-clause.
qselall:{[u;m;hw;r3]$[r3 2;[r:qbyx[qflt[u;m;hw];r3 1];nk:except[cols r 0;r 1];qby[r 0;r 1;nk!qlastf'nk]];qflt[u;m;hw]]}
/ qsel0: the select-list branch.  The filtered table is built ONLY here, and
/ only on the no-by path: the by path (qselby) takes the UNFILTERED table plus
/ the mask, so the fused group aggregate reads the raw columns and never
/ materialises the selected rows.  sel used to build that table eagerly for
/ EVERY query, before it knew which branch would run, so a
/ `select .. by .. where ..` copied every column of every selected row and then
/ threw the copy away.
qsel0:{[u;m;hw;selp;r3;C]$[(selp~,"*")|0=#selp;qselall[u;m;hw;r3];[it:qcut selp;d:(qnms[C;it])!{[C;e]qfn[C;(qname e)1]}[C]'it;$[r3 2;qselby[u;m;r3 1;it;d];qselect[qflt[u;m;hw];d]]]]}
sel:{[s]s:qnorm s;s:$[(6#s)~"select";trim 6_s;s];b:qbrk s;s:b 1;r:qsplit[s;" where "];wh:$[r 2;r 1;""];h:trim r 0;r2:$[(5#h)~"from ";("";trim 5_h;1b);qsplit[h;" from "]];r3:qsplit0[r2 0;" by "];dz:"distinct "~9#r3 0;r3:$[dz;@[r3;0;:;trim 9_r3 0];r3];u:. qtry r2 1;r:$[qpt u;qpsel[u;r3;wh;b 0];qlim[qsel0[u;qmask[u;wh];r 2;r3 0;r3;cols u];b 0]];$[dz;?r;r]}   / select distinct: the distinct rows, as q
qrun:{[u;wh;r3;lim]qlim[qsel0[u;qmask[u;wh];0<#wh;r3 0;r3;cols u];lim]}   / select on a table in hand (hdb.k runs it per partition)

/ A table on disk split in partitions (hdb.k's loaddb) is a dict that only names its partitions. select and exec
/ hand it to qpsel and qpexec (hdb.k): they keep the partitions the where-clause allows and map only the columns
/ the query names.
qpt:{[u]$[`m~@u;in[`amberpt;!u];0]}

/ ---- exec  (return one column / expression, not a table) ------------------
/ exec splits its select-list the SAME way select does (qcut, honouring brackets):
/   one item  -> the bare vector          exec px from t        exec spread:px-ask from t
/   many      -> a dict name!(vectors)     exec px,ask from t -> `px`ask!(..;..)
/ so `exec a,b` yields two named columns (q semantics) instead of joining them into
/ one vector, which is what lets `plot exec px,ask ...` draw a line per column.
exq:{[s]s:qnorm s;s:$[(4#s)~"exec";trim 4_s;s];r:qsplit[s;" where "];wh:$[r 2;r 1;""];r2:qsplit[r 0;" from "];u:. qtry r2 1;pq:$[qpt u;qpexec[u;r2 0;wh];(u;wh)];u:pq 0;wh:pq 1;eb:qsplit0[r2 0;" by "];$[eb 2;qexby[u;wh;eb];[t:$[#wh;qwhere[u;qmask[u;wh]];u];C:cols u;it:qcut trim r2 0;v:{[C;t;e]qfn[C;(qname e)1]t}[C;t]'it;$[1=#it;*v;(qnms[C;it])!v]]]}
/ exec e by b: a dict from the groups to e, as q (through select, then the key column and the value column)
qexby:{[u;wh;eb]r:qrun[u;wh;(eb 0;eb 1;1b);""];k:+key r;v:+value r;$[1=#!k;k@*!k;key r]!$[1=#!v;v@*!v;value r]}

/ ---- update  (add / replace columns; optional where and by) ---------------
qasg:{[t;idx;nm;v]@[$[in[nm;cols t];(+t)nm;(#t)#qnull v];idx;:;v]}
qasgby:{[t;nm;f;gi]{[t;f;c;iv]@[c;iv;:;f atr[t;iv]]}[t;f]/[$[in[nm;cols t];(+t)nm;(#t)#qnull f atr[t;*gi]];gi]}
upd:{[s]s:qnorm s;s:$[(6#s)~"update";trim 6_s;s];r:qsplit[s;" where "];wh:$[r 2;r 1;""];r2:qsplit[r 0;" from "];r3:qsplit[r2 0;" by "];t:. qtry r2 1;$[qpt t;`err"par";];C:cols t;it:qcut r3 0;nm:qnms[C;it];fns:{[C;e]qfn[C;(qname e)1]}[C]'it;$[r3 2;[gi:value(=rows[{(x 1)#+x 0}qbyx[t;r3 1]]);+((+t),nm!{[t;gi;n;f]qasgby[t;n;f;gi]}[t;gi]'[nm;fns])];0=#wh;+((+t),nm!{[t;f]v:f t;$[isat v;(#t)#v;v]}[t]'fns);[m:qmask[t;wh];idx:&m;sub:qwhere[t;m];+((+t),nm!{[t;idx;sub;n;f]qasg[t;idx;n;f sub]}[t;idx;sub]'[nm;fns])]]}   / no where: each column is the value itself, attribute and all

/ ---- delete  (rows via where, or named columns) ---------------------------
del:{[s]s:qnorm s;s:$[(6#s)~"delete";trim 6_s;s];b:$[(5#s)~"from ";("";trim 5_s);2#qsplit[s;" from "]];colp:trim b 0;rw:qsplit[b 1;" where "];t:. qtry trim rw 0;$[qpt t;`err"par";];wh:$[rw 2;rw 1;""];$[#colp;+(except[cols t;`$'{(qname x)1}'qcut colp])#+t;qwhere[t;~qmask[t;wh]]]}

/ ---- functional form ------------------------------------------------------
/ q's ?[t;c;b;a] (select) and ![t;c;b;a] (update, delete) over parse trees: a symbol is a column (or a global),
/ a one-item list like enlist`a is that item as a value, (f;x;y) applies f. c's clauses cascade, each on the
/ rows the last kept; b is 0b (no groups), 1b (distinct) or a dict of names to trees; a is () or a dict.
qfv:{in[@x;`o`p`q`r`u`v`w`x]}
qfe:{[t;e]$[`s~@e;$[in[e;cols t];(+t)e;e~`i;!#t;. e];(`S~@e)&1=#e;*e;(`A~@e)&0<#e;$[qfv[*e];(*e) . qfe[t]'1_e;1=#e;*e;e];e]}
qfi:{[t;c]{[t;i;w]i@&qfe[t i;w]}[t]/[!#t;c]}
qfa:{[t;a]v:qfe[t]'value a;k:!a;$[&/isat'v;+k!,'v;+k!{[l;x]$[isat x;l#x;x]}[#t]'v]}
qfby:{[t;b;a]bk:!b;t2:+(+t),bk!qfe[t]'value b;$[0=#a;[nk:except[cols t;bk];qby[t2;bk;nk!qlastf'nk]];qby[t2;bk;(!a)!{[e]{[e;x]qfe[x;e]}[e]}'value a]]}
qfsel:{[t;c;b;a]t:$[`s~@t;. t;t];t:$[0=#c;t;t qfi[t;c]];$[(b~0b)|b~();$[0=#a;t;qfa[t;a]];b~1b;?$[0=#a;t;qfa[t;a]];qfby[t;b;a]]}
qfub:{[t;idx;b;a]k:+(!b)!qfe[t idx]'value b;gi:idx@value(=rows k);+((+t),(!a)!{[t;gi;n;e]qasgby[t;n;{[e;x]qfe[x;e]}[e];gi]}[t;gi]'[!a;value a])}
qfupd:{[t;c;b;a]t:$[`s~@t;. t;t];idx:$[0=#c;!#t;qfi[t;c]];$[`S~@a;$[0=#a;t@except[!#t;idx];+(except[cols t;a])#+t];(b~0b)|b~();+((+t),(!a)!{[t;idx;n;e]qasg[t;idx;n;qfe[t idx;e]]}[t;idx]'[!a;value a]);qfub[t;idx;b;a]]}
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
/ is a valid keyword start at p?  The keyword must be followed by a space -- or,
/ for `select` only, by the "[" of a sorted/limited select[n;>c].  The other
/ three keywords take no bracket in q, so admitting one for them would only let
/ a typo reach the wrong parser.
qokc:{[kw;c]$[" "~c;1;("["~c)&kw~"select";1;0]}
qokp:{[t;sm;kw;p]((p=0)|(qpre?t[p-1])<#qpre)&(((p+#kw)<#t)&qokc[kw;t[p+#kw]])&~sm p}
qmin1:{[t;sm;kw]$[(#t)<#kw;qbig;[ps:ss[t;kw];$[0=#ps;qbig;[ok:qokp[t;sm;kw]'ps;v:ps+qbig*~ok;min v]]]]}  / earliest valid start of kw
qfind:{[t]sm:qsmask t;mins:qmin1[t;sm]'qkws;best:min mins;$[~best<qbig;();(best;mins?best)]}             / (pos;keyword-index) or ()
/ qml: fold a MULTI-LINE qSQL statement onto one line before the rewriter looks
/ at it.  qrw runs on one REPL input line, which never contains a newline -- but
/ the same rewriter is what an amberd client or a Grafana panel hands a whole
/ query to, and those are written across several lines for readability.  Such a
/ query has no " from " for qokp/qsplit to find and used to fail on its first
/ fragment ("undefined variable `select`").  Only text that both spans lines and
/ mentions a qSQL keyword is folded, so ordinary input is untouched.
/ amber 2.2: strip the trailing comment from a SINGLE line too, not only from a
/ multi-line block. qdec was reached only through qnorm, i.e. only when the
/ input had a newline, so a line typed at the prompt kept its comment -- and
/ qendx then found the `;` INSIDE that comment and ended the statement there:
/   select[<sym;>px] from trades   / two keys; the first listed is primary
/ rewrote to   sel"select[<sym;>px] from trades   / two keys"; the first ...
/ and the tail was evaluated as code ('primary undefined). The identical text
/ in a file loaded with \l was fine, which is the tell: one input, two paths,
/ two answers. Only lines that actually contain a qSQL keyword are touched, so
/ ` / ` used as an adverb anywhere else is left exactly as it was.
qml:{[s]$[~|/{0<#ss[x;y]}[s]'qkws;s;|/s="\n";qnorm s;qdec s]}
qrw:{[s]s:qml s;$[2>#s;s;[r:qfind s;$[0=#r;s;[p:r 0;ki:r 1;e:qendx[s;p];body:(e-p)#p_s;$[0<#ss[body;" from "];(p#s),qkfn[ki],"\"",(qesc body),"\"",qrw e_s;[L:p+#qkws ki;(L#s),qrw L_s]]]]]]}
/ qtry / qrwf: the SAME rewrite applied to a WHOLE .k file so bare qSQL works in
/ scripts, not just at the prompt -- the C loader bsl() calls qrwf before eval.
/ Line-by-line (exactly as the REPL rewrites each typed line); a line qrw can't
/ handle is left verbatim via the `ERR sentinel (never a real qrw result, which
/ is always a char vector) so a rewrite failure can never corrupt a script.
qtry:{[s]r:.[qrw;,s;{`ERR}];$[`ERR~r;s;r]}
qrwf:{$[|/{0<#ss[x;y]}[x]'qkws;sv["\n";qtry'vs["\n";x]];x]}
