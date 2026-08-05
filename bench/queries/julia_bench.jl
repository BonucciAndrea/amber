#!/usr/bin/env julia
# Julia implementation of bench/SPEC.md. GNU AGPLv3 - see LICENSE and NOTICE.
# Run: julia --startup-file=no julia_bench.jl <arith|reduce|groupby|join> [runs] [warmup]
#
# Julia JIT-compiles on first call, so the warm-up passes below are essential:
# they force specialisation of every kernel before the clock starts (SPEC.md
# §4.3). Each kernel returns its scalar and the driver stores it, so nothing is
# dead-code-eliminated. Data generation is outside the timed region.
using Statistics

const N, M, K, G = 10_000_000, 1_000_000, 1_000, 100

const h  = [ (262147 * i) % 1048573 for i in 0:N-1 ]
const a  = h .% 1000
const b  = h .% 997
const x  = Float64.(a)
const y  = Float64.(b)
const gk = a .% G

const kr = [ (7919 * j) % 1048573 for j in 0:K-1 ]
const vr = [ 2.0 * j for j in 0:K-1 ]
const kl = kr[(h[1:M] .% K) .+ 1]
const vl = x[1:M]

# Sorted right key column for the join lookup, built outside the timer.
const ord     = sortperm(kr)
const kr_sort = kr[ord]
const vr_sort = vr[ord]

const CHECK = sum(a) + 3 * sum(b)

function k_arith()
    s = 0.0
    @inbounds for i in 1:N
        if x[i] > 50.0
            s += x[i] * 2.5 + y[i]
        end
    end
    s
end

function k_reduce()
    s = 0.0; mx = x[1]; d = 0.0
    @inbounds for i in 1:N; s += x[i]; end
    @inbounds for i in 1:N; if x[i] > mx; mx = x[i]; end; end
    @inbounds for i in 1:N; d += x[i] * y[i]; end
    s + mx + d
end

function k_groupby()
    gs = zeros(Float64, G)
    @inbounds for i in 1:N
        gs[gk[i] + 1] += x[i]
    end
    s = 0.0
    @inbounds for g in 1:G
        s += g * gs[g]
    end
    s
end

function k_join()
    s = 0.0
    @inbounds for i in 1:M
        p = searchsortedfirst(kr_sort, kl[i])
        s += vl[i] * vr_sort[p]
    end
    s
end

const KERNELS = Dict("arith" => k_arith, "reduce" => k_reduce,
                     "groupby" => k_groupby, "join" => k_join)

function main()
    bid  = length(ARGS) >= 1 ? ARGS[1] : "reduce"
    runs = length(ARGS) >= 2 ? parse(Int, ARGS[2]) : 5
    warm = length(ARGS) >= 3 ? parse(Int, ARGS[3]) : 2
    haskey(KERNELS, bid) || (println(stderr, "unknown bench: $bid"); exit(2))
    kern = KERNELS[bid]

    ans = 0.0
    for _ in 1:warm; ans = kern(); end
    ts = Float64[]
    for _ in 1:runs
        t0 = time_ns()
        ans = kern()
        push!(ts, (time_ns() - t0) / 1e6)
    end

    println("BENCH ", bid)
    println("CHECK ", CHECK)
    println("ANSWER ", repr(ans))
    println("TIME_MS ", median(ts))
end

main()
