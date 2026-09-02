[CmdletBinding(SupportsShouldProcess)]
param([ValidateSet('current','previous')][string]$Slot = 'current')

$ErrorActionPreference = 'Stop'
$toolRoot = $PSScriptRoot
$backup = Join-Path (Join-Path $toolRoot 'managed-backups') $Slot
$manifestPath = Join-Path $backup 'manifest.json'
if (-not (Test-Path -LiteralPath $manifestPath)) { throw "Managed backup does not exist: $Slot" }
$manifest = Get-Content -LiteralPath $manifestPath -Raw | ConvertFrom-Json
foreach ($item in $manifest.files) {
    $path = Join-Path $backup $item.relativePath
    if (-not (Test-Path -LiteralPath $path)) { throw "Backup file missing: $($item.relativePath)" }
    if ((Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash -ne $item.sha256) { throw "Backup hash mismatch: $($item.relativePath)" }
}
if (-not $PSCmdlet.ShouldProcess($toolRoot,"Restore managed Windhawk safe-dock backup slot '$Slot'")) {
    [pscustomobject]@{validated=$true;restored=$false;slot=$Slot;fileCount=@($manifest.files).Count}
    return
}
$identity = [Security.Principal.WindowsIdentity]::GetCurrent()
$principal = [Security.Principal.WindowsPrincipal]::new($identity)
if (-not $principal.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)) { throw 'Administrator access is required to restore the registry and tasks.' }

foreach ($item in $manifest.files | Where-Object { $_.relativePath -match '\.(ps1|psm1|md)$' }) {
    $source = Join-Path $backup $item.relativePath
    $destination = Join-Path $toolRoot $item.relativePath
    $parent = Split-Path -Parent $destination
    if (-not (Test-Path -LiteralPath $parent)) { New-Item -ItemType Directory -Path $parent -Force | Out-Null }
    Copy-Item -LiteralPath $source -Destination $destination -Force
}
foreach ($regName in @('Mods.reg','ModsWritable.reg')) {
    $regPath = Join-Path $backup $regName
    if (Test-Path -LiteralPath $regPath) { & reg.exe import $regPath | Out-Null; if ($LASTEXITCODE -ne 0) { throw "Registry restore failed: $regName" } }
}
foreach ($xml in Get-ChildItem -LiteralPath $backup -Filter '*.xml' -File) {
    $taskName = if ($xml.Name -like 'Maxwell_Windhawk_Safe_Dock_Repair*') { 'Maxwell Windhawk Safe Dock Repair' } elseif ($xml.Name -like 'Maxwell_Windhawk_Dock_Circuit_Breaker*') { 'Maxwell Windhawk Dock Circuit Breaker' } else { $null }
    if ($taskName) { Register-ScheduledTask -TaskName $taskName -Xml (Get-Content -LiteralPath $xml.FullName -Raw) -Force | Out-Null }
}
& (Join-Path $toolRoot 'Ensure-WindhawkSafeDock.ps1') -Mode Repair -AllowExplorerRestart -Quiet
if ($LASTEXITCODE -ne 0) { throw "Post-restore repair failed with exit code $LASTEXITCODE" }
[pscustomobject]@{validated=$true;restored=$true;slot=$Slot;fileCount=@($manifest.files).Count}
