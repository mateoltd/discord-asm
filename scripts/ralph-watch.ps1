Param([string]$Log = ".ralph\cli.log")
New-Item -ItemType Directory -Force -Path ".ralph" | Out-Null
while ($true) {
  try {
    copilot --allow-all-tools | Tee-Object -FilePath $Log -Append
  } catch { Start-Sleep -Seconds 5 }
  Start-Sleep -Seconds 5
}
