[CmdletBinding()]
param(
    [string]$ExistingRollbackRoot
)

$ErrorActionPreference = 'Stop'
$guardPath = Join-Path $PSScriptRoot 'Ensure-WindhawkSafeDock.ps1'
$receiptPath = 'C:\Users\maxwe\AppData\Local\Maxwell\WindhawkSafeDock\install-last-run.json'
$receiptRoot = Split-Path -Parent $receiptPath
New-Item -ItemType Directory -Path $receiptRoot -Force | Out-Null
trap {
    [ordered]@{succeeded=$false;failedAt=[DateTimeOffset]::Now.ToString('o');error=($_|Out-String).Trim()} |
        ConvertTo-Json -Depth 6 | Set-Content -LiteralPath $receiptPath -Encoding UTF8
    exit 1
}
$repairTaskName = 'Maxwell Windhawk Safe Dock Repair'
$circuitTaskName = 'Maxwell Windhawk Dock Circuit Breaker'
$retiredTasks = @('Maxwell Windhawk ChatGPT Guard','Maxwell Windhawk Material Sync')
$identity = [Security.Principal.WindowsIdentity]::GetCurrent()
$principalCheck = [Security.Principal.WindowsPrincipal]::new($identity)
if (-not $principalCheck.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)) { throw 'Administrator access is required.' }

if ($ExistingRollbackRoot) {
    $stateRoot = 'C:\Users\maxwe\AppData\Local\Maxwell\WindhawkChatGPTGuard'
    $resolvedStateRoot = [IO.Path]::GetFullPath($stateRoot).TrimEnd('\')
    $resolvedRollbackRoot = [IO.Path]::GetFullPath($ExistingRollbackRoot).TrimEnd('\')
    if (-not $resolvedRollbackRoot.StartsWith($resolvedStateRoot + '\', [StringComparison]::OrdinalIgnoreCase)) {
        throw "Rollback root escaped the Windhawk guard state root: $resolvedRollbackRoot"
    }
    $rollbackArchive = Join-Path $resolvedRollbackRoot 'windhawk-before-stage.wharchive'
    if (-not (Test-Path -LiteralPath $rollbackArchive -PathType Leaf)) {
        throw "Existing rollback archive is missing: $rollbackArchive"
    }
} else {
    & (Join-Path $PSScriptRoot 'New-WindhawkSafeDockBackup.ps1') | Out-Null
}
foreach ($taskName in $retiredTasks) {
    $task = Get-ScheduledTask -TaskName $taskName -ErrorAction SilentlyContinue
    if ($task) {
        Stop-ScheduledTask -TaskName $taskName -ErrorAction SilentlyContinue
        Unregister-ScheduledTask -TaskName $taskName -Confirm:$false
    }
}
& powershell.exe -NoLogo -NoProfile -NonInteractive -ExecutionPolicy Bypass -File $guardPath -Mode Repair -AllowExplorerRestart -Quiet
if ($LASTEXITCODE -ne 0) { throw "Initial repair failed: $LASTEXITCODE" }

$powershellPath = 'C:\Windows\System32\WindowsPowerShell\v1.0\powershell.exe'
$repairArgs = '-NoLogo -NoProfile -NonInteractive -ExecutionPolicy Bypass -WindowStyle Hidden -File "{0}" -Mode Repair -AllowExplorerRestart -Quiet' -f $guardPath
$repairAction = New-ScheduledTaskAction -Execute $powershellPath -Argument $repairArgs
$startup = New-ScheduledTaskTrigger -AtStartup; $startup.Delay='PT30S'
$logon = New-ScheduledTaskTrigger -AtLogOn -User 'MSI\maxwe'; $logon.Delay='PT20S'
$principal = New-ScheduledTaskPrincipal -UserId SYSTEM -LogonType ServiceAccount -RunLevel Highest
$settings = New-ScheduledTaskSettingsSet -AllowStartIfOnBatteries -DontStopIfGoingOnBatteries -StartWhenAvailable -MultipleInstances IgnoreNew -RestartCount 2 -RestartInterval (New-TimeSpan -Minutes 1) -ExecutionTimeLimit (New-TimeSpan -Minutes 2)
Register-ScheduledTask -TaskName $repairTaskName -Action $repairAction -Trigger @($startup,$logon) -Principal $principal -Settings $settings -Description 'Drift-aware modular dock repair. Startup and logon only; healthy runs perform no writes.' -Force -ErrorAction Stop | Out-Null

$circuitArgs = '-NoLogo -NoProfile -NonInteractive -ExecutionPolicy Bypass -WindowStyle Hidden -File "{0}" -Mode CircuitBreak -Quiet' -f $guardPath
$xml = @"
<?xml version="1.0" encoding="UTF-16"?>
<Task version="1.4" xmlns="http://schemas.microsoft.com/windows/2004/02/mit/task"><RegistrationInfo><Description>Fails closed after a WER-confirmed Explorer XAML/taskbar crash containing a stable dock DLL.</Description></RegistrationInfo><Triggers><EventTrigger><Enabled>true</Enabled><Subscription>&lt;QueryList&gt;&lt;Query Id="0" Path="Application"&gt;&lt;Select Path="Application"&gt;*[System[Provider[@Name='Application Error'] and EventID=1000]]&lt;/Select&gt;&lt;/Query&gt;&lt;/QueryList&gt;</Subscription></EventTrigger></Triggers><Principals><Principal id="Author"><UserId>S-1-5-18</UserId><RunLevel>HighestAvailable</RunLevel></Principal></Principals><Settings><MultipleInstancesPolicy>IgnoreNew</MultipleInstancesPolicy><DisallowStartIfOnBatteries>false</DisallowStartIfOnBatteries><StopIfGoingOnBatteries>false</StopIfGoingOnBatteries><StartWhenAvailable>true</StartWhenAvailable><Enabled>true</Enabled><ExecutionTimeLimit>PT2M</ExecutionTimeLimit><Priority>6</Priority></Settings><Actions Context="Author"><Exec><Command>$([Security.SecurityElement]::Escape($powershellPath))</Command><Arguments>$([Security.SecurityElement]::Escape($circuitArgs))</Arguments></Exec></Actions></Task>
"@
Register-ScheduledTask -TaskName $circuitTaskName -Xml $xml -Force -ErrorAction Stop | Out-Null
Start-ScheduledTask -TaskName $repairTaskName
$deadline=(Get-Date).AddSeconds(30)
do{Start-Sleep -Milliseconds 400;$task=Get-ScheduledTask -TaskName $repairTaskName}while($task.State -eq 'Running' -and (Get-Date)-lt $deadline)
$info=Get-ScheduledTaskInfo -TaskName $repairTaskName
$result=[ordered]@{succeeded=$true;installedAt=[DateTimeOffset]::Now.ToString('o');rollbackRoot=$resolvedRollbackRoot;rollbackReused=[bool]$ExistingRollbackRoot;repairTask=$repairTaskName;repairResult=$info.LastTaskResult;repairTriggers=@($task.Triggers|ForEach-Object{$_.CimClass.CimClassName});repeatingTriggers=@($task.Triggers|Where-Object{$_.Repetition.Interval}).Count;circuitTask=$circuitTaskName;retiredTasksDisabled=@($retiredTasks|ForEach-Object{$t=Get-ScheduledTask -TaskName $_ -ErrorAction SilentlyContinue;(-not $t)-or(-not $t.Settings.Enabled)})}
$result | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath $receiptPath -Encoding UTF8
[pscustomobject]$result
