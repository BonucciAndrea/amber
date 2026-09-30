# bench/scout/engines/r_engines.R - base R / data.table scout engines.
#
# Run:  R --vanilla --no-echo -f bench/scout/engines/r_engines.R --args <engine> <op> <N> <runs> <warmup>
#       engine in {base, datatable}
# Protocol and data model: bench/scout/SCOUT_SPEC.md
#
# base      : base-R vectors only.  Keys are R integers (32-bit; every key and
#             code fits), values are doubles.  sum_i sums the int32 vector: R
#             accumulates integer sums in 64 bits and returns a double when the
#             result leaves the int range, so it is exact at N = 10M.
#             Group-by is rowsum() (hash grouping via unique/match); find, member
#             and join are match()/%in% (hash); sort/order use R's default radix
#             sort (stable); the as-of join is findInterval() on a (sym,time)
#             composite key, exact in a double; mmax_64 is 63 pmax() passes
#             over lagged copies (O(n*w)), msum/mavg the prefix-sum difference.
# datatable : the data.table idiom for the relational ops only (group-by with
#             GForce sum, X[i] join, rolling join, DT[order()], froll*); the
#             plain vector ops would be the base-R code again, so they SKIP.
#             setDTthreads(1) for the single-thread comparison.
#
# R is 1-based: every index the spec defines 0-based is shifted here.

MOD <- 1048573; MUL <- 262147
KJOIN <- 1000L; MJOIN <- 1000000L; NT <- 2000000L
MQ <- 200000L; QP <- 2000L; MT <- 1000000L

gen <- function(n) as.integer((MUL * (0:(n - 1))) %% MOD)   # exact in double

sort_answer <- function(s) {
  n <- length(s)
  o <- s[1] + s[n %/% 4 + 1] + s[n %/% 2 + 1] + s[(3 * n) %/% 4 + 1] + s[n]
  o + 1e9 * sum(s[-1] < s[-n])
}

lex_answer <- function(sp, rp) {
  n <- length(rp)
  o <- rp[1] + rp[n %/% 4 + 1] + rp[n %/% 2 + 1] + rp[(3 * n) %/% 4 + 1] + rp[n]
  a <- sp[-1]; b <- sp[-n]
  o + 1e9 * sum(a < b | (a == b & rp[-1] < rp[-n]))
}

# arbitrary-key group sum: rowsum() rows follow unique(g) when reorder = FALSE
grp_base <- function(g, v) {
  k <- unique(g)
  s <- rowsum(v, g, reorder = FALSE)
  sum((1 + k %% 251) * s)
}

msum <- function(x, w) {
  c <- cumsum(x)
  c - c(rep(0, w), c[seq_len(length(x) - w)])
}

mmax_lags <- function(x, w) {
  n <- length(x); m <- x
  for (k in seq_len(w - 1)) m <- pmax(m, c(rep(-Inf, k), x[seq_len(n - k)]))
  m
}

GROUPS <- c(group_10 = 10L, group_100 = 100L, group_10k = 10000L, group_100k = 100000L)

base_data <- function(N) {
  h <- gen(N); a <- h %% 1000L; b <- h %% 997L
  list(h = h, a = a, b = b, x = as.numeric(a), y = as.numeric(b),
       chk = sum(as.numeric(a)) + 3 * sum(as.numeric(b)),
       kr = as.integer((7919 * (0:(KJOIN - 1L))) %% MOD), vr = 2 * (0:(KJOIN - 1L)))
}

join_left <- function(kr) {
  hj <- gen(MJOIN)
  list(kl = kr[hj %% KJOIN + 1L], vl = as.numeric(hj %% 1000L))
}

table_data <- function() {
  ht <- gen(NT)
  list(sym = ht %% 100L, px = as.numeric(ht %% 1000L), sz = ht %% 500L)
}

asof_data <- function() {
  qj <- 0:(MQ - 1L); htr <- gen(MT)
  list(qs = qj %/% QP, qt = 1L + 500L * (qj %% QP), qb = as.numeric((qj %% QP) %% 1000L),
       ts = htr %% 100L, tt = 1000L + 0:(MT - 1L))
}

# ================================================================== base R
build_base <- function(op, N) {
  d <- base_data(N)
  h <- d$h; a <- d$a; x <- d$x; y <- d$y; kr <- d$kr; vr <- d$vr
  f <- NULL
  if (op == "sum_f") f <- function() sum(x)
  else if (op == "max_f") f <- function() max(x)
  else if (op == "scan_f") f <- function() { s <- cumsum(x); s[1] + s[N %/% 2 + 1] + s[N] }
  else if (op == "dot") f <- function() sum(x * y)
  else if (op == "sum_i") f <- function() as.numeric(sum(a))
  else if (op == "arith_mask") f <- function() sum((y + 2.5 * x)[x > 50])
  else if (op == "sort_f") f <- function() sort_answer(sort(x))
  else if (op == "sort_presorted") { p <- sort(x); f <- function() sort_answer(sort(p)) }
  else if (op == "grade_i") { k <- min(1000L, N); f <- function() as.numeric(sum(order(a)[1:k]) - k) }
  else if (op == "find") {
    pr <- kr[h %% KJOIN + 1L]
    f <- function() as.numeric(sum(match(pr, kr))) - length(pr)
  }
  else if (op == "member") f <- function() as.numeric(sum(h %in% kr))
  else if (op == "distinct") f <- function() { u <- unique(a); 1e6 * length(u) + sum(u) }
  else if (op == "distinct_100k") {
    g5 <- h %% 100000L
    f <- function() { u <- unique(g5); 1e6 * length(u) + sum(u) }
  }
  else if (op %in% names(GROUPS)) { g <- h %% GROUPS[[op]]; f <- function() grp_base(g, x) }
  else if (op == "join_inner") {
    l <- join_left(kr); kl <- l$kl; vl <- l$vl
    f <- function() { idx <- match(kl, kr); m <- !is.na(idx); sum(vl[m] * vr[idx[m]]) }
  }
  else if (op == "msum_16") f <- function() sum(msum(x, 16L))
  else if (op == "mavg_256") f <- function() sum(msum(x, 256L) / pmin(seq_along(x), 256L))
  else if (op == "mmax_64") f <- function() sum(mmax_lags(x, 64L))
  else if (op == "tablesort") {
    t <- table_data(); sy <- t$sym; px <- t$px
    f <- function() { o <- order(sy, px); lex_answer(sy[o], px[o]) }
  }
  else if (op == "qsql_select") {
    t <- table_data(); sy <- t$sym; px <- t$px; sz <- t$sz
    f <- function() { m <- sz > 250L; grp_base(sy[m], px[m]) }
  }
  else if (op == "asof") {
    q <- asof_data()
    cq <- q$qs * 2^32 + q$qt          # (sym,time) lexicographic in one exact double key
    ct <- q$ts * 2^32 + q$tt
    qb <- q$qb
    f <- function() sum(qb[findInterval(ct, cq)])
  }
  list(f = f, chk = d$chk)
}

# ================================================================== data.table
build_datatable <- function(op, N) {
  suppressPackageStartupMessages(library(data.table))
  setDTthreads(1L)
  d <- base_data(N)
  h <- d$h; a <- d$a; x <- d$x; kr <- d$kr; vr <- d$vr
  f <- NULL
  if (op == "distinct") {
    dt <- data.table(a = a)
    f <- function() { u <- unique(dt)$a; 1e6 * length(u) + sum(u) }
  }
  else if (op == "distinct_100k") {
    dt <- data.table(g = h %% 100000L)
    f <- function() { u <- unique(dt)$g; 1e6 * length(u) + sum(u) }
  }
  else if (op %in% names(GROUPS)) {
    dt <- data.table(g = h %% GROUPS[[op]], x = x)
    f <- function() { r <- dt[, .(s = sum(x)), by = g]; sum((1 + r$g %% 251) * r$s) }
  }
  else if (op == "join_inner") {
    l <- join_left(kr)
    L <- data.table(k = l$kl, vl = l$vl); R <- data.table(k = kr, vr = vr)
    f <- function() { m <- L[R, on = "k", nomatch = NULL]; sum(m$vl * m$vr) }
  }
  else if (op == "msum_16") f <- function() sum(frollsum(x, 16L, partial = TRUE))
  else if (op == "mavg_256") f <- function() sum(frollmean(x, 256L, partial = TRUE))
  else if (op == "mmax_64") f <- function() sum(frollmax(x, 64L, partial = TRUE))
  else if (op == "tablesort") {
    t <- table_data(); DT <- data.table(sym = t$sym, px = t$px)
    f <- function() { r <- DT[order(sym, px)]; lex_answer(r$sym, r$px) }
  }
  else if (op == "qsql_select") {
    t <- table_data(); DT <- data.table(sym = t$sym, px = t$px, sz = t$sz)
    f <- function() { r <- DT[sz > 250L, .(s = sum(px)), by = sym]; sum((1 + r$sym %% 251) * r$s) }
  }
  else if (op == "asof") {
    q <- asof_data()
    Q <- data.table(sym = q$qs, time = q$qt, bid = q$qb)
    setkey(Q, sym, time)              # already in (sym,time) order: this only marks it
    Tr <- data.table(sym = q$ts, time = q$tt)
    f <- function() sum(Q[Tr, on = .(sym, time), roll = TRUE]$bid)
  }
  list(f = f, chk = d$chk)
}

# ================================================================== driver
args <- commandArgs(trailingOnly = TRUE)
engine <- args[1]; op <- args[2]
N <- if (length(args) > 2) as.integer(args[3]) else 10000000L
runs <- if (length(args) > 3) as.integer(args[4]) else 5L
warm <- if (length(args) > 4) as.integer(args[5]) else 2L

b <- switch(engine, base = build_base(op, N), datatable = build_datatable(op, N))
if (is.null(b) || is.null(b$f)) {
  cat(sprintf("SKIP %s\n", op))
} else {
  f <- b$f
  ans <- 0
  for (w in seq_len(warm)) ans <- f()
  ts <- numeric(runs)
  for (r in seq_len(runs)) {
    t0 <- as.numeric(Sys.time())
    ans <- f()
    ts[r] <- (as.numeric(Sys.time()) - t0) * 1000
  }
  cat(sprintf("BENCH   %s\n", op))
  cat(sprintf("CHECK   %.0f\n", b$chk))
  cat(sprintf("ANSWER  %.17g\n", as.numeric(ans)))
  cat(sprintf("TIME_MS %.6f\n", sort(ts)[runs %/% 2 + 1]))
}
