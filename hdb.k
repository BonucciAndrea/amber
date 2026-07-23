/ ============================================================================
/ hdb.k  -  on-disk data for Amber.  Loaded after sys.k.
/   dset / dget         a value <-> a single file
/   splay / dload       a table  <-> a splayed directory (one file per column)
/   partsave / partload a table  <-> a value-partitioned database (dir per key)
/ Values are stored as portable Amber text (`k) and read back with eval, so
/ files are human-readable and version-independent.
/ ============================================================================

mkd:{`x(("/bin/mkdir";"-p";x);"");x}                        / mkdir -p
dset:{[f;x]f 1:`k x;f}                                      / write value x to file f
dget:{. `c$1:x}                                             / read a value back from file x

/ ---- splayed table : directory with one file per column ------------------
splay:{[dir;t]mkd dir;{[dir;t;c](dir,"/",$c)1:`k(+t)c}[dir;t]'cols t;(dir,"/.d")1:`k cols t;dir}
dload:{[dir]cs:. `c$1:dir,"/.d";+cs!{[dir;c]. `c$1:dir,"/",$c}[dir]'cs}

/ ---- value-partitioned database : one splayed dir per partition value ----
partsave:{[dir;t;bc]mkd dir;g:?(+t)bc;{[dir;t;bc;v]splay[dir,"/",$v;qwhere[t;((+t)bc)=v]]}[dir;t;bc]'g;(dir,"/par.txt")1:`k g;dir}
partload:{[dir],/{[dir;v]dload dir,"/",$v}[dir]'. `c$1:dir,"/par.txt"}
parts:{[dir]. `c$1:dir,"/par.txt"}                          / list a database's partition keys
