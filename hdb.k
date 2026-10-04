/ ============================================================================
/ hdb.k  -  on-disk data for Amber.  Loaded after sys.k.
/   dset / dget          a value <-> one file
/   splay / dload        a table <-> a splayed directory, one file per column plus a .d
/   set / get            the q names for both: `:db/t/ set t splays, `:f set x writes one file
/   Q.en / Q.dpft        enumerate a table's symbols, save one partition of a database
/   loaddb               map a whole database: partitioned tables, splayed tables, files
/   ptab                 a partitioned table (or some of its partitions) as one table
/   partsave / partload / parts   the older value-partitioned layout
/ Since 2.7 the files are binary. `wcol writes a column and `rcol maps it back (src/dsk.c), so a numeric
/ column costs one mmap to load and its pages come in as a query reads them. Symbol columns are kept as
/ indexes into the database's sym file, like q. Files written before 2.7 (Amber text) still read.
/ ============================================================================

/ ---- paths ----------------------------------------------------------------
hdb.p:{[p]s:$[in[@p;`s`S];$p;p];$[(0<#s)&":"=*s;1_s;s]}         / a path from a string or a q handle `:db/t
hdb.sl:{[s]$[(1<#s)&"/"=*|s;-1_s;s]}                          / without a trailing /
hdb.par:{[p]i:&p="/";$[0=#i;".";0=*|i;"/";(*|i)#p]}           / the directory p is in
hdb.amc:{[f]$[0<`fsz f;"AMBC"~`c$4#1:f;0]}                     / is f a binary Amber file

/ ---- the sym file ---------------------------------------------------------
/ One per database, at db/sym. It only ever grows, so a column written against it stays readable.
/ HDBK and HDBV keep each one read so far with the size it had; a different size means read it again.
HDBK:0#`
HDBV:()
hdb.symf:{[db]db,"/sym"}
hdb.symc:{[db;z;s]k:`$db;$[in[k;HDBK];HDBV::@[HDBV;HDBK?k;:;,(z;s)];[HDBK::HDBK,k;HDBV::HDBV,,(z;s)]];s}
hdb.syml:{[db;f;z]hdb.symc[db;z;`rcol f]}
hdb.syms:{[db]f:hdb.symf db;z:`fsz f;k:`$db;$[z<0;0#`;~in[k;HDBK];hdb.syml[db;f;z];~z~*HDBV HDBK?k;hdb.syml[db;f;z];(HDBV HDBK?k)1]}
/ The indexes of v in db's syms (new names are added to the file first) and how many syms there are
hdb.enum:{[db;v]d:hdb.syms db;u:?v;nw:u@&~in[u;d];$[#nw;[d:d,nw;`wcol(hdb.symf db;d;-1;0);hdb.symc[db;`fsz hdb.symf db;d]];0];(d?v;#d)}

/ ---- one value, one file -------------------------------------------------
dset:{[f;x]f:hdb.p f;`wcol(f;x;-1;0);f}
dget:{[f]f:hdb.p f;$[hdb.amc f;`rcol f;. `c$1:f]}

/ ---- splayed tables -------------------------------------------------------
/ splay["db/trades";t] writes db/trades/<col> for each column and db/trades/.d, symbols into db/sym.
hdb.atc:{[v]a:$`at v;$[#a;1+"supg"?*a;0]}                      / attribute code kept in the column file
hdb.wc:{[f;db;v]$[`S~@v;[e:hdb.enum[db;v];`wcol(f;e 0;e 1;hdb.atc v)];`wcol(f;v;-1;0)];f}
hdb.splay:{[dir;db;t]t:$[iskeyed t;unkey t;t];c:cols t;d:+t;{[dir;db;d;c]hdb.wc[dir,"/",$c;db;d c]}[dir;db;d]'c;`wcol(dir,"/.d";c;-1;0);dir}
splay:{[dir;t]dir:hdb.sl hdb.p dir;hdb.splay[dir;hdb.par dir;t]}
hdb.dl:{[dir;s]c:`rcol dir,"/.d";+c!{[dir;s;c]`rcol(dir,"/",$c;s)}[dir;s]'c}
hdb.old:{[dir]cs:. `c$1:dir,"/.d";+cs!{[dir;c]. `c$1:dir,"/",$c}[dir]'cs}   / a pre-2.7 text table
dload:{[dir]dir:hdb.sl hdb.p dir;$[hdb.amc dir,"/.d";hdb.dl[dir;hdb.syms hdb.par dir];hdb.old dir]}

/ ---- the q names ----------------------------------------------------------
/ `a set 5 and get`a are globals; `:path set x writes a file, and a path ending in / splays a table.
set:{[h;v]$[(`s~@h)&~":"=*$h;[.[h;();:;v];h];[p:hdb.p h;$["/"~*|p;splay[p;v];dset[p;v]];h]]}
get:{[h]$[(`s~@h)&~":"=*$h;. h;[p:hdb.sl hdb.p h;$[-2=`fsz p;dload p;dget p]]]}

/ ---- partitioned databases ------------------------------------------------
/ db/<partition>/<table>/<col>, partitions named by a date (2026.01.02) or an int, symbols in db/sym.
hdb.pv:{[p]$p}                                                  / partition value -> directory name
hdb.pok:{[s]d:in[s;"0123456789"];$[0=#s;0;&/d;1;(10=#s)&(".."~s 4 7)&&/d 0 1 2 3 5 6 8 9;1;0]}   / looks like a partition
hdb.pcn:{[p]$[`d~@p;`date;`int]}                                / the partition column's name
Q.en:{[d;t]d:hdb.sl hdb.p d;dd:+t;{[d;dd;c]$[`S~@dd c;hdb.enum[d;dd c];0]}[d;dd]'cols t;t}
Q.dpft:{[d;p;f;t]d:hdb.sl hdb.p d;v:. t;v:$[iskeyed v;unkey v;v];v:xasc[f;v];dd:+v;dd:@[dd;f;{`pa x}];hdb.splay[d,"/",(hdb.pv p),"/",$t;d;+dd];t}

/ A partitioned table is a dict naming where it lives: database, table, partition column, the partitions that
/ hold it and its columns. Nothing is read until a query asks: qpsel (select, exec) and ptab map what they need.
hdb.pto:{[db;pc;tn;ps]pv:{. x}'ps;i:<pv;c:`rcol db,"/",(ps i 0),"/",tn,"/.d";`amberpt`db`t`pc`pv`cols!(1b;db;`$tn;pc;pv i;c)}
/ One partition: (rows; columns cs of it as a dict, the partition column left out)
hdb.part:{[pt;cs;p]dir:(pt`db),"/",(hdb.pv p),"/",$pt`t;s:hdb.syms pt`db;c:cs@&~cs=pt`pc;v:{[dir;s;c]`rcol(dir,"/",$c;s)}[dir;s]'c;n:$[#c;#*v;#`rcol(dir,"/",($*pt`cols);s)];(n;c!v)}
/ Partitions pv, columns cs, as one table. One partition is its mapped columns as they are, no copy.
hdb.mat:{[pt;pv;cs]C:(,pt`pc),pt`cols;cs:C@&in[C;cs];$[0=#pv;0#hdb.mat[pt;1#pt`pv;cs];[r:hdb.part[pt;cs]'pv;n:{x 0}'r;d:{x 1}'r;c:cs@&~cs=pt`pc;v:{[d;c],/{[c;x]x c}[c]'d}[d]'c;v:$[in[pt`pc;cs];(,,/n#'pv),v;v];+cs!v]]}
ptab:{[t;pv]pt:$[`s~@t;. t;t];hdb.mat[pt;$[0=#pv;pt`pv;pv];(,pt`pc),pt`cols]}
/ select/exec on a partitioned table. Where-clauses on the partition column alone pick the partitions and are then
/ done with (every row of a kept partition passes them). Only the columns the query names are mapped, all of them
/ for select * or a bare select. Grouped by the partition column, the query runs per partition and the results
/ are stacked: no group spans two partitions, and the partition column is never built row by row.
hdb.refs:{[C;s]s:trim s;$[#s;qrefs[C;s];0#`]}                    / column names in a query part
hdb.js:{[l]$[#l;" "/l;""]}                                / join with spaces, "" for none
hdb.ppr:{[pt;pv;cl]pc:pt`pc;m:&/{[pc;pv;c]qfn[,pc;c](,pc)!,pv}[pc;pv]'cl;pv@&m}
hdb.wsplit:{[pt;wh]C:(,pt`pc),pt`cols;cl:$[#wh;qcut wh;()];isp:{[pc;C;c](qrefs[C;c])~,pc}[pt`pc;C]'cl;pv:$[|/isp;hdb.ppr[pt;pt`pv;cl@&isp];pt`pv];r:cl@&~isp;(pv;$[#r;","/r;""])}
hdb.need:{[pt;txt;sp]C:(,pt`pc),pt`cols;cs:$[(0=#sp)|sp~,"*";C;hdb.refs[C;txt]];$[#cs;cs;1#pt`cols]}
qpexec:{[pt;selp;wh]w:hdb.wsplit[pt;wh];(hdb.mat[pt;w 0;hdb.need[pt;selp," ",w 1;trim selp]];w 1)}
qpsel:{[pt;r3;wh;lim]pc:pt`pc;C:(,pc),pt`cols;w:hdb.wsplit[pt;wh];pv:w 0;wh:w 1;bi:$[r3 2;qcut r3 1;()];pb:bi@&~{[pc;b]b~$pc}[pc]'bi;mr:((#pb)<#bi)&(0<#pv)&~in[pc;hdb.refs[C;(r3 0)," ",(hdb.js pb)," ",wh]];$[mr;hdb.mr[pt;pv;r3;pb;wh;lim];qrun[hdb.mat[pt;pv;hdb.need[pt;(r3 0)," ",(r3 1)," ",wh;trim r3 0]];wh;r3;lim]]}
hdb.mr:{[pt;pv;r3;pb;wh;lim]pc:pt`pc;cs:hdb.need[pt;(r3 0)," ",(hdb.js pb)," ",wh;trim r3 0];r4:(r3 0;$[#pb;","/pb;""];0<#pb);rs:{[pt;cs;wh;r4;p]r:qrun[hdb.mat[pt;,p;cs];wh;r4;""];k:$[iskeyed r;keys r;0#`];u:$[iskeyed r;unkey r;r];(k;+((,pt`pc)!,(#u)#,p),+u)}[pt;cs;wh;r4]'pv;k:(*rs)0;qlim[xkey[(,pc),k;,/{x 1}'rs];lim]}

/ loaddb["db"] maps a database the way q's \l does and returns the names it defined: a partitioned table for
/ each table found under the partitions, a splayed table for each directory with a .d, a value for each file.
loaddb:{[db]db:hdb.sl hdb.p db;e:`ldir db;ds:e 0;fs:e 1;pd:ds@&hdb.pok'ds;sd:ds@&~hdb.pok'ds;sd:sd@&{[db;s]-1<`fsz db,"/",s,"/.d"}[db]'sd;fs:fs@&{(~x~"sym")&~"."=*x}'fs;a:{[db;s]set[`$s;dload db,"/",s];`$s}[db]'sd;b:{[db;s]set[`$s;dget db,"/",s];`$s}[db]'fs;c:$[#pd;hdb.ldp[db;pd];0#`];a,b,c}
hdb.ldp:{[db;pd]pc:hdb.pcn[. *pd];tn:?,/{[db;p]d:*`ldir db,"/",p;d@&{[db;p;t]-1<`fsz db,"/",p,"/",t,"/.d"}[db;p]'d}[db]'pd;{[db;pc;pd;tn]ps:pd@&{[db;tn;p]-1<`fsz db,"/",p,"/",tn,"/.d"}[db;tn]'pd;set[`$tn;hdb.pto[db;pc;tn;ps]];`$tn}[db;pc;pd]'tn}

/ ---- the older value-partitioned layout (one table, dir/<value>/, par.txt) ---
mkd:{`x(("/bin/mkdir";"-p";x);"");x}                         / mkdir -p
partsave:{[dir;t;bc]dir:hdb.sl hdb.p dir;g:?(+t)bc;{[dir;t;bc;v]splay[dir,"/",$v;qwhere[t;((+t)bc)=v]]}[dir;t;bc]'g;dset[dir,"/par.txt";g];dir}
partload:{[dir]dir:hdb.sl hdb.p dir;,/{[dir;v]dload dir,"/",$v}[dir]'parts dir}
parts:{[dir]dget (hdb.sl hdb.p dir),"/par.txt"}                    / a database's partition keys
