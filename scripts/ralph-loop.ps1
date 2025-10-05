# Ralph Loop Supervisor - PowerShell Wrapper
# Launches the Node.js Ralph loop supervisor for Windows

param(
    [switch]$Help,
    [switch]$NoRestart,
    [int]$InactivityTimeout = 120,
    [string]$CopilotCommand = "copilot"
)

function Show-Help {
    Write-Host "Ralph Loop Supervisor - Automated Copilot CLI Development Loop"
    Write-Host ""
    Write-Host "This script launches a persistent Copilot CLI session that automatically"
    Write-Host "drives development iterations based on the PROMPT.md template."
    Write-Host ""
    Write-Host "USAGE:"
    Write-Host "    .\scripts\ralph-loop.ps1 [options]"
    Write-Host ""
    Write-Host "OPTIONS:"
    Write-Host "    -Help              Show this help message"
    Write-Host "    -NoRestart         Don't restart on Copilot crashes"
    Write-Host "    -InactivityTimeout Timeout in seconds for inactivity detection (default: 120)"
    Write-Host "    -CopilotCommand    Copilot CLI command to use (default: 'copilot')"
    Write-Host ""
    Write-Host "EXAMPLES:"
    Write-Host "    .\scripts\ralph-loop.ps1"
    Write-Host "    .\scripts\ralph-loop.ps1 -InactivityTimeout 60"
    Write-Host "    .\scripts\ralph-loop.ps1 -CopilotCommand 'gh copilot'"
    Write-Host ""
    Write-Host "The supervisor will:"
    Write-Host "  - Monitor for RALPH_NEXT to detect iteration completion"
    Write-Host "  - Send next iteration prompts automatically"
    Write-Host "  - Handle inactivity with /continue commands"
    Write-Host "  - Auto-commit and push changes after each iteration"
    Write-Host "  - Restart Copilot CLI on crashes"
    Write-Host "  - Log everything to .ralph/cli.log"
    Write-Host ""
    Write-Host "Press Ctrl+C to stop the loop gracefully."
}

if ($Help) {
    Show-Help
    exit 0
}

# Check if Node.js is available
try {
    $nodeVersion = & node --version 2>$null
    if ($LASTEXITCODE -ne 0) {
        throw "Node.js not found"
    }
    Write-Host "Found Node.js: $nodeVersion"
} catch {
    Write-Host "ERROR: Node.js is required but not found. Please install Node.js 16+ from https://nodejs.org/" -ForegroundColor Red
    exit 1
}

# Check if the Ralph script exists
$ralphScript = Join-Path $PSScriptRoot "ralph-loop.js"
if (!(Test-Path $ralphScript)) {
    Write-Host "ERROR: Ralph loop script not found at $ralphScript" -ForegroundColor Red
    exit 1
}

# Check if dependencies are installed
$packageJson = Join-Path (Split-Path $PSScriptRoot -Parent) "package.json"
if (Test-Path $packageJson) {
    $nodeModules = Join-Path (Split-Path $PSScriptRoot -Parent) "node_modules"
    if (!(Test-Path $nodeModules)) {
        Write-Host "Installing Node.js dependencies..." -ForegroundColor Yellow
        Push-Location (Split-Path $PSScriptRoot -Parent)
        try {
            & npm install
            if ($LASTEXITCODE -ne 0) {
                Write-Host "ERROR: Failed to install dependencies. Please run 'npm install' manually." -ForegroundColor Red
                exit 1
            }
        } finally {
            Pop-Location
        }
    }
}

# Check if Copilot CLI is available
try {
    $copilotVersion = & $CopilotCommand --version 2>$null
    if ($LASTEXITCODE -ne 0) {
        throw "Copilot CLI not found"
    }
    Write-Host "Found Copilot CLI: $copilotVersion"
} catch {
    Write-Host "ERROR: Copilot CLI command '$CopilotCommand' not found or not working." -ForegroundColor Red
    Write-Host "Please ensure Copilot CLI is installed and authenticated." -ForegroundColor Yellow
    Write-Host "Installation instructions: https://docs.github.com/en/copilot/github-copilot-in-the-cli" -ForegroundColor Yellow
    exit 1
}

# Set environment variables for the Node.js script
$env:RALPH_COPILOT_CMD = $CopilotCommand
$env:RALPH_INACTIVITY_TIMEOUT = ($InactivityTimeout * 1000).ToString()  # Convert to milliseconds
$env:RALPH_NO_RESTART = $NoRestart.ToString().ToLower()

# Display startup information
Write-Host ""
Write-Host "🚀 Starting Ralph Loop Supervisor..." -ForegroundColor Green
Write-Host "  Copilot Command: $CopilotCommand" -ForegroundColor Cyan
Write-Host "  Inactivity Timeout: $InactivityTimeout seconds" -ForegroundColor Cyan
Write-Host "  Auto-restart: $(!$NoRestart)" -ForegroundColor Cyan
Write-Host "  Log file: $(Join-Path (Split-Path $PSScriptRoot -Parent) '.ralph\cli.log')" -ForegroundColor Cyan
Write-Host ""
Write-Host "The loop will run indefinitely. Press Ctrl+C to stop." -ForegroundColor Yellow
Write-Host ""

# Launch the Node.js supervisor
try {
    & node $ralphScript
} catch {
    Write-Host "ERROR: Failed to start Ralph loop supervisor: $($_.Exception.Message)" -ForegroundColor Red
    exit 1
}
