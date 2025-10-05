Param([string]$Log = ".ralph\cli.log")
New-Item -ItemType Directory -Force -Path ".ralph" | Out-Null

# Ralph loop is fundamentally incompatible with current Copilot CLI design
# Copilot CLI is interactive-only and doesn't exit automatically
# This script would run copilot once and wait indefinitely
#
# For Ralph loop to work, Copilot CLI would need:
# 1. Non-interactive mode that processes prompts and exits
# 2. Or ability to send commands to running interactive session
#
# Current workaround: Run copilot manually with:
# /model gpt-5
# /load PROMPT.md
# Then paste kickoff task from PROMPT.md
#
# The auto-commit script (ralph-commit.ps1) can still run separately

Write-Host "Ralph loop not implemented for Windows PowerShell due to Copilot CLI being interactive-only."
Write-Host "Please run copilot manually and use ralph-commit.ps1 for auto-committing changes."
Write-Host ""
Write-Host "To start manually:"
Write-Host "  copilot"
Write-Host "  /model gpt-5"
Write-Host "  /load PROMPT.md"
Write-Host "  # Then paste the kickoff task from PROMPT.md"
exit 1
