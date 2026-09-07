// =====================================================================
//  Opal shell rule table.
//
//  Originally produced by Export-MaxwellShellRules.ps1 from the live Windhawk
//  settings of the four styler mods; the generator no longer ships with Opal
//  and this file is now maintained by hand. Edits since the export carry a
//  comment explaining the observed problem they fix (search "26200").
//
//  MaxwellOwnedOverrides (maxwell-shell-owned-overrides.h) compiles AFTER this
//  table and states the final taskbar invariants; a rule here that disagrees
//  with an owned rule for the same element is a bug, not a layering choice.
// =====================================================================
#pragma once

namespace MaxwellRules {

enum class Host { Unknown, Explorer, StartMenu, Search, ShellFlyout };

struct Prop {
    const wchar_t* name;   // XAML property, may be attached e.g. Canvas.ZIndex
    const wchar_t* state;  // visual state this value applies to, or nullptr
    const wchar_t* value;
    bool           isXaml; // value must be parsed by XamlReader
};

struct Rule {
    Host           host;
    const wchar_t* selector;
    const Prop*    props;
    int            propCount;
};

struct Constant { Host host; const wchar_t* name; const wchar_t* value; };

inline constexpr Constant kConstants[] = {
    { Host::Explorer, L"Translucent", L"<WindhawkBlur BlurAmount=\"15\" TintColor=\"#10808080\"/>" },
    { Host::Explorer, L"Glass", L"<WindhawkBlur BlurAmount=\"5\" TintColor=\"{ThemeResource SystemChromeMediumColor}\" TintOpacity=\"0.7\" />" },
    { Host::Explorer, L"Frosted", L"<WindhawkBlur BlurAmount=\"20\" TintColor=\"{ThemeResource SystemChromeMediumColor}\" TintOpacity=\"0.7\" />" },
    { Host::Explorer, L"Acrylic", L"<WindhawkBlur BlurAmount=\"30\" TintColor=\"{ThemeResource SystemChromeMediumColor}\" TintOpacity=\"0.8\" />" },
    { Host::Explorer, L"Background", L"$Glass" },
    { Host::Explorer, L"BorderBrush", L"<LinearGradientBrush StartPoint=\"0,0\" EndPoint=\"0,1\"><GradientStop Color=\"#50808080\" Offset=\"0.0\" /><GradientStop Color=\"#30404040\" Offset=\"0.25\" /><GradientStop Color=\"#40808080\" Offset=\"1\" /></LinearGradientBrush>" },
    { Host::Explorer, L"BorderBrush2", L"<WindhawkBlur BlurAmount=\"10\" TintColor=\"#909090\" TintOpacity=\"0.2\"/>" },
    { Host::Explorer, L"ElementBG", L"<SolidColorBrush Color=\"{ThemeResource SystemChromeLowColor}\" Opacity=\"0.3\" />" },
    { Host::Explorer, L"ElementBorderThickness", L"0.3,0.3,0.3,1" },
    { Host::Explorer, L"ElementBorderBrush", L"<LinearGradientBrush StartPoint=\"0,0\" EndPoint=\"0,1\"><GradientStop Color=\"#50808080\" Offset=\"1\" /><GradientStop Color=\"#50606060\" Offset=\"0.15\" /></LinearGradientBrush>" },
    { Host::Explorer, L"ElementCornerRadius", L"12" },
    { Host::Explorer, L"ElementSysColor", L"<SolidColorBrush Color=\"{ThemeResource SystemAccentColorLight1}\" Opacity=\"1\" />" },
    { Host::Explorer, L"ElementSysColor2", L"<SolidColorBrush Color=\"{ThemeResource SystemAccentColorLight2}\" Opacity=\"1\" />" },
    { Host::Explorer, L"ElementSysColor3", L"<SolidColorBrush Color=\"{ThemeResource SystemAccentColorLight3}\" Opacity=\"1\" />" },
    { Host::Explorer, L"ElementSysColor4", L"<SolidColorBrush Color=\"{ThemeResource SystemAccentColorDark1}\" Opacity=\"1\" />" },
    { Host::Explorer, L"BorderThickness", L"0.5,1,0.5,1" },
    { Host::Explorer, L"CornerRadius", L"24" },
    { Host::Explorer, L"TrayPadding", L"2,4,2,4" },
    { Host::Explorer, L"Height", L"68" },
    { Host::Explorer, L"TaskbarFrameMaxWidth", L"1895" },
    { Host::Explorer, L"OpalTaskbarSurface", L"<WindhawkBlur BlurAmount=\"28\" TintColor=\"#16181D\" TintOpacity=\"0.58\" TintLuminosityOpacity=\"0.20\" TintSaturation=\"0.0\" NoiseOpacity=\"0.010\" FallbackColor=\"#D916181D\" />" },
    { Host::StartMenu, L"Translucent", L"<WindhawkBlur BlurAmount=\"15\" TintColor=\"#10808080\"/>" },
    { Host::StartMenu, L"Glass", L"<WindhawkBlur BlurAmount=\"5\" TintColor=\"{ThemeResource SystemChromeMediumColor}\" TintOpacity=\"0.7\" />" },
    { Host::StartMenu, L"Frosted", L"<WindhawkBlur BlurAmount=\"20\" TintColor=\"{ThemeResource SystemChromeMediumColor}\" TintOpacity=\"0.7\" />" },
    { Host::StartMenu, L"Acrylic", L"<WindhawkBlur BlurAmount=\"30\" TintColor=\"{ThemeResource SystemChromeMediumColor}\" TintOpacity=\"0.8\" />" },
    { Host::StartMenu, L"Background", L"$Glass" },
    { Host::StartMenu, L"BorderBrush", L"<LinearGradientBrush StartPoint=\"0,0\" EndPoint=\"0,1\"><GradientStop Color=\"#60808080\" Offset=\"0.0\" /><GradientStop Color=\"#50404040\" Offset=\"0.25\" /><GradientStop Color=\"#40808080\" Offset=\"1\" /></LinearGradientBrush>" },
    { Host::StartMenu, L"BorderBrush2", L"<WindhawkBlur BlurAmount=\"10\" TintColor=\"#909090\" TintOpacity=\"0.3\"/>" },
    { Host::StartMenu, L"ClockBG", L"<SolidColorBrush Color=\"{ThemeResource SystemAccentColor}\" Opacity=\"1\"/>" },
    { Host::StartMenu, L"BorderThickness", L"0.3,1,0.3,1" },
    { Host::StartMenu, L"CornerRadius", L"35" },
    { Host::StartMenu, L"SearchBoxRadius", L"20" },
    { Host::StartMenu, L"ElementBG", L"<SolidColorBrush Color=\"{ThemeResource SystemChromeAltHighColor}\" Opacity=\"0.3\" />" },
    { Host::StartMenu, L"ElementBorderThickness", L"0.3,0.3,0.3,1" },
    { Host::StartMenu, L"ElementCornerRadius", L"25" },
    { Host::StartMenu, L"ElementBorderBrush", L"<LinearGradientBrush StartPoint=\"0,0\" EndPoint=\"0,1\"><GradientStop Color=\"#50808080\" Offset=\"1\" /><GradientStop Color=\"#50606060\" Offset=\"0.15\" /></LinearGradientBrush>" },
    { Host::StartMenu, L"ElementBorderBrush2", L"<WindhawkBlur BlurAmount=\"30\" TintColor=\"#909090\" TintOpacity=\"0.3\"/>" },
    { Host::StartMenu, L"HoverCornerRadius", L"15" },
    { Host::ShellFlyout, L"Translucent", L"<WindhawkBlur BlurAmount=\"15\" TintColor=\"#10808080\"/>" },
    { Host::ShellFlyout, L"Glass", L"<WindhawkBlur BlurAmount=\"5\" TintColor=\"{ThemeResource SystemChromeMediumColor}\" TintOpacity=\"0.7\" />" },
    { Host::ShellFlyout, L"Frosted", L"<WindhawkBlur BlurAmount=\"20\" TintColor=\"{ThemeResource SystemChromeMediumColor}\" TintOpacity=\"0.7\" />" },
    { Host::ShellFlyout, L"Acrylic", L"<WindhawkBlur BlurAmount=\"30\" TintColor=\"{ThemeResource SystemChromeMediumColor}\" TintOpacity=\"0.8\" />" },
    { Host::ShellFlyout, L"Background", L"$Glass" },
    { Host::ShellFlyout, L"BorderBrush", L"<LinearGradientBrush StartPoint=\"0,0\" EndPoint=\"0,1\"><GradientStop Color=\"#50808080\" Offset=\"0.0\" /><GradientStop Color=\"#50404040\" Offset=\"0.25\" /><GradientStop Color=\"#50808080\" Offset=\"1\" /></LinearGradientBrush>" },
    { Host::ShellFlyout, L"BorderBrush2", L"<LinearGradientBrush StartPoint=\"0,0\" EndPoint=\"0,1\"><GradientStop Color=\"{ThemeResource SystemChromeHighColor}\" Offset=\"0.0\" /><GradientStop Color=\"{ThemeResource SystemChromeLowColor}\" Offset=\"0.15\" /><GradientStop Color=\"{ThemeResource SystemChromeHighColor}\" Offset=\"0.95\" /></LinearGradientBrush>" },
    { Host::ShellFlyout, L"overlay", L"<SolidColorBrush Color=\"{ThemeResource SystemChromeAltHighColor}\" Opacity=\"0.1\" />" },
    { Host::ShellFlyout, L"overlay2", L"<WindhawkBlur BlurAmount=\"20\" TintColor=\"#60353535\"/>" },
    { Host::ShellFlyout, L"CornerRadius", L"20" },
    { Host::ShellFlyout, L"CR2", L"14" },
    { Host::ShellFlyout, L"CR3", L"12" },
    { Host::ShellFlyout, L"BorderThickness", L"0.3,1,0.3,0.3" },
    { Host::ShellFlyout, L"ElementBG", L"<SolidColorBrush Color=\"{ThemeResource SystemChromeAltHighColor}\" Opacity=\"0.3\" />" },
    { Host::ShellFlyout, L"ElementBorderBrush", L"<LinearGradientBrush StartPoint=\"0,0\" EndPoint=\"0,1\"><GradientStop Color=\"#50808080\" Offset=\"1\" /><GradientStop Color=\"#50606060\" Offset=\"0.15\" /></LinearGradientBrush>" },
    { Host::ShellFlyout, L"ElementCornerRadius", L"20" },
    { Host::ShellFlyout, L"ElementBorderThickness", L"0.3,0.3,0.3,1" },
    { Host::ShellFlyout, L"ElementSysColor", L"<SolidColorBrush Color=\"{ThemeResource SystemAccentColorLight1}\" Opacity=\"1\" />" },
    { Host::ShellFlyout, L"ElementSysColor2", L"<SolidColorBrush Color=\"{ThemeResource SystemAccentColorLight2}\" Opacity=\"1\" />" },
    { Host::ShellFlyout, L"ElementSysColor3", L"<SolidColorBrush Color=\"{ThemeResource SystemAccentColorLight3}\" Opacity=\"1\" />" },
    { Host::ShellFlyout, L"ElementSysColor4", L"<SolidColorBrush Color=\"{ThemeResource SystemAccentColorDark1}\" Opacity=\"1\" />" },
    { Host::Explorer, L"Background", L"<WindhawkBlur BlurAmount=\"40\" TintColor=\"#050505\" TintOpacity=\"0.94\" TintLuminosityOpacity=\"0.06\" TintSaturation=\"0.0\" NoiseOpacity=\"0.006\" FallbackColor=\"#050505\" />" },
    { Host::Explorer, L"Glass", L"<WindhawkBlur BlurAmount=\"40\" TintColor=\"#050505\" TintOpacity=\"0.94\" TintLuminosityOpacity=\"0.06\" TintSaturation=\"0.0\" NoiseOpacity=\"0.006\" FallbackColor=\"#050505\" />" },
    { Host::Explorer, L"Frosted", L"<WindhawkBlur BlurAmount=\"40\" TintColor=\"#050505\" TintOpacity=\"0.94\" TintLuminosityOpacity=\"0.06\" TintSaturation=\"0.0\" NoiseOpacity=\"0.006\" FallbackColor=\"#050505\" />" },
    { Host::Explorer, L"Acrylic", L"<WindhawkBlur BlurAmount=\"40\" TintColor=\"#050505\" TintOpacity=\"0.94\" TintLuminosityOpacity=\"0.06\" TintSaturation=\"0.0\" NoiseOpacity=\"0.006\" FallbackColor=\"#050505\" />" },
    { Host::Explorer, L"ElementBG", L"<SolidColorBrush Color=\"#FFFFFF\" Opacity=\"0.045\" />" },
    { Host::Explorer, L"BorderBrush", L"<LinearGradientBrush StartPoint=\"0,0\" EndPoint=\"0,1\"><GradientStop Color=\"#3CFFFFFF\" Offset=\"0\" /><GradientStop Color=\"#14FFFFFF\" Offset=\"0.55\" /><GradientStop Color=\"#06FFFFFF\" Offset=\"1\" /></LinearGradientBrush>" },
    { Host::StartMenu, L"Background", L"<WindhawkBlur BlurAmount=\"40\" TintColor=\"#050505\" TintOpacity=\"0.94\" TintLuminosityOpacity=\"0.06\" TintSaturation=\"0.0\" NoiseOpacity=\"0.006\" FallbackColor=\"#050505\" />" },
    { Host::StartMenu, L"ElementBG", L"<SolidColorBrush Color=\"#FFFFFF\" Opacity=\"0.045\" />" },
    { Host::StartMenu, L"ClockBG", L"<SolidColorBrush Color=\"#F5F5F7\" Opacity=\"0.96\" />" },
    { Host::StartMenu, L"SearchBoxRadius", L"4" },
    { Host::StartMenu, L"HoverCornerRadius", L"4" },
    { Host::StartMenu, L"BorderBrush", L"<LinearGradientBrush StartPoint=\"0,0\" EndPoint=\"0,1\"><GradientStop Color=\"#3CFFFFFF\" Offset=\"0\" /><GradientStop Color=\"#14FFFFFF\" Offset=\"0.55\" /><GradientStop Color=\"#06FFFFFF\" Offset=\"1\" /></LinearGradientBrush>" },
    { Host::StartMenu, L"BorderThickness", L"0" },
    { Host::StartMenu, L"CornerRadius", L"8" },
    { Host::StartMenu, L"ElementBorderBrush", L"<LinearGradientBrush StartPoint=\"0,0\" EndPoint=\"0,1\"><GradientStop Color=\"#3CFFFFFF\" Offset=\"0\" /><GradientStop Color=\"#14FFFFFF\" Offset=\"0.55\" /><GradientStop Color=\"#06FFFFFF\" Offset=\"1\" /></LinearGradientBrush>" },
    { Host::StartMenu, L"ElementBorderThickness", L"1" },
    { Host::StartMenu, L"ElementCornerRadius", L"4" },
    { Host::StartMenu, L"CR2", L"20" },
    { Host::StartMenu, L"CR3", L"16" },
    { Host::ShellFlyout, L"Background", L"<WindhawkBlur BlurAmount=\"40\" TintColor=\"#050505\" TintOpacity=\"0.94\" TintLuminosityOpacity=\"0.06\" TintSaturation=\"0.0\" NoiseOpacity=\"0.006\" FallbackColor=\"#050505\" />" },
    { Host::ShellFlyout, L"ElementBG", L"<SolidColorBrush Color=\"#FFFFFF\" Opacity=\"0.045\" />" },
    { Host::ShellFlyout, L"overlay", L"<SolidColorBrush Color=\"#000000\" Opacity=\"0.32\" />" },
    { Host::ShellFlyout, L"BorderBrush", L"<LinearGradientBrush StartPoint=\"0,0\" EndPoint=\"0,1\"><GradientStop Color=\"#3CFFFFFF\" Offset=\"0\" /><GradientStop Color=\"#14FFFFFF\" Offset=\"0.55\" /><GradientStop Color=\"#06FFFFFF\" Offset=\"1\" /></LinearGradientBrush>" },
};
inline constexpr int kConstantCount = 82;

inline constexpr Prop kProps0[] = {
    { L"MaxWidth", nullptr, L"{{containerGridWidth>0?min($TaskbarFrameMaxWidth,containerGridWidth):$TaskbarFrameMaxWidth}}", false },
    { L"Width", nullptr, L"Auto", false },
    { L"MinWidth", nullptr, L"100", true },
    { L"Grid.Column", nullptr, L"1", false },
};
// The taskbar frame root is a layout host only (MaxwellOwnedOverrides::
// kFloatingTaskbarRoot). This rule used to carry the imported profile's glass
// slab, a 1-DIP top border and a left-only pill radius; on 26200 that is what
// rendered - a faint slab behind the app icons with a hard hairline along the
// top and a square right edge, which read as the taskbar being "cut off". The
// three rules that target RootGrid (this one, kProps193, and the owned rule)
// now agree. Padding is symmetric: the right side had none, so the last app
// button sat flush against the frame's clip edge and its hover zoom was cut.
inline constexpr Prop kProps1[] = {
    { L"Margin", nullptr, L"10,9,10,9", false },
    { L"BorderThickness", nullptr, L"0", false },
    { L"BorderBrush", nullptr, L"Transparent", false },
    { L"CornerRadius", nullptr, L"25", false },
    { L"Background", nullptr, L"$OpalTaskbarSurface", true },
    { L"Padding", nullptr, L"14,0,14,0", false },
};
inline constexpr Prop kProps2[] = {
    { L"Visibility", nullptr, L"Collapsed", false },
};
inline constexpr Prop kProps3[] = {
    { L"Visibility", nullptr, L"Collapsed", false },
};
inline constexpr Prop kProps4[] = {
    { L"Margin", nullptr, L"0,4,0,4", false },
    { L"Background", nullptr, L"Transparent", true },
    { L"CornerRadius", nullptr, L"0", false },
    { L"BorderThickness", nullptr, L"0,0,1,0", false },
    { L"BorderBrush", nullptr, L"#20808080", true },
    { L"Padding", nullptr, L"2,0,5,0", false },
    { L"MaxWidth", nullptr, L"200", true },
};
inline constexpr Prop kProps5[] = {
    { L"Grid.Column", nullptr, L"2", false },
    { L"Width", nullptr, L"Auto", false },
    { L"HorizontalAlignment", nullptr, L"Left", false },
    { L"Margin", nullptr, L"0", false },
};
inline constexpr Prop kProps6[] = {
    { L"Margin", nullptr, L"8,9,8,9", false },
    { L"Padding", nullptr, L"8,0,8,0", false },
    { L"Background", nullptr, L"$OpalTaskbarSurface", true },
    { L"BorderThickness", nullptr, L"0", false },
    { L"CornerRadius", nullptr, L"25", false },
};
inline constexpr Prop kProps7[] = {
    { L"Margin", nullptr, L"8,9,8,9", false },
    { L"Padding", nullptr, L"8,0,8,0", false },
    { L"Background", nullptr, L"$OpalTaskbarSurface", true },
    { L"BorderThickness", nullptr, L"0", false },
    { L"CornerRadius", nullptr, L"25", false },
};
inline constexpr Prop kProps8[] = {
    { L"ColumnDefinitions", nullptr, L"<ColumnDefinitionCollection><ColumnDefinition Width=\"*\"/><ColumnDefinition Width=\"Auto\"/><ColumnDefinition Width=\"Auto\"/><ColumnDefinition Width=\"*\"/></ColumnDefinitionCollection>", true },
    { L"ActualWidth", nullptr, L">containerGridWidth", false },
};
inline constexpr Prop kProps9[] = {
    { L"Padding", nullptr, L"6,0,6,0", false },
    { L"CornerRadius", nullptr, L"8", false },
    { L"Margin", nullptr, L"2,0,0,0", false },
};
inline constexpr Prop kProps10[] = {
    { L"Padding", nullptr, L"2,0,2,0", false },
    { L"CornerRadius", nullptr, L"8", false },
    { L"Margin", nullptr, L"2,0,0,0", false },
    { L"Clip", nullptr, L"None", false },
};
inline constexpr Prop kProps11[] = {
    { L"Padding", nullptr, L"6,0,6,0", false },
    { L"CornerRadius", nullptr, L"8", false },
    { L"Margin", nullptr, L"2,0,0,0", false },
};
inline constexpr Prop kProps12[] = {
    { L"Padding", nullptr, L"6,0,6,0", false },
    { L"Margin", nullptr, L"2,0,0,0", false },
};
inline constexpr Prop kProps13[] = {
    { L"Padding", nullptr, L"$TrayPadding", false },
};
inline constexpr Prop kProps14[] = {
    { L"Padding", nullptr, L"10", false },
    { L"CornerRadius", nullptr, L"10", false },
};
inline constexpr Prop kProps15[] = {
    { L"Padding", nullptr, L"0", false },
};
inline constexpr Prop kProps16[] = {
    { L"Visibility", nullptr, L"Visible", false },
};
inline constexpr Prop kProps17[] = {
    { L"Width", nullptr, L"Auto", false },
    { L"MinWidth", nullptr, L"24", false },
};
inline constexpr Prop kProps18[] = {
    { L"Margin", nullptr, L"4,0,0,0", false },
};
inline constexpr Prop kProps21[] = {
    { L"Text", nullptr, L"\\uED14", false },
};
inline constexpr Prop kProps22[] = {
    { L"CornerRadius", nullptr, L"24", false },
    { L"BorderThickness", nullptr, L"$BorderThickness", false },
    { L"BorderBrush", nullptr, L"$BorderBrush", true },
    { L"Background", nullptr, L"$Background", true },
};
inline constexpr Prop kProps23[] = {
    { L"Text", nullptr, L"Search", false },
    { L"FontSize", nullptr, L"10", false },
    { L"FontFamily", nullptr, L"vivo Sans EN VF", false },
};
inline constexpr Prop kProps24[] = {
    { L"Visibility", nullptr, L"Collapsed", false },
};
inline constexpr Prop kProps25[] = {
    { L"BorderThickness", nullptr, L"$BorderThickness", false },
};
inline constexpr Prop kProps26[] = {
    { L"BorderThickness", nullptr, L"$BorderThickness", false },
    { L"BorderBrush", nullptr, L"$BorderBrush", true },
    { L"Background", nullptr, L"Transparent", false },
    { L"CornerRadius", nullptr, L"$CornerRadius", false },
};
inline constexpr Prop kProps27[] = {
    { L"Background", nullptr, L"$Background", true },
};
inline constexpr Prop kProps28[] = {
    { L"BorderThickness", nullptr, L"$BorderThickness", false },
    { L"BorderBrush", nullptr, L"$BorderBrush", true },
    { L"Background", nullptr, L"$Background", true },
    { L"CornerRadius", nullptr, L"$CornerRadius", false },
};
inline constexpr Prop kProps29[] = {
    { L"Width", nullptr, L"Auto", false },
    { L"Visibility", nullptr, L"Visible", false },
    { L"HorizontalAlignment", nullptr, L"1", false },
};
inline constexpr Prop kProps30[] = {
    { L"Background", nullptr, L"Transparent", true },
};
inline constexpr Prop kProps31[] = {
    { L"CornerRadius", nullptr, L"10", false },
};
inline constexpr Prop kProps32[] = {
    { L"Background", nullptr, L"$Background", true },
    { L"BorderBrush", nullptr, L"$BorderBrush", true },
    { L"CornerRadius", nullptr, L"$CornerRadius", false },
    { L"BorderThickness", nullptr, L"$BorderThickness", false },
    { L"RenderTransform", nullptr, L"<TranslateTransform X=\"0\" Y=\"10\" />", true },
    { L"Margin", nullptr, L"0,0,0,-10", false },
};
inline constexpr Prop kProps33[] = {
    { L"Background", nullptr, L"$Background", true },
    { L"BorderBrush", nullptr, L"$BorderBrush", true },
    { L"CornerRadius", nullptr, L"$CornerRadius", false },
    { L"BorderThickness", nullptr, L"$BorderThickness", false },
};
inline constexpr Prop kProps34[] = {
    { L"BorderBrush", nullptr, L"$ElementBorderBrush", true },
    { L"CornerRadius", nullptr, L"$ElementCornerRadius", false },
    { L"BorderThickness", nullptr, L"$ElementBorderThickness", false },
    { L"MaxWidth", nullptr, L"100", true },
    { L"Width", nullptr, L"Auto", false },
};
inline constexpr Prop kProps35[] = {
    { L"RenderTransform", nullptr, L"<TranslateTransform X=\"0\" Y=\"0\" />", true },
};
inline constexpr Prop kProps36[] = {
    { L"RenderTransform", nullptr, L"<TranslateTransform X=\"0\" Y=\"0\" />", true },
};
inline constexpr Prop kProps37[] = {
    { L"Background", nullptr, L"$Background", true },
    { L"BorderBrush", nullptr, L"$BorderBrush", true },
    { L"BorderThickness", nullptr, L"$BorderThickness", true },
    { L"CornerRadius", nullptr, L"12", false },
};
inline constexpr Prop kProps38[] = {
    { L"MaxWidth", nullptr, L"300", true },
    { L"MinWidth", nullptr, L"10", true },
    { L"Width", nullptr, L"Auto", false },
    { L"Margin", nullptr, L"0", false },
};
inline constexpr Prop kProps39[] = {
    { L"Background", nullptr, L"$Background", true },
    { L"BorderBrush", nullptr, L"$BorderBrush", true },
    { L"BorderThickness", nullptr, L"$BorderThickness", false },
    { L"CornerRadius", nullptr, L"$CornerRadius", false },
};
inline constexpr Prop kProps40[] = {
    { L"Fill", nullptr, L"$Background", true },
};
inline constexpr Prop kProps41[] = {
    { L"Background", nullptr, L"$Background", true },
    { L"BorderBrush", nullptr, L"$BorderBrush", true },
    { L"BorderThickness", nullptr, L"$BorderThickness", false },
    { L"CornerRadius", nullptr, L"$CornerRadius", false },
};
inline constexpr Prop kProps42[] = {
    { L"Background", nullptr, L"$Background", true },
    { L"BorderBrush", nullptr, L"$BorderBrush", true },
    { L"BorderThickness", nullptr, L"$BorderThickness", false },
    { L"CornerRadius", nullptr, L"$CornerRadius", false },
};
inline constexpr Prop kProps43[] = {
    { L"Background", nullptr, L"Transparent", true },
    { L"BorderBrush", nullptr, L"$BorderBrush", true },
    { L"BorderThickness", nullptr, L"$BorderThickness", false },
    { L"CornerRadius", nullptr, L"$CornerRadius", false },
};
inline constexpr Prop kProps44[] = {
    { L"VerticalAlignment", nullptr, L"Top", false },
    { L"HorizontalAlignment", nullptr, L"Center", false },
    { L"RenderTransform", nullptr, L"<TranslateTransform X=\"0\" />", true },
};
inline constexpr Prop kProps45[] = {
    { L"HorizontalAlignment", nullptr, L"Center", true },
    { L"RenderTransform", nullptr, L"<TranslateTransform X=\"0\" Y=\"0\" />", true },
    { L"FontFamily", nullptr, L"Segoe UI Variable Display", false },
    { L"Foreground", nullptr, L"$ClockBG", true },
};
inline constexpr Prop kProps46[] = {
    { L"HorizontalAlignment", nullptr, L"Center", false },
    { L"RenderTransform", nullptr, L"<TranslateTransform X=\"0\" Y=\"0\" />", true },
    { L"FontFamily", nullptr, L"Segoe UI Variable Text", false },
    { L"Foreground", nullptr, L"$ClockBG", true },
};
inline constexpr Prop kProps47[] = {
    { L"Background", nullptr, L"$Background", true },
    { L"BorderBrush", nullptr, L"$BorderBrush", true },
    { L"BorderThickness", nullptr, L"$BorderThickness", false },
    { L"CornerRadius", nullptr, L"$CornerRadius", false },
};
inline constexpr Prop kProps48[] = {
    { L"HorizontalAlignment", nullptr, L"Center", false },
    { L"RenderTransform", nullptr, L"<TranslateTransform X=\"0\" Y=\"50\" />", true },
};
inline constexpr Prop kProps49[] = {
    { L"Background", nullptr, L"$Background", true },
    { L"BorderBrush", nullptr, L"$BorderBrush", true },
    { L"BorderThickness", nullptr, L"$BorderThickness", false },
    { L"CornerRadius", nullptr, L"$CornerRadius", false },
};
inline constexpr Prop kProps50[] = {
    { L"Visibility", nullptr, L"Visible", false },
    { L"RenderTransform", nullptr, L"<TranslateTransform X=\"0\" Y=\"0\" />", true },
    { L"Margin", nullptr, L"0,0,0,0", false },
    { L"CornerRadius", nullptr, L"$CornerRadius", false },
};
inline constexpr Prop kProps51[] = {
    { L"Margin", nullptr, L"-20,-20,-20,0", false },
};
inline constexpr Prop kProps52[] = {
    { L"Width", nullptr, L"860", false },
};
inline constexpr Prop kProps53[] = {
    { L"Visibility", nullptr, L"1", false },
};
inline constexpr Prop kProps54[] = {
    { L"Visibility", nullptr, L"1", false },
};
inline constexpr Prop kProps55[] = {
    { L"Visibility", nullptr, L"1", false },
};
inline constexpr Prop kProps56[] = {
    { L"Background", nullptr, L"$Background", true },
    { L"BorderBrush", nullptr, L"$BorderBrush", true },
    { L"BorderThickness", nullptr, L"$BorderThickness", false },
    { L"CornerRadius", nullptr, L"$CornerRadius", false },
    { L"Margin", nullptr, L"2", false },
    { L"Padding", nullptr, L"0", false },
};
inline constexpr Prop kProps57[] = {
    { L"Background", nullptr, L"$ElementBG", true },
    { L"BorderBrush", nullptr, L"$ElementBorderBrush", true },
    { L"BorderThickness", nullptr, L"$ElementBorderThickness", false },
    { L"CornerRadius", nullptr, L"$ElementCornerRadius", false },
    { L"Margin", nullptr, L"0,60,0,10", false },
    { L"Visibility", nullptr, L"1", false },
};
inline constexpr Prop kProps58[] = {
    { L"Visibility", nullptr, L"1", false },
};
inline constexpr Prop kProps59[] = {
    { L"Width", nullptr, L"650", false },
    { L"Height", nullptr, L"50", false },
    { L"Margin", nullptr, L"0,-15,0,0", false },
};
inline constexpr Prop kProps60[] = {
    { L"Background", nullptr, L"$ElementBG", true },
    { L"BorderBrush", nullptr, L"$ElementBorderBrush", true },
    { L"BorderThickness", nullptr, L"$ElementBorderThickness", false },
    { L"CornerRadius", nullptr, L"$ElementCornerRadius", false },
};
inline constexpr Prop kProps61[] = {
    { L"Text", nullptr, L"Search This Precision", false },
};
inline constexpr Prop kProps62[] = {
    { L"Visibility", nullptr, L"1", false },
};
inline constexpr Prop kProps63[] = {
    { L"Width", nullptr, L"550", false },
    { L"RenderTransform", nullptr, L"<TranslateTransform X=\"0\" Y=\"10\" />", true },
};
inline constexpr Prop kProps64[] = {
    { L"Visibility", nullptr, L"1", false },
};
inline constexpr Prop kProps65[] = {
    { L"Margin", nullptr, L"0", false },
    { L"Height", nullptr, L"280", false },
};
inline constexpr Prop kProps66[] = {
    { L"Background", nullptr, L"$ElementBG", true },
    { L"BorderBrush", nullptr, L"$ElementBorderBrush", true },
    { L"BorderThickness", nullptr, L"$ElementBorderThickness", false },
    { L"CornerRadius", nullptr, L"$ElementCornerRadius", false },
};
inline constexpr Prop kProps67[] = {
    { L"Visibility", nullptr, L"0", false },
    { L"Width", nullptr, L"650", false },
    { L"Margin", nullptr, L"0,-130,0,230", false },
    { L"Canvas.ZIndex", nullptr, L"1", false },
    { L"MaxHeight", nullptr, L"340", true },
};
inline constexpr Prop kProps68[] = {
    { L"Visibility", nullptr, L"0", false },
    { L"Margin", nullptr, L"-1600,190,115,-100", false },
    { L"MaxHeight", nullptr, L"330", false },
    { L"Background", nullptr, L"$ElementBG", true },
    { L"CornerRadius", nullptr, L"$ElementCornerRadius", false },
    { L"Width", nullptr, L"650", false },
    { L"BorderBrush", nullptr, L"$ElementBorderBrush", true },
    { L"BorderThickness", nullptr, L"$ElementBorderThickness", false },
};
inline constexpr Prop kProps69[] = {
    { L"Visibility", nullptr, L"1", false },
};
inline constexpr Prop kProps70[] = {
    { L"Visibility", nullptr, L"1", false },
};
inline constexpr Prop kProps71[] = {
    { L"Margin", nullptr, L"-20,-20,20,20", false },
};
inline constexpr Prop kProps72[] = {
    { L"Background", nullptr, L"$ElementBG", true },
    { L"BorderBrush", nullptr, L"$ElementBorderBrush", true },
    { L"BorderThickness", nullptr, L"$ElementBorderThickness", false },
    { L"CornerRadius", nullptr, L"$ElementCornerRadius", true },
};
inline constexpr Prop kProps73[] = {
    { L"Background", nullptr, L"$ElementBG", true },
    { L"BorderBrush", nullptr, L"$BorderBrush", true },
    { L"BorderThickness", nullptr, L"$BorderThickness", false },
    { L"CornerRadius", nullptr, L"$ElementCornerRadius", false },
    { L"Height", nullptr, L"40", false },
};
inline constexpr Prop kProps74[] = {
    { L"Background", nullptr, L"$ElementBG", true },
    { L"BorderBrush", nullptr, L"$BorderBrush", true },
    { L"BorderThickness", nullptr, L"$BorderThickness", false },
    { L"CornerRadius", nullptr, L"$ElementCornerRadius", false },
    { L"Height", nullptr, L"40", false },
};
inline constexpr Prop kProps75[] = {
    { L"Height", nullptr, L"730", false },
    { L"Margin", nullptr, L"0,-10,0,-10", false },
    { L"Padding", nullptr, L"10,0,-2,0", false },
};
inline constexpr Prop kProps76[] = {
    { L"Background", nullptr, L"$Background", true },
    { L"BorderBrush", nullptr, L"$BorderBrush", true },
    { L"BorderThickness", nullptr, L"$BorderThickness", false },
    { L"CornerRadius", nullptr, L"$ElementCornerRadius", false },
};
inline constexpr Prop kProps77[] = {
    { L"Background", nullptr, L"$Background", true },
    { L"BorderBrush", nullptr, L"$BorderBrush", true },
    { L"BorderThickness", nullptr, L"$BorderThickness", false },
    { L"CornerRadius", nullptr, L"$ElementCornerRadius", false },
};
inline constexpr Prop kProps78[] = {
    { L"Background", nullptr, L"$Background", true },
    { L"BorderBrush", nullptr, L"$BorderBrush", true },
    { L"BorderThickness", nullptr, L"$BorderThickness", false },
    { L"CornerRadius", nullptr, L"$ElementCornerRadius", false },
};
inline constexpr Prop kProps79[] = {
    { L"Background", nullptr, L"$Background", true },
    { L"BorderBrush", nullptr, L"$BorderBrush", true },
    { L"BorderThickness", nullptr, L"$BorderThickness", false },
    { L"CornerRadius", nullptr, L"$CornerRadius", false },
};
inline constexpr Prop kProps80[] = {
    { L"Visibility", nullptr, L"1", false },
};
inline constexpr Prop kProps81[] = {
    { L"Visibility", nullptr, L"1", false },
};
inline constexpr Prop kProps82[] = {
    { L"Visibility", nullptr, L"1", false },
};
inline constexpr Prop kProps83[] = {
    { L"Visibility", nullptr, L"1", false },
};
inline constexpr Prop kProps84[] = {
    { L"Background", nullptr, L"$Background", true },
    { L"BorderBrush", nullptr, L"$BorderBrush", true },
    { L"BorderThickness", nullptr, L"$BorderThickness", false },
    { L"CornerRadius", nullptr, L"15", false },
};
inline constexpr Prop kProps85[] = {
    { L"MaxHeight", nullptr, L"420", true },
    { L"MaxWidth", nullptr, L"420", true },
    { L"Height", nullptr, L"Auto", false },
    { L"Width", nullptr, L"Auto", false },
};
inline constexpr Prop kProps86[] = {
    { L"Width", nullptr, L"400", false },
    { L"Height", nullptr, L"400", false },
};
inline constexpr Prop kProps87[] = {
    { L"Background", nullptr, L"$Background", true },
    { L"BorderBrush", nullptr, L"$BorderBrush", true },
    { L"BorderThickness", nullptr, L"$BorderThickness", false },
    { L"CornerRadius", nullptr, L"$CornerRadius", false },
};
inline constexpr Prop kProps88[] = {
    { L"Margin", nullptr, L"0,30,0,-120", false },
};
inline constexpr Prop kProps89[] = {
    { L"Visibility", nullptr, L"1", false },
};
inline constexpr Prop kProps90[] = {
    { L"Height", nullptr, L"50", false },
    { L"Margin", nullptr, L"-20,20,-20,-20", false },
    { L"Width", nullptr, L"400", false },
};
inline constexpr Prop kProps91[] = {
    { L"Background", nullptr, L"$ElementBG", true },
    { L"BorderBrush", nullptr, L"$ElementBorderBrush", true },
    { L"BorderThickness", nullptr, L"$ElementBorderThickness", false },
    { L"CornerRadius", nullptr, L"$ElementCornerRadius", true },
};
inline constexpr Prop kProps92[] = {
    { L"Margin", nullptr, L"-50,40,0,0", false },
};
inline constexpr Prop kProps93[] = {
    { L"Visibility", nullptr, L"1", false },
};
inline constexpr Prop kProps94[] = {
    { L"Margin", nullptr, L"0,-10,0,0", false },
};
inline constexpr Prop kProps95[] = {
    { L"Visibility", nullptr, L"1", false },
};
inline constexpr Prop kProps96[] = {
    { L"Background", nullptr, L"$ElementBG", true },
    { L"BorderBrush", nullptr, L"$ElementBorderBrush", true },
    { L"BorderThickness", nullptr, L"$ElementBorderThickness", false },
    { L"CornerRadius", nullptr, L"$ElementCornerRadius", false },
};
inline constexpr Prop kProps97[] = {
    { L"Width", nullptr, L"460", false },
};
inline constexpr Prop kProps98[] = {
    { L"Width", nullptr, L"400", false },
    { L"Height", nullptr, L"450", false },
    { L"Margin", nullptr, L"0,0,0,30", false },
};
inline constexpr Prop kProps99[] = {
    { L"ScrollViewer.VerticalScrollMode", nullptr, L"2", false },
    { L"MaxHeight", nullptr, L"336", true },
    { L"MinHeight", nullptr, L"100", true },
    { L"Width", nullptr, L"300", false },
    { L"Margin", nullptr, L"0,0,60,0", false },
};
inline constexpr Prop kProps100[] = {
    { L"Height", nullptr, L"810", false },
    { L"Margin", nullptr, L"15,0,30,0", false },
};
inline constexpr Prop kProps101[] = {
    { L"Background", nullptr, L"$ElementBG", true },
    { L"BorderBrush", nullptr, L"$ElementBorderBrush", true },
    { L"BorderThickness", nullptr, L"$ElementBorderThickness", false },
    { L"CornerRadius", nullptr, L"$ElementCornerRadius", false },
};
inline constexpr Prop kProps102[] = {
    { L"Visibility", nullptr, L"0", false },
};
inline constexpr Prop kProps103[] = {
    { L"Margin", nullptr, L"0,15,0,0", false },
};
inline constexpr Prop kProps104[] = {
    { L"Visibility", nullptr, L"1", false },
};
inline constexpr Prop kProps105[] = {
    { L"BorderBrush", nullptr, L"$BorderBrush", true },
    { L"BorderThickness", nullptr, L"$BorderThickness", true },
    { L"CornerRadius", nullptr, L"$ElementCornerRadius", true },
    { L"Padding", nullptr, L"-1", false },
};
inline constexpr Prop kProps106[] = {
    { L"CornerRadius", nullptr, L"$ElementCornerRadius", true },
};
inline constexpr Prop kProps107[] = {
    { L"CornerRadius", nullptr, L"$HoverCornerRadius", false },
    { L"Margin", nullptr, L"-12,0,12,0", false },
};
inline constexpr Prop kProps108[] = {
    { L"CornerRadius", nullptr, L"$HoverCornerRadius", true },
    { L"Margin", nullptr, L"4,0,4,0", false },
};
inline constexpr Prop kProps109[] = {
    { L"CornerRadius", nullptr, L"$HoverCornerRadius", true },
    { L"Margin", nullptr, L"4,0,4,0", false },
};
inline constexpr Prop kProps110[] = {
    { L"Background", nullptr, L"$Background", true },
    { L"BorderThickness", nullptr, L"$BorderThickness", false },
    { L"CornerRadius", nullptr, L"$CornerRadius", false },
    { L"BorderBrush", nullptr, L"$BorderBrush", true },
};
inline constexpr Prop kProps111[] = {
    { L"Background", nullptr, L"$Background", true },
    { L"BorderThickness", nullptr, L"$BorderThickness", false },
    { L"CornerRadius", nullptr, L"$CornerRadius", false },
    { L"Margin", nullptr, L"0,6,0,6", false },
    { L"MinHeight", nullptr, L"40", false },
    { L"BorderBrush", nullptr, L"$BorderBrush", true },
};
inline constexpr Prop kProps112[] = {
    { L"Background", nullptr, L"$ElementBG", true },
    { L"CornerRadius", nullptr, L"$CR2", false },
    { L"Margin", nullptr, L"-10,11,-10,-14", false },
    { L"BorderBrush", nullptr, L"$ElementBorderBrush", true },
    { L"BorderThickness", nullptr, L"$ElementBorderThickness", false },
};
inline constexpr Prop kProps113[] = {
    { L"Background", nullptr, L"$ElementBG", true },
    { L"CornerRadius", nullptr, L"$CR2", false },
    { L"Margin", nullptr, L"-10,-6,-10,-8", false },
    { L"Height", nullptr, L"45", false },
    { L"BorderBrush", nullptr, L"$ElementBorderBrush", true },
    { L"BorderThickness", nullptr, L"$ElementBorderThickness", false },
};
inline constexpr Prop kProps114[] = {
    { L"Background", nullptr, L"$Background", true },
    { L"CornerRadius", nullptr, L"$CR2", false },
    { L"Margin", nullptr, L"6,7,6,6", false },
    { L"BorderThickness", nullptr, L"$BorderThickness", false },
    { L"BorderBrush", nullptr, L"$BorderBrush", true },
};
inline constexpr Prop kProps115[] = {
    { L"Background", nullptr, L"$Background", true },
    { L"BorderThickness", nullptr, L"$BorderThickness", false },
    { L"CornerRadius", nullptr, L"$CR3", false },
    { L"Padding", nullptr, L"1,2,1,2", false },
    { L"BorderBrush", nullptr, L"$BorderBrush", true },
};
inline constexpr Prop kProps116[] = {
    { L"CornerRadius", nullptr, L"6", false },
};
inline constexpr Prop kProps117[] = {
    { L"Background", nullptr, L"$Background", true },
    { L"BorderThickness", nullptr, L"$BorderThickness", false },
    { L"CornerRadius", nullptr, L"$CR3", false },
    { L"Margin", nullptr, L"-2,-2,-2,-2", false },
    { L"BorderBrush", nullptr, L"$BorderBrush", true },
};
inline constexpr Prop kProps118[] = {
    { L"Background", nullptr, L"$Background", true },
    { L"CornerRadius", nullptr, L"$CornerRadius", false },
    { L"BorderThickness", nullptr, L"$BorderThickness", false },
    { L"BorderBrush", nullptr, L"$BorderBrush", true },
};
inline constexpr Prop kProps119[] = {
    { L"Background", nullptr, L"Transparent", false },
};
inline constexpr Prop kProps120[] = {
    { L"Background", nullptr, L"$overlay", true },
    { L"CornerRadius", nullptr, L"$CR2", false },
    { L"Margin", nullptr, L"8,0,8,2", false },
};
inline constexpr Prop kProps121[] = {
    { L"Background", nullptr, L"Transparent", false },
};
inline constexpr Prop kProps122[] = {
    { L"Background", nullptr, L"$overlay", true },
    { L"CornerRadius", nullptr, L"$CR2", false },
    { L"Margin", nullptr, L"7,7,7,7", false },
};
inline constexpr Prop kProps123[] = {
    { L"Background", nullptr, L"$overlay", true },
    { L"CornerRadius", nullptr, L"$CR2", false },
    { L"Margin", nullptr, L"8,0,8,0", false },
};
inline constexpr Prop kProps124[] = {
    { L"Background", nullptr, L"Transparent", false },
    { L"Margin", nullptr, L"0", false },
    { L"RenderTransform", nullptr, L"<TranslateTransform X=\"0\" Y=\"0\" />", true },
    { L"Clip", nullptr, L"None", false },
};
inline constexpr Prop kProps125[] = {
    { L"Background", nullptr, L"$Background", true },
    { L"BorderThickness", nullptr, L"$BorderThickness", false },
    { L"CornerRadius", nullptr, L"8", false },
    { L"BorderBrush", nullptr, L"$BorderBrush", true },
};
inline constexpr Prop kProps126[] = {
    { L"Background", nullptr, L"$Background", true },
    { L"BorderThickness", nullptr, L"$BorderThickness", false },
    { L"CornerRadius", nullptr, L"8", false },
    { L"BorderBrush", nullptr, L"$BorderBrush", true },
};
inline constexpr Prop kProps127[] = {
    { L"CornerRadius", nullptr, L"8", false },
};
inline constexpr Prop kProps128[] = {
    { L"CornerRadius", nullptr, L"6", false },
};
inline constexpr Prop kProps129[] = {
    { L"CornerRadius", nullptr, L"8", false },
};
inline constexpr Prop kProps130[] = {
    { L"Background", nullptr, L"Transparent", false },
    { L"CornerRadius", nullptr, L"$CR2", false },
    { L"Margin", nullptr, L"6", false },
};
inline constexpr Prop kProps131[] = {
    { L"Background", nullptr, L"Transparent", false },
};
inline constexpr Prop kProps132[] = {
    { L"BorderThickness", nullptr, L"0", false },
};
inline constexpr Prop kProps133[] = {
    { L"Margin", nullptr, L"0,0,8,0", false },
    { L"CornerRadius", nullptr, L"$CR3", false },
};
inline constexpr Prop kProps134[] = {
    { L"Margin", nullptr, L"2,0,0,0", false },
    { L"CornerRadius", nullptr, L"$CR3", false },
};
inline constexpr Prop kProps135[] = {
    { L"Margin", nullptr, L"0,0,-1,0", false },
    { L"CornerRadius", nullptr, L"13", false },
    { L"BorderThickness", nullptr, L"$BorderThickness", false },
};
inline constexpr Prop kProps136[] = {
    { L"CornerRadius", nullptr, L"10", false },
};
inline constexpr Prop kProps137[] = {
    { L"CornerRadius", nullptr, L"10", false },
};
inline constexpr Prop kProps138[] = {
    { L"Height", nullptr, L"10", false },
    { L"Fill", nullptr, L"$overlay", true },
    { L"RadiusY", nullptr, L"5", false },
    { L"RadiusX", nullptr, L"5", false },
    { L"Margin", nullptr, L"0,-10,10,-10", false },
};
inline constexpr Prop kProps139[] = {
    { L"Height", nullptr, L"10", false },
    { L"RadiusY", nullptr, L"5", false },
    { L"RadiusX", nullptr, L"5", false },
    { L"Margin", nullptr, L"0,-10,-10,-10", false },
};
inline constexpr Prop kProps140[] = {
    { L"Visibility", nullptr, L"Visible", false },
    { L"Height", nullptr, L"25", false },
    { L"Width", nullptr, L"40", false },
    { L"Margin", nullptr, L"0", false },
};
inline constexpr Prop kProps141[] = {
    { L"Height", nullptr, L"100", false },
    { L"CornerRadius", nullptr, L"$CornerRadius", false },
    { L"BorderThickness", nullptr, L"$BorderThickness", false },
    { L"Background", nullptr, L"$Background", true },
    { L"Margin", nullptr, L"0,10,0,0", false },
    { L"BorderBrush", nullptr, L"$BorderBrush", true },
    { L"Grid.Row", nullptr, L"1", false },
};
inline constexpr Prop kProps142[] = {
    { L"Height", nullptr, L"55", false },
    { L"MaxWidth", nullptr, L"150", false },
    { L"HorizontalAlignment", nullptr, L"Left", false },
};
inline constexpr Prop kProps143[] = {
    { L"Visibility", nullptr, L"1", false },
};
inline constexpr Prop kProps144[] = {
    { L"VerticalAlignment", nullptr, L"Center", false },
    { L"HorizontalAlignment", nullptr, L"Left", false },
    { L"Margin", nullptr, L"0,0,10,0", false },
};
inline constexpr Prop kProps145[] = {
    { L"TextAlignment", nullptr, L"Center", false },
    { L"FontSize", nullptr, L"18", false },
};
inline constexpr Prop kProps146[] = {
    { L"TextAlignment", nullptr, L"Center", false },
    { L"FontFamily", nullptr, L"vivo Sans EN VF", false },
    { L"Margin", nullptr, L"0,3,0,0", false },
    { L"FontWeight", nullptr, L"600", false },
};
inline constexpr Prop kProps147[] = {
    { L"VerticalAlignment", nullptr, L"Center", false },
    { L"Height", nullptr, L"20", false },
    { L"Margin", nullptr, L"130,-60,0,0", false },
    { L"Width", nullptr, L"Auto", false },
    { L"HorizontalAlignment", nullptr, L"Right", false },
    { L"Visibility", nullptr, L"2", false },
};
inline constexpr Prop kProps148[] = {
    { L"Width", nullptr, L"40", false },
    { L"Height", nullptr, L"40", false },
    { L"Margin", nullptr, L"10,0,0,0", false },
};
inline constexpr Prop kProps149[] = {
    { L"Width", nullptr, L"40", false },
    { L"Height", nullptr, L"40", false },
    { L"Margin", nullptr, L"0", false },
};
inline constexpr Prop kProps150[] = {
    { L"Width", nullptr, L"40", false },
    { L"Height", nullptr, L"30", false },
    { L"Margin", nullptr, L"0,0,10,0", false },
};
inline constexpr Prop kProps151[] = {
    { L"FontFamily", nullptr, L"vivo Sans EN VF", false },
    { L"FontSize", nullptr, L"16", false },
};
inline constexpr Prop kProps152[] = {
    { L"Height", nullptr, L"24", false },
    { L"Width", nullptr, L"24", false },
    { L"Clip", nullptr, L"None", false },
    { L"Stretch", nullptr, L"Uniform", false },
};
inline constexpr Prop kProps153[] = {
    { L"Background", nullptr, L"Transparent", false },
};
// The imported theme moved the notification centre's toast peek region by a
// fixed -495/+395 DIP translate and re-parented it to another grid cell - a
// layout hack authored for one screen. On 26200 that shoves the incoming-toast
// region off the panel, which is what "notifications cut off" looks like from
// the outside. Clearing Background alone does not undo a leftover transform
// from an earlier install, so identity layout is applied every tap.
inline constexpr Prop kProps154[] = {
    { L"Background", nullptr, L"Transparent", false },
    { L"Margin", nullptr, L"0", false },
    { L"Padding", nullptr, L"0", false },
    { L"RenderTransform", nullptr, L"<TranslateTransform X=\"0\" Y=\"0\" />", true },
    { L"Clip", nullptr, L"None", false },
    { L"HorizontalAlignment", nullptr, L"Stretch", false },
    { L"VerticalAlignment", nullptr, L"Stretch", false },
};
inline constexpr Prop kProps155[] = {
    { L"CornerRadius", nullptr, L"8", false },
    { L"Margin", nullptr, L"1,2,1,2", false },
};
inline constexpr Prop kProps156[] = {
    { L"CornerRadius", nullptr, L"8", false },
};
inline constexpr Prop kProps157[] = {
    { L"CornerRadius", nullptr, L"8", false },
};
inline constexpr Prop kProps158[] = {
    { L"CornerRadius", nullptr, L"8", false },
};
inline constexpr Prop kProps159[] = {
    { L"Margin", nullptr, L"50,6,50,2", false },
    { L"CornerRadius", nullptr, L"8", false },
    { L"Height", nullptr, L"35", false },
};
inline constexpr Prop kProps160[] = {
    { L"CornerRadius", nullptr, L"4", false },
};
inline constexpr Prop kProps161[] = {
    { L"CornerRadius", nullptr, L"4", false },
};
inline constexpr Prop kProps162[] = {
    { L"Margin", nullptr, L"6,0,0,0", false },
};
inline constexpr Prop kProps163[] = {
    { L"Margin", nullptr, L"1,2,1,2", false },
};
inline constexpr Prop kProps164[] = {
    { L"Background", nullptr, L"$ElementSysColor", true },
    { L"CornerRadius", nullptr, L"8", false },
    { L"Margin", nullptr, L"4,0,4,0", false },
    { L"Padding", nullptr, L"0,-5,0,-3", false },
};
inline constexpr Prop kProps165[] = {
    { L"CornerRadius", nullptr, L"$CR3", false },
};
inline constexpr Prop kProps166[] = {
    { L"Background", nullptr, L"<SolidColorBrush Color=\"{ThemeResource SystemAccentColorLight1}\" Opacity=\"0.5\"/>", true },
    { L"BorderThickness", nullptr, L"0", false },
    { L"CornerRadius", nullptr, L"8", false },
};
inline constexpr Prop kProps167[] = {
    { L"Background", nullptr, L"$overlay2", true },
    { L"BorderThickness", nullptr, L"0", false },
    { L"CornerRadius", nullptr, L"$CR3", false },
};
inline constexpr Prop kProps168[] = {
    { L"Margin", nullptr, L"12,0,12,0", false },
    { L"CornerRadius", nullptr, L"0", false },
    { L"Height", nullptr, L"150", false },
};
inline constexpr Prop kProps169[] = {
    { L"Visibility", nullptr, L"1", false },
};
inline constexpr Prop kProps170[] = {
    { L"Margin", nullptr, L"0-2,0,0", false },
};
inline constexpr Prop kProps171[] = {
    { L"CornerRadius", nullptr, L"$CR3", false },
};
inline constexpr Prop kProps172[] = {
    { L"RadiusX", nullptr, L"8", false },
    { L"RadiusY", nullptr, L"8", false },
    { L"Height", nullptr, L"18", false },
};
inline constexpr Prop kProps173[] = {
    { L"RadiusY", nullptr, L"8", false },
    { L"RadiusX", nullptr, L"8", false },
};
inline constexpr Prop kProps174[] = {
    { L"CornerRadius", nullptr, L"8", false },
};
inline constexpr Prop kProps175[] = {
    { L"RadiusX", nullptr, L"8", false },
    { L"RadiusY", nullptr, L"8", false },
    { L"Height", nullptr, L"18", false },
};
inline constexpr Prop kProps176[] = {
    { L"Margin", nullptr, L"5,2,5,3", false },
};
inline constexpr Prop kProps177[] = {
    { L"BorderThickness", nullptr, L"0", false },
};
inline constexpr Prop kProps178[] = {
    { L"CornerRadius", nullptr, L"12", false },
};
inline constexpr Prop kProps179[] = {
    { L"BorderThickness", nullptr, L"0", false },
};
inline constexpr Prop kProps180[] = {
    { L"AccessKey", nullptr, L"x", false },
};
inline constexpr Prop kProps181[] = {
    { L"AccessKey", nullptr, L"e", false },
};
inline constexpr Prop kProps182[] = {
    { L"CornerRadius", nullptr, L"$CR2", false },
    { L"BorderThickness", nullptr, L"$ElementBorderThickness", false },
    { L"BorderBrush", nullptr, L"$ElementBorderBrush", true },
};
inline constexpr Prop kProps183[] = {
    { L"CornerRadius", nullptr, L"30", false },
    { L"BorderThickness", nullptr, L"$ElementBorderThickness", false },
    { L"BorderBrush", nullptr, L"$ElementBorderBrush", true },
};
inline constexpr Prop kProps184[] = {
    { L"CornerRadius", nullptr, L"12", false },
    { L"Background", nullptr, L"$Background", true },
    { L"BorderBrush", nullptr, L"$BorderBrush", true },
    { L"BorderThickness", nullptr, L"$BorderThickness", false },
};
inline constexpr Prop kProps185[] = {
    { L"Visibility", nullptr, L"Collapsed", false },
};
inline constexpr Prop kProps186[] = {
    { L"Background", nullptr, L"$Background", true },
    { L"BorderBrush", nullptr, L"$BorderBrush", true },
    { L"BorderThickness", nullptr, L"$BorderThickness", false },
    { L"CornerRadius", nullptr, L"10", false },
};
inline constexpr Prop kProps187[] = {
    { L"Foreground", L"Normal", L"$ElementSysColor", true },
    { L"Foreground", L"PointerOver", L"$ElementSysColor2", true },
    { L"Foreground", L"Pressed", L"$ElementSysColor3", true },
    { L"Foreground", L"Disabled", L"$ElementSysColor4", true },
    { L"Background", nullptr, L"Transparent", false },
};
inline constexpr Prop kProps188[] = {
    { L"Foreground", L"Normal", L"$ElementSysColor", true },
    { L"Foreground", L"PointerOver", L"$ElementSysColor2", true },
    { L"Foreground", L"Pressed", L"$ElementSysColor3", true },
    { L"Foreground", L"Disabled", L"$ElementSysColor4", true },
    { L"Background", nullptr, L"Transparent", false },
};
inline constexpr Prop kProps189[] = {
    { L"Foreground", L"Normal", L"$ElementSysColor", true },
    { L"Foreground", L"PointerOver", L"$ElementSysColor2", true },
    { L"Foreground", L"Pressed", L"$ElementSysColor3", true },
    { L"Foreground", L"Disabled", L"$ElementSysColor4", true },
    { L"Background", nullptr, L"Transparent", false },
};
inline constexpr Prop kProps190[] = {
    { L"Grid.Row", nullptr, L"0", false },
};
inline constexpr Prop kProps191[] = {
    { L"VerticalAlignment", nullptr, L"2", false },
    { L"Grid.Row", nullptr, L"1", false },
    { L"Canvas.ZIndex", nullptr, L"1", false },
};
inline constexpr Prop kProps192[] = {
    { L"VerticalAlignment", nullptr, L"3", false },
    { L"MinHeight", nullptr, L"0", false },
};
// Second RootGrid rule; kept in step with kProps1 and the owned invariant so
// rule order cannot bring back a slab or border (see the kProps1 comment).
inline constexpr Prop kProps193[] = {
    { L"BorderBrush", nullptr, L"Transparent", false },
    { L"BorderThickness", nullptr, L"0", false },
    { L"Padding", nullptr, L"14,0,14,0", false },
    { L"Margin", nullptr, L"10,9,10,9", false },
    { L"CornerRadius", nullptr, L"25", false },
    { L"Background", nullptr, L"$OpalTaskbarSurface", true },
};
inline constexpr Prop kProps194[] = {
    { L"BorderBrush", nullptr, L"Transparent", false },
    { L"BorderThickness", nullptr, L"0", false },
    { L"Padding", nullptr, L"8,0,8,0", false },
    { L"Margin", nullptr, L"8,9,8,9", false },
    { L"CornerRadius", nullptr, L"25", false },
    { L"Background", nullptr, L"$OpalTaskbarSurface", true },
};
inline constexpr Prop kProps195[] = {
    { L"Background", nullptr, L"Transparent", false },
    { L"BorderBrush", nullptr, L"Transparent", false },
    { L"BorderThickness", nullptr, L"0", false },
    { L"CornerRadius", nullptr, L"0", false },
    { L"Height", nullptr, L"50", false },
    { L"MinWidth", nullptr, L"168", false },
    { L"Padding", nullptr, L"8,0,8,0", false },
    { L"Margin", nullptr, L"0", false },
    { L"RenderTransform", nullptr, L"<TranslateTransform X=\"0\" Y=\"0\" />", true },
};
inline constexpr Prop kProps196[] = {
    { L"FontFamily", nullptr, L"Segoe UI Variable Display", false },
    { L"FontWeight", nullptr, L"SemiBold", false },
    { L"FontSize", nullptr, L"21", false },
    { L"Margin", nullptr, L"0", false },
    { L"Padding", nullptr, L"0", false },
    { L"RenderTransform", nullptr, L"<TranslateTransform X=\"0\" Y=\"0\" />", true },
};
inline constexpr Prop kProps197[] = {
    { L"Visibility", nullptr, L"Visible", false },
    { L"FontFamily", nullptr, L"Segoe UI Variable Text", false },
    { L"FontWeight", nullptr, L"Medium", false },
    { L"FontSize", nullptr, L"11", false },
    { L"Margin", nullptr, L"0", false },
    { L"Foreground", nullptr, L"<SolidColorBrush Color=\"#B8F5F5F7\" />", true },
    { L"RenderTransform", nullptr, L"<TranslateTransform X=\"0\" Y=\"0\" />", true },
};
inline constexpr Prop kProps199[] = {
    { L"Height", nullptr, L"2", false },
    { L"RadiusX", nullptr, L"2", false },
    { L"RadiusY", nullptr, L"2", false },
    { L"Width", L"InactiveRunningIndicator", L"5", false },
    { L"Fill", L"InactiveRunningIndicator", L"<SolidColorBrush Color=\"#FFFFFF\" Opacity=\"0.38\" />", true },
    { L"Width", L"ActiveRunningIndicator", L"12", false },
    { L"Fill", L"ActiveRunningIndicator", L"<SolidColorBrush Color=\"#F5F5F7\" Opacity=\"0.96\" />", true },
    { L"Width", L"RequestingAttentionRunningIndicator", L"12", false },
    { L"Fill", L"RequestingAttentionRunningIndicator", L"<SolidColorBrush Color=\"#B8B8BD\" Opacity=\"0.96\" />", true },
    { L"Margin", nullptr, L"0,0,0,2", false },
};
inline constexpr Prop kProps200[] = {
    { L"CornerRadius", nullptr, L"4", false },
    { L"Background", L"NoRunningIndicator", L"Transparent", false },
    { L"Background", L"InactiveRunningIndicator", L"Transparent", false },
    { L"Background", L"InactiveRunningIndicatorPointerOver", L"Transparent", false },
    { L"Background", L"ActiveRunningIndicator", L"Transparent", false },
    { L"Background", L"ActiveRunningIndicatorPointerOver", L"Transparent", false },
    { L"Background", L"RequestingAttentionRunningIndicator", L"Transparent", false },
    { L"BorderThickness", L"ActiveRunningIndicator", L"0", false },
    { L"BorderBrush", L"ActiveRunningIndicator", L"Transparent", false },
};
inline constexpr Prop kProps201[] = {
    { L"CornerRadius", nullptr, L"4", false },
    { L"Background", L"NoRunningIndicator", L"Transparent", false },
    { L"Background", L"InactiveRunningIndicator", L"Transparent", false },
    { L"Background", L"InactiveRunningIndicatorPointerOver", L"Transparent", false },
    { L"Background", L"ActiveRunningIndicator", L"Transparent", false },
    { L"Background", L"ActiveRunningIndicatorPointerOver", L"Transparent", false },
    { L"Background", L"RequestingAttentionRunningIndicator", L"Transparent", false },
    { L"BorderThickness", L"ActiveRunningIndicator", L"0", false },
    { L"BorderBrush", L"ActiveRunningIndicator", L"Transparent", false },
};
inline constexpr Prop kProps202[] = {
    { L"RenderTransformOrigin", nullptr, L"0.5,0.5", false },
    { L"RenderTransform", L"InactivePointerOver", L"<TransformGroup><ScaleTransform ScaleX=\"1.05\" ScaleY=\"1.05\" /></TransformGroup>", true },
    { L"RenderTransform", L"ActivePointerOver", L"<TransformGroup><ScaleTransform ScaleX=\"1.05\" ScaleY=\"1.05\" /></TransformGroup>", true },
    { L"RenderTransform", L"MultiWindowPointerOver", L"<TransformGroup><ScaleTransform ScaleX=\"1.05\" ScaleY=\"1.05\" /></TransformGroup>", true },
    { L"RenderTransform", L"RequestingAttentionPointerOver", L"<TransformGroup><ScaleTransform ScaleX=\"1.05\" ScaleY=\"1.05\" /></TransformGroup>", true },
    { L"RenderTransform", L"InactivePressed", L"<TransformGroup><ScaleTransform ScaleX=\"0.97\" ScaleY=\"0.97\" /></TransformGroup>", true },
    { L"RenderTransform", L"ActivePressed", L"<TransformGroup><ScaleTransform ScaleX=\"0.97\" ScaleY=\"0.97\" /></TransformGroup>", true },
    { L"RenderTransform", L"MultiWindowPressed", L"<TransformGroup><ScaleTransform ScaleX=\"0.97\" ScaleY=\"0.97\" /></TransformGroup>", true },
    { L"RenderTransform", L"RequestingAttentionPressed", L"<TransformGroup><ScaleTransform ScaleX=\"0.97\" ScaleY=\"0.97\" /></TransformGroup>", true },
};
inline constexpr Prop kProps203[] = {
    { L"RenderTransformOrigin", nullptr, L"0.5,0.5", false },
    { L"RenderTransform", L"InactivePointerOver", L"<TransformGroup><ScaleTransform ScaleX=\"1.05\" ScaleY=\"1.05\" /></TransformGroup>", true },
    { L"RenderTransform", L"ActivePointerOver", L"<TransformGroup><ScaleTransform ScaleX=\"1.05\" ScaleY=\"1.05\" /></TransformGroup>", true },
    { L"RenderTransform", L"MultiWindowPointerOver", L"<TransformGroup><ScaleTransform ScaleX=\"1.05\" ScaleY=\"1.05\" /></TransformGroup>", true },
    { L"RenderTransform", L"RequestingAttentionPointerOver", L"<TransformGroup><ScaleTransform ScaleX=\"1.05\" ScaleY=\"1.05\" /></TransformGroup>", true },
    { L"RenderTransform", L"InactivePressed", L"<TransformGroup><ScaleTransform ScaleX=\"0.97\" ScaleY=\"0.97\" /></TransformGroup>", true },
    { L"RenderTransform", L"ActivePressed", L"<TransformGroup><ScaleTransform ScaleX=\"0.97\" ScaleY=\"0.97\" /></TransformGroup>", true },
    { L"RenderTransform", L"MultiWindowPressed", L"<TransformGroup><ScaleTransform ScaleX=\"0.97\" ScaleY=\"0.97\" /></TransformGroup>", true },
    { L"RenderTransform", L"RequestingAttentionPressed", L"<TransformGroup><ScaleTransform ScaleX=\"0.97\" ScaleY=\"0.97\" /></TransformGroup>", true },
};
inline constexpr Prop kProps204[] = {
    { L"RenderTransform", nullptr, L"<TranslateTransform X=\"4\" Y=\"0\" />", true },
};
inline constexpr Prop kProps205[] = {
    { L"Margin", nullptr, L"0,0,12,0", false },
};
inline constexpr Prop kProps206[] = {
    { L"MinWidth", nullptr, L"12", false },
    { L"Width", nullptr, L"12", false },
    { L"Height", nullptr, L"12", false },
    { L"CornerRadius", nullptr, L"4", false },
    { L"Background", nullptr, L"<SolidColorBrush Color=\"#F5F5F7\" Opacity=\"0.96\" />", true },
    { L"Foreground", nullptr, L"<SolidColorBrush Color=\"#F5F5F7\" />", true },
    { L"BorderBrush", nullptr, L"<SolidColorBrush Color=\"#F5F5F7\" Opacity=\"0.72\" />", true },
    { L"BorderThickness", nullptr, L"1", false },
    { L"Canvas.ZIndex", nullptr, L"3", false },
};
inline constexpr Prop kProps207[] = {
    { L"MinWidth", nullptr, L"12", false },
    { L"Width", nullptr, L"12", false },
    { L"Height", nullptr, L"12", false },
    { L"CornerRadius", nullptr, L"4", false },
    { L"Background", nullptr, L"<SolidColorBrush Color=\"#F5F5F7\" Opacity=\"0.96\" />", true },
    { L"Foreground", nullptr, L"<SolidColorBrush Color=\"#F5F5F7\" />", true },
    { L"BorderBrush", nullptr, L"<SolidColorBrush Color=\"#F5F5F7\" Opacity=\"0.72\" />", true },
    { L"BorderThickness", nullptr, L"1", false },
    { L"Canvas.ZIndex", nullptr, L"3", false },
};
inline constexpr Prop kProps208[] = {
    { L"FontFamily", nullptr, L"Segoe UI Variable Text", false },
    { L"FontSize", nullptr, L"8", false },
    { L"FontWeight", nullptr, L"SemiBold", false },
};
inline constexpr Prop kProps209[] = {
    { L"FontFamily", nullptr, L"Segoe UI Variable Text", false },
    { L"FontSize", nullptr, L"8", false },
    { L"FontWeight", nullptr, L"SemiBold", false },
};
// 38 leaves a 16-DIP overlay (Firefox, ChatGPT) inside the 50-DIP button
// without clipping the corner. 44 filled the slot; overlay sat on the clip
// edge. Hover zoom 1.05 of 38 is 39.9, still inside 50.
inline constexpr Prop kProps210[] = {
    { L"MaxWidth", nullptr, L"38", false },
    { L"MaxHeight", nullptr, L"38", false },
    { L"Stretch", nullptr, L"Uniform", false },
};
inline constexpr Prop kProps211[] = {
    { L"MaxWidth", nullptr, L"38", false },
    { L"MaxHeight", nullptr, L"38", false },
    { L"Stretch", nullptr, L"Uniform", false },
};
inline constexpr Prop kProps212[] = {
    { L"CornerRadius", nullptr, L"4", false },
    { L"Margin", nullptr, L"2,5,2,5", false },
    { L"Background", L"Normal", L"Transparent", false },
    { L"Background", L"PointerOver", L"<SolidColorBrush Color=\"#FFFFFF\" Opacity=\"0.07\" />", true },
    { L"Background", L"Pressed", L"<SolidColorBrush Color=\"#F5F5F7\" Opacity=\"0.12\" />", true },
    { L"BorderBrush", nullptr, L"Transparent", false },
    { L"BorderBrush", L"PointerOver", L"<SolidColorBrush Color=\"#F5F5F7\" Opacity=\"0.12\" />", true },
    { L"BorderBrush", L"Pressed", L"<SolidColorBrush Color=\"#F5F5F7\" Opacity=\"0.24\" />", true },
    { L"BorderThickness", nullptr, L"1", false },
};
inline constexpr Prop kProps213[] = {
    { L"CornerRadius", nullptr, L"4", false },
    { L"Margin", nullptr, L"2,5,2,5", false },
    { L"Background", L"Normal", L"Transparent", false },
    { L"Background", L"PointerOver", L"<SolidColorBrush Color=\"#FFFFFF\" Opacity=\"0.07\" />", true },
    { L"Background", L"Pressed", L"<SolidColorBrush Color=\"#F5F5F7\" Opacity=\"0.12\" />", true },
    { L"BorderBrush", nullptr, L"Transparent", false },
    { L"BorderBrush", L"PointerOver", L"<SolidColorBrush Color=\"#F5F5F7\" Opacity=\"0.12\" />", true },
    { L"BorderBrush", L"Pressed", L"<SolidColorBrush Color=\"#F5F5F7\" Opacity=\"0.24\" />", true },
    { L"BorderThickness", nullptr, L"1", false },
};
inline constexpr Prop kProps214[] = {
    { L"CornerRadius", nullptr, L"4", false },
    { L"Margin", nullptr, L"2,5,2,5", false },
    { L"Background", L"Normal", L"Transparent", false },
    { L"Background", L"PointerOver", L"<SolidColorBrush Color=\"#FFFFFF\" Opacity=\"0.07\" />", true },
    { L"Background", L"Pressed", L"<SolidColorBrush Color=\"#F5F5F7\" Opacity=\"0.12\" />", true },
    { L"BorderBrush", nullptr, L"Transparent", false },
    { L"BorderBrush", L"PointerOver", L"<SolidColorBrush Color=\"#F5F5F7\" Opacity=\"0.12\" />", true },
    { L"BorderBrush", L"Pressed", L"<SolidColorBrush Color=\"#F5F5F7\" Opacity=\"0.24\" />", true },
    { L"BorderThickness", nullptr, L"1", false },
};
inline constexpr Prop kProps215[] = {
    { L"CornerRadius", nullptr, L"4", false },
    { L"Margin", nullptr, L"2,4,2,4", false },
    { L"Background", L"Normal", L"Transparent", false },
    { L"Background", L"PointerOver", L"<SolidColorBrush Color=\"#FFFFFF\" Opacity=\"0.07\" />", true },
    { L"Background", L"Pressed", L"<SolidColorBrush Color=\"#F5F5F7\" Opacity=\"0.12\" />", true },
    { L"BorderBrush", nullptr, L"Transparent", false },
    { L"BorderBrush", L"PointerOver", L"<SolidColorBrush Color=\"#F5F5F7\" Opacity=\"0.12\" />", true },
    { L"BorderBrush", L"Pressed", L"<SolidColorBrush Color=\"#F5F5F7\" Opacity=\"0.24\" />", true },
    { L"BorderThickness", nullptr, L"1", false },
};
inline constexpr Prop kProps216[] = {
    { L"Background", nullptr, L"$Background", true },
    { L"BorderBrush", nullptr, L"Transparent", false },
    { L"BorderThickness", nullptr, L"0", false },
    { L"CornerRadius", nullptr, L"8", false },
    { L"UseLayoutRounding", nullptr, L"True", false },
};
inline constexpr Prop kProps217[] = {
    { L"Background", nullptr, L"$Background", true },
    { L"BorderBrush", nullptr, L"Transparent", false },
    { L"BorderThickness", nullptr, L"0", false },
    { L"CornerRadius", nullptr, L"8", false },
    { L"UseLayoutRounding", nullptr, L"True", false },
};
inline constexpr Prop kProps218[] = {
    { L"Background", nullptr, L"$ElementBG", true },
    { L"BorderBrush", nullptr, L"Transparent", false },
    { L"BorderThickness", nullptr, L"0", false },
    { L"CornerRadius", nullptr, L"8", false },
    { L"UseLayoutRounding", nullptr, L"True", false },
};
inline constexpr Prop kProps219[] = {
    { L"Background", nullptr, L"$ElementBG", true },
    { L"BorderBrush", nullptr, L"$BorderBrush", true },
    { L"BorderThickness", nullptr, L"1", false },
    { L"CornerRadius", nullptr, L"4", false },
    { L"UseLayoutRounding", nullptr, L"True", false },
};
inline constexpr Prop kProps220[] = {
    { L"Background", nullptr, L"$Background", true },
    { L"BorderBrush", nullptr, L"Transparent", false },
    { L"BorderThickness", nullptr, L"0", false },
    { L"CornerRadius", nullptr, L"8", false },
    { L"UseLayoutRounding", nullptr, L"True", false },
};
inline constexpr Prop kProps221[] = {
    { L"BorderBrush", nullptr, L"Transparent", false },
    { L"BorderThickness", nullptr, L"0", false },
};
inline constexpr Prop kProps222[] = {
    { L"BorderBrush", nullptr, L"Transparent", false },
    { L"BorderThickness", nullptr, L"0", false },
};
inline constexpr Prop kProps223[] = {
    { L"Background", nullptr, L"$ElementBG", true },
    { L"BorderBrush", nullptr, L"Transparent", false },
    { L"BorderThickness", nullptr, L"0", false },
    { L"CornerRadius", nullptr, L"8", false },
    { L"UseLayoutRounding", nullptr, L"True", false },
};
inline constexpr Prop kProps224[] = {
    { L"Background", nullptr, L"$ElementBG", true },
    { L"UseLayoutRounding", nullptr, L"True", false },
};
inline constexpr Prop kProps225[] = {
    { L"Background", nullptr, L"$ElementBG", true },
    { L"UseLayoutRounding", nullptr, L"True", false },
};
inline constexpr Prop kProps226[] = {
    { L"Background", nullptr, L"$Background", true },
    { L"CornerRadius", nullptr, L"8", false },
    { L"UseLayoutRounding", nullptr, L"True", false },
};
inline constexpr Prop kProps227[] = {
    { L"Background", nullptr, L"$Background", true },
    { L"UseLayoutRounding", nullptr, L"True", false },
};
inline constexpr Prop kProps228[] = {
    { L"Background", nullptr, L"$Background", true },
    { L"BorderBrush", nullptr, L"Transparent", false },
    { L"BorderThickness", nullptr, L"0", false },
    { L"CornerRadius", nullptr, L"4", false },
    { L"UseLayoutRounding", nullptr, L"True", false },
};
inline constexpr Prop kProps229[] = {
    { L"Background", nullptr, L"Transparent", false },
    { L"CornerRadius", nullptr, L"8", false },
    { L"UseLayoutRounding", nullptr, L"True", false },
};
inline constexpr Prop kProps230[] = {
    { L"Background", nullptr, L"Transparent", false },
};
inline constexpr Prop kProps231[] = {
    { L"Background", nullptr, L"<WindhawkBlur BlurAmount=\"40\" TintColor=\"#050505\" TintOpacity=\"0.94\" TintLuminosityOpacity=\"0.06\" TintSaturation=\"0.0\" NoiseOpacity=\"0.006\" FallbackColor=\"#050505\" />", true },
    { L"CornerRadius", nullptr, L"8", false },
};
inline constexpr Prop kProps232[] = {
    { L"Background", nullptr, L"<WindhawkBlur BlurAmount=\"40\" TintColor=\"#050505\" TintOpacity=\"0.94\" TintLuminosityOpacity=\"0.06\" TintSaturation=\"0.0\" NoiseOpacity=\"0.006\" FallbackColor=\"#050505\" />", true },
};
inline constexpr Prop kProps233[] = {
    { L"Background", nullptr, L"<WindhawkBlur BlurAmount=\"40\" TintColor=\"#050505\" TintOpacity=\"0.94\" TintLuminosityOpacity=\"0.06\" TintSaturation=\"0.0\" NoiseOpacity=\"0.006\" FallbackColor=\"#050505\" />", true },
    { L"BorderBrush", nullptr, L"Transparent", false },
    { L"BorderThickness", nullptr, L"0", false },
    { L"CornerRadius", nullptr, L"4", false },
};
inline constexpr Prop kProps234[] = {
    { L"Background", nullptr, L"<WindhawkBlur BlurAmount=\"40\" TintColor=\"#050505\" TintOpacity=\"0.94\" TintLuminosityOpacity=\"0.06\" TintSaturation=\"0.0\" NoiseOpacity=\"0.006\" FallbackColor=\"#050505\" />", true },
    { L"BorderBrush", nullptr, L"Transparent", false },
    { L"BorderThickness", nullptr, L"0", false },
    { L"CornerRadius", nullptr, L"8", false },
};
inline constexpr Prop kProps235[] = {
    { L"CornerRadius", nullptr, L"4", false },
};
inline constexpr Prop kProps236[] = {
    { L"CornerRadius", nullptr, L"4", false },
};
inline constexpr Prop kProps237[] = {
    { L"CornerRadius", nullptr, L"4", false },
};
inline constexpr Prop kProps238[] = {
    { L"Background", nullptr, L"$Background", true },
    { L"BorderBrush", nullptr, L"Transparent", false },
    { L"BorderThickness", nullptr, L"0", false },
    { L"CornerRadius", nullptr, L"8", false },
    { L"UseLayoutRounding", nullptr, L"True", false },
};
inline constexpr Prop kProps239[] = {
    { L"Background", nullptr, L"$Background", true },
    { L"BorderBrush", nullptr, L"Transparent", false },
    { L"BorderThickness", nullptr, L"0", false },
    { L"CornerRadius", nullptr, L"8", false },
    { L"UseLayoutRounding", nullptr, L"True", false },
};
inline constexpr Prop kProps240[] = {
    { L"Background", nullptr, L"$Background", true },
    { L"BorderBrush", nullptr, L"Transparent", false },
    { L"BorderThickness", nullptr, L"0", false },
    { L"CornerRadius", nullptr, L"8", false },
    { L"UseLayoutRounding", nullptr, L"True", false },
};
inline constexpr Prop kProps241[] = {
    { L"Background", nullptr, L"$ElementBG", true },
    { L"BorderBrush", nullptr, L"$BorderBrush", true },
    { L"BorderThickness", nullptr, L"1", false },
    { L"CornerRadius", nullptr, L"8", false },
    { L"UseLayoutRounding", nullptr, L"True", false },
};

inline constexpr Rule kRules[] = {
    { Host::Explorer, L"Taskbar.TaskbarFrame", kProps0, 4 },
    { Host::Explorer, L"Taskbar.TaskbarFrame > Grid#RootGrid", kProps1, 6 },
    { Host::Explorer, L"Taskbar.TaskbarBackground#BackgroundControl > Windows.UI.Xaml.Controls.Grid > Windows.UI.Xaml.Shapes.Rectangle#BackgroundFill", kProps2, 1 },
    { Host::Explorer, L"Taskbar.TaskbarBackground#BackgroundControl > Windows.UI.Xaml.Controls.Grid > Windows.UI.Xaml.Shapes.Rectangle#BackgroundStroke", kProps3, 1 },
    { Host::Explorer, L"Taskbar.AugmentedEntryPointButton#AugmentedEntryPointButton > Taskbar.TaskListButtonPanel#ExperienceToggleButtonRootPanel", kProps4, 7 },
    { Host::Explorer, L"SystemTray.SystemTrayFrame", kProps5, 4 },
    { Host::Explorer, L"StackPanel#SystemTrayFrameGrid", kProps6, 5 },
    { Host::Explorer, L"Grid#SystemTrayFrameGrid", kProps7, 5 },
    { Host::Explorer, L":root > ScrollViewer > ScrollContentPresenter > Border > Grid", kProps8, 2 },
    { Host::Explorer, L"SystemTray.ChevronIconView", kProps9, 3 },
    { Host::Explorer, L"SystemTray.NotifyIconView#NotifyItemIcon", kProps10, 4 },
    { Host::Explorer, L"SystemTray.OmniButton", kProps11, 3 },
    { Host::Explorer, L"SystemTray.CopilotIcon", kProps12, 2 },
    { Host::Explorer, L"SystemTray.OmniButton#NotificationCenterButton > Grid > ContentPresenter > ItemsPresenter > StackPanel > ContentPresenter > SystemTray.IconView#SystemTrayIcon > Grid", kProps13, 1 },
    { Host::Explorer, L"SystemTray.IconView#SystemTrayIcon > Grid#ContainerGrid > ContentPresenter#ContentPresenter > Grid#ContentGrid > SystemTray.TextIconContent > Grid#ContainerGrid", kProps14, 2 },
    { Host::Explorer, L"SystemTray.StackListView#IconStack > ItemsPresenter > StackPanel > ContentPresenter > SystemTray.IconView#SystemTrayIcon", kProps15, 1 },
    { Host::Explorer, L"SystemTray.Stack#ShowDesktopStack", kProps16, 1 },
    { Host::Explorer, L"Taskbar.Gripper#GripperControl", kProps17, 2 },
    { Host::Explorer, L"Windows.UI.Xaml.Controls.Grid#AugmentedEntryPointContentGrid", kProps18, 1 },
    { Host::Explorer, L"TextBlock#InnerTextBlock[Text=\\uE971]", kProps21, 1 },
    { Host::Explorer, L"Windows.UI.Xaml.Controls.Grid#ConfirmatorMainGrid", kProps22, 4 },
    { Host::Explorer, L"TextBlock#SearchBoxTextBlock", kProps23, 3 },
    { Host::Explorer, L"SystemTray.OmniButton#NotificationCenterButton > Grid > ContentPresenter > ItemsPresenter > StackPanel > ContentPresenter > SystemTray.IconView#SystemTrayIcon > Grid > Grid > SystemTray.TextIconContent", kProps24, 1 },
    { Host::Explorer, L"Windows.UI.Xaml.Controls.Button", kProps25, 1 },
    { Host::Explorer, L"WindowsInternal.ComposableShell.Experiences.Switcher.AltTab > Windows.UI.Xaml.Controls.Grid#ModalRootGrid > Windows.UI.Xaml.Controls.Border#BackgroundElement", kProps26, 4 },
    { Host::Explorer, L"WindowsInternal.ComposableShell.Experiences.Switcher.AltTab > Windows.UI.Xaml.Controls.Grid#ModalRootGrid > Windows.UI.Xaml.Controls.Border#BackgroundElement > WindowsInternal.ComposableShell.Experiences.Switcher.SwitchItemList", kProps27, 1 },
    { Host::Explorer, L"WindowsInternal.ComposableShell.Experiences.Switcher.VirtualDesktopBarElement#VirtualDesktopBar > Grid > Border", kProps28, 4 },
    { Host::Explorer, L"WindowsInternal.ComposableShell.Experiences.Switcher.VirtualDesktopBarElement#VirtualDesktopBar", kProps29, 3 },
    { Host::Explorer, L"Windows.UI.Xaml.Controls.Border#BackgroundDimmingLayer", kProps30, 1 },
    { Host::Explorer, L"Taskbar.TaskListButton#TaskListButton", kProps31, 1 },
    { Host::Explorer, L"Windows.UI.Xaml.Controls.Border#SnapBarBorder", kProps32, 6 },
    { Host::Explorer, L"Windows.UI.Xaml.Controls.Border#SnapPickerBorder", kProps33, 4 },
    { Host::Explorer, L"Windows.UI.Xaml.Controls.Border#SearchPillBackgroundElement", kProps34, 5 },
    { Host::Explorer, L"Taskbar.TaskbarExtensionElement", kProps35, 1 },
    { Host::Explorer, L"Taskbar.TaskListButtonPanel#ExperienceToggleButtonRootPanel", kProps36, 1 },
    { Host::Explorer, L"Windows.UI.Xaml.Controls.ToolTip > Windows.UI.Xaml.Controls.ContentPresenter#LayoutRoot", kProps37, 4 },
    { Host::Explorer, L"SearchUx.SearchUI.SearchButtonControl", kProps38, 4 },
    { Host::Explorer, L"WindowsInternal.ComposableShell.Experiences.Switcher.VirtualDesktopBarElement > Windows.UI.Xaml.Controls.Grid#GridElement > Windows.UI.Xaml.Controls.Border#VirtualDesktopSwitcherBackground", kProps39, 4 },
    { Host::Explorer, L"Windows.UI.Xaml.Shapes.Rectangle#BackgroundFill", kProps40, 1 },
    { Host::Explorer, L"Windows.UI.Xaml.Controls.Border#OverflowFlyoutBackgroundBorder", kProps41, 4 },
    { Host::Explorer, L"Windows.UI.Xaml.Controls.MenuFlyoutPresenter > Windows.UI.Xaml.Controls.Border", kProps42, 4 },
    { Host::StartMenu, L"StackPanel#TimeAndDatePanel", kProps44, 3 },
    { Host::StartMenu, L"StackPanel#TimePanel > TextBlock#Time", kProps45, 4 },
    { Host::StartMenu, L"StackPanel#TimeAndDatePanel > TextBlock#Date", kProps46, 4 },
    { Host::StartMenu, L"Windows.UI.Xaml.Controls.Grid#WidgetFrameGrid", kProps47, 4 },
    { Host::StartMenu, L"Windows.UI.Xaml.Controls.Grid#WidgetCanvasPanel", kProps48, 2 },
    { Host::StartMenu, L"Windows.UI.Xaml.Controls.Grid#MediaTransportControls", kProps49, 4 },
    { Host::StartMenu, L"Windows.UI.Xaml.Controls.Grid#MediaControlsContainer", kProps50, 4 },
    { Host::StartMenu, L"Windows.UI.Xaml.Controls.Grid#RootPanel > Windows.UI.Xaml.Controls.Grid#RootGrid > Windows.UI.Xaml.Controls.Grid#RootContent", kProps51, 1 },
    { Host::StartMenu, L"StartDocked.StartSizingFrame", kProps52, 1 },
    { Host::StartMenu, L"Windows.UI.Xaml.Controls.Border#RootGridDropShadow", kProps53, 1 },
    { Host::StartMenu, L"Windows.UI.Xaml.Controls.Border#RightCompanionDropShadow", kProps54, 1 },
    { Host::StartMenu, L"Windows.UI.Xaml.Controls.Border#StartDropShadow", kProps55, 1 },
    { Host::StartMenu, L"Windows.UI.Xaml.Controls.Border#DropShadowDismissTarget", kProps56, 6 },
    { Host::StartMenu, L"Windows.UI.Xaml.Controls.Grid#RootContent > Windows.UI.Xaml.Controls.Border#AcrylicBorder", kProps57, 6 },
    { Host::StartMenu, L"Windows.UI.Xaml.Controls.Border#AcrylicOverlay", kProps58, 1 },
    { Host::StartMenu, L"StartDocked.SearchBoxToggleButton#StartMenuSearchBox", kProps59, 3 },
    { Host::StartMenu, L"StartDocked.SearchBoxToggleButton#StartMenuSearchBox > Windows.UI.Xaml.Controls.Grid > Windows.UI.Xaml.Controls.Border#BorderElement", kProps60, 4 },
    { Host::StartMenu, L"StartDocked.SearchBoxToggleButton#StartMenuSearchBox > Windows.UI.Xaml.Controls.Grid > Windows.UI.Xaml.Controls.ContentPresenter#ContentPresenter > Windows.UI.Xaml.Controls.TextBlock#PlaceholderText", kProps61, 1 },
    { Host::StartMenu, L"Windows.UI.Xaml.Controls.Grid#TopLevelRoot > Windows.UI.Xaml.Controls.Grid", kProps62, 1 },
    { Host::StartMenu, L"StartDocked.NavigationPaneView#NavigationPane", kProps63, 2 },
    { Host::StartMenu, L"Windows.UI.Xaml.Controls.Button#ShowAllAppsButton", kProps64, 1 },
    { Host::StartMenu, L"StartMenu.PinnedList#StartMenuPinnedList", kProps65, 2 },
    { Host::StartMenu, L"StartMenu.PinnedList#StartMenuPinnedList > Windows.UI.Xaml.Controls.Grid#Root", kProps66, 4 },
    { Host::StartMenu, L"Windows.UI.Xaml.Controls.Grid#UndockedRoot", kProps67, 5 },
    { Host::StartMenu, L"Windows.UI.Xaml.Controls.Grid#AllAppsRoot", kProps68, 8 },
    { Host::StartMenu, L"Windows.UI.Xaml.Controls.Button#CloseAllAppsButton", kProps69, 1 },
    { Host::StartMenu, L"Windows.UI.Xaml.Controls.TextBlock#AllAppsHeading", kProps70, 1 },
    { Host::StartMenu, L"StartDocked.AllAppsPane#AllAppsPanel", kProps71, 1 },
    { Host::StartMenu, L"StartDocked.StartMenuCompanion#RightCompanion > Windows.UI.Xaml.Controls.Grid#CompanionRoot > Windows.UI.Xaml.Controls.Border#AcrylicBorder", kProps72, 4 },
    { Host::StartMenu, L"Windows.UI.Xaml.Controls.Grid#CompanionRoot > Windows.UI.Xaml.Controls.Grid#MainContent > Windows.UI.Xaml.Controls.Grid#ActionsBar > Windows.UI.Xaml.Controls.Button#PrimaryActionBarButton > Windows.UI.Xaml.Controls.ContentPresenter#ContentPresenter", kProps73, 5 },
    { Host::StartMenu, L"Windows.UI.Xaml.Controls.Grid#ActionsBar > Windows.UI.Xaml.Controls.Button#ActionBarOverflowButton", kProps74, 5 },
    { Host::StartMenu, L"StartDocked.StartMenuCompanion#RightCompanion > Windows.UI.Xaml.Controls.Grid#CompanionRoot", kProps75, 3 },
    { Host::StartMenu, L"Windows.UI.Xaml.Controls.Border#OverflowFlyoutBackgroundBorder", kProps76, 4 },
    { Host::StartMenu, L"Windows.UI.Xaml.Controls.MenuFlyoutPresenter > Windows.UI.Xaml.Controls.Border", kProps77, 4 },
    { Host::StartMenu, L"Windows.UI.Xaml.Controls.Grid#HoverFlyoutGrid > Windows.UI.Xaml.Controls.Border#HoverFlyoutBackground", kProps78, 4 },
    { Host::StartMenu, L"Cortana.UI.Views.TaskbarSearchPage > Windows.UI.Xaml.Controls.Grid#RootGrid > Windows.UI.Xaml.Controls.Grid#OuterBorderGrid", kProps79, 4 },
    { Host::StartMenu, L"Windows.UI.Xaml.Controls.Border#LayerBorder", kProps80, 1 },
    { Host::StartMenu, L"Windows.UI.Xaml.Controls.Border#AccentLayerBorder", kProps81, 1 },
    { Host::StartMenu, L"Windows.UI.Xaml.Controls.Border#dropshadow", kProps82, 1 },
    { Host::StartMenu, L"Windows.UI.Xaml.Controls.Border#AppBorder", kProps83, 1 },
    { Host::StartMenu, L"Windows.UI.Xaml.Controls.ToolTip > Windows.UI.Xaml.Controls.ContentPresenter#LayoutRoot", kProps84, 4 },
    { Host::StartMenu, L"StartMenu.FolderModal#StartFolderModal > Windows.UI.Xaml.Controls.Grid#Root", kProps85, 4 },
    { Host::StartMenu, L"StartMenu.FolderModal#StartFolderModal > Windows.UI.Xaml.Controls.Grid#Root > Windows.UI.Xaml.Controls.ContentControl#ContentControl > Windows.UI.Xaml.Controls.ContentPresenter > StartMenu.UniversalTileContainer#UniversalTileContainer > Windows.UI.Xaml.Controls.Grid#GridViewContainer", kProps86, 2 },
    { Host::StartMenu, L"Windows.UI.Xaml.Controls.Grid#Root > Windows.UI.Xaml.Controls.Border", kProps87, 4 },
    { Host::StartMenu, L"StartMenu.ExpandedFolderList", kProps88, 1 },
    { Host::StartMenu, L"Windows.UI.Xaml.Controls.Grid#MainMenu > Windows.UI.Xaml.Controls.Border#AcrylicBorder", kProps89, 1 },
    { Host::StartMenu, L"StartMenu.SearchBoxToggleButton#SearchBoxToggleButton", kProps90, 3 },
    { Host::StartMenu, L"StartMenu.SearchBoxToggleButton#SearchBoxToggleButton > Windows.UI.Xaml.Controls.Grid > Windows.UI.Xaml.Controls.Border#BorderElement", kProps91, 4 },
    { Host::StartMenu, L"Windows.UI.Xaml.Controls.Primitives.ToggleButton#ShowHideCompanion", kProps92, 1 },
    { Host::StartMenu, L"Windows.UI.Xaml.Controls.TextBlock#PinnedListHeaderText", kProps93, 1 },
    { Host::StartMenu, L"Windows.UI.Xaml.Controls.Grid#AllListHeading", kProps94, 1 },
    { Host::StartMenu, L"Windows.UI.Xaml.Controls.Grid#AllListHeading > Windows.UI.Xaml.Controls.TextBlock#AllListHeadingText", kProps95, 1 },
    { Host::StartMenu, L"StartMenu.CategoryControl > Windows.UI.Xaml.Controls.Grid#RootGrid > Windows.UI.Xaml.Controls.Border", kProps96, 4 },
    { Host::StartMenu, L"Windows.UI.Xaml.Controls.Grid#MainMenu", kProps97, 1 },
    { Host::StartMenu, L"StartMenu.PinnedList#StartMenuPinnedList", kProps98, 3 },
    { Host::StartMenu, L"Windows.UI.Xaml.Controls.GridView#PinnedList > Border > Windows.UI.Xaml.Controls.ScrollViewer", kProps99, 5 },
    { Host::StartMenu, L"StartMenu.StartMenuCompanion#RightCompanion", kProps100, 2 },
    { Host::StartMenu, L"Windows.UI.Xaml.Controls.Grid#CompanionRoot > Windows.UI.Xaml.Controls.Border#AcrylicBorder", kProps101, 4 },
    { Host::StartMenu, L"Windows.UI.Xaml.Controls.GridView#AllAppsGrid > Windows.UI.Xaml.Controls.ItemsWrapGrid", kProps102, 1 },
    { Host::StartMenu, L"Windows.UI.Xaml.Controls.GridView#AllAppsGrid", kProps103, 1 },
    { Host::StartMenu, L"Windows.UI.Xaml.Controls.Grid#TopLevelHeader > Windows.UI.Xaml.Controls.Grid > Windows.UI.Xaml.Controls.Button", kProps104, 1 },
    { Host::StartMenu, L"Windows.UI.Xaml.Controls.FlyoutPresenter", kProps105, 4 },
    { Host::StartMenu, L"Windows.UI.Xaml.Controls.MenuFlyoutPresenter", kProps106, 1 },
    { Host::StartMenu, L"Windows.UI.Xaml.Controls.Grid#AllListHeading > Microsoft.UI.Xaml.Controls.DropDownButton#ViewSelectionButton > Grid#RootGrid", kProps107, 2 },
    { Host::StartMenu, L"MenuFlyoutItem", kProps108, 2 },
    { Host::StartMenu, L"ToggleMenuFlyoutItem", kProps109, 2 },
    { Host::ShellFlyout, L"Grid#NotificationCenterGrid", kProps110, 4 },
    { Host::ShellFlyout, L"Grid#CalendarCenterGrid", kProps111, 6 },
    { Host::ShellFlyout, L"ScrollViewer#CalendarControlScrollViewer", kProps112, 5 },
    { Host::ShellFlyout, L"Border#CalendarHeaderMinimizedOverlay", kProps113, 6 },
    { Host::ShellFlyout, L"ActionCenter.FocusSessionControl#FocusSessionControl > Grid#FocusGrid", kProps114, 5 },
    { Host::ShellFlyout, L"MenuFlyoutPresenter > Border", kProps115, 5 },
    { Host::ShellFlyout, L"MenuFlyoutItem > Grid#LayoutRoot", kProps116, 1 },
    { Host::ShellFlyout, L"Border#JumpListRestyledAcrylic", kProps117, 5 },
    { Host::ShellFlyout, L"Windows.UI.Xaml.Controls.Grid#ControlCenterRegion", kProps118, 4 },
    { Host::ShellFlyout, L"ContentPresenter#PageContent", kProps119, 1 },
    { Host::ShellFlyout, L"ContentPresenter#PageContent > Grid > Border", kProps120, 3 },
    { Host::ShellFlyout, L"QuickActions.ControlCenter.AccessibleWindow#PageWindow > ContentPresenter > Grid#FullScreenPageRoot", kProps121, 1 },
    { Host::ShellFlyout, L"QuickActions.ControlCenter.AccessibleWindow#PageWindow > ContentPresenter > Grid#FullScreenPageRoot > ContentPresenter#PageHeader", kProps122, 3 },
    { Host::ShellFlyout, L"ScrollViewer#ListContent", kProps123, 3 },
    { Host::ShellFlyout, L"ActionCenter.FlexibleToastView#FlexibleNormalToastView", kProps124, 4 },
    { Host::ShellFlyout, L"Border#ToastBackgroundBorder2", kProps125, 4 },
    { Host::ShellFlyout, L"Border#ToastBackgroundBorder", kProps126, 4 },
    { Host::ShellFlyout, L"JumpViewUI.SystemItemListViewItem > Grid#LayoutRoot > Border#BackgroundBorder", kProps127, 1 },
    { Host::ShellFlyout, L"JumpViewUI.JumpListListViewItem > Grid#LayoutRoot > Border#BackgroundBorder", kProps128, 1 },
    { Host::ShellFlyout, L"ActionCenter.FlexibleItemView", kProps129, 1 },
    { Host::ShellFlyout, L"Grid#NotificationCenterTopBanner", kProps130, 3 },
    { Host::ShellFlyout, L"Windows.UI.Xaml.Controls.Grid#L1Grid > Border", kProps131, 1 },
    { Host::ShellFlyout, L"Windows.UI.Xaml.Controls.ContentPresenter", kProps132, 1 },
    { Host::ShellFlyout, L"Windows.UI.Xaml.Controls.Button#FooterButton[AutomationProperties.Name=Edit quick settings]", kProps133, 2 },
    { Host::ShellFlyout, L"Windows.UI.Xaml.Controls.Button[AutomationProperties.AutomationId=Microsoft.QuickAction.Battery]", kProps134, 2 },
    { Host::ShellFlyout, L"Windows.UI.Xaml.Controls.Button#FooterButton[AutomationProperties.Name=All settings]", kProps135, 3 },
    { Host::ShellFlyout, L"Windows.UI.Xaml.Controls.Button[AutomationProperties.AutomationId=Microsoft.QuickAction.Volume]", kProps136, 1 },
    { Host::ShellFlyout, L"Windows.UI.Xaml.Controls.Button#VolumeL2Button[AutomationProperties.Name=Select a sound output]", kProps137, 1 },
    { Host::ShellFlyout, L"Windows.UI.Xaml.Shapes.Rectangle#HorizontalTrackRect", kProps138, 5 },
    { Host::ShellFlyout, L"Windows.UI.Xaml.Shapes.Rectangle#HorizontalDecreaseRect", kProps139, 4 },
    { Host::ShellFlyout, L"Windows.UI.Xaml.Controls.Primitives.Thumb#HorizontalThumb", kProps140, 4 },
    { Host::ShellFlyout, L"Windows.UI.Xaml.Controls.Grid#MediaTransportControlsRegion", kProps141, 7 },
    { Host::ShellFlyout, L"Windows.UI.Xaml.Controls.Grid#AlbumTextAndArtContainer", kProps142, 3 },
    { Host::ShellFlyout, L"Windows.UI.Xaml.Controls.Grid#ThumbnailImage", kProps143, 1 },
    { Host::ShellFlyout, L"Windows.UI.Xaml.Controls.StackPanel#PrimaryAndSecondaryTextContainer", kProps144, 3 },
    { Host::ShellFlyout, L"Windows.UI.Xaml.Controls.StackPanel#PrimaryAndSecondaryTextContainer > Windows.UI.Xaml.Controls.TextBlock#Title", kProps145, 2 },
    { Host::ShellFlyout, L"Windows.UI.Xaml.Controls.StackPanel#PrimaryAndSecondaryTextContainer > Windows.UI.Xaml.Controls.TextBlock#Subtitle", kProps146, 4 },
    { Host::ShellFlyout, L"Windows.UI.Xaml.Controls.ListView#MediaButtonsListView", kProps147, 6 },
    { Host::ShellFlyout, L"Windows.UI.Xaml.Controls.Primitives.RepeatButton#PreviousButton", kProps148, 3 },
    { Host::ShellFlyout, L"Windows.UI.Xaml.Controls.Button#PlayPauseButton", kProps149, 3 },
    { Host::ShellFlyout, L"Windows.UI.Xaml.Controls.Primitives.RepeatButton#NextButton", kProps150, 3 },
    { Host::ShellFlyout, L"Windows.UI.Xaml.Controls.TextBlock#AppNameText", kProps151, 2 },
    { Host::ShellFlyout, L"Windows.UI.Xaml.Controls.Image#IconImage", kProps152, 4 },
    { Host::ShellFlyout, L"Grid#MediaTransportControlsRoot", kProps153, 1 },
    { Host::ShellFlyout, L"Grid#ToastPeekRegion", kProps154, 7 },
    { Host::ShellFlyout, L"Windows.UI.Xaml.Controls.CalendarViewDayItem > Windows.UI.Xaml.Controls.Border", kProps155, 2 },
    { Host::ShellFlyout, L"Windows.UI.Xaml.Controls.CalendarViewDayItem", kProps156, 1 },
    { Host::ShellFlyout, L"Windows.UI.Xaml.Controls.Control > Windows.UI.Xaml.Controls.Border", kProps157, 1 },
    { Host::ShellFlyout, L"Windows.UI.Xaml.Controls.Primitives.CalendarViewItem", kProps158, 1 },
    { Host::ShellFlyout, L"Windows.UI.Xaml.Controls.ListViewHeaderItem", kProps159, 3 },
    { Host::ShellFlyout, L"Windows.UI.Xaml.Controls.Button#SettingsButton", kProps160, 1 },
    { Host::ShellFlyout, L"Windows.UI.Xaml.Controls.Button#DismissButton", kProps161, 1 },
    { Host::ShellFlyout, L"Windows.UI.Xaml.Controls.StackPanel#CalendarHeader", kProps162, 1 },
    { Host::ShellFlyout, L"Windows.UI.Xaml.Controls.ScrollContentPresenter#ScrollContentPresenter", kProps163, 1 },
    { Host::ShellFlyout, L"Windows.UI.Xaml.Controls.Grid#WeekDayNames", kProps164, 4 },
    { Host::ShellFlyout, L"Windows.UI.Xaml.Controls.ListViewItem", kProps165, 1 },
    { Host::ShellFlyout, L"Windows.UI.Xaml.Controls.Grid#RootGrid > Windows.UI.Xaml.Controls.ContentPresenter#ContentPresenter", kProps166, 3 },
    { Host::ShellFlyout, L"Windows.UI.Xaml.Controls.Grid > Windows.UI.Xaml.Controls.Border#ItemOpaquePlating", kProps167, 3 },
    { Host::ShellFlyout, L"Windows.UI.Xaml.Controls.Grid#StandardHeroContainer", kProps168, 3 },
    { Host::ShellFlyout, L"Windows.UI.Xaml.Controls.Primitives.ScrollBar#VerticalScrollBar", kProps169, 1 },
    { Host::ShellFlyout, L"Windows.UI.Xaml.Controls.Grid#SliderContainer", kProps170, 1 },
    { Host::ShellFlyout, L"Windows.UI.Xaml.Controls.Button#BackButton", kProps171, 1 },
    { Host::ShellFlyout, L"Windows.UI.Xaml.Shapes.Rectangle#OuterBorder", kProps172, 3 },
    { Host::ShellFlyout, L"Windows.UI.Xaml.Shapes.Rectangle#SwitchKnobOff", kProps173, 2 },
    { Host::ShellFlyout, L"Windows.UI.Xaml.Controls.Border#SwitchKnobOn", kProps174, 1 },
    { Host::ShellFlyout, L"Windows.UI.Xaml.Shapes.Rectangle#SwitchKnobBounds", kProps175, 3 },
    { Host::ShellFlyout, L"ActionCenter.NotificationListViewItem", kProps176, 1 },
    { Host::ShellFlyout, L"Windows.UI.Xaml.Controls.Grid[AutomationProperties.LocalizedLandmarkType=Footer]", kProps177, 1 },
    { Host::ShellFlyout, L"NetworkUX.View.SettingsListViewItem > Windows.UI.Xaml.Controls.Primitives.ListViewItemPresenter#Root", kProps178, 1 },
    { Host::ShellFlyout, L"Windows.UI.Xaml.Controls.ContentPresenter > Windows.UI.Xaml.Controls.Border", kProps179, 1 },
    { Host::ShellFlyout, L"Button#ClearAll", kProps180, 1 },
    { Host::ShellFlyout, L"Button#ExpandCollapseButton", kProps181, 1 },
    { Host::ShellFlyout, L"ControlCenter.PaginatedToggleButton#ToggleButton > Windows.UI.Xaml.Controls.ContentPresenter#ContentPresenter", kProps182, 3 },
    { Host::ShellFlyout, L"ControlCenter.PaginatedToggleButton#SplitL2Button > Windows.UI.Xaml.Controls.ContentPresenter#ContentPresenter", kProps183, 3 },
    { Host::ShellFlyout, L"Windows.UI.Xaml.Controls.Primitives.Thumb#HorizontalThumb > Windows.UI.Xaml.Controls.Border", kProps184, 4 },
    { Host::ShellFlyout, L"Windows.UI.Xaml.Controls.Primitives.Thumb#HorizontalThumb > Windows.UI.Xaml.Controls.Border > Windows.UI.Xaml.Shapes.Ellipse#SliderInnerThumb", kProps185, 1 },
    { Host::ShellFlyout, L"Windows.UI.Xaml.Controls.ToolTip > Windows.UI.Xaml.Controls.ContentPresenter#LayoutRoot", kProps186, 4 },
    { Host::ShellFlyout, L"Windows.UI.Xaml.Controls.Primitives.RepeatButton#PreviousButton > Windows.UI.Xaml.Controls.ContentPresenter#ContentPresenter@CommonStates", kProps187, 5 },
    { Host::ShellFlyout, L"Windows.UI.Xaml.Controls.Button#PlayPauseButton > Windows.UI.Xaml.Controls.ContentPresenter#ContentPresenter@CommonStates", kProps188, 5 },
    { Host::ShellFlyout, L"Windows.UI.Xaml.Controls.Primitives.RepeatButton#NextButton > Windows.UI.Xaml.Controls.ContentPresenter#ContentPresenter@CommonStates", kProps189, 5 },
    // kProps190-192 (Quick Settings row re-ordering: ControlCenterRegion to
    // row 0, the media transport controls to row 1 bottom-aligned, RootGrid
    // stretched with MinHeight 0) were the imported theme's layout for an
    // older Quick Settings. On 26200 the media panel is a separate flyout and
    // re-rowing the control centre clipped it. Windows owns that layout now;
    // Opal styles the materials only.
    { Host::Explorer, L"Taskbar.TaskbarFrame > Grid#RootGrid", kProps193, 5 },
    { Host::Explorer, L"Grid#SystemTrayFrameGrid", kProps194, 6 },
    { Host::Explorer, L"SystemTray.DateTimeIconContent > Grid#ContainerGrid", kProps195, 9 },
    { Host::Explorer, L"TextBlock#TimeInnerTextBlock", kProps196, 6 },
    { Host::Explorer, L"TextBlock#DateInnerTextBlock", kProps197, 7 },
    { Host::Explorer, L"Taskbar.TaskListLabeledButtonPanel@RunningIndicatorStates > Rectangle#RunningIndicator", kProps199, 10 },
    { Host::Explorer, L"Grid#IconPanel@RunningIndicatorStates > Border#BackgroundElement", kProps200, 9 },
    { Host::Explorer, L"Taskbar.TaskListLabeledButtonPanel@RunningIndicatorStates > Border#BackgroundElement", kProps201, 9 },
    { Host::Explorer, L"Taskbar.TaskListButton#TaskListButton > Grid#IconPanel@CommonStates", kProps202, 9 },
    { Host::Explorer, L"Taskbar.TaskListButton#TaskListButton > Taskbar.TaskListLabeledButtonPanel#IconPanel@CommonStates", kProps203, 9 },
    { Host::Explorer, L"SystemTray.Stack#SecondaryClockStack", kProps204, 1 },
    { Host::Explorer, L"Taskbar.ExperienceToggleButton#LaunchListButton[AutomationProperties.AutomationId=TaskViewButton]", kProps205, 1 },
    { Host::Explorer, L"Grid#IconPanel > Taskbar.Badge#BadgeControl", kProps206, 9 },
    { Host::Explorer, L"Taskbar.TaskListLabeledButtonPanel#IconPanel > Taskbar.Badge#BadgeControl", kProps207, 9 },
    { Host::Explorer, L"Grid#IconPanel > Taskbar.Badge#BadgeControl > Grid > TextBlock#BadgeText", kProps208, 3 },
    { Host::Explorer, L"Taskbar.TaskListLabeledButtonPanel#IconPanel > Taskbar.Badge#BadgeControl > Grid > TextBlock#BadgeText", kProps209, 3 },
    { Host::Explorer, L"Grid#IconPanel > Windows.UI.Xaml.Controls.Image#Icon", kProps210, 3 },
    { Host::Explorer, L"Taskbar.TaskListLabeledButtonPanel#IconPanel > Windows.UI.Xaml.Controls.Image#Icon", kProps211, 3 },
    { Host::Explorer, L"SystemTray.ChevronIconView > Grid#ContainerGrid@CommonStates > Border#BackgroundBorder", kProps212, 9 },
    { Host::Explorer, L"SystemTray.NotifyIconView > Grid#ContainerGrid@CommonStates > Border#BackgroundBorder", kProps213, 9 },
    { Host::Explorer, L"SystemTray.IconView#SystemTrayIcon > Grid#ContainerGrid@CommonStates > Border#BackgroundBorder", kProps214, 9 },
    { Host::Explorer, L"SystemTray.OmniButton > Grid@CommonStates > Border#BackgroundBorder", kProps215, 9 },
    { Host::StartMenu, L"Windows.UI.Xaml.Controls.Border#DropShadowDismissTarget", kProps216, 5 },
    { Host::StartMenu, L"Windows.UI.Xaml.Controls.Grid#RootContent > Windows.UI.Xaml.Controls.Border#AcrylicBorder", kProps217, 5 },
    { Host::StartMenu, L"StartDocked.StartMenuCompanion#RightCompanion > Windows.UI.Xaml.Controls.Grid#CompanionRoot > Windows.UI.Xaml.Controls.Border#AcrylicBorder", kProps218, 5 },
    { Host::StartMenu, L"StartDocked.SearchBoxToggleButton#StartMenuSearchBox > Windows.UI.Xaml.Controls.Grid > Windows.UI.Xaml.Controls.Border#BorderElement", kProps219, 5 },
    { Host::StartMenu, L"Cortana.UI.Views.TaskbarSearchPage > Windows.UI.Xaml.Controls.Grid#RootGrid > Windows.UI.Xaml.Controls.Grid#OuterBorderGrid", kProps220, 5 },
    { Host::StartMenu, L"Windows.UI.Xaml.Controls.Border#LayerBorder", kProps221, 2 },
    { Host::StartMenu, L"Windows.UI.Xaml.Controls.Border#AccentLayerBorder", kProps222, 2 },
    { Host::StartMenu, L"Windows.UI.Xaml.Controls.Border#AcrylicBorder", kProps223, 5 },
    { Host::StartMenu, L"Windows.UI.Xaml.Controls.Grid#CompanionRoot", kProps225, 2 },
    { Host::StartMenu, L"Cortana.UI.Views.TaskbarSearchPage > Windows.UI.Xaml.Controls.Grid#RootGrid", kProps226, 3 },
    { Host::StartMenu, L"Windows.UI.Xaml.Controls.Grid#RootGrid@SearchBoxLocationStates", kProps227, 2 },
    { Host::StartMenu, L"Windows.UI.Xaml.Controls.Border#TaskbarSearchBackground", kProps228, 5 },
    // Do not restyle every Grid under TaskbarSearchPage. Matching is descendant,
    // not parent, so a bare "> Grid" rule paints HostedWebView2Control hosts and
    // SearchHost shows an error when results load. Chrome stays on named roots.
    { Host::Search, L"Cortana.UI.Views.TaskbarSearchPage", kProps229, 3 },
    { Host::Search, L"Cortana.UI.Views.TaskbarSearchPage > Windows.UI.Xaml.Controls.Grid#RootGrid", kProps231, 2 },
    { Host::Search, L"Cortana.UI.Views.TaskbarSearchPage > Windows.UI.Xaml.Controls.Grid#RootGrid@SearchBoxLocationStates", kProps232, 1 },
    { Host::Search, L"Windows.UI.Xaml.Controls.Border#TaskbarSearchBackground", kProps233, 4 },
    { Host::Search, L"Cortana.UI.Views.TaskbarSearchPage > Windows.UI.Xaml.Controls.Grid#RootGrid > Windows.UI.Xaml.Controls.Grid#OuterBorderGrid", kProps234, 4 },
    { Host::Search, L"Cortana.UI.Views.TaskbarSearchPage > Windows.UI.Xaml.Controls.AutoSuggestBox > Windows.UI.Xaml.Controls.Grid > Windows.UI.Xaml.Controls.Border", kProps235, 1 },
    { Host::Search, L"Cortana.UI.Views.TaskbarSearchPage > Windows.UI.Xaml.Controls.TextBox > Windows.UI.Xaml.Controls.Grid > Windows.UI.Xaml.Controls.Border", kProps236, 1 },
    { Host::Search, L"Cortana.UI.Views.TaskbarSearchPage > Windows.UI.Xaml.Controls.Border#BorderElement", kProps237, 1 },
    { Host::ShellFlyout, L"Grid#NotificationCenterGrid", kProps238, 5 },
    { Host::ShellFlyout, L"Grid#CalendarCenterGrid", kProps239, 5 },
    { Host::ShellFlyout, L"Windows.UI.Xaml.Controls.Grid#ControlCenterRegion", kProps240, 5 },
    { Host::ShellFlyout, L"Border#ToastBackgroundBorder", kProps241, 5 },
};
inline constexpr int kRuleCount = static_cast<int>(sizeof(kRules) / sizeof(kRules[0]));

}  // namespace MaxwellRules
