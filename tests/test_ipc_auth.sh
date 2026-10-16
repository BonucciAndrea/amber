#!/usr/bin/env bash
# tests/test_ipc_auth.sh - who may connect to \p: the bind (this machine unless a host is given), .z.pw, -u/-U files,
# and the one-line warning for a port open past this machine with neither. Real servers and clients, both Amber.
# GNU AGPLv3 - see LICENSE and NOTICE.
set -u
cd "$(dirname "$0")/.." || exit 1
AMBER=${AMBER:-./amber}
[ -x "$AMBER" ] || { echo "test_ipc_auth: $AMBER not built"; exit 1; }
fail=0
tmp=$(mktemp -d)
pids=()
trap 'for p in "${pids[@]}"; do kill "$p" 2>/dev/null; done; rm -rf "$tmp"' EXIT
P=$(( 20000 + $$ % 20000 ))

ok() { echo "  PASS $1"; }
no() { echo "  FAIL $1: $2"; fail=1; }

# serve <script text> <args...>: start a server on the next port, wait until it listens; $P is its port
serve() {
  P=$((P + 1)); printf '%s\n' "$1" > "$tmp/s$P.q"; shift
  "$AMBER" "$tmp/s$P.q" "$@" -p "${HOSTP:-}$P" > "$tmp/out$P" 2> "$tmp/err$P" < /dev/null &
  pids+=($!)
  for _ in $(seq 100); do (exec 3<>"/dev/tcp/127.0.0.1/$P") 2>/dev/null && return; sleep 0.1; done
  no "server on $P" "never listened"
}
# ext_ip: an address of this machine past loopback, empty if it has none
ext_ip() { { hostname -I 2>/dev/null; ipconfig getifaddr en0 2>/dev/null; } | tr ' ' '\n' | grep -v '^127\.' | grep -m1 '^[0-9][0-9.]*$'; }
# in10 <cmd...>: run it, killed after 10 s (macOS has no timeout, perl's alarm does the same)
in10() { if command -v timeout > /dev/null; then timeout 10 "$@"; else perl -e 'alarm 10; exec @ARGV' "$@"; fi; }
# try <handle> : what a client gets back, "ok 42" or the error
try() {
  printf '%s\n' "r:@[{h:hopen x;v:h\"a\";hclose h;\"ok \",string v};$1;{\"err \",x}]" '-1 r;' 'exit 0' > "$tmp/c.q"
  in10 "$AMBER" "$tmp/c.q" 2>&1 < /dev/null | tail -1
}
expect() { # name got want
  if [ "$2" = "$3" ]; then ok "$1"; else no "$1" "got [$2], wanted [$3]"; fi
}

# 1. no -u/-U, no .z.pw: anyone on this machine gets in, and the port is bound to 127.0.0.1 only
serve 'a:42'
expect "open port, no checks" "$(try $P)" "ok 42"
# not there from another address of this machine (WSL1 has no /proc/net/tcp to read the bind from)
X=$(ext_ip)
if [ -z "$X" ]; then echo "  SKIP bound to 127.0.0.1 by default: no address past loopback here"
elif (exec 3<>"/dev/tcp/$X/$P") 2>/dev/null; then no "default bind" "$X:$P took a connection"
else ok "bound to 127.0.0.1 by default"; fi
[ -s "$tmp/err$P" ] && no "no warning on loopback" "$(cat "$tmp/err$P")" || ok "no warning on loopback"

# 2. .z.pw decides
serve '.z.pw:{[u;p] p~"secret"}
a:42'
expect ".z.pw: right password" "$(try "\`::$P:bob:secret")" "ok 42"
expect ".z.pw: wrong password" "$(try "\`::$P:bob:nope")" "err access"
expect ".z.pw: no password" "$(try $P)" "err access"
serve '.z.pw:{[u;p] 0b}
a:42'
expect ".z.pw saying no to all" "$(try "\`::$P:bob:secret")" "err access"
serve '.z.pw:{[u;p] u~`alice}
a:42'
expect ".z.pw: the user as a symbol" "$(try "\`::$P:alice:x")" "ok 42"

# 3. -U file: plain passwords, and md5 ones as q's files have them
printf 'alice:pw1\nbob:6d5779b9b85bd4f11e44c9772e0de602\n' > "$tmp/users"   # bob's is the md5 of pw2
serve 'a:42' -U "$tmp/users"
expect "-U: plain password" "$(try "\`::$P:alice:pw1")" "ok 42"
expect "-U: md5 password" "$(try "\`::$P:bob:pw2")" "ok 42"
expect "-U: wrong password" "$(try "\`::$P:alice:pw2")" "err access"
expect "-U: unknown user" "$(try "\`::$P:carol:pw1")" "err access"
serve 'a:42' -u "$tmp/users"
expect "-u: plain password" "$(try "\`::$P:alice:pw1")" "ok 42"
serve '.z.pw:{[u;p] u~`bob}
a:42' -U "$tmp/users"
expect "-U and .z.pw: both have to say yes" "$(try "\`::$P:alice:pw1")" "err access"
expect "-U and .z.pw: both say yes" "$(try "\`::$P:bob:pw2")" "ok 42"
if "$AMBER" "$tmp/s$P.q" -U "$tmp/nofile" > /dev/null 2>&1 < /dev/null; then no "-U with no such file" "started"
else ok "-U with no such file stops"; fi

# 4. a host opens the port past this machine: one warning line, unless .z.pw or a file is set
HOSTP=0.0.0.0: serve 'a:42'
sleep 0.3
grep -q "open past this machine" "$tmp/err$P" && ok "warning with nothing set" || no "warning" "none"
[ "$(grep -c "open past" "$tmp/err$P")" = 1 ] && ok "warning given once" || no "warning once" "$(cat "$tmp/err$P")"
expect "0.0.0.0 still serves this machine" "$(try $P)" "ok 42"
HOSTP=0.0.0.0: serve '.z.pw:{[u;p] 1b}
a:42'
sleep 0.3
[ -s "$tmp/err$P" ] && no "no warning with .z.pw" "$(cat "$tmp/err$P")" || ok "no warning with .z.pw"
HOSTP=0.0.0.0: serve 'a:42' -U "$tmp/users"
sleep 0.3
[ -s "$tmp/err$P" ] && no "no warning with -U" "$(cat "$tmp/err$P")" || ok "no warning with -U"

exit $fail
