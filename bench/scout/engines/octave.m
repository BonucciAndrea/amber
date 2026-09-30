1; % bench/scout/engines/octave.m - GNU Octave side of the scout matrix (core tier).
%
% Run:  octave-cli --norc --no-history --quiet bench/scout/engines/octave.m <op> <N> <runs> <warmup>
% Protocol and data model: bench/scout/SCOUT_SPEC.md
%
% Vectorised Octave: sum/max/cumsum/sort, x'*y for the dot product (BLAS ddot;
% OPENBLAS_NUM_THREADS=1 from the runner), ismember for find/member/join (sort +
% binary search), unique + accumarray for group-by (the canonical MATLAB
% group-sum, arbitrary keys), stable sort for grade_i.  a is int64; Octave's
% default sum() of an integer array accumulates in double (exact here).
% Moving windows: msum/mavg are prefix-sum differences (as in the J/BQN/R
% adapters); mmax_64 is 63 vectorised max() passes over lagged copies,
% O(n*w) -- Octave's built-in movmax (m-code movfun) takes 3.6 s at N = 100k,
% so it is not used.  No table layer, so the table ops SKIP.
% Octave is 1-based: every index the spec defines 0-based is shifted here.

function r = sort_answer(s)
  n = numel(s);
  r = s(1) + s(floor(n/4)+1) + s(floor(n/2)+1) + s(floor(3*n/4)+1) + s(n);
  r = r + 1e9 * sum(s(2:end) < s(1:end-1));
end

function r = scan3(x)
  s = cumsum(x);
  n = numel(s);
  r = s(1) + s(floor(n/2)+1) + s(n);
end

function r = grade_sum(a, k)
  [~, p] = sort(a);                 % Octave's sort is stable
  r = sum(p(1:k)) - k;              % 0-based indices
end

function r = find_sum(pr, kr)
  [~, loc] = ismember(pr, kr);
  r = sum(loc) - numel(pr);         % 0-based indices
end

function r = dsum(v)
  u = unique(v);
  r = 1e6 * numel(u) + double(sum(u));
end

function r = grp(g, v)
  [u, ~, j] = unique(g);
  s = accumarray(j, v);
  r = sum((1 + mod(u, 251)) .* s);
end

function r = join_sum(kl, vl, kr, vr)
  [tf, loc] = ismember(kl, kr);
  r = sum(vl(tf) .* vr(loc(tf)));
end

function m = msum(x, w)
  c = cumsum(x);
  m = c - [zeros(w, 1); c(1:end-w)];
end

function m = mmax_lags(x, w)
  m = x;
  for k = 1:w-1
    m = max(m, [-inf(k, 1); x(1:end-k)]);
  end
end

args = argv();
op = args{1};
N = str2double(args{2});
RUNS = str2double(args{3});
WARM = str2double(args{4});

MOD = 1048573; MUL = 262147;
i = (0:N-1)';
h = mod(MUL * i, MOD);              % doubles, exact (< 2^53)
a = int64(mod(h, 1000));
b = int64(mod(h, 997));
x = double(a);
y = double(b);
CHK = sum(double(a)) + 3 * sum(double(b));
kr = mod(7919 * (0:999)', MOD);
vr = 2 * (0:999)';

f = [];
switch op
  case "sum_f",      f = @() sum(x);
  case "max_f",      f = @() max(x);
  case "scan_f",     f = @() scan3(x);
  case "dot",        f = @() x' * y;
  case "sum_i",      f = @() sum(a);
  case "arith_mask", f = @() sum(y(x > 50) + 2.5 * x(x > 50));
  case "sort_f",     f = @() sort_answer(sort(x));
  case "sort_presorted"
    p = sort(x);
    f = @() sort_answer(sort(p));
  case "grade_i"
    k = min(1000, N);
    f = @() grade_sum(a, k);
  case "find"
    pr = kr(mod(h, 1000) + 1);
    f = @() find_sum(pr, kr);
  case "member",     f = @() sum(ismember(h, kr));
  case "distinct",   f = @() dsum(a);
  case "distinct_100k"
    g5 = mod(h, 100000);
    f = @() dsum(g5);
  case {"group_10", "group_100", "group_10k", "group_100k"}
    c = struct("group_10", 10, "group_100", 100, "group_10k", 10000, "group_100k", 100000).(op);
    g = mod(h, c);
    f = @() grp(g, x);
  case "join_inner"
    hj = mod(MUL * (0:999999)', MOD);
    kl = kr(mod(hj, 1000) + 1);
    vl = mod(hj, 1000);
    f = @() join_sum(kl, vl, kr, vr);
  case "msum_16",    f = @() sum(msum(x, 16));
  case "mavg_256",   f = @() sum(msum(x, 256) ./ min((1:N)', 256));
  case "mmax_64",    f = @() sum(mmax_lags(x, 64));
end

if isempty(f)
  printf("SKIP %s\n", op);
else
  ans_ = 0;
  for w = 1:WARM
    ans_ = f();
  end
  ts = zeros(RUNS, 1);
  for r = 1:RUNS
    t0 = tic;
    ans_ = f();
    ts(r) = toc(t0) * 1e3;
  end
  ts = sort(ts);
  printf("BENCH   %s\n", op);
  printf("CHECK   %.0f\n", CHK);
  printf("ANSWER  %.17g\n", double(ans_));
  printf("TIME_MS %.6f\n", ts(floor(RUNS/2) + 1));
end
