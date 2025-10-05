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
