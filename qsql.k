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
qcut:{[s]d:sums(s="[")-s="]";b:0,&(s=",")&d=0;trim'{$[#x;$[","=*x;1_x;x];x]}'b _ s}  / comma-split at depth 0
qname:{[s]d:sums(s="[")-s="]";i:&(s=":")&d=0;$[#i;(trim(*i)#s;trim(1+*i)_s);("";trim s)]}  / name:expr
qword:{((x>64)&x<91)|((x>96)&x<123)|((x>47)&x<58)|x=95}                            / word-char mask
qsimple:{(0<#x)&&/qword 0+x}                                                       / is a bare identifier
qidsub:{[cols;s]$[0=#s;s;,/{[cols;t;f]$[f&in[`$t;cols];"(x`",t,")";t]}[cols]'[bnd _ s;(qword 0+s)bnd:&differ qword 0+s]]}
qfn:{[cols;e]. "{[x]",qidsub[cols;e],"}"}                                          / expr string -> 1-arg fn
qmask:{[t;w]$[0=#w;(#t)#1b;&/{[t;c]qfn[cols t;c]t}[t]'qcut w]}                      / where -> boolean mask
qbc:{[byp]`$'{(qname x)1}'qcut byp}                                                / by-clause -> symbol cols
qnm:{[i;it]p:qname it;$[#p 0;`$p 0;qsimple p 1;`$p 1;`$"x",$i]}                     / item -> column name
qnull:{$[in[@x;`I`i];0N;in[@x;`F`f];0n;in[@x;`S`s];`;in[@x;`C`c];" ";0N]}

/ ---- select ---------------------------------------------------------------
sel:{[s]s:trim s;s:$[(6#s)~"select";trim 6_s;s];r:qsplit[s;" where "];wh:$[r 2;r 1;""];r2:qsplit[r 0;" from "];r3:qsplit[r2 0;" by "];selp:r3 0;u:. r2 1;t:qwhere[u;qmask[u;wh]];C:cols u;$[selp~,"*";t;[it:qcut selp;d:(qnm'[!#it;it])!{[C;e]qfn[C;(qname e)1]}[C]'it;$[r3 2;qby[t;qbc r3 1;d];qselect[t;d]]]]}

/ ---- exec  (return one column / expression, not a table) ------------------
exq:{[s]s:trim s;s:$[(4#s)~"exec";trim 4_s;s];r:qsplit[s;" where "];wh:$[r 2;r 1;""];r2:qsplit[r 0;" from "];u:. r2 1;t:qwhere[u;qmask[u;wh]];(qfn[cols u;(qname trim r2 0)1])t}

/ ---- update  (add / replace columns; optional where and by) ---------------
qasg:{[t;idx;nm;v]@[$[in[nm;cols t];(+t)nm;(#t)#qnull v];idx;:;v]}
qasgby:{[t;nm;f;gi]{[t;f;c;iv]@[c;iv;:;f atr[t;iv]]}[t;f]/[$[in[nm;cols t];(+t)nm;(#t)#qnull f atr[t;*gi]];gi]}
upd:{[s]s:trim s;s:$[(6#s)~"update";trim 6_s;s];r:qsplit[s;" where "];wh:$[r 2;r 1;""];r2:qsplit[r 0;" from "];r3:qsplit[r2 0;" by "];t:. r2 1;C:cols t;it:qcut r3 0;nm:qnm'[!#it;it];fns:{[C;e]qfn[C;(qname e)1]}[C]'it;$[r3 2;[gi:value(=rows[qbc[r3 1]#+t]);+((+t),nm!{[t;gi;n;f]qasgby[t;n;f;gi]}[t;gi]'[nm;fns])];[m:qmask[t;wh];idx:&m;sub:qwhere[t;m];+((+t),nm!{[t;idx;sub;n;f]qasg[t;idx;n;f sub]}[t;idx;sub]'[nm;fns])]]}

/ ---- delete  (rows via where, or named columns) ---------------------------
del:{[s]s:trim s;s:$[(6#s)~"delete";trim 6_s;s];b:$[(5#s)~"from ";("";trim 5_s);2#qsplit[s;" from "]];colp:trim b 0;rw:qsplit[b 1;" where "];t:. trim rw 0;wh:$[rw 2;rw 1;""];$[#colp;+(except[cols t;`$'{(qname x)1}'qcut colp])#+t;qwhere[t;~qmask[t;wh]]]}

/ ---- functional form ------------------------------------------------------
qexec:{[t;w;b;d]t:$[0=#w;t;qwhere[t;w]];$[0=#b;$[0=#d;t;qselect[t;d]];qby[t;b,();d]]}
