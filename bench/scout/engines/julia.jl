# bench/scout/engines/julia.jl - Julia side of the scout matrix (Base Julia only).
# Run:  julia -t 1 --startup-file=no bench/scout/engines/julia.jl <op> <N> <runs> <warmup>
# Protocol and data model: bench/scout/SCOUT_SPEC.md
#
# Idiomatic Base Julia, no packages: a Base function where one exists (sum,
# maximum, cumsum, sort, sortperm, unique, indexin, searchsortedlast), and the
# short loop a Julia programmer writes where Base has none (hash group-by over a
# Dict, hash join).  Every kernel is a closure over local data behind a function
# barrier, so it is compiled type-stable; the warm-up passes absorb the JIT.
# Table ops use plain column vectors (like the NumPy adapter): the (sym,px) sort
# is two stable sortperm passes, the as-of join is searchsortedlast on a
# (sym,time) composite key, and qsql_select is the Dict group-by after a mask.
# Julia is 1-based: every index the spec defines 0-based is shifted here.

using Printf, LinearAlgebra
LinearAlgebra.BLAS.set_num_threads(1)

const MOD, MUL = 1048573, 262147
const KJOIN, MJOIN, NT = 1000, 1_000_000, 2_000_000
const MQ, QP, MT = 200_000, 2000, 1_000_000

gen(n) = (MUL .* (0:n-1)) .% MOD                 # Vector{Int64}

function sort_answer(s)
    n = length(s)
    o = s[1] + s[n÷4+1] + s[n÷2+1] + s[(3n)÷4+1] + s[n]
    inv = count(i -> s[i] < s[i-1], 2:n)
    o + 1e9 * inv
end

function lex_answer(sp, rp)
    n = length(rp)
    o = rp[1] + rp[n÷4+1] + rp[n÷2+1] + rp[(3n)÷4+1] + rp[n]
    inv = count(i -> sp[i] < sp[i-1] || (sp[i] == sp[i-1] && rp[i] < rp[i-1]), 2:n)
    o + 1e9 * inv
end

# hash group-sum over arbitrary keys; answer = sum_g (1 + g mod 251) * groupsum[g]
function grp(g, v)
    d = Dict{eltype(g),Float64}()
    @inbounds for i in eachindex(g, v)
        k = g[i]
        d[k] = get(d, k, 0.0) + v[i]
    end
    s = 0.0
    for (k, t) in d
        s += (1 + k % 251) * t
    end
    s
end

function msum(x, w)
    c = cumsum(x)
    m = copy(c)
    @views m[w+1:end] .-= c[1:end-w]
    m
end

function build(op, N)
    h = gen(N)
    a = h .% 1000
    b = h .% 997
    x = Float64.(a)
    y = Float64.(b)
    chk = sum(a) + 3 * sum(b)
    kr = (7919 .* (0:KJOIN-1)) .% MOD
    vr = 2.0 .* (0:KJOIN-1)
    f = nothing
    if op == "sum_f"
        f = () -> sum(x)
    elseif op == "max_f"
        f = () -> maximum(x)
    elseif op == "scan_f"
        f = () -> (s = cumsum(x); s[1] + s[N÷2+1] + s[N])
    elseif op == "dot"
        f = () -> dot(x, y)
    elseif op == "sum_i"
        f = () -> Float64(sum(a))
    elseif op == "arith_mask"
        f = () -> sum(y[i] + 2.5 * x[i] for i in eachindex(x, y) if x[i] > 50.0)
    elseif op == "sort_f"
        f = () -> sort_answer(sort(x))
    elseif op == "sort_presorted"
        f = let p = sort(x)
            () -> sort_answer(sort(p))
        end
    elseif op == "grade_i"
        f = let k = min(1000, N)
            () -> Float64(sum(@view sortperm(a)[1:k]) - k)        # 0-based indices
        end
    elseif op == "find"
        f = let pr = kr[h .% KJOIN .+ 1]
            () -> Float64(sum(indexin(pr, kr)) - length(pr))      # 0-based indices
        end
    elseif op == "member"
        f = () -> Float64(count(in(Set(kr)), h))
    elseif op == "distinct"
        f = () -> (u = unique(a); 1e6 * length(u) + sum(u))
    elseif op == "distinct_100k"
        f = let g5 = h .% 100000
            () -> (u = unique(g5); 1e6 * length(u) + sum(u))
        end
    elseif startswith(op, "group_")
        c = Dict("group_10" => 10, "group_100" => 100, "group_10k" => 10000,
                 "group_100k" => 100000)[op]
        f = let g = h .% c
            () -> grp(g, x)
        end
    elseif op == "join_inner"
        f = let hj = gen(MJOIN), kl = kr[hj .% KJOIN .+ 1], vl = Float64.(hj .% 1000)
            () -> begin
                d = Dict(zip(kr, vr))                    # build side, inside the timer
                s = 0.0
                @inbounds for i in eachindex(kl, vl)
                    v = get(d, kl[i], nothing)
                    v === nothing || (s += vl[i] * v)
                end
                s
            end
        end
    elseif op == "msum_16"
        f = () -> sum(msum(x, 16))
    elseif op == "mavg_256"
        f = () -> sum(msum(x, 256) ./ min.(1:length(x), 256))
    elseif op == "mmax_64"
        f = () -> sum([maximum(@view x[max(1, i - 63):i]) for i in eachindex(x)])
    elseif op == "tablesort"
        f = let ht = gen(NT), sy = ht .% 100, px = Float64.(ht .% 1000)
            () -> begin
                p = sortperm(px)                 # stable LSD: minor key first,
                p = p[sortperm(sy[p])]           # then a stable pass on the major key
                lex_answer(sy[p], px[p])
            end
        end
    elseif op == "qsql_select"
        f = let ht = gen(NT), sy = ht .% 100, px = Float64.(ht .% 1000), sz = ht .% 500
            () -> (m = sz .> 250; grp(sy[m], px[m]))
        end
    elseif op == "asof"
        f = let qj = 0:MQ-1, htr = gen(MT)
            qs = qj .÷ QP
            qt = 1 .+ 500 .* (qj .% QP)
            qb = Float64.((qj .% QP) .% 1000)
            tsy = htr .% 100
            tt = 1000 .+ (0:MT-1)
            cq = (qs .<< 32) .| qt               # (sym,time) lexicographic in one key
            ct = (tsy .<< 32) .| tt
            () -> sum(qb[searchsortedlast.(Ref(cq), ct)])
        end
    end
    f, chk
end

function bench(f, runs, warm)
    ans = 0.0
    for _ in 1:warm
        ans = f()
    end
    ts = Float64[]
    for _ in 1:runs
        t0 = time_ns()
        ans = f()
        push!(ts, (time_ns() - t0) / 1e6)
    end
    Float64(ans), sort(ts)[runs ÷ 2 + 1]
end

function main()
    op = ARGS[1]
    N = length(ARGS) > 1 ? parse(Int, ARGS[2]) : 10_000_000
    runs = length(ARGS) > 2 ? parse(Int, ARGS[3]) : 5
    warm = length(ARGS) > 3 ? parse(Int, ARGS[4]) : 2
    f, chk = build(op, N)
    if f === nothing
        println("SKIP ", op)
        return
    end
    ans, ms = bench(f, runs, warm)
    println("BENCH   ", op)
    println("CHECK   ", chk)
    @printf("ANSWER  %.17g\n", ans)
    @printf("TIME_MS %.6f\n", ms)
end

main()
