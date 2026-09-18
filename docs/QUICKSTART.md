# Quickstart — the long version

The demo, the full install recipe, a worked code tour and every command worth
knowing. The README keeps a short version of each; this is the whole thing.

Moved out of [`README.md`](../README.md) so the landing page stays a landing page.
Nothing here changed in the move.

---

<a name="quick-showcase"></a>
## Quick showcase

One command builds Amber with maximum optimization and runs the full **Mega Demo**: a
5,000,000-row HFT tick session (VWAP, a tacit 50-period EMA, a native as-of join), a
10,000,000-element vector-op benchmark (scalar vs native vs `peach`), a couple of `\ast`/
`\disasm` samples straight from the real parser/compiler, and pointers to the in-browser
notebook.

```sh
./demo.sh                    # portable -O3 build
AMBER_NATIVE=1 ./demo.sh     # -march=native build (fastest on this machine)
```

`demo/hft_demo.k` — a realistic multi-symbol tick session end to end:

```q
\l amber.k
\l fin.k
N:1000000                                                  / trade count (gentq builds N trades + 2N quotes)
gentq N                                                     / sets globals `trades` and `quotes`, realistic microstructure
qby[trades; `sym; (,`vwap)!,{round[4]wavg[x`sz;x`px]}]      / VWAP per symbol
ema[2%51; trades`px]                                        / 50-period EMA, tacit call into the C kernel
taq[trades; quotes]                                         / as-of join: every trade -> its nearest quote
```

<details>
<summary>Sample benchmark output from a real run (timings vary by machine)</summary>

`demo/hft_demo.k` prints its own summary table every run:

```
== benchmark summary =======================================
stage      ms
----------
gentq   2381.9
vwap     151.2
ema        0.1
asof     704.0
------------------------------------------------------------
total: ~3.2 s end-to-end for 500,000 trades
```

`demo/bench_showcase.k` — the same 10,000,000-element vector three ways, with a printed
speedup table:

```
op   method   ms
--------------
add  scalar   ███████████████████░  (per-element `'` each)
add  vector   ██░                   (native C kernel, auto-SIMD)   ~2-9x faster
mul  scalar   ██████████████░
mul  vector   █░                                                    ~2-9x faster
sum  serial   ░
sum  peach    ███░   (fork/IPC overhead dominates a cheap reduction --
                       peach earns its keep on per-task-heavy work, see examples/peach.k)
```

</details>

Prefer to explore interactively? Open `notebooks/Amber-Notebook-Studio.html` directly in a
browser (no build step) and hit **🚀 Load HFT Demo** in the header — it generates a tick
session, charts price & volume on a canvas, and benchmarks a naive per-symbol filter against
the vectorised `qby` call, right there in the page.

---


<a name="download--install"></a>
## Download & install

Amber compiles from source (portable **C99**, builds clean on `gcc` and `clang`) on first run —
nothing is installed system-wide. Clone the repo:

```sh
git clone https://github.com/BonucciAndrea/amber.git
cd amber
```

**Linux / WSL** — needs a C compiler (`gcc` or `clang`):

```sh
sudo apt-get update && sudo apt-get install -y build-essential   # one-time
chmod +x a build.sh install.sh                                    # restore exec bits if the copy dropped them
./a                                                               # builds, then opens the REPL
# note: do NOT install or use rlwrap for Amber -- line editing is built in
```

**macOS** (Intel or Apple Silicon) — needs Apple's `clang`:

```sh
xcode-select --install        # installs the Command Line Tools (clang); one-time
chmod +x a build.sh install.sh
./a                           # line editing / history / Tab are built in -- no rlwrap
```

That's it — `./a` compiles the interpreter (portable `-O3`, no `-march=native`) and drops you at
the prompt; it recompiles automatically whenever the C sources change, so you never run a stale
build. If `./a` prints **`Permission denied`**, the executable bit was lost in transfer — the
`chmod +x` line above fixes it (or just run `bash a`).

**Machine-tuned build.** The default build is portable C99 and always includes `-pthread` (needed
by the multithreaded vector engine) and the scalar SIMD fallback. For a build that turns on
**AVX2** (x86_64) or **NEON** (Apple Silicon / any `aarch64`) vector kernels, set `AMBER_NATIVE=1`:

```sh
AMBER_NATIVE=1 ./build.sh      # machine-tuned; check with: `simd 0
```

`` `simd 0 `` prints which backend actually got selected (`scalar` / `avx2` / `neon`) to stderr.
NEON activates unconditionally on Apple Silicon regardless of `AMBER_NATIVE`, since `aarch64`
implies it. The tuning flag is **probed, not assumed**: `-march=native` is x86 syntax that Apple
clang rejects on Apple Silicon, so `build.sh` falls back to `-mcpu=native` (aarch64) and, failing
both, to a portable build — so `AMBER_NATIVE=1 ./build.sh` succeeds on every platform rather than
breaking CI on arm64 runners.

**One command instead of all of the above:**

```sh
./install.sh                 # or: bash install.sh   (if the +x bit was lost)
AMBER_NATIVE=1 ./install.sh  # ... with a machine-tuned build
```

`install.sh` checks you have a C compiler (and prints the exact package command for your distro
if you do not), repairs the executable bit on every script, builds, runs the self-test, and writes
the shell block below into the rc file **your login shell actually reads** — `~/.zshrc` for zsh,
`~/.bash_profile` on macOS bash, `~/.bashrc` on Linux bash, `~/.profile` otherwise. Re-running it
replaces that block rather than appending a second copy.

<details>
<summary>The shell block, to add by hand</summary>

<a name="shell-configuration"></a>
The full shell-rc recipe -- `AMBER_HOME`, the `amber` / `amberx` / `amber-ai`
aliases, thread pinning, and why you symlink the launcher rather than put the
repo root on `PATH` -- is in [`docs/INSTALL.md`](docs/INSTALL.md).

</details>

Three notes on that block, because the obvious-looking variants do not work:

| pitfall | why |
|---|---|
| **`$AMBER_HOME` is the checkout, not a prefix** | Amber has no `bin/`, `lib/` or `share/` split and installs nothing outside its folder. `export PATH="$AMBER_HOME/bin:$PATH"` points at a directory that does not exist. |
| **Alias the launcher `a`, not the binary `amber`** | The bare `amber` binary is the interpreter with **no** stdlib: `amber` alone gives you a REPL where `select`, `aj` and `sum` are undefined. `./a` loads `repl.k`, which loads everything else. |
| **`AMBER_NATIVE` is a *build*-time variable** | It is read by `build.sh`, not by the interpreter. It belongs on the `a` alias — which may rebuild — and does nothing on `amberx`. There is no `AMBER_MEM_MB`: the heap is `mmap`'d with `MAP_NORESERVE` and sized lazily by the OS, so there is nothing to tune. |

The variables the engine itself reads at run time are exactly: `AMBER_THREADS` (vector-engine
lanes), `AMBER_DIAG` (rich diagnostics on/off), `AMBER_NO_EDIT` and `AMBER_RLWRAP` (line editor),
plus `AMBER_AI_*` once the [amber-ai](https://github.com/bonucciandrea/amber-ai) extension is
installed. Nothing is installed system-wide — see [Isolation](#isolation).

---


<a name="a-quick-taste"></a>
## A quick taste

> These snippets are written as you'd type them at the interactive prompt (`./a`), where a bare
> table auto-renders as a grid and qSQL sugar (`select … by … from … where …`) works directly on
> the input line. **Since 2.0.0 bare qSQL also works inside a `.k` script** run via `./amber
> file.k` — the loader runs each file through the same rewriter the REPL uses — so the `sel"…"`
> wrapper is no longer required in files (it still works). For the grid view of a bare table inside
> a script, wrap it in `show` (`show t`).

```q
/ tables are first-class and render without `show` -- at the interactive prompt
([]sym:`a`b`c; px:100 200 300)
/  sym px
/  -------
/  a   100
/  b   200
/  c   300

meta ([]sym:`a`b; px:1.5 2.5)      / column types + attributes (c | t a)

/ the join every tick shop needs — as-of (native C kernel)
trade:([]sym:`a`b`a; time:3 4 9; px:100 200 300; sz:10 20 30)
quote:([]sym:`a`a`b`a; time:1 5 2 8; bid:10 11 20 12)
aj[`sym`time; trade; quote]        / last quote at/ before each trade

/ qSQL — type select/exec/update/delete straight, no sel"…" wrapper
select last px by sym from trade   / grouped aggregate
select from trade where px>150     / filter rows

/ two-argument library dyads work infix now, identical to the bracket form
2 3 9 in 2 3 4                     / 1 1 0        (same as in[2 3 9;2 3 4])
`sym xasc trade                    / sort by sym  (same as xasc[`sym;trade])

/ 1-minute OHLCV bars (classic tickerplant query)
tb:+@[+trade; ,`time; minbar[1]@]
qby[tb; `sym`time; `o`h`l`c`v!({first x`px};{max x`px};{min x`px};{last x`px};{sum x`sz})]

/ sorted attribute => binary-search lookups
v:asc 2000000?1000000000                     / `s attribute set by asc
`at v                                        / `s
v ? 12345 67890                              / O(log n)  (see bench.k: ~1000x faster)

/ binary serialization: -8! encodes any value to bytes, -9! decodes it back
b:-8!+`a`b!(1 2 3;4 5 6)     / table -> compact byte vector
(-9!b)~+`a`b!(1 2 3;4 5 6)   / 1b  -- exact, attributes and nulls included

/ multi-core: peach runs f over items in parallel worker processes (no GIL)
peach[{avg x?1.0}; 8#1000000]                / 8 heavy tasks across AMBER_THREADS cores
```

Big tables print Q-style — the first `CROWS` rows (default 20) then `..`; set `CROWS:10`
to shorten. The cap is applied before formatting, so previewing a million-row table is
instant.

Check the interpreter version, or list every option and REPL command:

```sh
./amber --version           # amber 2.1.0
./amber --help              # options + the full \-command reference
```

Run the guided tours:

```sh
./amber examples/tour.k     # a worked example of EVERY function
./amber examples/basics.k   # a 2-minute intro
./amber examples/tick.k     # realistic trades & quotes: as-of/window joins, VWAP, OHLC
AMBER_THREADS=8 ./amber examples/peach.k   # multi-core speedup demo (serial vs peach)
./amber bench.k             # attribute speed benchmark
./amber test.k              # core suite (202); also test-fin.k (35) + test-ext.k (79)
tests/run_tests.sh          # EVERYTHING: build + all K suites + C unit tests + fuzz pass
./amber tests/test_matrix.k # the suites are self-locating - run them from any directory
tests/run_tests.sh --asan   # ... and re-run it all under AddressSanitizer + UBSan
bash bench/run.sh           # cross-engine sanity + speed (Amber vs numpy/pandas/…; see BENCHMARKS.md)
python3 bench/scout/scout.py     # the widest run: 23 ops x 13 engines (PeachQ, ngn/k,
                                 #   CBQN, J, NumPy, pandas, Polars, DuckDB, a C reference)
python3 bench/scout/webgen.py --md   # regenerate the tables in docs/BENCHMARKS.md from results.json
```

---


<a name="terminal-charts"></a>
## Terminal charts (`chart` · `plot` · `plots` · `candle`)

Pipe a query straight into a chart. Charts are framed and axis-labelled, take several series
at once, and downsample a million-point series without turning it into a smear. The surface is
**Braille** — a 2×4 dot bitmask per character cell, so a `W`×`H` box is a `2W`×`4H` raster.

```
chart `y`title`ylabel!(14*{sin x%7}@!74;"AAPL mid";"price")

                               AAPL mid
price
    ┌────────────────────────────────────────────────────────────────┐
    │      ⣀⠔⠒⠒⠤⡀                             ⣀⠔⠒⠒⢄⡀                 │
 10 ┤⠄   ⡠⠊  ⠄  ⠈⠑⢄  ⠄   ⠄   ⠄   ⠄   ⠄   ⠄  ⡰⠉   ⠄ ⠈⠑⢄   ⠄   ⠄   ⠄   │
    │  ⢀⠎          ⠑⡄                     ⢀⠎          ⠣⡀             │
    │ ⡰⠁            ⠈⢢                  ⢀⠔⠁            ⠈⢢            │
  0 ┤⠜   ⠄   ⠄   ⠄   ⠄⠣⡀ ⠄   ⠄   ⠄   ⠄ ⢠⠊⠄   ⠄   ⠄   ⠄   ⠣⡀  ⠄   ⠄   │
    │                  ⠈⢆             ⡠⠃                  ⠑⢄         │
    │                    ⠱⡀         ⢀⠜                      ⠱⡀       │
-10 ┤⠂   ⠂   ⠂   ⠂   ⠂   ⠂⠈⢆⡀⠂   ⠂⣀⠔⠁⠂   ⠂   ⠂   ⠂   ⠂   ⠂   ⠈⢆⡀ ⠂   │
    │                       ⠈⠑⠤⠤⠔⠊                                   │
    └┬───────────────┬───────────────┬──────────────┬───────────────┬┘
     0              20              40             60              80
```

| verb | draws |
|---|---|
| `chart d` | anything below, from an option dictionary |
| `plot v` · `plot (v;W;H)` | one line series |
| `plots (a;b;c)` · ``plots `a`b!(x;y)`` | several series, with a legend |
| `xyplot (xs;ys)` · `scatter (xs;ys)` | y against a real x axis; points |
| `step v` · `area v` | piecewise-constant; filled |
| `hist v` · ``barh `a`b!3 7`` · `heat m` | distribution; labelled bars; matrix |
| `spark v` | **returns** a one-line string, to embed in a row of text |
| `candle t` | OHLC candlesticks, green up / red down |

`chart` takes `y x w h ylim xlim grid axis legend colour title xlabel ylabel names col style`.
Two details do most of the work: the axis range is the data's own range **snapped outward to a
1/2/5 boundary** (round tick values, without the wasted margin a coarse step would leave), and
past roughly `4W` points each pixel column is drawn as its **min→max envelope** rather than by
joining consecutive points — which keeps every spike instead of filling the box in solid.

```
plots `bid`ask!(q`bid;q`ask)                              / two series and a legend
chart `y`ylim`title!(px;98 102;"AAPL")                    / pin the axis to zoom
chart `y`x`names!((px;ma);(!#px;50+!#ma);("px";"MA(50)")) / per-series x, so a shorter
                                                          / moving average still lines up
candle bars[10; select from trades where sym=`AAPL]
```

See [`examples/graphs.k`](examples/graphs.k) for a 31-chart tour and
[docs/AMBER.md §9c](docs/AMBER.md) for the full option table.

