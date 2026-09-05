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
$checked = @()
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
    $checked += $component
}
[pscustomobject]@{passed=$true; explorerPid=$explorer.Id; attached=$checked; limitation='Attachment evidence does not replace visual inspection of an unobscured taskbar.'}
