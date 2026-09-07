Set-StrictMode -Version 2.0

$script:ToolRoot = Split-Path -Parent $PSCommandPath
$script:StateRoot = Join-Path $env:LOCALAPPDATA 'Maxwell\Opal\safedock'
$script:CanonicalRoot = Join-Path $env:LOCALAPPDATA 'Maxwell\WindhawkChatGPTGuard\backup-20260820-003318-before-source-owned-visual-stack'
# Historical snapshot name only. New Opal source and rollbacks use the Opal name.

function Get-WindhawkSafeDockLegacyConfig {
    [CmdletBinding()]
    param()

    $mods = @(
        [pscustomobject]@{
            Id = 'local@maxwell-taskbar-styler'; FallbackId = 'windows-11-taskbar-styler'; Version = '2.0.0'; Library = 'local_at_maxwell-taskbar-styler_2.0.0_owned.dll'
            DllSha256 = 'DD22D7B1B98AC1F640E8E17EEBBA0BA9C6D5A6581FEA915F465937732C845716'
            SourceSha256 = '0C9947F9DB81EB487348E5E5767C26C3485E364362C441A9C927B8A300E401D5'
            CanonicalDll = Join-Path $script:ToolRoot 'build\visual-clones\local_at_maxwell-taskbar-styler_2.0.0_owned.dll'
            CanonicalSource = Join-Path $script:ToolRoot 'mod\visual-clones\maxwell-taskbar-styler.wh.cpp'
            Settings = [ordered]@{
                theme = 'WindowGlass'
                'styleConstants[0]' = 'Background=<WindhawkBlur BlurAmount="32" TintColor="#08090D" TintOpacity="0.60" TintLuminosityOpacity="0.44" TintSaturation="0.68" NoiseOpacity="0.008" FallbackColor="#121318" />'
                'styleConstants[1]' = 'Glass=<WindhawkBlur BlurAmount="32" TintColor="#08090D" TintOpacity="0.60" TintLuminosityOpacity="0.44" TintSaturation="0.68" NoiseOpacity="0.008" FallbackColor="#121318" />'
                'styleConstants[2]' = 'Frosted=<WindhawkBlur BlurAmount="32" TintColor="#08090D" TintOpacity="0.60" TintLuminosityOpacity="0.44" TintSaturation="0.68" NoiseOpacity="0.008" FallbackColor="#121318" />'
                'styleConstants[3]' = 'Acrylic=<WindhawkBlur BlurAmount="32" TintColor="#08090D" TintOpacity="0.60" TintLuminosityOpacity="0.44" TintSaturation="0.68" NoiseOpacity="0.008" FallbackColor="#121318" />'
                'styleConstants[4]' = 'ElementBG=<SolidColorBrush Color="#FFFFFF" Opacity="0.045" />'
                'styleConstants[5]' = 'BorderBrush=<LinearGradientBrush StartPoint="0,0" EndPoint="0,1"><GradientStop Color="#3CFFFFFF" Offset="0" /><GradientStop Color="#14FFFFFF" Offset="0.55" /><GradientStop Color="#06FFFFFF" Offset="1" /></LinearGradientBrush>'
                'controlStyles[0].target' = 'Taskbar.TaskbarFrame > Grid#RootGrid'
                'controlStyles[0].styles[0]' = 'BorderBrush:=$BorderBrush'
                'controlStyles[0].styles[1]' = 'BorderThickness=1'
                'controlStyles[0].styles[2]' = 'Padding=14,0,0,0'
                'controlStyles[0].styles[3]' = 'CornerRadius=8'
                'controlStyles[1].target' = 'Grid#SystemTrayFrameGrid'
                'controlStyles[1].styles[0]' = 'BorderBrush:=$BorderBrush'
                'controlStyles[1].styles[1]' = 'BorderThickness=1'
                'controlStyles[1].styles[2]' = 'Padding=0'
                'controlStyles[1].styles[3]' = 'Margin=14,2,0,2'
                'controlStyles[1].styles[4]' = 'CornerRadius=8'
                'controlStyles[1].styles[5]' = 'Background:=$Background'
                'controlStyles[2].target' = 'SystemTray.DateTimeIconContent > Grid#ContainerGrid'
                'controlStyles[2].styles[0]' = 'Background=Transparent'
                'controlStyles[2].styles[1]' = 'BorderBrush=Transparent'
                'controlStyles[2].styles[2]' = 'BorderThickness=0'
                'controlStyles[2].styles[3]' = 'CornerRadius=0'
                'controlStyles[2].styles[4]' = 'Height=50'
                'controlStyles[2].styles[5]' = 'MinWidth=208'
                'controlStyles[2].styles[6]' = 'Padding=8,0,8,0'
                'controlStyles[2].styles[7]' = 'Margin=8,9,8,9'
                'controlStyles[2].styles[8]' = 'RenderTransform:=<TranslateTransform X="0" Y="-8" />'
                'controlStyles[3].target' = 'TextBlock#TimeInnerTextBlock'
                'controlStyles[3].styles[0]' = 'FontFamily=Segoe UI Variable Display'
                'controlStyles[3].styles[1]' = 'FontWeight=Medium'
                'controlStyles[3].styles[2]' = 'FontSize=26'
                'controlStyles[3].styles[3]' = 'Margin=0'
                'controlStyles[3].styles[4]' = 'Padding=0'
                'controlStyles[3].styles[5]' = 'RenderTransform:=<TranslateTransform X="0" Y="0" />'
                'controlStyles[4].target' = 'TextBlock#DateInnerTextBlock'
                'controlStyles[4].styles[0]' = 'Visibility=Visible'
                'controlStyles[4].styles[1]' = 'FontFamily=Segoe UI Variable Text'
                'controlStyles[4].styles[2]' = 'FontWeight=Medium'
                'controlStyles[4].styles[3]' = 'FontSize=11'
                'controlStyles[4].styles[4]' = 'Margin=0,-2,0,0'
                'controlStyles[4].styles[5]' = 'Foreground:=<SolidColorBrush Color="#A8F5F5F7" />'
                'controlStyles[4].styles[6]' = 'RenderTransform:=<TranslateTransform X="0" Y="-2" />'
                'controlStyles[5].target' = 'SystemTray.DateTimeIconContent > Grid#ContainerGrid'
                'controlStyles[5].styles[0]' = 'Background=Transparent'
                'controlStyles[5].styles[1]' = 'BorderBrush=Transparent'
                'controlStyles[5].styles[2]' = 'BorderThickness=0'
                'controlStyles[5].styles[3]' = 'CornerRadius=0'
                'controlStyles[5].styles[4]' = 'Height=50'
                'controlStyles[5].styles[5]' = 'MinWidth=208'
                'controlStyles[5].styles[6]' = 'Padding=8,0,8,0'
                'controlStyles[5].styles[7]' = 'Margin=8,9,8,9'
                'controlStyles[5].styles[8]' = 'RenderTransform:=<TranslateTransform X="0" Y="-8" />'
                'controlStyles[6].target' = 'Taskbar.TaskListLabeledButtonPanel@RunningIndicatorStates > Rectangle#RunningIndicator'
                'controlStyles[6].styles[0]' = 'Height=2'
                'controlStyles[6].styles[1]' = 'RadiusX=2'
                'controlStyles[6].styles[2]' = 'RadiusY=2'
                'controlStyles[6].styles[3]' = 'Width@InactiveRunningIndicator=5'
                'controlStyles[6].styles[4]' = 'Fill@InactiveRunningIndicator:=<SolidColorBrush Color="#FFFFFF" Opacity="0.38" />'
                'controlStyles[6].styles[5]' = 'Width@ActiveRunningIndicator=12'
                'controlStyles[6].styles[6]' = 'Fill@ActiveRunningIndicator:=<SolidColorBrush Color="#A85C2A" Opacity="0.96" />'
                'controlStyles[6].styles[7]' = 'Width@RequestingAttentionRunningIndicator=12'
                'controlStyles[6].styles[8]' = 'Fill@RequestingAttentionRunningIndicator:=<SolidColorBrush Color="#FFFFB900" Opacity="0.96" />'
                'controlStyles[6].styles[9]' = 'Margin=0,0,0,2'
                'controlStyles[7].target' = 'Grid#IconPanel@RunningIndicatorStates > Border#BackgroundElement, Taskbar.TaskListLabeledButtonPanel@RunningIndicatorStates > Border#BackgroundElement'
                'controlStyles[7].styles[0]' = 'CornerRadius=4'
                'controlStyles[7].styles[1]' = 'Background@NoRunningIndicator=Transparent'
                'controlStyles[7].styles[2]' = 'Background@InactiveRunningIndicator=Transparent'
                'controlStyles[7].styles[3]' = 'Background@InactiveRunningIndicatorPointerOver:=<SolidColorBrush Color="#FFFFFF" Opacity="0.045" />'
                'controlStyles[7].styles[4]' = 'Background@ActiveRunningIndicator:=<SolidColorBrush Color="#A85C2A" Opacity="0.075" />'
                'controlStyles[7].styles[5]' = 'Background@ActiveRunningIndicatorPointerOver:=<SolidColorBrush Color="#A85C2A" Opacity="0.13" />'
                'controlStyles[7].styles[6]' = 'Background@RequestingAttentionRunningIndicator:=<SolidColorBrush Color="#FFFFB900" Opacity="0.14" />'
                'controlStyles[7].styles[7]' = 'BorderThickness@ActiveRunningIndicator=1'
                'controlStyles[7].styles[8]' = 'BorderBrush@ActiveRunningIndicator:=<SolidColorBrush Color="#A85C2A" Opacity="0.24" />'
                'controlStyles[8].target' = 'Taskbar.TaskListButton#TaskListButton > Grid#IconPanel@CommonStates, Taskbar.TaskListButton#TaskListButton > Taskbar.TaskListLabeledButtonPanel#IconPanel@CommonStates'
                'controlStyles[8].styles[0]' = 'RenderTransformOrigin=0.5,0.5'
                'controlStyles[8].styles[1]' = 'RenderTransform@InactivePointerOver:=<TransformGroup><ScaleTransform ScaleX="1.05" ScaleY="1.05" /></TransformGroup>'
                'controlStyles[8].styles[2]' = 'RenderTransform@ActivePointerOver:=<TransformGroup><ScaleTransform ScaleX="1.05" ScaleY="1.05" /></TransformGroup>'
                'controlStyles[8].styles[3]' = 'RenderTransform@MultiWindowPointerOver:=<TransformGroup><ScaleTransform ScaleX="1.05" ScaleY="1.05" /></TransformGroup>'
                'controlStyles[8].styles[4]' = 'RenderTransform@RequestingAttentionPointerOver:=<TransformGroup><ScaleTransform ScaleX="1.05" ScaleY="1.05" /></TransformGroup>'
                'controlStyles[8].styles[5]' = 'RenderTransform@InactivePressed:=<TransformGroup><ScaleTransform ScaleX="0.97" ScaleY="0.97" /></TransformGroup>'
                'controlStyles[8].styles[6]' = 'RenderTransform@ActivePressed:=<TransformGroup><ScaleTransform ScaleX="0.97" ScaleY="0.97" /></TransformGroup>'
                'controlStyles[8].styles[7]' = 'RenderTransform@MultiWindowPressed:=<TransformGroup><ScaleTransform ScaleX="0.97" ScaleY="0.97" /></TransformGroup>'
                'controlStyles[8].styles[8]' = 'RenderTransform@RequestingAttentionPressed:=<TransformGroup><ScaleTransform ScaleX="0.97" ScaleY="0.97" /></TransformGroup>'
                'controlStyles[9].target' = 'SystemTray.Stack#SecondaryClockStack'
                'controlStyles[9].styles[0]' = 'RenderTransform:=<TranslateTransform X="4" Y="0" />'
                'controlStyles[10].target' = 'Taskbar.ExperienceToggleButton#LaunchListButton[AutomationProperties.AutomationId=TaskViewButton]'
                'controlStyles[10].styles[0]' = 'Margin=0,0,12,0'
                'controlStyles[11].target' = 'Grid#IconPanel > Taskbar.Badge#BadgeControl, Taskbar.TaskListLabeledButtonPanel#IconPanel > Taskbar.Badge#BadgeControl'
                'controlStyles[11].styles[0]' = 'MinWidth=12'
                'controlStyles[11].styles[1]' = 'Width=12'
                'controlStyles[11].styles[2]' = 'Height=12'
                'controlStyles[11].styles[3]' = 'CornerRadius=4'
                'controlStyles[11].styles[4]' = 'Background:=<SolidColorBrush Color="#A85C2A" Opacity="0.96" />'
                'controlStyles[11].styles[5]' = 'Foreground:=<SolidColorBrush Color="#F5F5F7" />'
                'controlStyles[11].styles[6]' = 'BorderBrush:=<SolidColorBrush Color="#A85C2A" Opacity="0.72" />'
                'controlStyles[11].styles[7]' = 'BorderThickness=1'
                'controlStyles[11].styles[8]' = 'Canvas.ZIndex=3'
                'controlStyles[12].target' = 'Grid#IconPanel > Taskbar.Badge#BadgeControl > Grid > TextBlock#BadgeText, Taskbar.TaskListLabeledButtonPanel#IconPanel > Taskbar.Badge#BadgeControl > Grid > TextBlock#BadgeText'
                'controlStyles[12].styles[0]' = 'FontFamily=Segoe UI Variable Text'
                'controlStyles[12].styles[1]' = 'FontSize=8'
                'controlStyles[12].styles[2]' = 'FontWeight=SemiBold'
                'controlStyles[13].target' = 'Grid#IconPanel > Windows.UI.Xaml.Controls.Image#Icon, Taskbar.TaskListLabeledButtonPanel#IconPanel > Windows.UI.Xaml.Controls.Image#Icon'
                'controlStyles[13].styles[0]' = 'MaxWidth=30'
                'controlStyles[13].styles[1]' = 'MaxHeight=30'
                'controlStyles[13].styles[2]' = 'Stretch=Uniform'
                'controlStyles[14].target' = 'SystemTray.ChevronIconView > Grid#ContainerGrid@CommonStates > Border#BackgroundBorder, SystemTray.NotifyIconView > Grid#ContainerGrid@CommonStates > Border#BackgroundBorder, SystemTray.IconView#SystemTrayIcon > Grid#ContainerGrid@CommonStates > Border#BackgroundBorder'
                'controlStyles[14].styles[0]' = 'CornerRadius=4'
                'controlStyles[14].styles[1]' = 'Margin=2,5,2,5'
                'controlStyles[14].styles[2]' = 'Background@Normal=Transparent'
                'controlStyles[14].styles[3]' = 'Background@PointerOver:=<SolidColorBrush Color="#FFFFFF" Opacity="0.07" />'
                'controlStyles[14].styles[4]' = 'Background@Pressed:=<SolidColorBrush Color="#A85C2A" Opacity="0.12" />'
                'controlStyles[14].styles[5]' = 'BorderBrush=Transparent'
                'controlStyles[14].styles[6]' = 'BorderBrush@PointerOver:=<SolidColorBrush Color="#A85C2A" Opacity="0.12" />'
                'controlStyles[14].styles[7]' = 'BorderBrush@Pressed:=<SolidColorBrush Color="#A85C2A" Opacity="0.24" />'
                'controlStyles[14].styles[8]' = 'BorderThickness=1'
                'controlStyles[15].target' = 'SystemTray.OmniButton > Grid@CommonStates > Border#BackgroundBorder'
                'controlStyles[15].styles[0]' = 'CornerRadius=4'
                'controlStyles[15].styles[1]' = 'Margin=2,4,2,4'
                'controlStyles[15].styles[2]' = 'Background@Normal=Transparent'
                'controlStyles[15].styles[3]' = 'Background@PointerOver:=<SolidColorBrush Color="#FFFFFF" Opacity="0.07" />'
                'controlStyles[15].styles[4]' = 'Background@Pressed:=<SolidColorBrush Color="#A85C2A" Opacity="0.12" />'
                'controlStyles[15].styles[5]' = 'BorderBrush=Transparent'
                'controlStyles[15].styles[6]' = 'BorderBrush@PointerOver:=<SolidColorBrush Color="#A85C2A" Opacity="0.12" />'
                'controlStyles[15].styles[7]' = 'BorderBrush@Pressed:=<SolidColorBrush Color="#A85C2A" Opacity="0.24" />'
                'controlStyles[15].styles[8]' = 'BorderThickness=1'
            }
        }
        [pscustomobject]@{
            Id = 'local@maxwell-taskbar-icons'; FallbackId = 'taskbar-icon-size'; Version = '2.0.0'; Library = 'local_at_maxwell-taskbar-icons_2.0.0_owned.dll'
            DllSha256 = '8EFB7FCC268F14F7B60FF77C26075F408BFC4E6347568EC5EABE70D59A5E5ACB'
            SourceSha256 = '958CDF776950485F8ADF5E61427CB924A886354EF109A270F981336B6AF855B9'
            CanonicalDll = Join-Path $script:ToolRoot 'build\visual-clones\local_at_maxwell-taskbar-icons_2.0.0_owned.dll'
            CanonicalSource = Join-Path $script:ToolRoot 'mod\visual-clones\maxwell-taskbar-icons.wh.cpp'
            # Use the native 32 px icon asset instead of scaling it to 40 px.
            # A 64 px bar keeps the glass/widgets spacious without soft icons.
            Settings = [ordered]@{ TaskbarHeight = 64; IconSize = 32; TaskbarButtonWidth = 52; IconSizeSmall = 16; TaskbarButtonWidthSmall = 32 }
        }
        [pscustomobject]@{
            Id = 'local@maxwell-taskbar-clock'; FallbackId = 'taskbar-clock-customization'; Version = '2.2.0'; Library = 'local_at_maxwell-taskbar-clock_2.2.0_owned.dll'
            DllSha256 = '5FFD04CD09FC6B543DA61FFE720677F2F93975875999D4ECAAF3627F6B875BC2'
            SourceSha256 = '3F60395547442547FA15E333316F1B3BB145D255ABC7A835560E08D925331311'
            CanonicalDll = Join-Path $script:ToolRoot 'build\visual-clones\local_at_maxwell-taskbar-clock_2.2.0_owned.dll'
            CanonicalSource = Join-Path $script:ToolRoot 'mod\visual-clones\maxwell-taskbar-clock.wh.cpp'
            Settings = [ordered]@{
                ShowSeconds = 0; TimeFormat = "h':'mm"; DateFormat = 'MMM d'; WeekdayFormat = 'custom'; WeekdayFormatCustom = 'Sun, Mon, Tue, Wed, Thu, Fri, Sat'
                TopLine = '%time%'; MiddleLine = ''; BottomLine = ('%weekday%{0}{1}{0}%date%{0}{1}{0}%weather%{0}{1}{0}%ambient_status%' -f [char]0x2009, [char]0x00B7)
                TooltipLine = ('%date% | %time% | BAT %battery% %battery_time% | {0}%download_speed% {1}%upload_speed%' -f [char]0x2193, [char]0x2191); TooltipLineMode = 'replace'; Width = 360; Height = 50; MaxWidth = 360; TextSpacing = [long]4294967294
                WebContentWeatherLocation = 'Katy,Texas'; WebContentWeatherFormat = '%c %t'; WebContentWeatherUnits = 'uscs'
                'DataCollection.UpdateInterval' = 10; 'DataCollection.NetworkIdleThresholdKBps' = 2; 'DataCollection.NetworkMetricsFormat' = 'mbsDynamic'; 'DataCollection.NetworkMetricsFixedDecimals' = 1
                'WebContentsItems[0].Url' = ''; 'TimeStyle.TextColor' = '#FFF5F5F7'; 'TimeStyle.TextAlignment' = 'Center'; 'TimeStyle.FontSize' = 26
                'TimeStyle.FontFamily' = 'Segoe UI Variable Display'; 'TimeStyle.FontWeight' = 'Medium'; 'TimeStyle.CharacterSpacing' = [long]4294967293
                'TimeStyle.LineHeight' = 27; 'DateStyle.Hidden' = 0; 'DateStyle.TextColor' = '#A8F5F5F7'; 'DateStyle.TextAlignment' = 'Center'
                'DateStyle.FontSize' = 11; 'DateStyle.FontFamily' = 'Segoe UI Variable Text'; 'DateStyle.FontWeight' = 'Medium'
                'DateStyle.FontStretch' = 'Normal'; 'DateStyle.CharacterSpacing' = 20; 'DateStyle.LineHeight' = 13
            }
        }
    )

    [pscustomobject]@{
        ToolRoot = $script:ToolRoot; StateRoot = $script:StateRoot; CanonicalRoot = $script:CanonicalRoot
        LogPath = Join-Path $script:StateRoot 'safe-dock.log'; StatePath = Join-Path $script:StateRoot 'state.json'
        LatchPath = Join-Path $script:StateRoot 'circuit-breaker-latched.json'
        EngineModRoot = 'C:\ProgramData\Windhawk\Engine\Mods\64'; SourceRoot = 'C:\ProgramData\Windhawk\ModsSource'
        StableMods = $mods
        UnsafeIds = @('taskbar-clock-customization','taskbar-icon-size','windows-11-taskbar-styler','local@maxwell-taskbar-experience','local@maxwell-window-finish','taskbar-dock-animation')
        RetiredTaskNames = @('Maxwell Windhawk ChatGPT Guard','Maxwell Windhawk Material Sync')
        RepairTaskName = 'Maxwell Windhawk Safe Dock Repair'; CircuitTaskName = 'Maxwell Windhawk Dock Circuit Breaker'
        EvidenceRetryDelaysSeconds = @(2,3,5,8,13)
    }
}

function Get-WindhawkSafeDockConfig {
    [CmdletBinding()]
    param()

    $receiptPath = Join-Path $script:ToolRoot 'build\opal-suite\build-receipt.json'
    if (-not (Test-Path -LiteralPath $receiptPath)) {
        throw "The canonical Opal build receipt is missing: $receiptPath"
    }
    $receipt = @(Get-Content -LiteralPath $receiptPath -Raw | ConvertFrom-Json)
    $expectedIds = @('local@opal')
    if ($receipt.Count -ne 1 -or (Compare-Object $expectedIds @($receipt.localId))) {
        throw 'The canonical Opal build receipt is not the single unified mod.'
    }
    $mods = @($receipt | ForEach-Object {
        [pscustomobject]@{
            Id = [string]$_.localId
            FallbackId = [string]$_.localId
            Version = [string]$_.version
            Library = [string]$_.dllName
            Include = [string]$_.include
            DllSha256 = ([string]$_.sha256).ToUpperInvariant()
            SourceSha256 = ([string]$_.packageSourceSha256).ToUpperInvariant()
            CanonicalDll = [string]$_.output
            CanonicalSource = [string]$_.packageSource
            Settings = [ordered]@{}
        }
    })

    [pscustomobject]@{
        ToolRoot = $script:ToolRoot; StateRoot = $script:StateRoot; CanonicalRoot = $script:CanonicalRoot
        LogPath = Join-Path $script:StateRoot 'safe-dock.log'; StatePath = Join-Path $script:StateRoot 'state.json'
        LatchPath = Join-Path $script:StateRoot 'circuit-breaker-latched.json'
        EngineModRoot = 'C:\ProgramData\Windhawk\Engine\Mods\64'; SourceRoot = 'C:\ProgramData\Windhawk\ModsSource'
        StableMods = $mods; UnsafeIds = @()
        RetiredTaskNames = @('Maxwell Windhawk ChatGPT Guard','Maxwell Windhawk Material Sync')
        RepairTaskName = 'Maxwell Windhawk Safe Dock Repair'; CircuitTaskName = 'Maxwell Windhawk Dock Circuit Breaker'
        EvidenceRetryDelaysSeconds = @(2,3,5,8,13)
    }
}

function Test-WindhawkAdministrator {
    $identity = [Security.Principal.WindowsIdentity]::GetCurrent()
    $principal = [Security.Principal.WindowsPrincipal]::new($identity)
    $principal.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)
}

function Get-WindhawkFileHash {
    param([Parameter(Mandatory)][string]$Path)
    if (-not (Test-Path -LiteralPath $Path)) { return $null }
    (Get-FileHash -LiteralPath $Path -Algorithm SHA256).Hash.ToUpperInvariant()
}

function Get-WindhawkRegistryValue {
    param([Parameter(Mandatory)][string]$Path, [Parameter(Mandatory)][string]$Name)
    if (-not (Test-Path -LiteralPath $Path)) { return $null }
    $item = Get-ItemProperty -LiteralPath $Path -Name $Name -ErrorAction SilentlyContinue
    if (-not $item) { return $null }
    $item.$Name
}

function Test-WindhawkValueEqual {
    param($Actual, $Expected)
    if ($null -eq $Actual -and $null -eq $Expected) { return $true }
    if ($null -eq $Actual -or $null -eq $Expected) { return $false }
    [string]::Equals([string]$Actual, [string]$Expected, [StringComparison]::Ordinal)
}

function Get-WindhawkCircuitLatch {
    $config = Get-WindhawkSafeDockConfig
    if (-not (Test-Path -LiteralPath $config.LatchPath)) { return $null }
    try { Get-Content -LiteralPath $config.LatchPath -Raw | ConvertFrom-Json }
    catch { [pscustomobject]@{ Latched = $true; Reason = 'Unreadable latch fails closed.' } }
}

function Write-WindhawkSafeDockLog {
    param([Parameter(Mandatory)][string]$Message)
    $config = Get-WindhawkSafeDockConfig
    New-Item -ItemType Directory -Path $config.StateRoot -Force | Out-Null
    if ((Test-Path -LiteralPath $config.LogPath) -and (Get-Item -LiteralPath $config.LogPath).Length -gt 1MB) {
        Move-Item -LiteralPath $config.LogPath -Destination ($config.LogPath + '.previous') -Force
    }
    Add-Content -LiteralPath $config.LogPath -Value ('{0:o} {1}' -f [DateTimeOffset]::Now, $Message)
}

function Test-WindhawkSafeDockState {
    [CmdletBinding()]
    param([switch]$RequireLoaded)

    $config = Get-WindhawkSafeDockConfig
    $latch = Get-WindhawkCircuitLatch
    $latched = [bool]($latch -and $latch.Latched)
    $modDrift = [Collections.Generic.List[string]]::new()
    $safetyDrift = [Collections.Generic.List[string]]::new()
    $systemDrift = [Collections.Generic.List[string]]::new()

    $service = Get-Service Windhawk -ErrorAction SilentlyContinue
    if (-not $service) { $systemDrift.Add('Windhawk service is missing.') }
    else {
        if ($service.Status -ne 'Running') { $systemDrift.Add("Windhawk service status is $($service.Status).") }
        if ($service.StartType -ne 'Automatic') { $systemDrift.Add("Windhawk service start type is $($service.StartType).") }
    }
    foreach ($taskName in $config.RetiredTaskNames) {
        $task = Get-ScheduledTask -TaskName $taskName -ErrorAction SilentlyContinue
        if ($task -and $task.Settings.Enabled) { $safetyDrift.Add("Retired task is enabled: $taskName") }
    }

    $expectedDisabled = if ($latched) { 1 } else { 0 }
    foreach ($mod in $config.StableMods) {
        if ($latched) {
            foreach ($store in @('Mods','ModsWritable')) {
                $root = "HKLM:\SOFTWARE\Windhawk\Engine\$store\$($mod.Id)"
                if ((Test-Path -LiteralPath $root) -and -not (Test-WindhawkValueEqual (Get-WindhawkRegistryValue $root 'Disabled') 1)) {
                    $modDrift.Add("$store/$($mod.Id): latched fallback requires Disabled=1.")
                }
            }
            continue
        }
        $dllPath = Join-Path $config.EngineModRoot $mod.Library
        $sourcePath = Join-Path $config.SourceRoot ($mod.Id + '.wh.cpp')
        if ((Get-WindhawkFileHash $dllPath) -ne $mod.DllSha256) { $modDrift.Add("$($mod.Id): DLL hash drift.") }
        if ((Get-WindhawkFileHash $sourcePath) -ne $mod.SourceSha256) { $modDrift.Add("$($mod.Id): source hash drift.") }
        foreach ($store in @('Mods','ModsWritable')) {
            $root = "HKLM:\SOFTWARE\Windhawk\Engine\$store\$($mod.Id)"
            if (-not (Test-Path -LiteralPath $root)) { $modDrift.Add("$store/$($mod.Id): key missing."); continue }
            if (-not (Test-WindhawkValueEqual (Get-WindhawkRegistryValue $root 'Disabled') $expectedDisabled)) { $modDrift.Add("$store/$($mod.Id): Disabled must be $expectedDisabled.") }
            if ($store -eq 'Mods') {
                if (-not (Test-WindhawkValueEqual (Get-WindhawkRegistryValue $root 'LibraryFileName') $mod.Library)) { $modDrift.Add("$store/$($mod.Id): library drift.") }
                if (-not (Test-WindhawkValueEqual (Get-WindhawkRegistryValue $root 'Version') $mod.Version)) { $modDrift.Add("$store/$($mod.Id): version drift.") }
                if (-not (Test-WindhawkValueEqual (Get-WindhawkRegistryValue $root 'Include') $mod.Include)) { $modDrift.Add("$store/$($mod.Id): process scope drift.") }
            }
            $settingsPath = Join-Path $root 'Settings'
            foreach ($entry in $mod.Settings.GetEnumerator()) {
                if (-not (Test-WindhawkValueEqual (Get-WindhawkRegistryValue $settingsPath $entry.Key) $entry.Value)) { $modDrift.Add("$store/$($mod.Id): setting drift: $($entry.Key)") }
            }
        }
    }

    foreach ($id in $config.UnsafeIds) {
        foreach ($store in @('Mods','ModsWritable')) {
            $root = "HKLM:\SOFTWARE\Windhawk\Engine\$store\$id"
            if ((Test-Path -LiteralPath $root) -and -not (Test-WindhawkValueEqual (Get-WindhawkRegistryValue $root 'Disabled') 1)) { $safetyDrift.Add("Unsafe mod is enabled: $store/$id") }
        }
    }
    $expectedIds = @($config.StableMods | ForEach-Object Id)
    foreach ($store in @('Mods','ModsWritable')) {
        $storeRoot = "HKLM:\SOFTWARE\Windhawk\Engine\$store"
        foreach ($key in @(Get-ChildItem -LiteralPath $storeRoot -ErrorAction SilentlyContinue)) {
            if ($key.PSChildName -notin $expectedIds) {
                $safetyDrift.Add("Obsolete mod key remains: $store/$($key.PSChildName)")
            }
        }
    }

    $explorer = Get-Process explorer -ErrorAction SilentlyContinue | Sort-Object StartTime | Select-Object -First 1
    $loaded = @()
    if ($explorer) {
        try { $loaded = @($explorer.Modules | Where-Object { $_.FileName -match '\\Windhawk\\Engine\\Mods\\' } | Select-Object -ExpandProperty ModuleName) }
        catch { $systemDrift.Add('Explorer module inventory unavailable.') }
    } else { $systemDrift.Add('Explorer is not running.') }
    foreach ($id in $config.UnsafeIds) { if ($loaded -match [regex]::Escape($id)) { $safetyDrift.Add("Unsafe mod loaded in Explorer: $id") } }
    if ($RequireLoaded -and -not $latched) { foreach ($mod in $config.StableMods) { if ($loaded -notcontains $mod.Library) { $modDrift.Add("Stable mod not loaded: $($mod.Id)") } } }

    $allDrift = @($modDrift) + @($safetyDrift) + @($systemDrift)
    [pscustomobject]@{
        Healthy = $allDrift.Count -eq 0; Mode = if ($latched) { 'stock-fallback' } else { 'opal-unified' }
        CircuitBreakerLatched = $latched; Latch = $latch; ModDrift = @($modDrift); SafetyDrift = @($safetyDrift)
        SystemDrift = @($systemDrift); Drift = $allDrift; ExplorerPid = if ($explorer) { $explorer.Id } else { $null }
        LoadedModules = $loaded; CheckedAt = [DateTimeOffset]::Now
    }
}

function Set-WindhawkRegistryValueIfDifferent {
    param([string]$Path, [string]$Name, $Value, [Collections.Generic.List[string]]$Actions, [switch]$DryRun)
    if (Test-WindhawkValueEqual (Get-WindhawkRegistryValue $Path $Name) $Value) { return $false }
    $Actions.Add("Set $Path::$Name")
    if (-not $DryRun) {
        if (-not (Test-Path -LiteralPath $Path)) { New-Item -Path $Path -Force | Out-Null }
        $kind = if ($Value -is [int] -or $Value -is [long]) { 'DWord' } else { 'String' }
        New-ItemProperty -LiteralPath $Path -Name $Name -Value $Value -PropertyType $kind -Force | Out-Null
    }
    $true
}

function Import-WindhawkCanonicalRegistry {
    param($Mod, [ValidateSet('Mods','ModsWritable')][string]$Store, [Collections.Generic.List[string]]$Actions, [switch]$DryRun)
    $sourceNative = "HKLM\SOFTWARE\Windhawk\Engine\$Store\$($Mod.FallbackId)"
    $targetNative = "HKLM\SOFTWARE\Windhawk\Engine\$Store\$($Mod.Id)"
    $sourcePath = "HKLM:\SOFTWARE\Windhawk\Engine\$Store\$($Mod.FallbackId)"
    if (-not (Test-Path -LiteralPath $sourcePath)) { throw "Fallback registry source missing: $sourcePath" }
    $Actions.Add("Clone $Store/$($Mod.FallbackId) to $Store/$($Mod.Id)")
    if (-not $DryRun) { & reg.exe copy $sourceNative $targetNative /s /f | Out-Null; if ($LASTEXITCODE -ne 0) { throw "Registry clone failed: $sourceNative -> $targetNative" } }
}

function Start-WindhawkServiceReliable {
    param([switch]$Restart)
    $service = Get-Service Windhawk -ErrorAction Stop
    if ($Restart -and $service.Status -ne 'Stopped') {
        Stop-Service Windhawk -Force -ErrorAction Stop
        (Get-Service Windhawk).WaitForStatus('Stopped',[TimeSpan]::FromSeconds(15))
        Start-Sleep -Seconds 2
    }
    $errors = [Collections.Generic.List[string]]::new()
    for ($attempt = 1; $attempt -le 3; $attempt++) {
        try {
            $service = Get-Service Windhawk -ErrorAction Stop
            if ($service.Status -ne 'Running') { Start-Service Windhawk -ErrorAction Stop }
            (Get-Service Windhawk).WaitForStatus('Running',[TimeSpan]::FromSeconds(12))
            return
        } catch {
            $errors.Add("attempt ${attempt}: $($_.Exception.Message)")
            if ($attempt -lt 3) { Start-Sleep -Seconds (2 * $attempt) }
        }
    }
    throw ('Windhawk failed to reach Running after three bounded attempts. ' + ($errors -join ' | '))
}

function Restart-WindhawkExplorerOnce {
    param([switch]$DryRun)
    if ($DryRun) { return }
    Start-WindhawkServiceReliable -Restart
    Get-Process explorer -ErrorAction SilentlyContinue | Stop-Process -Force
    $deadline = (Get-Date).AddSeconds(15)
    do { Start-Sleep -Milliseconds 400; $explorer = Get-Process explorer -ErrorAction SilentlyContinue | Sort-Object StartTime | Select-Object -First 1 } while (-not $explorer -and (Get-Date) -lt $deadline)
    if (-not $explorer) { Start-Process 'C:\Windows\explorer.exe'; Start-Sleep -Seconds 3 }
}

function Invoke-WindhawkSafeDockRepair {
    [CmdletBinding()]
    param([switch]$DryRun, [switch]$AllowExplorerRestart)
    $config = Get-WindhawkSafeDockConfig
    $before = Test-WindhawkSafeDockState
    $actions = [Collections.Generic.List[string]]::new()
    if ($before.Healthy) { return [pscustomobject]@{ Changed = $false; Actions = @(); Before = $before; After = $before } }
    if (-not $DryRun -and -not (Test-WindhawkAdministrator)) { throw 'Administrator access is required for repair.' }
    $expectedStableDisabled = if ($before.CircuitBreakerLatched) { 1 } else { 0 }
    $now = [int][DateTimeOffset]::UtcNow.ToUnixTimeSeconds()
    $liveModChange = $false

    foreach ($taskName in $config.RetiredTaskNames) {
        $task = Get-ScheduledTask -TaskName $taskName -ErrorAction SilentlyContinue
        if ($task -and $task.Settings.Enabled) { $actions.Add("Disable retired task: $taskName"); if (-not $DryRun) { Disable-ScheduledTask -TaskName $taskName | Out-Null } }
    }
    foreach ($mod in $config.StableMods) {
        if ($before.CircuitBreakerLatched) {
            foreach ($store in @('Mods','ModsWritable')) {
                $root = "HKLM:\SOFTWARE\Windhawk\Engine\$store\$($mod.Id)"
                if (-not (Test-Path -LiteralPath $root)) { continue }
                if (Set-WindhawkRegistryValueIfDifferent $root 'Disabled' 1 $actions -DryRun:$DryRun) {
                    if (-not $DryRun) { New-ItemProperty $root -Name SettingsChangeTime -Value $now -PropertyType DWord -Force | Out-Null }
                    $liveModChange = $true
                }
            }
            continue
        }
        $dllCanonical = $mod.CanonicalDll; $sourceCanonical = $mod.CanonicalSource
        if ((Get-WindhawkFileHash $dllCanonical) -ne $mod.DllSha256) { throw "Frozen DLL hash mismatch: $dllCanonical" }
        if ((Get-WindhawkFileHash $sourceCanonical) -ne $mod.SourceSha256) { throw "Frozen source hash mismatch: $sourceCanonical" }
        $dllLive = Join-Path $config.EngineModRoot $mod.Library; $sourceLive = Join-Path $config.SourceRoot ($mod.Id + '.wh.cpp')
        if ((Get-WindhawkFileHash $dllLive) -ne $mod.DllSha256) { $actions.Add("Restore DLL: $($mod.Id)"); if (-not $DryRun) { Copy-Item $dllCanonical $dllLive -Force }; $liveModChange = $true }
        if ((Get-WindhawkFileHash $sourceLive) -ne $mod.SourceSha256) { $actions.Add("Restore source: $($mod.Id)"); if (-not $DryRun) { Copy-Item $sourceCanonical $sourceLive -Force }; $liveModChange = $true }
        foreach ($store in @('Mods','ModsWritable')) {
            $root = "HKLM:\SOFTWARE\Windhawk\Engine\$store\$($mod.Id)"; $changed = $false
            if (-not (Test-Path -LiteralPath $root)) {
                $actions.Add("Create $store/$($mod.Id)")
                if (-not $DryRun) { New-Item -Path $root -Force | Out-Null }
                $changed = $true
            }
            if ($store -eq 'Mods') {
                $changed = (Set-WindhawkRegistryValueIfDifferent $root 'LibraryFileName' $mod.Library $actions -DryRun:$DryRun) -or $changed
                $changed = (Set-WindhawkRegistryValueIfDifferent $root 'Version' $mod.Version $actions -DryRun:$DryRun) -or $changed
                $changed = (Set-WindhawkRegistryValueIfDifferent $root 'Include' $mod.Include $actions -DryRun:$DryRun) -or $changed
                $changed = (Set-WindhawkRegistryValueIfDifferent $root 'Exclude' '' $actions -DryRun:$DryRun) -or $changed
                $changed = (Set-WindhawkRegistryValueIfDifferent $root 'Architecture' 'x86-64' $actions -DryRun:$DryRun) -or $changed
            }
            $changed = (Set-WindhawkRegistryValueIfDifferent $root 'Disabled' $expectedStableDisabled $actions -DryRun:$DryRun) -or $changed
            $settingsPath = Join-Path $root 'Settings'
            foreach ($entry in $mod.Settings.GetEnumerator()) { $changed = (Set-WindhawkRegistryValueIfDifferent $settingsPath $entry.Key $entry.Value $actions -DryRun:$DryRun) -or $changed }
            if ($changed) { $actions.Add("Signal one reload: $store/$($mod.Id)"); if (-not $DryRun) { New-ItemProperty $root -Name SettingsChangeTime -Value $now -PropertyType DWord -Force | Out-Null }; $liveModChange = $true }
        }
    }
    foreach ($id in $config.UnsafeIds) { foreach ($store in @('Mods','ModsWritable')) {
        $root = "HKLM:\SOFTWARE\Windhawk\Engine\$store\$id"; if (-not (Test-Path $root)) { continue }
        if (Set-WindhawkRegistryValueIfDifferent $root 'Disabled' 1 $actions -DryRun:$DryRun) { if (-not $DryRun) { New-ItemProperty $root -Name SettingsChangeTime -Value $now -PropertyType DWord -Force | Out-Null }; $liveModChange = $true }
    } }
    $expectedIds = @($config.StableMods | ForEach-Object Id)
    foreach ($store in @('Mods','ModsWritable')) {
        $storeRoot = "HKLM:\SOFTWARE\Windhawk\Engine\$store"
        foreach ($key in @(Get-ChildItem -LiteralPath $storeRoot -ErrorAction SilentlyContinue)) {
            if ($key.PSChildName -in $expectedIds) { continue }
            $actions.Add("Remove obsolete registry key: $store/$($key.PSChildName)")
            if (-not $DryRun) { Remove-Item -LiteralPath $key.PSPath -Recurse -Force }
            $liveModChange = $true
        }
    }
    $service = Get-Service Windhawk -ErrorAction Stop
    if ($service.StartType -ne 'Automatic') { $actions.Add('Set Windhawk service Automatic'); if (-not $DryRun) { Set-Service Windhawk -StartupType Automatic } }
    if ($service.Status -ne 'Running') { $actions.Add('Start Windhawk service with bounded retry'); if (-not $DryRun) { Start-WindhawkServiceReliable } }
    if ($liveModChange -and $AllowExplorerRestart) { $actions.Add('Restart Windhawk and Explorer once'); Restart-WindhawkExplorerOnce -DryRun:$DryRun }
    if (-not $DryRun) { Start-Sleep -Seconds 4 }
    $after = if ($DryRun) { $before } else { Test-WindhawkSafeDockState -RequireLoaded:(-not $before.CircuitBreakerLatched) }
    if (-not $DryRun) { New-Item -ItemType Directory -Path $config.StateRoot -Force | Out-Null; $after | ConvertTo-Json -Depth 8 | Set-Content $config.StatePath -Encoding UTF8; Write-WindhawkSafeDockLog ("Repair changed={0}; actions={1}; mode={2}" -f ($actions.Count -gt 0),$actions.Count,$after.Mode) }
    [pscustomobject]@{ Changed = $actions.Count -gt 0; Actions = @($actions); Before = $before; After = $after }
}

function Test-WindhawkCrashEvidence {
    [CmdletBinding()]
    param([Parameter(Mandatory)][string]$EventMessage, [Parameter(Mandatory)][string]$WerText)
    $config = Get-WindhawkSafeDockConfig
    if ($EventMessage -notmatch '(?i)Faulting application name:\s*Explorer\.EXE') { return $false }
    if ($EventMessage -notmatch '(?i)Faulting module name:\s*(Windows\.UI\.Xaml\.dll|Microsoft\.UI\.Xaml\.dll|Taskbar\.View\.dll)') { return $false }
    foreach ($mod in $config.StableMods) { if ($WerText -match [regex]::Escape($mod.Library)) { return $true } }
    $false
}

function Get-WindhawkLatestCrashEvidence {
    [CmdletBinding()]
    param([datetime]$Since = (Get-Date).AddMinutes(-5))
    $events = Get-WinEvent -FilterHashtable @{LogName='Application';Id=1000;StartTime=$Since} -ErrorAction SilentlyContinue | Where-Object { $_.ProviderName -eq 'Application Error' -and $_.Message -match '(?i)Faulting application name:\s*Explorer\.EXE' } | Sort-Object TimeCreated -Descending
    $reports = Get-ChildItem 'C:\ProgramData\Microsoft\Windows\WER\ReportArchive' -Directory -Filter 'AppCrash_Explorer.EXE*' -ErrorAction SilentlyContinue | Sort-Object LastWriteTime -Descending
    foreach ($appEvent in $events) {
        if ($appEvent.Message -notmatch '(?i)Faulting module name:\s*(Windows\.UI\.Xaml\.dll|Microsoft\.UI\.Xaml\.dll|Taskbar\.View\.dll)') { continue }
        foreach ($report in $reports) {
            if ($report.LastWriteTime -lt $appEvent.TimeCreated.AddMinutes(-2) -or $report.LastWriteTime -gt $appEvent.TimeCreated.AddMinutes(3)) { continue }
            $path = Join-Path $report.FullName 'Report.wer'; if (-not (Test-Path $path)) { continue }; $text = Get-Content $path -Raw -ErrorAction SilentlyContinue
            if ($text -and (Test-WindhawkCrashEvidence $appEvent.Message $text)) { return [pscustomobject]@{ EventMessage=$appEvent.Message; WerText=$text; WerPath=$path } }
        }
    }
    $null
}

function Invoke-WindhawkSafeDockCircuitBreak {
    [CmdletBinding()]
    param([string]$EventMessage, [string]$WerText, [switch]$DryRun)
    $config = Get-WindhawkSafeDockConfig
    if ($EventMessage -or $WerText) { if (-not ($EventMessage -and $WerText)) { throw 'EventMessage and WerText must be supplied together.' }; $evidence = if (Test-WindhawkCrashEvidence $EventMessage $WerText) { [pscustomobject]@{WerPath='synthetic-test'} } else { $null } }
    else {
        $evidence = Get-WindhawkLatestCrashEvidence -Since (Get-Date).AddMinutes(-10)
        if (-not $evidence) {
            foreach ($delay in $config.EvidenceRetryDelaysSeconds) {
                Start-Sleep -Seconds $delay
                $evidence = Get-WindhawkLatestCrashEvidence -Since (Get-Date).AddMinutes(-10)
                if ($evidence) { break }
            }
        }
    }
    if (-not $evidence) { return [pscustomobject]@{Triggered=$false;Reason='No qualifying Explorer XAML/taskbar crash with a stable dock DLL in WER.'} }
    $existing = Get-WindhawkCircuitLatch; if ($existing -and $existing.Latched) { return [pscustomobject]@{Triggered=$false;Reason='Already latched.';Latch=$existing} }
    if ($DryRun) { return [pscustomobject]@{Triggered=$true;DryRun=$true;Reason='Evidence would latch stock fallback.'} }
    if (-not (Test-WindhawkAdministrator)) { throw 'Administrator access is required.' }
    New-Item -ItemType Directory -Path $config.StateRoot -Force | Out-Null
    $latch = [pscustomobject]@{Latched=$true;TriggeredAt=[DateTimeOffset]::Now;Reason='Explorer XAML/taskbar crash with a stable dock DLL in WER.';WerPath=$evidence.WerPath}
    $latch | ConvertTo-Json -Depth 5 | Set-Content $config.LatchPath -Encoding UTF8
    Write-WindhawkSafeDockLog ("CIRCUIT BREAKER LATCHED. WER={0}" -f $evidence.WerPath)
    [pscustomobject]@{Triggered=$true;Latch=$latch;Repair=(Invoke-WindhawkSafeDockRepair -AllowExplorerRestart)}
}

function Clear-WindhawkSafeDockCircuitBreaker {
    [CmdletBinding()]
    param([switch]$DryRun)
    $config = Get-WindhawkSafeDockConfig
    if (-not (Test-Path $config.LatchPath)) { return [pscustomobject]@{Changed=$false;Reason='Not latched.'} }
    if ($DryRun) { return [pscustomobject]@{Changed=$true;DryRun=$true} }
    if (-not (Test-WindhawkAdministrator)) { throw 'Administrator access is required.' }
    Remove-Item $config.LatchPath -Force; Write-WindhawkSafeDockLog 'Circuit breaker explicitly cleared.'
    [pscustomobject]@{Changed=$true;Repair=(Invoke-WindhawkSafeDockRepair -AllowExplorerRestart)}
}

Export-ModuleMember -Function Get-WindhawkSafeDockConfig,Test-WindhawkSafeDockState,Invoke-WindhawkSafeDockRepair,Test-WindhawkCrashEvidence,Get-WindhawkLatestCrashEvidence,Invoke-WindhawkSafeDockCircuitBreak,Clear-WindhawkSafeDockCircuitBreaker
