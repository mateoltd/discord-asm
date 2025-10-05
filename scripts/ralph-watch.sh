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
