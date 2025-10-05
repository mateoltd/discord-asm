## The Ralph Loop (hands-off development)

This project supports a **Ralph loop**—an unattended, long-running agentic workflow that continuously plans → edits → builds/tests → documents → commits → repeats. The pattern comes from Geoffrey Huntley’s field reports (“Ralph Wiggum as a software engineer”) and his three-month **cursed** experiment, where a coding agent ran in a `while(true)` loop to finish a whole project. ([Geoffrey Huntley][1])

### What this loop does

* Launches the **standalone Copilot CLI** in your repo and feeds it `PROMPT.md` (our control prompt).
* Lets Copilot **apply diffs and run commands** in your workspace (build/tests, etc).
* **Auto-commits** any file changes with meaningful messages and **pushes** to your remote.
* **Restarts** Copilot if it exits, and **nudges** it if it stalls (keeps the loop from getting stuck).

> Tip: Copilot CLI model can be changed anytime with `/model`; it’s in public preview and adds features frequently. ([The GitHub Blog][2])

---

### One-time setup

1. Ensure you’ve installed the standalone Copilot CLI and authenticated:

```bash
npm install -g @github/copilot
copilot
# inside the CLI:
#   /login
#   /model gpt-5
```

(See GitHub’s docs for install/auth/model switching if needed.) ([GitHub Docs][3])

2. Make sure your repo has a remote set:

```bash
git remote -v   # should show origin
```

3. Put your project control spec in `PROMPT.md` (already part of this repo).

---

### Start the loop (two small helper processes)

You’ll run **Copilot** in one terminal and a tiny **autocommitter/watchdog** in another. Together they create the Ralph loop.

#### A) Launch Copilot (interactive, but then hands-off)

From the repo root:

```text
copilot
/model gpt-5
/load PROMPT.md
# then: paste the Kickoff task from PROMPT.md
```

> The CLI can autonomously apply diffs and run commands it proposes—accept suggestions you’re happy with, then let it drive. If it exits (e.g., network hiccup), the watchdog below will restart it.

#### B) Auto-commit and watchdog (cross-platform)

Add these files to `scripts/`:

**`scripts/ralph-commit.sh` (macOS/Linux)**

```bash
#!/usr/bin/env bash
set -euo pipefail
INTERVAL="${INTERVAL:-30}"          # seconds between scans
BRANCH="${BRANCH:-main}"
MSG_PREFIX="${MSG_PREFIX:-ralph}"

while true; do
  if ! git diff --quiet || ! git diff --cached --quiet; then
    # Try to derive a nice message from the top of CHANGELOG; fallback to a generic message
    if [[ -f CHANGELOG.md ]]; then
      TITLE="$(grep -m1 -E '^- ' CHANGELOG.md | sed 's/^- //;s/`//g' | head -n1)"
    fi
    MSG="${MSG_PREFIX}: ${TITLE:-automated commit}"
    git add -A
    git commit -m "$MSG" || true
    git pull --rebase --autostash || true
    git push origin "${BRANCH}" || true
  fi
  sleep "${INTERVAL}"
done
```

**`scripts/ralph-commit.ps1` (Windows/PowerShell 7+)**

```powershell
Param(
  [int]$Interval = 30,
  [string]$Branch = "main",
  [string]$MsgPrefix = "ralph"
)

while ($true) {
  $dirty = (git status --porcelain) -ne $null -and (git status --porcelain).Count -gt 0
  if ($dirty) {
    $title = ""
    if (Test-Path "CHANGELOG.md") {
      $firstBullet = Select-String -Path "CHANGELOG.md" -Pattern '^- ' -SimpleMatch | Select-Object -First 1
      if ($firstBullet) { $title = ($firstBullet.Line -replace '^- ', '') -replace '`','' }
    }
    $msg = "$MsgPrefix: " + ($(if ($title) {$title} else {"automated commit"}))
    git add -A
    git commit -m $msg 2>$null | Out-Null
    git pull --rebase --autostash 2>$null | Out-Null
    git push origin $Branch 2>$null | Out-Null
  }
  Start-Sleep -Seconds $Interval
}
```

**`scripts/ralph-watch.sh` (macOS/Linux)** — keeps Copilot alive and nudges it

```bash
#!/usr/bin/env bash
set -euo pipefail

LOG="${LOG:-./.ralph/cli.log}"
mkdir -p .ralph

while true; do
  # Run Copilot and tee transcript (use 'script' to capture the TTY session if available)
  if command -v script >/dev/null 2>&1; then
    script -q -c "copilot" "$LOG"
  else
    copilot | tee -a "$LOG"
  fi

  # Simple backoff before restart (e.g., on crash or exit)
  sleep 5
done
```

**`scripts/ralph-watch.ps1` (Windows/PowerShell 7+)**

```powershell
Param([string]$Log = ".ralph\cli.log")
New-Item -ItemType Directory -Force -Path ".ralph" | Out-Null
while ($true) {
  # Start copilot and log output
  try {
    copilot | Tee-Object -FilePath $Log -Append
  } catch { Start-Sleep -Seconds 5 }
  Start-Sleep -Seconds 5
}
```

Make them executable:

```bash
chmod +x scripts/ralph-commit.sh scripts/ralph-watch.sh
```

---

### Run it

**macOS/Linux (two terminals):**

```bash
# Terminal A
./scripts/ralph-watch.sh   # keeps Copilot running and logs transcript

# Terminal B
./scripts/ralph-commit.sh  # auto-commit/push whenever files change
```

**Windows (two terminals):**

```powershell
# Terminal A (PowerShell 7+)
./scripts/ralph-watch.ps1

# Terminal B
./scripts/ralph-commit.ps1
```

Once Copilot is up, inside the Copilot prompt run:

```text
/model gpt-5
/load PROMPT.md
# Paste the Kickoff task from PROMPT.md
```

From here, the loop should self-sustain: Copilot proposes diffs & runs builds/tests; the committer pushes progress; the watcher restarts the agent if it exits. This mirrors the **cursed** setup where the agent ran continuously and shipped a full project over weeks. ([Geoffrey Huntley][4])

---

### Keeping the loop unstuck

* **Provide a clear task list** in `PROMPT.md` (we include an “Iteration template” and Next TODOs).
* **Nudge prompts**: If you notice Copilot pausing for confirmation, type `/continue` or restate the next iteration (“PLAN: … CODE: … VERIFY: … DOCS: … NEXT: …”).
* **Model control**: Switch models with `/model` if you see regressions or slowdowns. ([The GitHub Blog][2])
* **Ground truth**: Keep a crisp failure signal—tests that fail loudly, and ADRs/CHANGELOG updated every iteration.

> Background: Huntley’s write-ups show that the success of the loop came from (1) a rock-solid control prompt, (2) visible artifacts (tests/docs), and (3) an external loop that keeps committing and restarting so momentum never dies. ([Geoffrey Huntley][1])

---

### Safety rails

* Commits run `git pull --rebase --autostash` before pushing to minimize conflicts.
* The watcher restarts Copilot on exit—use `Ctrl+C` twice to stop both processes.
* Keep tokens in env vars; never commit secrets.

---

### FAQ

**Can I truly leave it unattended?**
Mostly, yes. The combination of Copilot’s autonomous diffs/commands, plus the auto-committer and watcher, reproduces the **Ralph** behavior seen in practice. You may still step in occasionally to approve big changes or tweak the prompt. ([Geoffrey Huntley][1])

**Where do I see what happened?**

* `CHANGELOG.md`/`docs/ADRs` (agent writes these each iteration per our prompt).
* `.ralph/cli.log` (session transcript).
* Git history (every change is committed and pushed).

---

If you want, I can also add a small Node or Python “keep-alive” that watches `.ralph/cli.log` for inactivity and sends a `/continue` keystroke—but in practice the **watcher + autocommitter** pair above is enough to match the **cursed** loop pattern. ([Geoffrey Huntley][4])

[1]: https://ghuntley.com/ralph/?utm_source=chatgpt.com "Ralph Wiggum as a \"software engineer\""
[2]: https://github.blog/changelog/2025-10-03-github-copilot-cli-enhanced-model-selection-image-support-and-streamlined-ui/?utm_source=chatgpt.com "GitHub Copilot CLI: Enhanced model selection, image ..."
[3]: https://docs.github.com/en/copilot/how-tos/set-up/install-copilot-cli?utm_source=chatgpt.com "Installing GitHub Copilot CLI"
[4]: https://ghuntley.com/cursed/?utm_source=chatgpt.com "i ran Claude in a loop for three months, and it created ..."
