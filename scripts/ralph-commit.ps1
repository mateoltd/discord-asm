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
