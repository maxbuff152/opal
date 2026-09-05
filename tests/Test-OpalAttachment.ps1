[CmdletBinding()]
param()
$ErrorActionPreference = 'Stop'
$session = (Get-Process -Id $PID).SessionId
$explorer = Get-Process explorer | Where-Object SessionId -eq $session | Select-Object -First 1
if (-not $explorer) { throw 'Interactive Explorer is not running.' }
$root = Join-Path $env:LOCALAPPDATA 'Maxwell\Opal'
function Read-IniValue([string]$Path, [string]$Name) {
    $line = Get-Content -LiteralPath $Path | Where-Object { $_ -match "^$([regex]::Escape($Name))=" } | Select-Object -First 1
    if (-not $line) { throw "Missing $Name in $Path" }
    $line.Substring($line.IndexOf('=') + 1)
}
$runtime = Join-Path $root "runtime-$($explorer.Id).ini"
$settings = Get-ItemProperty 'HKLM:\SOFTWARE\Windhawk\Engine\Mods\local@opal\Settings'
if (-not ('OpalAttachmentWindows' -as [type])) {
    Add-Type @'
using System;
using System.Collections.Generic;
using System.Text;
using System.Runtime.InteropServices;
public static class OpalAttachmentWindows {
    delegate bool EnumProc(IntPtr hwnd, IntPtr arg);
    [DllImport("user32.dll")] static extern bool EnumWindows(EnumProc callback, IntPtr arg);
    [DllImport("user32.dll", CharSet=CharSet.Unicode)] static extern int GetClassName(IntPtr hwnd, StringBuilder name, int size);
    [DllImport("user32.dll")] static extern uint GetWindowThreadProcessId(IntPtr hwnd, out uint pid);
    public static Dictionary<long,string> Bars(uint owner) {
        var result = new Dictionary<long,string>();
        EnumWindows((hwnd, arg) => {
            uint pid; GetWindowThreadProcessId(hwnd, out pid);
            var name = new StringBuilder(64); GetClassName(hwnd, name, 64);
            if (pid == owner && (name.ToString() == "Shell_TrayWnd" || name.ToString() == "Shell_SecondaryTrayWnd")) result[hwnd.ToInt64()] = name.ToString();
            return true;
        }, IntPtr.Zero);
        return result;
    }
}
'@
}
$bars = [OpalAttachmentWindows]::Bars($explorer.Id)
if (-not $bars.Count) { throw 'Explorer has no taskbars.' }
$checked = @()
$views = @()
foreach ($component in @('Media', 'Performance')) {
    $name = $component.ToLowerInvariant()
    if ($settings.PSObject.Properties["$name.${name}Enabled"].Value -ne 1) { continue }
    $health = Join-Path $root "health-$name.ini"
    if ((Read-IniValue $runtime "${component}RuntimeActive") -ne '1') { throw "$component runtime inactive." }
    if ((Read-IniValue $runtime "${component}RuntimePid") -ne [string]$explorer.Id) { throw "$component runtime belongs to another Explorer." }
    if ((Read-IniValue $health 'Quarantined') -ne '0') { throw "$component is quarantined." }
    $liveSince = [long](Read-IniValue $health 'LiveSince')
    if ((Read-IniValue $health 'Dirty') -ne '1' -or $liveSince -lt $explorer.StartTime.ToUniversalTime().ToFileTimeUtc()) {
        throw "$component initialized but has not attached in the current Explorer session."
    }
    $proof = (Read-IniValue $runtime "${component}Attachment").Split('|')
    if ($proof.Count -ne 4) { throw "$component attachment proof is invalid." }
    $age = ([DateTime]::UtcNow - [DateTime]::FromFileTimeUtc([long]$proof[0])).TotalSeconds
    $fullWindow = [long]$proof[1]
    $target = [string]$settings.PSObject.Properties["screens.${name}Monitor"].Value
    $expected = if ($target -eq 'both') { $bars.Count } else { 1 }
    if ($age -lt -1 -or $age -gt 15 -or -not $bars.ContainsKey($fullWindow) -or
        [int]$proof[2] -ne $expected -or [int]$proof[3] -ne $expected) {
        throw "$component has stale or incomplete screen attachment: $($proof -join '|'); expected $expected views."
    }
    $fullDisplay = if ($target -eq 'both') { [string]$settings.PSObject.Properties["screens.${name}FullDisplay"].Value } else { $target }
    $fullClass = if ($fullDisplay -eq 'secondary') { 'Shell_SecondaryTrayWnd' } else { 'Shell_TrayWnd' }
    if ($bars.Values -contains $fullClass -and $bars[$fullWindow] -ne $fullClass) { throw "$component attached to the wrong display." }
    $views += [pscustomobject]@{component=$component; expected=$expected; attached=[int]$proof[3]; fullWindow=$fullWindow; ageSeconds=[math]::Round($age,2)}
    $checked += $component
}
[pscustomobject]@{passed=$true; explorerPid=$explorer.Id; attached=$checked; views=$views; limitation='Attachment evidence does not replace visual inspection of an unobscured taskbar.'}
