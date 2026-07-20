/ ============================================================================
/ Amber standard library  -  q/kdb+ functionality implemented on top of ngn/k
/ (c) 2026.  Amber is a derivative of ngn/k (GNU AGPLv3).
/ One definition per line (ngn/k statement = line).  Loaded at startup.
/ ============================================================================

/ ---- scalar / monadic primitives ------------------------------------------
neg:{-x}
not:{~x}
null:{^x}
reciprocal:{1%x}
sqrt:{%x}
floor:{_x}
ceiling:{-_-x}
signum:{(x>0)-x<0}
abs:{x*signum x}
exp:{`exp x}
log:{`ln x}
sin:{`sin x}
cos:{`cos x}
til:{!x}
enlist:{,x}
string:{$x}
type:{@x}
key:{!x}
value:{. x}
first:{*x}
last:{*|x}
reverse:{|x}
distinct:{?x}
group:{=x}
where:{&x}
flip:{+x}
count:{#x}
mod:{y!x}
div:{_x%y}
xbar:{x*_y%x}
xlog:{(`ln y)%`ln x}

/ ---- aggregations ----------------------------------------------------------
sum:{$[0=#x;0;+/x]}
prd:{$[0=#x;1;*/x]}
min:{&/x}
max:{|/x}
avg:{(+/x)%#x}
wsum:{+/x*y}
wavg:{(+/x*y)%+/x}
var:{avg{x*x}x-avg x}
dev:{sqrt var x}
svar:{(n*var x)%-1+n:#x}
sdev:{sqrt svar x}
cov:{avg[x*y]-avg[x]*avg y}
scov:{(n*cov[x;y])%-1+n:#x}
cor:{cov[x;y]%dev[x]*dev y}
med:{a:x@<x;n:#a;m:_n%2;$[2!n;0.0+a m;avg a@m-1 0]}
all:{&/`b$x}
any:{|/`b$x}
sums:{+\x}
prds:{*\x}
mins:{&\x}
maxs:{|\x}
deltas:{x-0,-1_x}
ratios:{x%1,-1_x}
differ:{~~':x}
prev:{-1_0N,x}
next:{1_x,0N}

/ ---- ranking / ordering ----------------------------------------------------
rank:{<<x}
iasc:{<x}
idesc:{>x}
asc:{`sa x@<x}
desc:{x@>x}
xrank:{_y*x%#y:<<y}
xprev:{y[(!#y)-x]}
rotate:{$[0=#y;y;n:(#y)!x;(n_y),n#y]}

/ ---- membership / set ops --------------------------------------------------
in:{~^y?x}
except:{x@&~in[x;y]}
inter:{x@&in[x;y]}
union:{?x,y}
raze:{,/x}
sublist:{n:(abs x)&#y;$[x<0;(0-n)#y;n#y]}
cross:{,/x,/:\:y}

/ ---- table predicates & basics ---------------------------------------------
istable:{`M=@x}
isdict:{`m=@x}
iskeyed:{(`m=@x)&`M=@!x}
cols:{$[iskeyed x;(!!x),!.x;!x]}
keys:{$[iskeyed x;!!x;0#`]}
rows:{+. x}
atr:{[t;i]+(cols t)!{y@x}[i]'. t}
xkey:{[k;t]k:k,();d:+t;(+k#d)!+(except[cols t;k])#d}
unkey:{$[iskeyed x;+(+!x),+.x;x]}
xcolall:{[nm;t]+nm!. t}
xcols:{[c;t]c:c,();+(c,except[cols t;c])#+t}
xasc:{[c;t]c:c,();o:<+(+t)c;t:t@o;d:+t;+d,(1#c)!`sa'd@1#c}
xdesc:{[c;t]c:c,();t@>+(+t)c}
meta:{d:+unkey x;xkey[`c;+`c`t`a!(!d;{@*x}'value d;{`at x}'value d)]}
insert:{[t;r]t,r}

/ ---- joins -----------------------------------------------------------------
lj:{[t;kt]yk:rows[!kt];xk:+. (keys kt)#+t;i:yk?xk;vc:cols value kt;+(+t),vc!{y@x}[i]'value value kt}
ij:{[t;kt]yk:rows[!kt];xk:+. (keys kt)#+t;i:yk?xk;j:&~^i;+(+atr[t;j]),(cols value kt)!{y@x}[i@j]'value value kt}
gc:{[t;n;ot;c]$[in[c;cols t];(+t)c;n#0#(+ot)c]}
uj:{[x;y]c:distinct cols[x],cols y;+c!{[x;y;c]gc[x;#x;y;c],gc[y;#y;x;c]}[x;y]'c}
pj:{[t;kt]yk:rows[!kt];xk:+. (keys kt)#+t;i:(#yk)^yk?xk;vt:value kt;vc:cols vt;+(+t),vc!{[i;t;vt;c]((+t)c)+((vt c),0)@i}[i;t;vt]'vc}
ej:{[c;x;y]ij[x;xkey[c;y]]}

/ ---- asof join (aj/aj0) : y must contain matching + time cols -----------------
ajm:{[c;x;y]c:c,();g:-1_c;tc:*|c;yt:(+y)tc;grp:$[#g;=rows[g#+y];(,0)!,!#y];xr:$[#g;rows[g#+x];(#x)#0];xt:(+x)tc;{[grp;yt;xr;xt;i]gk:xr i;$[~in[gk;!grp];0N;[p:grp gk;b:(yt p)'xt i;$[b<0;0N;p b]]]}[grp;yt;xr;xt]'!#x}
aj:{[c;x;y]c:c,();y:xasc[c;y];m:ajm[c;x;y];vc:except[cols y;c];+(+x),vc!{[m;yd;c](yd c)@m}[m;+y]'vc}
aj0:{[c;x;y]c:c,();y:xasc[c;y];m:ajm[c;x;y];vc:except[cols y;-1_c];+(+x),vc!{[m;yd;c](yd c)@m}[m;+y]'vc}
asof:{[t;d]c:!d;g:-1_c;tc:*|c;td:+t;msk:$[#g;&/({[td;d;c]td[c]=d c}[td;d]'g);(#t)#1b];i:&msk&~td[tc]>d tc;$[#i;(except[cols t;c])#t[*|i];()]}

/ ---- window join wj : w=(begins;ends) aligned to t rows -----------------------
wj:{[w;c;t;q;aggs]c:c,();g:-1_c;tc:*|c;q:xasc[c;q];qt:(+q)tc;grp:$[#g;=rows[g#+q];(,0)!,!#q];tr:$[#g;rows[g#+t];(#t)#0];tt:(+t)tc;res:{[grp;qt;tr;w0;w1;qd;aggs;i]gk:tr i;p:$[in[gk;!grp];grp gk;0#0];p:p@&(~qt[p]<w0 i)&~qt[p]>w1 i;{[qd;p;a](a 1)(qd a 2)@p}[qd;p]'aggs}[grp;qt;tr;w 0;w 1;+q;aggs]'!#t;+(+t),(aggs[;0])!+res}

/ ---- qSQL-style select / group-by --------------------------------------------
qwhere:{[t;m]t@&m}
qselect:{[t;a]+(!a)!{[t;f]f t}[t]'value a}
qby:{[t;b;a]b:b,();g:=rows[b#+t];kt:+b!+!g;subs:atr[t]'value g;vt:+(!a)!{[subs;f]f'subs}[subs]'value a;kt!vt}
xgroup:{[k;t]k:k,();g:=rows[k#+t];kt:+k!+!g;vc:except[cols t;k];+(+kt),vc!{[t;iv;c]((+t)c)@/:iv}[t;value g]'vc}
ungroup:{[x]t:unkey x;d:+t;v:value d;m:{`A=@x}'v;cnt:#:'*v@&m;+(!d)!{[cnt;m;c]$[m;raze c;raze cnt#'c]}[cnt]'[m;v]}
fby:{[fc;g]f:fc 0;d:fc 1;gi:=g;iv:value gi;agg:{[f;d;i]f d@i}[f;d]'iv;@[(#g)#*agg;raze iv;:;raze(#:'iv)#'agg]}

/ ---- strings ---------------------------------------------------------------
lower:{_x}
upper:{c:0+x;`c$c-32*(c>96)&c<123}
ltrim:{(+/&\x=" ")_x}
rtrim:{(0-+/&\|x=" ")_x}
trim:{ltrim rtrim x}
ss:{[s;p]&{[s;p;i]p~(#p)#i_s}[s;p]'!1+(#s)-#p}
ssr:{[s;f;t]t/f\s}
sv:{x/y}
vs:{x\y}
lk1:{[s;p]$[~#p;~#s;"*"=*p;$[#s;lk1[s;1_p]|lk1[1_s;p];lk1[s;1_p]];~#s;0;("?"=*p)|(*p)=*s;lk1[1_s;1_p];0]}
like:{[x;p]$[`C=@x;lk1[x;p];lk1[;p]'x]}

/ ---- table visualization (q-style grids) -----------------------------------
amblk:{[t;c]cl:{[t;x](+t)x}[t]'c;sc:{$x}'cl;h:$c;w:(#:'h)|{$[#x;|/#:'x;0]}'sc;pc:{[w;h;sc;i](,(w i)$h i),(w i)$'sc i}[w;h;sc]'!#c;+pc}
amtab:{[t]lns:{" "/x}'amblk[t;cols t];(1#lns),(,(#*lns)#"-"),1_lns}
amkeyed:{[kt]t:unkey kt;kc:keys kt;vc:except[cols t;kc];kr:{" "/x}'amblk[t;kc];vr:{" "/x}'amblk[t;vc];lns:(kr,\:"| "),'vr;(1#lns),(,(#*lns)#"-"),1_lns}
amdict:{[d]ks:$!d;vs:$. d;kw:|/#:'ks;{[kw;ks;vs;i](kw$ks i),"| ",vs i}[kw;ks;vs]'!#ks}
amfmt:{$[iskeyed x;amkeyed x;`M=@x;amtab x;`m=@x;amdict x;`k x]}
show:{`0:amfmt x;}

repl.amfmt:amfmt
