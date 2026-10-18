param(
    [Parameter(Mandatory = $true)]
    [string]$ExePath
)

$ErrorActionPreference = 'Stop'

$resolvedExe = (Resolve-Path -LiteralPath $ExePath).Path
if (-not (Test-Path -LiteralPath $resolvedExe -PathType Leaf)) {
    throw "Handler executable was not found: $resolvedExe"
}

$taskAction = '"' + $resolvedExe + '" schedule run-due'
$taskNames = @('Handler-Scheduler', 'Handler-Startup')

try {
    # Poll once per minute; the C++ scheduler decides which saved tasks are due.
    & schtasks.exe /Create /SC MINUTE /MO 1 /TN 'Handler-Scheduler' /TR $taskAction /F /RL LIMITED
    if ($LASTEXITCODE -ne 0) {
        throw "Failed to register Handler-Scheduler (exit code $LASTEXITCODE)."
    }

    # Run a due-task check immediately after the current user logs in.
    & schtasks.exe /Create /SC ONLOGON /TN 'Handler-Startup' /TR $taskAction /F /RL LIMITED
    if ($LASTEXITCODE -ne 0) {
        throw "Failed to register Handler-Startup (exit code $LASTEXITCODE)."
    }

    Write-Host 'Handler scheduled execution is installed for this Windows user.'
    Write-Host 'The scheduler checks due tasks every minute and at logon.'
    Write-Host 'Only the built-in safe diagnostic commands can be scheduled.'
}
catch {
    foreach ($taskName in $taskNames) {
        & schtasks.exe /Delete /TN $taskName /F 2>$null | Out-Null
    }
    throw
}
