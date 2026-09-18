# Shell configuration

The `~/.bashrc` / `~/.zshrc` recipe: `AMBER_HOME`, the four aliases, thread
pinning, and why you symlink the launcher instead of putting the repo on
`PATH`.

Moved out of [`README.md`](../README.md) so the landing page stays a landing
page. The install steps themselves stay in the README.

---

```sh
# === Amber - native engine configuration ===================================
# AMBER_HOME is the Amber checkout ITSELF. Amber is one self-contained folder:
# there is no bin/ directory, nothing is copied anywhere, and deleting the
# folder uninstalls it completely.
export AMBER_HOME="$HOME/amber"

# amber  -> the full REPL: repl.k, the q/kdb+ vocabulary and the stdlib.
# AMBER_NATIVE is read by build.sh, which ./a re-runs whenever the sources are
# newer than the binary -- so the first run after a git pull rebuilds with
# -march=native (or -mcpu=native on Apple Silicon / aarch64).
alias amber='AMBER_NATIVE=1 "$AMBER_HOME/a"'

# amberx -> the bare interpreter for scripts and pipes: no REPL, no stdlib.
alias amberx='"$AMBER_HOME/amber"'

# Pin the vector engine to your physical cores; omit to use every core.
# alias amber='AMBER_NATIVE=1 AMBER_THREADS=8 "$AMBER_HOME/a"'

# amber-ai -> the same REPL with the local AI co-pilot pointed at your model
# server and given a longer answer budget. Needs the amber-ai extension;
# without it these variables are simply ignored.
alias amber-ai='AMBER_NATIVE=1 AMBER_AI=1 AMBER_AI_URL="http://127.0.0.1:11434/api/generate" AMBER_AI_TIMEOUT_MS=10000 "$AMBER_HOME/a"'

# Aliases do not exist in non-interactive shells. For scripts, cron and CI,
# symlink the launcher instead of putting the repo root on PATH (which would
# also expose install.sh and demo.sh as commands):
#     mkdir -p ~/.local/bin && ln -sf "$AMBER_HOME/a" ~/.local/bin/amber
```
