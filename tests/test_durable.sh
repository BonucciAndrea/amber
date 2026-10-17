#!/usr/bin/env bash
# tests/test_durable.sh - on-disk writes survive a crash, a full disk and a failed sync (src/dsk.c, hdb.k).
# A small preloaded library kills the writer at its nth write, fsync or rename, for every n the update makes, then a
# fresh process loads the database: every table must be all old or all new, and nothing half-written may be left.
# The same library fills the disk at chosen sizes and fails fsync. The fault parts need LD_PRELOAD (Linux); the lock,
# truncation and loaddb checks run everywhere.
# GNU AGPLv3 - see LICENSE and NOTICE.
set -u
shopt -s extglob
cd "$(dirname "$0")/.." || exit 1
AMBER=${AMBER:-./amber}
[ -x "$AMBER" ] || { echo "test_durable: $AMBER not built"; exit 1; }
fail=0
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT
ok() { echo "  PASS $1"; }
no() { echo "  FAIL $1: $2"; fail=1; }
DB=$tmp/db
LIBS='\l amber.k
\l sys.k
\l hdb.k'

# the database before (base) and the update: a partition rewritten, a new one, a splayed table rewritten whole, and
# rows appended to another (upsert to a path)
cat > "$tmp/base.k" <<EOF
$LIBS
t:([]sym:\`a\`b\`c\`a;px:1 2 3 4.0)
Q.dpft["$DB";2026.01.01;\`sym;\`t]
splay["$DB/sp";([]sym:\`a\`c;q:10 20)]
splay["$DB/ap";([]sym:\`a\`b;px:1 2.0)]
EOF
cat > "$tmp/upd.k" <<EOF
$LIBS
t:([]sym:\`a\`d\`e;px:5 6 7.0)
Q.dpft["$DB";2026.01.01;\`sym;\`t]
t:([]sym:\`d\`f;px:8 9.0)
Q.dpft["$DB";2026.01.02;\`sym;\`t]
splay["$DB/sp";([]sym:\`f\`g\`a;q:1 2 3;r:\`x\`y\`z)]
(\`\$":$DB/ap/") upsert ([]sym:\`a\`q;px:3 4.0)
EOF
# what a fresh process finds: each table old, new or none, and any hidden file or directory left behind
cat > "$tmp/check.k" <<EOF
$LIBS
db:"$DB"
loaddb db
same:{[a;b](\`j a)~\`j b}
p:{[d]r:ptab[\`t;,d];+\`sym\`px!(r\`sym;r\`px)}
d1:\$[~in[2026.01.01;t\`pv];"none";same[p 2026.01.01;xasc[\`sym;([]sym:\`a\`b\`c\`a;px:1 2 3 4.0)]];"old";same[p 2026.01.01;xasc[\`sym;([]sym:\`a\`d\`e;px:5 6 7.0)]];"new";"BAD"]
d2:\$[~in[2026.01.02;t\`pv];"none";same[p 2026.01.02;xasc[\`sym;([]sym:\`d\`f;px:8 9.0)]];"new";"BAD"]
s:dload db,"/sp"
sp:\$[same[s;([]sym:\`a\`c;q:10 20)];"old";same[s;([]sym:\`f\`g\`a;q:1 2 3;r:\`x\`y\`z)];"new";"BAD"]
a:dload db,"/ap"
ap:\$[same[a;([]sym:\`a\`b;px:1 2.0)];"old";same[a;([]sym:\`a\`b\`a\`q;px:1 2 3 4.0)];"new";"BAD"]
hid:{[d]e:\`ldir d;n:(e 0),e 1;n@&{("."=*x)&~x~".d"}'n}
left:,/hid'(db;db,"/2026.01.01";db,"/2026.01.01/t";db,"/sp";db,"/ap")
\`0:,"day1 ",d1," day2 ",d2," sp ",sp," ap ",ap," left ",(" "/left)
EOF
mkbase() { rm -rf "$DB" "$tmp/.db.amber-tmp"; "$AMBER" "$tmp/base.k" > /dev/null 2>&1; }
check() { "$AMBER" "$tmp/check.k" 2>&1 | tail -1; }

# 0. the plain update, and loaddb finding a splayed table next to the partitions
mkbase
expect_base=$(check)
[ "$expect_base" = "day1 old day2 none sp old ap old left " ] && ok "base database reads back" || no "base" "$expect_base"
"$AMBER" "$tmp/upd.k" > /dev/null 2>&1
got=$(check)
[ "$got" = "day1 new day2 new sp new ap new left " ] && ok "update reads back" || no "update" "$got"
mkbase
AMBER_FSYNC=0 "$AMBER" "$tmp/upd.k" > /dev/null 2>&1
[ "$(check)" = "day1 new day2 new sp new ap new left " ] && ok "AMBER_FSYNC=0 still writes" || no "AMBER_FSYNC=0" "$(check)"

# 1. a truncated column file: an error, never a crash, and the rest still loads
mkbase
printf 'AMBC' > "$DB/sp/q"
printf '%s\n' "$LIBS" "r:@[{dload x};\"$DB/sp\";{\"err \",x}]" '`0:,r' > "$tmp/tr.k"
out=$("$AMBER" "$tmp/tr.k" 2>&1 | tail -1)
case "$out" in *format*) ok "truncated column is 'format";; *) no "truncated column" "[$out]";; esac

# 2. two writers adding symbols to one database at once lose none (the sym file's lock)
rm -rf "$tmp/lk"; mkdir -p "$tmp/lk"
for w in a b; do
  cat > "$tmp/en$w.k" <<EOF
$LIBS
{[i]Q.en["$tmp/lk";([]s:\`\$("$w",/:string 5#i),'string til 5)];i}'til 40
EOF
done
"$AMBER" "$tmp/ena.k" > /dev/null 2>&1 & pa=$!
"$AMBER" "$tmp/enb.k" > /dev/null 2>&1 & pb=$!
wait $pa $pb
printf '%s\n' "$LIBS" "\`0:,string count distinct \`rcol \"$tmp/lk/sym\"" > "$tmp/n.k"
n=$("$AMBER" "$tmp/n.k" 2>&1 | tail -1)
[ "$n" = "400" ] && ok "two writers, one sym file: 400 of 400 names" || no "sym lock" "$n names, wanted 400"

# 3. the fault injector (Linux)
if [ "$(uname -s)" != Linux ] || ! command -v "${CC:-cc}" > /dev/null; then
  echo "  SKIP crash, disk-full and fsync checks: they need LD_PRELOAD and a C compiler"
  [ $fail = 0 ] && exit 0 || exit 1
fi
cat > "$tmp/fi.c" <<'EOF'
#define _GNU_SOURCE
#include <dlfcn.h>
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
/* FI_KILL=n: SIGKILL at the nth write or pwrite (fd > 2), fsync or rename. FI_FULL=b: writes past b bytes fail with ENOSPC.
 * FI_FSYNC=1: fsync fails with EIO. FI_COUNT=file: how many of those calls ran, written there at exit. */
static long ops, killat, full = -1, wrote; static int fsf, init;
static void ini(void) { if (init) return; init = 1; char *e;
  if ((e = getenv("FI_KILL"))) killat = atol(e); if ((e = getenv("FI_FULL"))) full = atol(e); fsf = !!getenv("FI_FSYNC"); }
static void step(void) { ini(); if (__atomic_add_fetch(&ops, 1, __ATOMIC_SEQ_CST) == killat) raise(SIGKILL); }   /* syncs run on several threads */
ssize_t write(int fd, const void *b, size_t n) {
  static ssize_t (*w)(int, const void *, size_t); if (!w) w = (ssize_t (*)(int, const void *, size_t))dlsym(RTLD_NEXT, "write");
  if (fd > 2) { step();
    if (full >= 0) { if (wrote >= full) { errno = ENOSPC; return -1; } if (wrote + (long)n > full) n = (size_t)(full - wrote); }
    ssize_t k = w(fd, b, n); if (k > 0) wrote += k; return k; }
  return w(fd, b, n); }
ssize_t pwrite(int fd, const void *b, size_t n, off_t o) {
  static ssize_t (*w)(int, const void *, size_t, off_t); if (!w) w = (ssize_t (*)(int, const void *, size_t, off_t))dlsym(RTLD_NEXT, "pwrite");
  if (fd > 2) { step(); if (full >= 0 && wrote + (long)n > full) { errno = ENOSPC; return -1; } }
  ssize_t k = w(fd, b, n, o); if (k > 0 && fd > 2) wrote += k; return k; }
int fsync(int fd) { static int (*f)(int); if (!f) f = (int (*)(int))dlsym(RTLD_NEXT, "fsync");
  step(); if (fsf) { errno = EIO; return -1; } return f(fd); }
int rename(const char *a, const char *b) { static int (*r)(const char *, const char *);
  if (!r) r = (int (*)(const char *, const char *))dlsym(RTLD_NEXT, "rename"); step(); return r(a, b); }
__attribute__((destructor)) static void done(void) { char *e = getenv("FI_COUNT"); if (!e) return;
  FILE *f = fopen(e, "w"); if (f) { fprintf(f, "%ld\n", ops); fclose(f); } }
EOF
"${CC:-cc}" -shared -fPIC -O1 -o "$tmp/fi.so" "$tmp/fi.c" -ldl 2>"$tmp/cc.err" || { no "fault injector" "$(head -3 "$tmp/cc.err")"; exit 1; }

# DURABLE_DEBUG=k: kill at call k only, then show the tree and everything the check prints
if [ -n "${DURABLE_DEBUG:-}" ]; then
  mkbase; ( FI_KILL=$DURABLE_DEBUG LD_PRELOAD=$tmp/fi.so "$AMBER" "$tmp/upd.k" > /dev/null 2>&1; true ) 2> /dev/null
  find "$DB" | sort; "$AMBER" "$tmp/check.k"; exit 1
fi

# 3a. killed at every step of the update: each table old or new, nothing left once loaded
mkbase
FI_COUNT=$tmp/count LD_PRELOAD=$tmp/fi.so "$AMBER" "$tmp/upd.k" > /dev/null 2>&1
N=$(cat "$tmp/count" 2>/dev/null || echo 0)
[ "$N" -gt 10 ] || no "fault injector" "counted $N calls"
bad=0; seen=""
for k in $(seq 1 "$N"); do
  mkbase
  ( FI_KILL=$k LD_PRELOAD=$tmp/fi.so "$AMBER" "$tmp/upd.k" > /dev/null 2>&1; true ) 2> /dev/null
  got=$(check)
  case "$got" in
    "day1 "@(old|new)" day2 "@(none|new)" sp "@(old|new)" ap "@(old|new)" left ") seen="$seen|$got";;
    *) no "killed at call $k of $N" "$got"; bad=1;;
  esac
done
[ $bad = 0 ] && ok "killed at each of $N writes, syncs and renames: every table whole, nothing left"
# every table must have been seen both ways, or the kills never reached the renames
case "$seen" in *"day1 old"*) ;; *) no "kill coverage" "day1 never old";; esac
case "$seen" in *"day1 new"*) ;; *) no "kill coverage" "day1 never new";; esac
case "$seen" in *"sp old"*) ;; *) no "kill coverage" "sp never old";; esac
case "$seen" in *"sp new"*) ;; *) no "kill coverage" "sp never new";; esac
case "$seen" in *"ap old"*) ;; *) no "kill coverage" "ap never old";; esac
case "$seen" in *"ap new"*) ;; *) no "kill coverage" "ap never new";; esac

# 3b. a full disk at different points: the write fails, what was there stays
bad=0
for b in 0 100 4096 5000 9000 13000 17000 30000; do
  mkbase
  out=$(FI_FULL=$b LD_PRELOAD=$tmp/fi.so "$AMBER" "$tmp/upd.k" 2>&1)
  got=$(check)
  case "$got" in
    "day1 "@(old|new)" day2 "@(none|new)" sp "@(old|new)" ap "@(old|new)" left ") ;;
    *) no "disk full at $b bytes" "$got"; bad=1;;
  esac
done
[ $bad = 0 ] && ok "disk full at 8 sizes: every table whole, nothing left"
mkbase
FI_FULL=0 LD_PRELOAD=$tmp/fi.so "$AMBER" "$tmp/upd.k" > /dev/null 2>&1
[ "$(check)" = "day1 old day2 none sp old ap old left " ] && ok "disk full from the start: nothing changed" || no "disk full at 0" "$(check)"

# 3c. fsync failing: the write is an error and nothing changes
mkbase
out=$(FI_FSYNC=1 LD_PRELOAD=$tmp/fi.so "$AMBER" "$tmp/upd.k" 2>&1)
case "$out" in *io*) ok "failed fsync is 'io";; *) no "failed fsync" "no 'io in [$out]";; esac
[ "$(check)" = "day1 old day2 none sp old ap old left " ] && ok "failed fsync: nothing changed" || no "failed fsync" "$(check)"

[ $fail = 0 ] && exit 0 || exit 1
