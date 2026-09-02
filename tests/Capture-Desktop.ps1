[CmdletBinding()]
param(
    [Parameter(Mandatory)][string]$Name,
    [string]$CaptureRoot = [IO.Path]::Combine([IO.Path]::GetTempPath(), 'Opal', 'qa-captures'),
    [int]$MonitorIndex = -1,
    [ValidateRange(0, 1080)][int]$BottomPixels = 0
)

$ErrorActionPreference = 'Stop'
if ($Name -notmatch '^qa-capture-[a-z0-9][a-z0-9-]*\.png$') {
    throw 'Name must use the qa-capture-*.png disposable-capture convention.'
}

New-Item -ItemType Directory -Path $CaptureRoot -Force | Out-Null
New-Item -ItemType File -Path (Join-Path $CaptureRoot '.codex-disposable-captures') -Force | Out-Null

Add-Type -AssemblyName System.Drawing
Add-Type -AssemblyName System.Windows.Forms
$bounds = if ($MonitorIndex -ge 0) {
    $screens = [Windows.Forms.Screen]::AllScreens
    if ($MonitorIndex -ge $screens.Count) { throw "Monitor index $MonitorIndex is out of range." }
    $screens[$MonitorIndex].Bounds
} else {
    [Windows.Forms.SystemInformation]::VirtualScreen
}
if ($BottomPixels -gt 0) {
    $height = [Math]::Min($BottomPixels, $bounds.Height)
    $bounds = [Drawing.Rectangle]::new($bounds.Left, $bounds.Bottom - $height, $bounds.Width, $height)
}
$output = Join-Path $CaptureRoot $Name
$bitmap = [Drawing.Bitmap]::new($bounds.Width, $bounds.Height)
$graphics = [Drawing.Graphics]::FromImage($bitmap)
try {
    $graphics.CopyFromScreen($bounds.Left, $bounds.Top, 0, 0, $bitmap.Size)
    $bitmap.Save($output, [Drawing.Imaging.ImageFormat]::Png)
} finally {
    $graphics.Dispose()
    $bitmap.Dispose()
}

Get-Item -LiteralPath $output | Select-Object FullName, Length, LastWriteTime
