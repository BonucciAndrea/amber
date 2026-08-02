█████╗ ███╗   ███╗██████╗ ███████╗██████╗ ██╔══██╗████╗ ████║██╔══██╗██╔════╝██╔══██╗ ███████║██╔████╔██║██████╔╝█████╗  ██████╔╝ ██╔══██║██║╚██╔╝██║██╔══██╗██╔══╝  ██╔══██╗ ██║  ██║██║ ╚═╝ ██║██████╔╝███████╗██║  ██║ ╚═╝  ╚═╝╚═╝     ╚═╝╚═════╝ ╚══════╝╚═╝  ╚═╝ 

## **A low-latency array language — columnar, vectorised, in-memory.** 



Amber is a small, fast, self-contained array language with the working vocabulary of **q/kdb+** — dictionaries, **tables & keyed tables** with ([]…) literal syntax, the full **join family** (left · inner · union · plus · equi · **as-of** · **window** ), qSQL-style select/by, strings, intraday **tick / OHLC** temporals, and **column attributes implemented in C** that turn search from O(n) into O(log n) — **~1000–2000× faster** on large data. 

New in **1.7** : **native temporal types** with literal syntax (2026.07.30, 10:00:05.000, 2026.07.30D09:30:00.000000000) and type-aware arithmetic · a **C-kernel window join** and **ema** · **terminal charting** (plot braille line charts, candle Unicode candlesticks) · and a **zero-dependency Apache Arrow C Data Interface** (arrow.export/arrow.import) for zero-copy interop with PyArrow / Polars / DuckDB. 

t:([]sym:`AAPL`MSFT`AAPL; px:187.3 411.2 187.4; sz:100 250 50)   / a table, rendered instantly 

qby[t; `sym; (,`vwap)!,{wavg[x`sz;x`px]}]                         / vwap by symbol 

# Download & install 

Amber compiles from source (C11) on first run — nothing is installed system‑wide. Clone the repo: 

git clone https://github.com/BonucciAndrea/amber.git 

cd amber 

**Linux / WSL** — needs a C compiler (gcc or clang): 

sudo apt-get update && sudo apt-get install -y build-essential   # one-time 

sudo apt-get install -y rlwrap                                    # optional: history / arrow keys 

chmod +x a build.sh install.sh                                    # restore exec bits if the copy dropped them 

./a                                                               # builds, then opens the REPL 

**macOS** (Intel or Apple Silicon) — needs Apple's clang: 

xcode-select --install        # installs the Command Line Tools (clang); one-time 

brew install rlwrap           # optional: history / arrow keys (needs Homebrew) 

chmod +x a build.sh install.sh 

./a 

That's it — ./a compiles the interpreter (portable -O3, no -march=native) and drops you at the prompt; it recompiles automatically whenever the C sources change, so you never run a stale build. If ./a prints **Permission denied** , the executable bit was lost in transfer — the chmod +x line above fixes it (or just run bash a). 

One‑shot alternative: bash install.sh builds, runs the self‑test, and adds an a alias to your shell rc (~/.zshrc on macOS, ~/.bashrc on Linux). To add the alias by hand: 

echo "alias a='$PWD/a'" >> ~/.zshrc     # macOS (zsh);  use ~/.bashrc on Linux, then: source it 

Nothing is installed system‑wide — see Isolation. 

# A quick taste 

/ tables are first-class and render without `show` 

([]sym:`a`b`c; px:100 200 300) 

/  sym px 

- /  ------- 

- /  a   100 

- /  b   200 

- /  c   300 

meta ([]sym:`a`b; px:1.5 2.5)      / column types + attributes (c | t a) 

/ the join every tick shop needs — as-of 

trade:([]sym:`a`b`a; time:3 4 9; px:100 200 300) 

quote:([]sym:`a`a`b`a; time:1 5 2 8; bid:10 11 20 12) 

aj[`sym`time; trade; quote]        / last quote at/ before each trade 

/ qSQL — type select/exec/update/delete straight, no sel"…" wrapper 

select last px by sym from trade   / grouped aggregate 

select from trade where px>150     / filter rows 

/ 1-minute OHLCV bars (classic tickerplant query) 

tb:+@[+trade; ,`time; minbar[1]@] 

qby[tb; `sym`time; `o`h`l`c`v!({first x`px};{max x`px};{min x`px};{last x`px};{sum x`sz})] 

/ sorted attribute => binary-search lookups 

v:asc 2000000?1000000000                     / `s attribute set by asc 

`at v                                        / `s 

v ? 12345 67890                              / O(log n)  (see bench.k: ~1000x faster) 

/ multi-core: peach runs f over items in parallel worker processes (no GIL) 

peach[{avg x?1.0}; 8#1000000]                / 8 heavy tasks across AMBER_THREADS cores 

Big tables print Q-style — the first CROWS rows (default 20) then ..; set CROWS:10 to shorten. The cap is applied before formatting, so previewing a million-row table is instant. 

Run the guided tours: 

./amber examples/tour.k     # a worked example of EVERY function 

./amber examples/basics.k   # a 2-minute intro 

./amber examples/tick.k     # realistic trades & quotes: as-of/window joins, VWAP, OHLC 

AMBER_THREADS=8 ./amber examples/peach.k   # multi-core speedup demo (serial vs peach) 

./amber bench.k             # attribute speed benchmark 

./amber test.k              # core suite (153); also test-fin.k (35) + test-ext.k (79) = 267 

bash bench/run.sh           # cross-engine sanity + speed (Amber vs numpy/pandas/…; see BENCHMARKS.md) 

# Terminal charts (plot · candle) 

Pipe a query straight into a chart. plot renders a numeric vector as a **Braille** line chart (2×4 dots per cell → 2× horizontal, 4× vertical resolution); candle renders an OHLC table as **Unicode candlesticks** (green up / red down, box-drawing wicks, block bodies). 

plot (14*{sin x%7}'!74;60;9) 

|13.99999|│<br>⢀⠤⠒⠉⠉⠑⠤⡀|⢀⠤⠊⠉⠉⠒⠤⡀|
|---|---|---|
|10.50000|│<br>⡔⠁<br>⠈⠑⢄|⢀⠔⠁<br>⠈⠱⡀|
|7.000004|│<br>⡠⠊<br>⠈⠢⡀|⡠⠃<br>⠈⢢|
|3.500007|│<br>⡰⠁<br>⠑⡄|⢀⠔⠁<br>⠣⡀|
|1.119252|│<br>⠜<br>⠘⢄|⢠⠊<br>⠘⢄|
|-3.49998 │|<br>⠈⠢⡀|⡔⠁<br>⠈⢢|
|-6.99998 │|<br>⠑⡄|⢀⠜<br>⠣⡀|
|-10.4999 │|<br>⠈⠢⡀|⢀⡠⠊<br>⠈⢢|



-13.9999 │                       ⠈⠒⠤⣀⣀⠤⠒⠁ ⠉ 

candle bars[10; select from trades where sym=`AAPL]      / OHLC candlesticks in colour 

See examples/graphs.k for a 13-chart tour (sine, random walk + EMA, logistic map, distributions, price + moving average, rolling volatility, candlesticks). 

# Native temporal types 

Dates, times and timestamps are **first-class types** with literal syntax, auto-display and typeaware arithmetic — no wrappers: 

2026.07.30                          / date         -> 2026.07.30 

10:00:00.000 + 00:00:05.000         / time + time  -> 10:00:05.000 

2026.08.15 - 2026.07.30             / date - date  -> 16   (days) 

2026.07.30D09:30:00.000000000       / timestamp (ns since 2000.01.01) 

year 2026.07.30                     / accessors: year month day dow / thh tmm tss 

"D"$"2026.12.25"                    / string casts: "D"$ "T"$ "P"$ 

Date columns sort with xasc and carry the s# attribute like any other; a time column in a table auto-renders as HH:MM:SS.mmm. 

# Apache Arrow C Data Interface (zero dependency) 

Interop with **PyArrow / Polars / DuckDB** over the stable Arrow C ABI — no libarrow linkage. Export is **zero-copy** (Arrow buffers alias Amber's column payloads; a release callback drops the refcount when the consumer is done): 

p:arrow.export t                    / table  -> (schemaAddr; arrayAddr)  64-bit C-ABI pointers 

arrow.import p                      / (schemaAddr; arrayAddr) -> Amber table 

# Why attributes matter 

bench.k measures ? (find) on identical data, sorted-attributed vs not: 

|**rows**|**linear scan**|**binary ( `s)**|**speedup**|
|---|---|---|---|
|100 k|87 ms|0.6 ms|**141×**|
|500 k|417 ms|0.9 ms|**470×**|
|2 M|1.73 s|1.4 ms|**1244×**|
|5 M|4.23 s|1.9 ms|**2261×**|



Results are identical; only the time differs. asc / xasc set the attribute for you, and meta shows it in the a column. 

# Language notes (30-second version) 

Amber uses a terse array notation. A few things that differ from kdb+/q: 

- **Two-argument library functions take brackets:** aj[c;x;y], lj[t;kt], in[x;y], xasc[sym;t]. Built-in symbols (+ - * % ! & | < > = ~ , ^ # _ $ ? @ .`) are still infix. 

- **No >= / <=** — write ~a<b and ~a>b. 

- **qSQL is bare:** type select … by … from … where … (also exec / update / delete) with no sel"…" wrapper — bare column names like wavg[sz;px] just work. 

- **peach[f;y] is real multi-core** — it forks AMBER_THREADS worker processes (default 4; =1 forces serial), so heavy per-item work scales across cores with no GIL. Identical results to f'y; best for coarse-grained compute (see BENCHMARKS.md §4). 

- **Grids preview Q-style** — show t prints the first CROWS rows (default 20) then .., with a dimmed [N rows x M cols] size footer and **ANSI syntax highlighting** — a vivid 256-colour, 14-hue per-column palette (COLOR:0 to disable). Errors show a ^ caret under the failing token plus a descriptive message ('length: operands have mismatched counts). 

- **Symbols have no _** — use a quoted symbol `"a_b". 

- Tables: ([]col:vals;…); keyed tables: ([key:vals]col:vals). A bare table at the prompt auto-renders as a grid (or show t). 

Full reference: **<u>AMBER.md</u>** . Built-in help: \ then \q \j \z for the Amber vocabulary, `\0 + ' `` for the core. 

# Finance / HFT module (fin.k) 

Auto-loaded after amber.k. Generate a market session and analyse it the way an HFT desk does: 

gentq 100000                       / sets global `trades` and `quotes` (numeric times, `s on time, `p on sym) 

m:aj[`sym`time; trades; quotes]    / TAQ: prevailing quote for every trade 

tsign m                            / Lee-Ready trade sign (+1 buy / -1 sell) 

effspread m                        / effective spread = 2|px-mid| 

qby[trades;`sym; `vwap!enlist {wavg[x`sz;x`px]}]   / VWAP by symbol 

bars[1; trades]                    / 1-minute OHLCV bars 

g:bysym trades                     / O(1) grouped index 

symrows[trades; g; `AAPL]          / all AAPL rows in O(1) 

Included: mid spread spreadbps micro imbal (book), vwap twap tsign signedvol effspread notional (trades), ret logret rvol movavg movsum movmax movmin ema rollstd (returns/vol), bars symstats (aggregation), bysym symrows gidx (O(1) index), pt (time-formatted print). Walkthrough: ./amber examples/hft.k. 

**Attributes.** Amber has all four kdb-style attributes in C:  `sa sorted,  `ua unique,  `pa parted,  `ga grouped ( `at reads them, meta shows them). Sorted/parted give O(log n) kernel find; grouped + the group index give O(1) per-symbol slicing (bench-fin.k ~ 20,000x vs a scan). 

# What's inside 

|**file**||
|---|---|
|a,build.sh|launcher (build-if-stale) and portable compile|
|src/*.c,src/*.h|the interpreter (src/p.ccarries the([]…)|
||table-literal parser;src/ar.cthe Apache|
||Arrow C Data Interface)|
|amber.k|the q/kdb+ vocabulary (auto-loaded)|



|**file**||
|---|---|
|repl.k|the REPL — banner, grid rendering, help|
|fin.k|finance / HFT module (auto-loaded) — see<br>\mhelp|
|std.k qsql.k temporal.k sys.k hdb.k<br>ipc.k tick.k|modules (auto-loaded): moving aggregates +<br>C-kernelema, bare qSQL, native temporal<br>types,.z/.Q/.j/.h+plot/candle+<br>arrow, on-disk, tick,**parallelpeach**|
|examples/|tour.k·basics.k·tick.k·hft.k·<br>peach.k·wj.k·graphs.k· …|
|test.k test-fin.k test-ext.k|267-assertion suite (153 + 35 + 79)|
|bench.k bench-fin.k bench-std.k<br>bench/|attribute / index / window benchmarks; cross-<br>engine harness|
|docs/|AMBER.md(reference) ·MISSING.md<br>(roadmap) ·CHANGELOG.md(history) ·<br>BENCHMARKS.md|
|notebooks/|standalone HTML notebooks|



# Roadmap 

Amber covers a large slice of q. MISSING.md is an honest map of what's next — top picks: a **binary serialiser** ( -8!/ -9!) to replace the text transfer that peach, IPC and the on-disk layer all use; wiring the  `g grouped attribute into the C find path; the missing atom types (short/real/byte/guid); and a true partitioned / memory-mapped HDB beyond the current text splay. 

# Isolation 

Amber is a single self-contained folder. The interpreter is named amber (never k or q), built only inside the folder, never placed on your PATH. It reads/writes no config, no QHOME, no dotfiles. Your kdb+, kona and other k/q installs are untouched; deleting the folder uninstalls Amber completely. 

# Licence 

GNU AGPLv3 (see LICENSE). Amber's interpreter core derives from an AGPLv3 k interpreter; that attribution is preserved in NOTICE, as the licence requires. 

