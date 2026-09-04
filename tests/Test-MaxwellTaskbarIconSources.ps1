[CmdletBinding()]
param(
    [string]$ReceiptPath = [IO.Path]::Combine(
        [Environment]::GetFolderPath('LocalApplicationData'),
        'Maxwell',
        'Opal',
        'taskbar-icon-source-audit.json'
    )
)

$ErrorActionPreference = 'Stop'
$pinnedRoot = Join-Path $env:APPDATA 'Microsoft\Internet Explorer\Quick Launch\User Pinned\TaskBar'
$shell = New-Object -ComObject WScript.Shell
$startApps = @(Get-StartApps)
$expected = @('Spotify','Firefox','Google Chrome','File Explorer','Notepad','WhatsApp','ChatGPT','Call of Duty','PowerShell')

$shortcuts = foreach ($file in @(Get-ChildItem -LiteralPath $pinnedRoot -Filter '*.lnk' -File -ErrorAction SilentlyContinue)) {
    $shortcut = $shell.CreateShortcut($file.FullName)
    $targetExists = if ($shortcut.TargetPath) { Test-Path -LiteralPath $shortcut.TargetPath -PathType Leaf } else { $false }
    [pscustomobject]@{
        name = $file.BaseName
        shortcut = $file.FullName
        target = $shortcut.TargetPath
        targetExists = $targetExists
        iconLocation = $shortcut.IconLocation
        source = if ($shortcut.IconLocation) { 'explicit-official-resource' } elseif ($targetExists) { 'target-embedded-icon' } else { 'shell-app-registration' }
    }
}

$registered = foreach ($name in $expected) {
    $matched = @($startApps | Where-Object { $_.Name -eq $name -or $_.Name -like "$name*" })
    [pscustomobject]@{
        name = $name
        registered = $matched.Count -gt 0
        appIds = @($matched | Select-Object -ExpandProperty AppID)
    }
}

$result = [ordered]@{
    passed = @($registered | Where-Object { -not $_.registered }).Count -eq 0 -and @($shortcuts | Where-Object { $_.target -and -not $_.targetExists }).Count -eq 0
    checkedAt = [DateTimeOffset]::Now.ToString('o')
    policy = 'Retain official app-provided artwork; do not substitute a brittle icon pack.'
    iconSize = 40
    buttonWidth = 58
    registeredApps = @($registered)
    pinnedShortcuts = @($shortcuts)
}
New-Item -ItemType Directory -Path (Split-Path -Parent $ReceiptPath) -Force | Out-Null
$result | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath $ReceiptPath -Encoding UTF8
[pscustomobject]$result
if (-not $result.passed) { exit 1 }
