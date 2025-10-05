#!/usr/bin/env bash
set -euo pipefail

LOG="${LOG:-./.ralph/cli.log}"
mkdir -p .ralph

while true; do
  if command -v script >/dev/null 2>&1; then
    script -q -c "copilot --allow-all-tools" "$LOG"
  else
    copilot --allow-all-tools | tee -a "$LOG"
  fi
  sleep 5
done
