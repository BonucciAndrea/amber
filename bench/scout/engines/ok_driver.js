#!/usr/bin/env node
// bench/scout/engines/ok_driver.js - host driver for the oK side of the scout matrix.
// Run:  node bench/scout/engines/ok_driver.js bench/scout/engines/ok.k <op> <N> <runs> <warmup>
// Protocol and data model: bench/scout/SCOUT_SPEC.md
//
// oK (github.com/JohnEarnest/ok) is a K interpreter written in JavaScript with
// no clock primitive, so this driver does what its repl.js does for a script
// (bind x to the argument strings, run the file in a fresh base environment)
// and then times the kernel itself: F 0 is parsed once and evaluated by oK's
// own interpreter inside process.hrtime.bigint() (monotonic) brackets.  Data
// generation, setup and printing stay outside the timed region, as in every
// other adapter.  OK_DIR points at the oK checkout (default ~/opt/src/ok).
//
// oK numbers are JavaScript doubles and every list element is a boxed object,
// so memory grows ~50-100 bytes per element: N=1e6 is fine, N=1e7 needs far
// more than node's default heap.
"use strict";
var fs = require("fs"), path = require("path"), os = require("os");
var OKDIR = process.env.OK_DIR || path.join(os.homedir(), "opt", "src", "ok");
var ok = require(path.join(OKDIR, "oK.js"));
var conv = require(path.join(OKDIR, "convert.js"));

var a = process.argv.slice(2);
var script = a[0], op = a[1], n = a[2] || "10000000";
var runs = parseInt(a[3] || "5", 10), warm = parseInt(a[4] || "2", 10);

var env = ok.baseEnv();
env.put("x", true, conv.tok([op, n, String(runs), String(warm)]));
ok.run(ok.parse(fs.readFileSync(script, "utf8")), env);

function get(name) { return env.lookup({ t: 2, v: name }); }
var F = get("F");
if (F.t !== 5) { console.log("SKIP " + op); process.exit(0); }

var call = ok.parse("F 0");
var ans = null;
for (var i = 0; i < warm; i++) ans = ok.run(call, env);
var ts = [];
for (var j = 0; j < runs; j++) {
	var t0 = process.hrtime.bigint();
	ans = ok.run(call, env);
	ts.push(Number(process.hrtime.bigint() - t0) / 1e6);
}
ts.sort(function (p, q) { return p - q; });

console.log("BENCH   " + op);
console.log("CHECK   " + conv.tojs(get("CHK")));
console.log("ANSWER  " + Number(conv.tojs(ans)).toPrecision(17));
console.log("TIME_MS " + ts[Math.floor(runs / 2)].toFixed(6));
