# Releasing Amber

How a version gets out. Short on purpose.

## Version numbers

`MAJOR.MINOR.PATCH`, with one rule until 3.0:

- **patch** (2.9.0 to 2.9.1): fixes, speed, new things that change no existing answer.
- **minor** (2.9 to 2.10): anything that changes an answer someone could already get, even towards q's. Those are the
  breaking changes, and they get listed as such.
- **major**: 3.0 is the first one that promises no breaking changes inside a major.

The number lives in three places: `src/a.h` (`AMBER_VERSION_*`), the version badge in `README.md`, and the heading
in `CHANGELOG.md`. Satellites pin it on their own (python-amber's `pyproject.toml`, amber-notepad's wasm build).

## The notes

`CHANGELOG.md` is the only place they're written. A version's section is one short bullet per change, plain words,
numbers where there are some. The GitHub release copies that section and adds:

- **breaking changes**: every changed answer since the last minor, old and new side by side (`1 2 3?4` was `0N`, is
  `3`). When the last release was a patch, it covers everything since the minor before it.
- **files**: what's attached.

kdb+ timings never go in either: the kdb+ personal licence forbids sharing them.

## What gets attached

- `amber-X.Y.Z-linux-x86_64.tar.gz`: the portable build (`./build.sh`, no `-march=native`), `amber`, `a`, the `.k`
  libraries, `LICENSE`, `NOTICE`.
- `amber-X.Y.Z-macos-arm64.tar.gz`: the same, built on macOS.
- `SHA256SUMS` for both.

Windows users build in WSL (docs/INSTALL.md). The source is the tag's archive, which GitHub adds by itself.

## The checklist

1. `tests/run_tests.sh` passes, and `tests/run_tests.sh --asan`.
2. `gate_ab.sh` against the last release, on a quiet machine: nothing slower than noise. The numbers go in the commit
   body.
3. The re-baseline against kdb+ (amber-work, private): no new wrong answers. Anything slower than before is a fix or
   a note in the release.
4. `docs/MISSING.md` matches: gaps closed come out, new differences from q go in.
5. The version bumped in the three places, the `CHANGELOG.md` section written.
6. CI green on the release commit, all jobs (Linux, macOS, Windows, sanitizers).
7. Tag `vX.Y.Z` on that commit, build the two archives from the tag, write `SHA256SUMS`, and publish the release with
   the notes above.
8. The satellites: python-amber's wheel (tag `v*`, trusted publishing), the notepad's wasm, when they need it.
