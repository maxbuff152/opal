[CmdletBinding()]
param(
    [ValidateRange(3, 60)]
    [int]$Seconds = 10,
    [string]$OutputPath
)

$ErrorActionPreference = 'Stop'

Add-Type -TypeDefinition @'
using System;
using System.Runtime.InteropServices;
public static class MaxwellShellGuiResources {
    [DllImport("user32.dll")]
    public static extern int GetGuiResources(IntPtr process, int flag);
}
'@ -ErrorAction SilentlyContinue

function Get-ShellGuiResourceCount {
    param(
        [Parameter(Mandatory)][System.Diagnostics.Process]$Process,
        [Parameter(Mandatory)][ValidateSet(0, 1)][int]$Kind
    )

    try {
        if ($Process.HasExited -or $Process.Handle -eq [IntPtr]::Zero) { return $null }
        [MaxwellShellGuiResources]::GetGuiResources($Process.Handle, $Kind)
    } catch {
        # Protected or exiting shell processes can deny Handle access. Keep the
        # performance sample and mark only the unavailable GUI counter as null.
        $null
    }
}

$processNames = @(
    'explorer'
    'StartMenuExperienceHost'
    'ShellExperienceHost'
    'SearchHost'
    'dwm'
    'windhawk'
    'Maxwell.Shell.Core'
)

$samples = @{}
foreach ($name in $processNames) {
    foreach ($process in @(Get-Process -Name $name -ErrorAction SilentlyContinue)) {
        $process.Refresh()
        $samples[$process.Id] = [pscustomobject]@{
            name = $process.ProcessName
            cpu = $process.CPU
            handles = $process.HandleCount
            gdi = Get-ShellGuiResourceCount -Process $process -Kind 0
            user = Get-ShellGuiResourceCount -Process $process -Kind 1
        }
    }
}

Start-Sleep -Seconds $Seconds

$rows = foreach ($id in $samples.Keys) {
    $process = Get-Process -Id $id -ErrorAction SilentlyContinue
    if (-not $process) { continue }
    $process.Refresh()
    $start = $samples[$id]
    $gdi = Get-ShellGuiResourceCount -Process $process -Kind 0
    $user = Get-ShellGuiResourceCount -Process $process -Kind 1
    [pscustomobject]@{
        name = $process.ProcessName
        pid = $process.Id
        cpuSeconds = [math]::Round($process.CPU - $start.cpu, 3)
        privateMB = [math]::Round($process.PrivateMemorySize64 / 1MB, 2)
        workingMB = [math]::Round($process.WorkingSet64 / 1MB, 2)
        handles = $process.HandleCount
        handleDelta = $process.HandleCount - $start.handles
        gdi = $gdi
        gdiDelta = if ($null -ne $gdi -and $null -ne $start.gdi) { $gdi - $start.gdi } else { $null }
        user = $user
        userDelta = if ($null -ne $user -and $null -ne $start.user) { $user - $start.user } else { $null }
    }
}

$result = [ordered]@{
    measuredAt = [DateTimeOffset]::Now.ToString('o')
    seconds = $Seconds
    processes = @($rows | Sort-Object name, pid)
}
if ($OutputPath) {
    $result | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath $OutputPath -Encoding UTF8
}
$result | ConvertTo-Json -Depth 6
