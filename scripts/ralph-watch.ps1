Param([string]$Log = ".ralph\cli.log")
New-Item -ItemType Directory -Force -Path ".ralph" | Out-Null
while ($true) {
  # Start copilot and log output
  try {
    copilot | Tee-Object -FilePath $Log -Append
  } catch { Start-Sleep -Seconds 5 }
  Start-Sleep -Seconds 5
}
