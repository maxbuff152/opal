[CmdletBinding()]
param()

$ErrorActionPreference = 'Stop'
$toolRoot = $PSScriptRoot
$backupRoot = Join-Path $toolRoot 'managed-backups'
$current = Join-Path $backupRoot 'current'
$previous = Join-Path $backupRoot 'previous'
$resolvedToolRoot = [IO.Path]::GetFullPath($toolRoot).TrimEnd('\')
$resolvedBackupRoot = [IO.Path]::GetFullPath($backupRoot).TrimEnd('\')
if (-not $resolvedBackupRoot.StartsWith($resolvedToolRoot + '\',[StringComparison]::OrdinalIgnoreCase)) { throw 'Managed backup root escaped the tool root.' }

New-Item -ItemType Directory -Path $backupRoot -Force | Out-Null
if (Test-Path -LiteralPath $previous) { Remove-Item -LiteralPath $previous -Recurse -Force }
if (Test-Path -LiteralPath $current) { Move-Item -LiteralPath $current -Destination $previous }
New-Item -ItemType Directory -Path $current -Force | Out-Null
New-Item -ItemType Directory -Path (Join-Path $current 'tests') -Force | Out-Null

$patterns = @('*.ps1','*.psm1','*.md')
$copied = [Collections.Generic.List[string]]::new()
foreach ($pattern in $patterns) {
    foreach ($file in Get-ChildItem -LiteralPath $toolRoot -File -Filter $pattern) {
        Copy-Item -LiteralPath $file.FullName -Destination $current -Force
        $copied.Add($file.Name)
    }
}
$test = Join-Path $toolRoot 'tests\Test-WindhawkSafeDock.ps1'
if (Test-Path -LiteralPath $test) { Copy-Item -LiteralPath $test -Destination (Join-Path $current 'tests') -Force; $copied.Add('tests\Test-WindhawkSafeDock.ps1') }

reg export "HKLM\SOFTWARE\Windhawk\Engine\Mods" (Join-Path $current 'Mods.reg') /y | Out-Null
if ($LASTEXITCODE -ne 0) { throw 'Failed to export Windhawk Mods registry.' }
reg export "HKLM\SOFTWARE\Windhawk\Engine\ModsWritable" (Join-Path $current 'ModsWritable.reg') /y | Out-Null
if ($LASTEXITCODE -ne 0) { throw 'Failed to export Windhawk ModsWritable registry.' }

$taskNames = @('Maxwell Windhawk Safe Dock Repair','Maxwell Windhawk Dock Circuit Breaker')
foreach ($taskName in $taskNames) {
    $task = Get-ScheduledTask -TaskName $taskName -ErrorAction SilentlyContinue
    if ($task) { Export-ScheduledTask -TaskName $taskName | Set-Content -LiteralPath (Join-Path $current (($taskName -replace '[^A-Za-z0-9.-]','_') + '.xml')) -Encoding Unicode }
}

$files = Get-ChildItem -LiteralPath $current -File -Recurse | ForEach-Object {
    [pscustomobject]@{ relativePath=$_.FullName.Substring($current.Length+1); bytes=$_.Length; sha256=(Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash }
}
$manifest = [pscustomobject]@{
    schemaVersion = 1
    createdAt = [DateTimeOffset]::Now
    machine = $env:COMPUTERNAME
    retention = 2
    slots = @('current','previous')
    sourceRoot = $toolRoot
    files = @($files)
}
$manifest | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath (Join-Path $current 'manifest.json') -Encoding UTF8

$slots = @(Get-ChildItem -LiteralPath $backupRoot -Directory | Where-Object Name -in @('current','previous'))
if ($slots.Count -gt 2) { throw 'Managed backup retention exceeded two slots.' }
[pscustomobject]@{backupRoot=$backupRoot;current=$current;previousExists=(Test-Path $previous);slotCount=$slots.Count;fileCount=@($files).Count;manifest=(Join-Path $current 'manifest.json')}
