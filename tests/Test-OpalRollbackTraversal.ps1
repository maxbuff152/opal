[CmdletBinding()]
param()
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$source = [IO.File]::ReadAllText((Join-Path $root 'New-MaxwellShellRollbackPoint.ps1'))
$start = $source.IndexOf('function Copy-Tree(')
$end = $source.IndexOf('function Copy-One(', $start)
if ($start -lt 0 -or $end -lt 0) { throw 'Rollback copy implementation not found.' }
Invoke-Expression $source.Substring($start, $end - $start)
$fixture = Join-Path $root ('build\rollback-traversal-' + [guid]::NewGuid().ToString('N'))
$old = Join-Path $fixture 'rollback-old'
$destination = Join-Path $fixture 'rollback-new'
$active = Join-Path $fixture 'state'
foreach ($path in @($old,$destination,$active)) { New-Item -ItemType Directory -Path $path -Force | Out-Null }
Set-Content -LiteralPath (Join-Path $old 'historical.txt') 'never-copy'
Set-Content -LiteralPath (Join-Path $active 'runtime.ini') 'active-state'
$files = [Collections.Generic.List[object]]::new()
# Observe every enumerated directory. An exclusion is insufficient if the
# implementation descends into historical or self-generated rollback content.
$visited = [Collections.Generic.List[string]]::new()
function Get-ChildItem {
    param($LiteralPath,[switch]$Directory,[switch]$File,$ErrorAction)
    $visited.Add([IO.Path]::GetFullPath($LiteralPath))
    Microsoft.PowerShell.Management\Get-ChildItem -LiteralPath $LiteralPath -Directory:$Directory -File:$File -ErrorAction Stop
}
Copy-Tree $fixture (Join-Path $destination 'copy') 'fixture' @($old,$destination)
if ($visited.Contains($old) -or $visited.Contains($destination)) { throw 'Rollback traversed an excluded tree.' }
if ($files.Count -ne 1) { throw "Expected one active file, got $($files.Count)." }
$copied = Join-Path $destination 'copy\state\runtime.ini'
if ((Get-Content -LiteralPath $copied -Raw).Trim() -ne 'active-state') { throw 'Active state was not preserved.' }
if ((Get-FileHash -LiteralPath $copied).Hash -ne $files[0].Sha256) { throw 'Backup hash mismatch.' }
[pscustomobject]@{passed=$true; assertions=4; evidence='Excluded directories are never enumerated; nested destination cannot recurse; active data and hash preserved.'}
