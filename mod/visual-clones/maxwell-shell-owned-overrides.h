#pragma once

// Final Opal taskbar invariants. These owned rules intentionally run after the
// generated compatibility profile so a stale Windows grouping preference or
// an inherited styler rule cannot bring back labels, outlines, or button
// capsules. The icons retain a quiet luminance hover and the running indicator
// remains the only persistent state marker.
namespace MaxwellOwnedOverrides {

using Host = MaxwellRules::Host;
using Prop = MaxwellRules::Prop;
using Rule = MaxwellRules::Rule;

inline constexpr Prop kIconOnlyButton[] = {
    { L"Width", nullptr, L"50", false },
    { L"MinWidth", nullptr, L"50", false },
    { L"MaxWidth", nullptr, L"50", false },
    { L"Padding", nullptr, L"0", false },
    { L"Clip", nullptr, L"None", false },
};

inline constexpr Prop kIconOnlyPanel[] = {
    { L"Width", nullptr, L"50", false },
    { L"MinWidth", nullptr, L"50", false },
    { L"MaxWidth", nullptr, L"50", false },
    { L"Padding", nullptr, L"0", false },
    { L"Margin", nullptr, L"0", false },
    { L"HorizontalAlignment", nullptr, L"Center", false },
    { L"Clip", nullptr, L"None", false },
};

inline constexpr Prop kHideAppLabel[] = {
    { L"Visibility", nullptr, L"Collapsed", false },
    { L"Width", nullptr, L"0", false },
    { L"MinWidth", nullptr, L"0", false },
    { L"MaxWidth", nullptr, L"0", false },
    { L"Margin", nullptr, L"0", false },
    { L"Opacity", nullptr, L"0", false },
};

inline constexpr Prop kBorderlessRunningSurface[] = {
    { L"CornerRadius", nullptr, L"4", false },
    // Keep the template's background element in the tree for stable layout,
    // but make the element itself non-rendering. Windows can introduce new or
    // restore-default visual states after our state setters run; zero opacity
    // is the fail-closed invariant that prevents any of them repainting a box.
    { L"Opacity", nullptr, L"0", false },
    { L"BorderThickness", nullptr, L"0", false },
    { L"BorderBrush", nullptr, L"Transparent", false },
    { L"Background", nullptr, L"Transparent", false },
    { L"Background", L"NoRunningIndicator", L"Transparent", false },
    { L"Background", L"InactiveRunningIndicator", L"Transparent", false },
    { L"Background", L"InactiveRunningIndicatorPointerOver", L"<SolidColorBrush Color=\"#FFFFFF\" Opacity=\"0.055\" />", true },
    { L"Background", L"ActiveRunningIndicator", L"Transparent", false },
    { L"Background", L"ActiveRunningIndicatorPointerOver", L"<SolidColorBrush Color=\"#FFFFFF\" Opacity=\"0.075\" />", true },
    { L"Background", L"RequestingAttentionRunningIndicator", L"Transparent", false },
};

inline constexpr Prop kBorderlessCommonSurface[] = {
    { L"CornerRadius", nullptr, L"4", false },
    { L"Opacity", nullptr, L"0", false },
    { L"BorderThickness", nullptr, L"0", false },
    { L"BorderBrush", nullptr, L"Transparent", false },
    { L"Background", nullptr, L"Transparent", false },
    { L"Background", L"InactiveNormal", L"Transparent", false },
    { L"Background", L"ActiveNormal", L"Transparent", false },
    { L"Background", L"InactivePointerOver", L"<SolidColorBrush Color=\"#FFFFFF\" Opacity=\"0.055\" />", true },
    { L"Background", L"ActivePointerOver", L"<SolidColorBrush Color=\"#FFFFFF\" Opacity=\"0.075\" />", true },
    { L"Background", L"InactivePressed", L"<SolidColorBrush Color=\"#FFFFFF\" Opacity=\"0.10\" />", true },
    { L"Background", L"ActivePressed", L"<SolidColorBrush Color=\"#FFFFFF\" Opacity=\"0.10\" />", true },
};

inline constexpr Prop kBorderlessTraySurface[] = {
    { L"CornerRadius", nullptr, L"4", false },
    { L"Opacity", nullptr, L"0", false },
    { L"BorderThickness", nullptr, L"0", false },
    { L"BorderBrush", nullptr, L"Transparent", false },
    { L"BorderBrush", L"PointerOver", L"Transparent", false },
    { L"BorderBrush", L"Pressed", L"Transparent", false },
    { L"Background", L"Normal", L"Transparent", false },
    { L"Background", L"PointerOver", L"<SolidColorBrush Color=\"#FFFFFF\" Opacity=\"0.055\" />", true },
    { L"Background", L"Pressed", L"<SolidColorBrush Color=\"#FFFFFF\" Opacity=\"0.10\" />", true },
};

inline constexpr Prop kBorderlessContainer[] = {
    { L"BorderThickness", nullptr, L"0", false },
    { L"BorderBrush", nullptr, L"Transparent", false },
};

inline constexpr Prop kFloatingTaskbarRoot[] = {
    // The app lane is a first-class 50-DIP capsule, matching the tray, clock,
    // Media, and Computer stats instead of leaving its icons ungrounded.
    // Shared 10-DIP island grid: 10 outer, 14 inner on the icon lane so the
    // 25-DIP pill radius cannot clip the first/last glyphs.
    { L"Background", nullptr, L"$OpalTaskbarSurface", true },
    { L"BorderThickness", nullptr, L"0", false },
    { L"BorderBrush", nullptr, L"Transparent", false },
    { L"Margin", nullptr, L"10,9,10,9", false },
    { L"Padding", nullptr, L"14,0,14,0", false },
    { L"CornerRadius", nullptr, L"25", false },
    { L"Clip", nullptr, L"None", false },
};

inline constexpr Prop kHiddenTaskbarBackground[] = {
    // The stock taskbar background control paints a fill and a 1-DIP top
    // stroke. The imported profile hides both through one exact template path
    // (TaskbarBackground > Grid > Rectangle); the 26200 template inserts layers
    // that path does not name, so the stroke survived as a hairline across the
    // top of the dock. Match the rectangles by name alone.
    { L"Visibility", nullptr, L"Collapsed", false },
    { L"Opacity", nullptr, L"0", false },
};

inline constexpr Prop kFrostedTraySurface[] = {
    // Fixed neutral frost: enough tint for white clock/tray glyph contrast on
    // a bright wallpaper, while the compositor blur and luminosity lift keep
    // the surface visibly glass instead of an opaque black capsule.
    { L"Background", nullptr, L"<WindhawkBlur BlurAmount=\"28\" TintColor=\"#16181D\" TintOpacity=\"0.58\" TintLuminosityOpacity=\"0.20\" TintSaturation=\"0.0\" NoiseOpacity=\"0.010\" FallbackColor=\"#D916181D\" />", true },
    { L"BorderThickness", nullptr, L"0", false },
    { L"BorderBrush", nullptr, L"Transparent", false },
    // Keep the native tray measure contract intact. Nine-DIP vertical margins
    // inside Opal's 68-DIP taskbar produce the same 50-DIP visible envelope
    // without a competing Height constraint or a XAML measure loop. Horizontal
    // inset matches the app lane so the floating islands share one grid.
    { L"Margin", nullptr, L"10,9,10,9", false },
    { L"Padding", nullptr, L"10,0,10,0", false },
    { L"CornerRadius", nullptr, L"25", false },
};

inline constexpr Prop kFrostedHardwareSurface[] = {
    { L"Background", nullptr, L"<WindhawkBlur BlurAmount=\"28\" TintColor=\"#16181D\" TintOpacity=\"0.58\" TintLuminosityOpacity=\"0.20\" TintSaturation=\"0.0\" NoiseOpacity=\"0.010\" FallbackColor=\"#D916181D\" />", true },
    { L"BorderThickness", nullptr, L"0", false },
    { L"BorderBrush", nullptr, L"Transparent", false },
    { L"Height", nullptr, L"50", false },
    { L"VerticalAlignment", nullptr, L"Center", false },
    { L"CornerRadius", nullptr, L"25", false },
};

inline constexpr Prop kFrostedMediaSurface[] = {
    { L"Background", nullptr, L"<WindhawkBlur BlurAmount=\"28\" TintColor=\"#16181D\" TintOpacity=\"0.58\" TintLuminosityOpacity=\"0.20\" TintSaturation=\"0.0\" NoiseOpacity=\"0.010\" FallbackColor=\"#D916181D\" />", true },
    { L"BorderThickness", nullptr, L"0", false },
    { L"BorderBrush", nullptr, L"Transparent", false },
    { L"Height", nullptr, L"50", false },
    { L"VerticalAlignment", nullptr, L"Center", false },
    { L"CornerRadius", nullptr, L"25", false },
};

inline constexpr Prop kInvisibleButtonChrome[] = {
    { L"Opacity", nullptr, L"0", false },
    { L"Background", nullptr, L"Transparent", false },
    { L"BorderThickness", nullptr, L"0", false },
    { L"BorderBrush", nullptr, L"Transparent", false },
};

inline constexpr Prop kClockContainer[] = {
    // The tray owns the capsule margin. Repeating it here shrinks the clock's
    // content box and clips the second line on a 50-DIP taskbar surface.
    { L"Height", nullptr, L"50", false },
    { L"MinWidth", nullptr, L"168", false },
    { L"Padding", nullptr, L"10,0,10,0", false },
    { L"Margin", nullptr, L"0", false },
    { L"RenderTransform", nullptr, L"<TranslateTransform X=\"0\" Y=\"0\" />", true },
};

inline constexpr Prop kClockTime[] = {
    { L"Visibility", nullptr, L"Visible", false },
    { L"FontFamily", nullptr, L"Segoe UI Variable Display", false },
    { L"FontWeight", nullptr, L"SemiBold", false },
    { L"FontSize", nullptr, L"21", false },
    { L"Margin", nullptr, L"0", false },
    { L"Padding", nullptr, L"0", false },
    { L"RenderTransform", nullptr, L"<TranslateTransform X=\"0\" Y=\"0\" />", true },
};

inline constexpr Prop kClockDateWeather[] = {
    // Fail visible: stale imported profiles must never collapse the date line.
    { L"Visibility", nullptr, L"Visible", false },
    { L"FontFamily", nullptr, L"Segoe UI Variable Text", false },
    { L"FontWeight", nullptr, L"Medium", false },
    { L"FontSize", nullptr, L"11", false },
    { L"Foreground", nullptr, L"<SolidColorBrush Color=\"#B8D1D1D6\" />", true },
    { L"Margin", nullptr, L"0", false },
    { L"Padding", nullptr, L"0", false },
    { L"RenderTransform", nullptr, L"<TranslateTransform X=\"0\" Y=\"0\" />", true },
};

inline constexpr Prop kOverlayIcon[] = {
    { L"Width", nullptr, L"16", false },
    { L"Height", nullptr, L"16", false },
    { L"MaxWidth", nullptr, L"16", false },
    { L"MaxHeight", nullptr, L"16", false },
    { L"MinWidth", nullptr, L"16", false },
    { L"MinHeight", nullptr, L"16", false },
    { L"HorizontalAlignment", nullptr, L"Right", false },
    { L"VerticalAlignment", nullptr, L"Bottom", false },
    { L"Margin", nullptr, L"0,0,2,2", false },
    { L"Canvas.ZIndex", nullptr, L"6", false },
    { L"Clip", nullptr, L"None", false },
    { L"Stretch", nullptr, L"Uniform", false },
};

inline constexpr Prop kAppBadge[] = {
    { L"MinWidth", nullptr, L"16", false },
    { L"Width", nullptr, L"Auto", false },
    { L"Height", nullptr, L"16", false },
    { L"MinHeight", nullptr, L"16", false },
    { L"Padding", nullptr, L"3,0,3,0", false },
    { L"Margin", nullptr, L"0,1,1,0", false },
    { L"CornerRadius", nullptr, L"8", false },
    { L"HorizontalAlignment", nullptr, L"Right", false },
    { L"VerticalAlignment", nullptr, L"Top", false },
    { L"Canvas.ZIndex", nullptr, L"7", false },
    { L"Clip", nullptr, L"None", false },
    { L"Background", nullptr, L"<SolidColorBrush Color=\"#16181D\" Opacity=\"0.96\" />", true },
    { L"Foreground", nullptr, L"<SolidColorBrush Color=\"#F5F5F7\" />", true },
    { L"BorderThickness", nullptr, L"0", false },
};

inline constexpr Prop kAppBadgeText[] = {
    { L"FontFamily", nullptr, L"Segoe UI Variable Text", false },
    { L"FontSize", nullptr, L"10", false },
    { L"FontWeight", nullptr, L"SemiBold", false },
    { L"Padding", nullptr, L"0", false },
    { L"Margin", nullptr, L"0", false },
};

inline constexpr Rule kRules[] = {
    { Host::Explorer, L"Taskbar.TaskListButton#TaskListButton", kIconOnlyButton, static_cast<int>(std::size(kIconOnlyButton)) },
    { Host::Explorer, L"Taskbar.TaskListButton#TaskListButton > Taskbar.TaskListLabeledButtonPanel#IconPanel", kIconOnlyPanel, static_cast<int>(std::size(kIconOnlyPanel)) },
    { Host::Explorer, L"Taskbar.TaskListLabeledButtonPanel > TextBlock#LabelControl", kHideAppLabel, static_cast<int>(std::size(kHideAppLabel)) },
    { Host::Explorer, L"Taskbar.TaskListLabeledButtonPanel@CommonStates > TextBlock#LabelControl", kHideAppLabel, static_cast<int>(std::size(kHideAppLabel)) },
    // State-independent selectors are deliberate. They cover stock, active,
    // grouped, and future taskbar visual-state names before the scoped rules
    // below add their compatibility values.
    { Host::Explorer, L"Grid#IconPanel > Border#BackgroundElement", kInvisibleButtonChrome, static_cast<int>(std::size(kInvisibleButtonChrome)) },
    { Host::Explorer, L"Taskbar.TaskListLabeledButtonPanel#IconPanel > Border#BackgroundElement", kInvisibleButtonChrome, static_cast<int>(std::size(kInvisibleButtonChrome)) },
    { Host::Explorer, L"Taskbar.TaskListButtonPanel > Border#BackgroundElement", kInvisibleButtonChrome, static_cast<int>(std::size(kInvisibleButtonChrome)) },
    { Host::Explorer, L"Grid#IconPanel@RunningIndicatorStates > Border#BackgroundElement", kBorderlessRunningSurface, static_cast<int>(std::size(kBorderlessRunningSurface)) },
    { Host::Explorer, L"Taskbar.TaskListLabeledButtonPanel@RunningIndicatorStates > Border#BackgroundElement", kBorderlessRunningSurface, static_cast<int>(std::size(kBorderlessRunningSurface)) },
    { Host::Explorer, L"Grid#IconPanel@CommonStates > Border#BackgroundElement", kBorderlessCommonSurface, static_cast<int>(std::size(kBorderlessCommonSurface)) },
    { Host::Explorer, L"Taskbar.TaskListLabeledButtonPanel@CommonStates > Border#BackgroundElement", kBorderlessCommonSurface, static_cast<int>(std::size(kBorderlessCommonSurface)) },
    { Host::Explorer, L"Taskbar.TaskListButtonPanel#ExperienceToggleButtonRootPanel@CommonStates > Border#BackgroundElement", kBorderlessCommonSurface, static_cast<int>(std::size(kBorderlessCommonSurface)) },
    { Host::Explorer, L"Taskbar.TaskListButtonPanel@CommonStates > Border#BackgroundElement", kBorderlessCommonSurface, static_cast<int>(std::size(kBorderlessCommonSurface)) },
    { Host::Explorer, L"Taskbar.TaskbarFrame > Grid#RootGrid", kFloatingTaskbarRoot, static_cast<int>(std::size(kFloatingTaskbarRoot)) },
    { Host::Explorer, L"Rectangle#BackgroundStroke", kHiddenTaskbarBackground, static_cast<int>(std::size(kHiddenTaskbarBackground)) },
    { Host::Explorer, L"Rectangle#BackgroundFill", kHiddenTaskbarBackground, static_cast<int>(std::size(kHiddenTaskbarBackground)) },
    { Host::Explorer, L"Taskbar.AugmentedEntryPointButton#AugmentedEntryPointButton > Taskbar.TaskListButtonPanel#ExperienceToggleButtonRootPanel", kBorderlessContainer, static_cast<int>(std::size(kBorderlessContainer)) },
    { Host::Explorer, L"StackPanel#SystemTrayFrameGrid", kFrostedTraySurface, static_cast<int>(std::size(kFrostedTraySurface)) },
    { Host::Explorer, L"Grid#SystemTrayFrameGrid", kFrostedTraySurface, static_cast<int>(std::size(kFrostedTraySurface)) },
    { Host::Explorer, L"Grid#OpalSystemInfo", kFrostedHardwareSurface, static_cast<int>(std::size(kFrostedHardwareSurface)) },
    { Host::Explorer, L"Grid#OpalSystemInfoMirror", kFrostedHardwareSurface, static_cast<int>(std::size(kFrostedHardwareSurface)) },
    { Host::Explorer, L"Grid#WindhawkTaskbarSystemInfo", kFrostedHardwareSurface, static_cast<int>(std::size(kFrostedHardwareSurface)) },
    { Host::Explorer, L"Grid#WindhawkTaskbarSystemInfoMirror", kFrostedHardwareSurface, static_cast<int>(std::size(kFrostedHardwareSurface)) },
    { Host::Explorer, L"Border#OpalMediaGlass", kFrostedMediaSurface, static_cast<int>(std::size(kFrostedMediaSurface)) },
    { Host::Explorer, L"SystemTray.DateTimeIconContent > Grid#ContainerGrid", kClockContainer, static_cast<int>(std::size(kClockContainer)) },
    { Host::Explorer, L"TextBlock#TimeInnerTextBlock", kClockTime, static_cast<int>(std::size(kClockTime)) },
    { Host::Explorer, L"TextBlock#DateInnerTextBlock", kClockDateWeather, static_cast<int>(std::size(kClockDateWeather)) },
    { Host::Explorer, L"SystemTray.ChevronIconView > Grid#ContainerGrid@CommonStates > Border#BackgroundBorder", kBorderlessTraySurface, static_cast<int>(std::size(kBorderlessTraySurface)) },
    { Host::Explorer, L"SystemTray.NotifyIconView > Grid#ContainerGrid@CommonStates > Border#BackgroundBorder", kBorderlessTraySurface, static_cast<int>(std::size(kBorderlessTraySurface)) },
    { Host::Explorer, L"SystemTray.IconView#SystemTrayIcon > Grid#ContainerGrid@CommonStates > Border#BackgroundBorder", kBorderlessTraySurface, static_cast<int>(std::size(kBorderlessTraySurface)) },
    { Host::Explorer, L"SystemTray.OmniButton > Grid@CommonStates > Border#BackgroundBorder", kBorderlessTraySurface, static_cast<int>(std::size(kBorderlessTraySurface)) },
    { Host::Explorer, L"Grid#IconPanel > Image#OverlayIcon", kOverlayIcon, static_cast<int>(std::size(kOverlayIcon)) },
    { Host::Explorer, L"Taskbar.TaskListLabeledButtonPanel#IconPanel > Image#OverlayIcon", kOverlayIcon, static_cast<int>(std::size(kOverlayIcon)) },
    { Host::Explorer, L"Grid#IconPanel > Windows.UI.Xaml.Controls.Image#OverlayIcon", kOverlayIcon, static_cast<int>(std::size(kOverlayIcon)) },
    { Host::Explorer, L"Taskbar.TaskListLabeledButtonPanel#IconPanel > Windows.UI.Xaml.Controls.Image#OverlayIcon", kOverlayIcon, static_cast<int>(std::size(kOverlayIcon)) },
    { Host::Explorer, L"Grid#IconPanel > Taskbar.Badge#BadgeControl", kAppBadge, static_cast<int>(std::size(kAppBadge)) },
    { Host::Explorer, L"Taskbar.TaskListLabeledButtonPanel#IconPanel > Taskbar.Badge#BadgeControl", kAppBadge, static_cast<int>(std::size(kAppBadge)) },
    { Host::Explorer, L"Grid#IconPanel > Taskbar.Badge#BadgeControl > Grid > TextBlock#BadgeText", kAppBadgeText, static_cast<int>(std::size(kAppBadgeText)) },
    { Host::Explorer, L"Taskbar.TaskListLabeledButtonPanel#IconPanel > Taskbar.Badge#BadgeControl > Grid > TextBlock#BadgeText", kAppBadgeText, static_cast<int>(std::size(kAppBadgeText)) },
};

inline constexpr int kRuleCount = static_cast<int>(std::size(kRules));

}  // namespace MaxwellOwnedOverrides
