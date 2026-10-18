$ErrorActionPreference = 'Continue'

foreach ($taskName in @('Handler-Scheduler', 'Handler-Startup')) {
    & schtasks.exe /Delete /TN $taskName /F 2>$null | Out-Null
    if ($LASTEXITCODE -eq 0) {
        Write-Host "Removed scheduled task: $taskName"
    }
}
