// =====================================================================
//  maxwell-shell-apply.h
//
//  Sets XAML properties by name.
//
//  There is no generic "set property called X" in C++/WinRT - no reflection,
//  and DependencyProperty lookup by string is not exposed. So every property
//  the profile uses gets an explicit, typed setter. That is 24 branches, and
//  it is the honest way to do it: a missing branch is a compile-visible gap
//  rather than a silent no-op.
//
//  The set below is exactly what the generated rule table contains. If a new
//  property appears in the profile, ApplyProperty returns false for it and
//  the caller logs it, so it surfaces instead of quietly doing nothing - the
//  failure mode that cost the floating taskbar the first time round.
//
//  Written fresh for maxwell-shell.
// =====================================================================
#pragma once

#include <string>
#include <string_view>
#include <vector>

#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.UI.Text.h>
#include <winrt/Windows.UI.Xaml.h>
#include <winrt/Windows.UI.Xaml.Controls.h>
#include <winrt/Windows.UI.Xaml.Markup.h>
#include <winrt/Windows.UI.Xaml.Media.h>
#include <winrt/Windows.UI.Xaml.Shapes.h>

namespace MaxwellShell {

namespace wux  = winrt::Windows::UI::Xaml;
namespace wuxc = winrt::Windows::UI::Xaml::Controls;
namespace wuxm = winrt::Windows::UI::Xaml::Media;
namespace wuxs = winrt::Windows::UI::Xaml::Shapes;

// Diagnostic hand-off: what columns the matched Grid already had, and whether
// we are permitted to rewrite them.
inline std::wstring g_lastGridColumns;
inline bool         g_applyColumnDefinitions = false;

// ---------------------------------------------------------------------
//  Value parsing
// ---------------------------------------------------------------------
inline std::vector<double> ParseNumbers(std::wstring_view s) {
    std::vector<double> out;
    std::wstring cur;
    for (wchar_t c : s) {
        if (c == L',' || c == L' ') {
            if (!cur.empty()) { out.push_back(_wtof(cur.c_str())); cur.clear(); }
        } else {
            cur.push_back(c);
        }
    }
    if (!cur.empty()) { out.push_back(_wtof(cur.c_str())); }
    return out;
}

// "8" -> uniform, "14,0,0,0" -> per-side, "8,4" -> horizontal/vertical.
inline wux::Thickness ParseThickness(std::wstring_view s) {
    const auto n = ParseNumbers(s);
    if (n.size() == 1) { return {n[0], n[0], n[0], n[0]}; }
    if (n.size() == 2) { return {n[0], n[1], n[0], n[1]}; }
    if (n.size() >= 4) { return {n[0], n[1], n[2], n[3]}; }
    return {0, 0, 0, 0};
}

inline wux::CornerRadius ParseCornerRadius(std::wstring_view s) {
    const auto n = ParseNumbers(s);
    if (n.size() == 1) { return {n[0], n[0], n[0], n[0]}; }
    if (n.size() >= 4) { return {n[0], n[1], n[2], n[3]}; }
    return {0, 0, 0, 0};
}

inline bool ParseBool(std::wstring_view s) {
    return s == L"True" || s == L"true" || s == L"1";
}

inline winrt::Windows::UI::Text::FontWeight ParseFontWeight(std::wstring_view s) {
    using namespace winrt::Windows::UI::Text;
    if (s == L"Thin")       { return FontWeights::Thin(); }
    if (s == L"ExtraLight") { return FontWeights::ExtraLight(); }
    if (s == L"Light")      { return FontWeights::Light(); }
    if (s == L"SemiLight")  { return FontWeights::SemiLight(); }
    if (s == L"Normal")     { return FontWeights::Normal(); }
    if (s == L"Medium")     { return FontWeights::Medium(); }
    if (s == L"SemiBold")   { return FontWeights::SemiBold(); }
    if (s == L"Bold")       { return FontWeights::Bold(); }
    if (s == L"ExtraBold")  { return FontWeights::ExtraBold(); }
    if (s == L"Black")      { return FontWeights::Black(); }
    return FontWeights::Normal();
}

// XamlReader::Load needs the root element to declare the XAML namespace. The
// profile's fragments never do - they were written for a styler that adds it -
// so every brush and transform fails to parse without this. The failure is
// quiet: Load throws, we catch, and the property is simply never set.
inline std::wstring WithXmlns(std::wstring_view fragment) {
    constexpr std::wstring_view kNs =
        L" xmlns=\"http://schemas.microsoft.com/winfx/2006/xaml/presentation\"";

    if (fragment.find(L"xmlns") != std::wstring_view::npos) { return std::wstring(fragment); }

    const size_t lt = fragment.find(L'<');
    if (lt == std::wstring_view::npos) { return std::wstring(fragment); }

    // Insert immediately after the root element's name.
    size_t i = lt + 1;
    while (i < fragment.size() && (iswalnum(fragment[i]) || fragment[i] == L'.' || fragment[i] == L':')) { ++i; }

    std::wstring out(fragment.substr(0, i));
    out += kNs;
    out += fragment.substr(i);
    return out;
}

// "#RRGGBB" or "#AARRGGBB".
inline bool ParseColor(std::wstring_view s, winrt::Windows::UI::Color& out) {
    if (s.size() < 7 || s[0] != L'#') { return false; }
    auto hex = [](std::wstring_view v) -> uint8_t {
        return static_cast<uint8_t>(wcstoul(std::wstring(v).c_str(), nullptr, 16));
    };
    if (s.size() >= 9) {
        out = {hex(s.substr(1, 2)), hex(s.substr(3, 2)), hex(s.substr(5, 2)), hex(s.substr(7, 2))};
    } else {
        out = {255, hex(s.substr(1, 2)), hex(s.substr(3, 2)), hex(s.substr(5, 2))};
    }
    return true;
}

inline std::wstring AttrValue(std::wstring_view xml, std::wstring_view attr) {
    std::wstring needle(attr);
    needle += L"=\"";
    const size_t a = xml.find(needle);
    if (a == std::wstring_view::npos) { return L""; }
    const size_t b = a + needle.size();
    const size_t e = xml.find(L'"', b);
    if (e == std::wstring_view::npos) { return L""; }
    return std::wstring(xml.substr(b, e - b));
}

// <WindhawkBlur ... /> is NOT XAML. It is the styler profile's own shorthand for
// "give me an acrylic brush with these parameters", and XamlReader will never
// parse it. It is translated here into a real AcrylicBrush.
//
// TintColor may be a {ThemeResource ...} reference rather than a literal; in
// that case FallbackColor is used, which is exactly what it is there for.
inline wuxm::Brush ParseWindhawkBlur(std::wstring_view value) {
    winrt::Windows::UI::Color tint{};
    if (!ParseColor(AttrValue(value, L"TintColor"), tint)) {
        if (!ParseColor(AttrValue(value, L"FallbackColor"), tint)) {
            tint = {255, 18, 19, 24};   // the profile's own fallback shade
        }
    }

    const std::wstring opacity    = AttrValue(value, L"TintOpacity");
    const std::wstring luminosity = AttrValue(value, L"TintLuminosityOpacity");

    try {
        wuxm::AcrylicBrush brush;
        brush.TintColor(tint);
        if (!opacity.empty())    { brush.TintOpacity(_wtof(opacity.c_str())); }
        if (!luminosity.empty()) { brush.TintLuminosityOpacity(_wtof(luminosity.c_str())); }

        winrt::Windows::UI::Color fallback{};
        if (ParseColor(AttrValue(value, L"FallbackColor"), fallback)) { brush.FallbackColor(fallback); }
        else { brush.FallbackColor(tint); }
        return brush;
    } catch (...) {
        // Acrylic is unavailable in some hosts / with transparency disabled.
        // A flat tint is far better than no background at all.
        return wuxm::SolidColorBrush(tint);
    }
}

// Parses a brush value. "Transparent" is spelled as a bare word in the profile,
// so it is special-cased rather than round-tripped through the parser.
inline wuxm::Brush ParseBrush(std::wstring_view value) {
    if (value == L"Transparent") {
        return wuxm::SolidColorBrush(winrt::Windows::UI::Colors::Transparent());
    }
    if (value.find(L"WindhawkBlur") != std::wstring_view::npos) {
        return ParseWindhawkBlur(value);
    }
    try {
        return wux::Markup::XamlReader::Load(WithXmlns(value)).try_as<wuxm::Brush>();
    } catch (...) {
        return nullptr;
    }
}

template <typename T>
inline T ParseXaml(std::wstring_view value) {
    try {
        return wux::Markup::XamlReader::Load(WithXmlns(value)).try_as<T>();
    } catch (...) {
        return nullptr;
    }
}

// ---------------------------------------------------------------------
//  Property application
//
//  Returns false when the property name is not one we know how to set, so the
//  caller can log a gap rather than silently skipping it.
// ---------------------------------------------------------------------
inline bool ApplyProperty(const wux::DependencyObject& obj,
                          std::wstring_view name,
                          std::wstring_view value) {
    auto fe      = obj.try_as<wux::FrameworkElement>();
    auto ui      = obj.try_as<wux::UIElement>();
    auto border  = obj.try_as<wuxc::Border>();
    auto panel   = obj.try_as<wuxc::Panel>();
    auto control = obj.try_as<wuxc::Control>();
    auto shape   = obj.try_as<wuxs::Shape>();
    auto text    = obj.try_as<wuxc::TextBlock>();
    auto image   = obj.try_as<wuxc::Image>();

    // Grid is a Panel, but since Windows 10 1809 it ALSO has its own
    // BorderBrush / BorderThickness / CornerRadius / Padding. Treating it as
    // "just a Panel" makes every border and corner rule on Grid#RootGrid fail,
    // which is most of the taskbar's shape.
    auto grid = obj.try_as<wuxc::Grid>();

    // ContentPresenter grew the same chrome properties in 1809. Tooltips
    // template their entire visible surface on one (ToolTip >
    // ContentPresenter#LayoutRoot), so without this branch every tooltip
    // background/border/corner rule fails.
    auto presenter = obj.try_as<wuxc::ContentPresenter>();

    // ---- brushes ------------------------------------------------------
    if (name == L"Background") {
        auto b = ParseBrush(value);
        if (!b) { return false; }
        if (border)    { border.Background(b);    return true; }
        if (panel)     { panel.Background(b);     return true; }
        if (control)   { control.Background(b);   return true; }
        if (shape)     { shape.Fill(b);           return true; }
        if (presenter) { presenter.Background(b); return true; }
        return false;
    }
    if (name == L"Fill") {
        auto b = ParseBrush(value);
        if (shape && b) { shape.Fill(b); return true; }
        return false;
    }
    if (name == L"BorderBrush") {
        auto b = ParseBrush(value);
        if (!b) { return false; }
        if (border)    { border.BorderBrush(b);    return true; }
        if (grid)      { grid.BorderBrush(b);      return true; }
        if (control)   { control.BorderBrush(b);   return true; }
        if (shape)     { shape.Stroke(b);          return true; }
        if (presenter) { presenter.BorderBrush(b); return true; }
        return false;
    }
    if (name == L"Foreground") {
        auto b = ParseBrush(value);
        if (!b) { return false; }
        if (text)    { text.Foreground(b);    return true; }
        if (control) { control.Foreground(b); return true; }
        return false;
    }

    // ---- box model ----------------------------------------------------
    if (name == L"BorderThickness") {
        const auto t = ParseThickness(value);
        if (border)    { border.BorderThickness(t);    return true; }
        if (grid)      { grid.BorderThickness(t);      return true; }
        if (control)   { control.BorderThickness(t);   return true; }
        if (presenter) { presenter.BorderThickness(t); return true; }
        return false;
    }
    if (name == L"CornerRadius") {
        const auto r = ParseCornerRadius(value);
        if (border)    { border.CornerRadius(r);    return true; }
        if (grid)      { grid.CornerRadius(r);      return true; }
        if (control)   { control.CornerRadius(r);   return true; }
        if (presenter) { presenter.CornerRadius(r); return true; }
        return false;
    }
    if (name == L"Padding") {
        const auto t = ParseThickness(value);
        if (border)    { border.Padding(t);    return true; }
        if (grid)      { grid.Padding(t);      return true; }
        if (control)   { control.Padding(t);   return true; }
        if (text)      { text.Padding(t);      return true; }
        if (presenter) { presenter.Padding(t); return true; }
        return false;
    }
    if (name == L"Margin")    { if (fe) { fe.Margin(ParseThickness(value)); return true; } return false; }

    // ---- sizing -------------------------------------------------------
    if (name == L"Height")    { if (fe) { fe.Height(_wtof(std::wstring(value).c_str()));    return true; } return false; }
    if (name == L"MinWidth")  { if (fe) { fe.MinWidth(_wtof(std::wstring(value).c_str()));  return true; } return false; }
    if (name == L"MaxWidth")  { if (fe) { fe.MaxWidth(_wtof(std::wstring(value).c_str()));  return true; } return false; }
    if (name == L"MaxHeight") { if (fe) { fe.MaxHeight(_wtof(std::wstring(value).c_str())); return true; } return false; }
    if (name == L"Width") {
        if (!fe) { return false; }
        // "Auto" is spelled out in the profile; NaN is how XAML says auto.
        if (value == L"Auto") { fe.Width(std::numeric_limits<double>::quiet_NaN()); return true; }
        fe.Width(_wtof(std::wstring(value).c_str()));
        return true;
    }

    // ---- shape corners ------------------------------------------------
    if (name == L"RadiusX" || name == L"RadiusY") {
        auto rect = obj.try_as<wuxs::Rectangle>();
        if (!rect) { return false; }
        const double v = _wtof(std::wstring(value).c_str());
        if (name == L"RadiusX") { rect.RadiusX(v); } else { rect.RadiusY(v); }
        return true;
    }

    // ---- text ---------------------------------------------------------
    if (name == L"FontSize") {
        const double v = _wtof(std::wstring(value).c_str());
        if (text)    { text.FontSize(v);    return true; }
        if (control) { control.FontSize(v); return true; }
        return false;
    }
    if (name == L"FontWeight") {
        const auto w = ParseFontWeight(value);
        if (text)    { text.FontWeight(w);    return true; }
        if (control) { control.FontWeight(w); return true; }
        return false;
    }
    if (name == L"FontFamily") {
        wuxm::FontFamily ff{std::wstring(value)};
        if (text)    { text.FontFamily(ff);    return true; }
        if (control) { control.FontFamily(ff); return true; }
        return false;
    }

    // ---- visual -------------------------------------------------------
    if (name == L"Visibility") {
        if (!ui) { return false; }
        ui.Visibility(value == L"Collapsed" ? wux::Visibility::Collapsed : wux::Visibility::Visible);
        return true;
    }
    if (name == L"UseLayoutRounding") { if (ui) { ui.UseLayoutRounding(ParseBool(value)); return true; } return false; }
    if (name == L"RenderTransform") {
        auto t = ParseXaml<wuxm::Transform>(value);
        if (ui && t) { ui.RenderTransform(t); return true; }
        return false;
    }
    if (name == L"RenderTransformOrigin") {
        const auto n = ParseNumbers(value);
        if (ui && n.size() >= 2) {
            ui.RenderTransformOrigin({static_cast<float>(n[0]), static_cast<float>(n[1])});
            return true;
        }
        return false;
    }
    if (name == L"Stretch") {
        if (!image) { return false; }
        if (value == L"None")           { image.Stretch(wuxm::Stretch::None); }
        else if (value == L"Fill")      { image.Stretch(wuxm::Stretch::Fill); }
        else if (value == L"UniformToFill") { image.Stretch(wuxm::Stretch::UniformToFill); }
        else                            { image.Stretch(wuxm::Stretch::Uniform); }
        return true;
    }

    // ---- alignment ----------------------------------------------------
    if (name == L"HorizontalAlignment") {
        if (!fe) { return false; }
        if (value == L"Left")        { fe.HorizontalAlignment(wux::HorizontalAlignment::Left); }
        else if (value == L"Center") { fe.HorizontalAlignment(wux::HorizontalAlignment::Center); }
        else if (value == L"Right")  { fe.HorizontalAlignment(wux::HorizontalAlignment::Right); }
        else                         { fe.HorizontalAlignment(wux::HorizontalAlignment::Stretch); }
        return true;
    }
    if (name == L"VerticalAlignment") {
        if (!fe) { return false; }
        if (value == L"Top")         { fe.VerticalAlignment(wux::VerticalAlignment::Top); }
        else if (value == L"Center") { fe.VerticalAlignment(wux::VerticalAlignment::Center); }
        else if (value == L"Bottom") { fe.VerticalAlignment(wux::VerticalAlignment::Bottom); }
        else                         { fe.VerticalAlignment(wux::VerticalAlignment::Stretch); }
        return true;
    }
    if (name == L"MinHeight") { if (fe) { fe.MinHeight(_wtof(std::wstring(value).c_str())); return true; } return false; }

    // ---- text content -------------------------------------------------
    if (name == L"Text")          { if (text) { text.Text(std::wstring(value)); return true; } return false; }
    if (name == L"TextAlignment") {
        if (!text) { return false; }
        if (value == L"Center")      { text.TextAlignment(wux::TextAlignment::Center); }
        else if (value == L"Right")  { text.TextAlignment(wux::TextAlignment::Right); }
        else if (value == L"Justify"){ text.TextAlignment(wux::TextAlignment::Justify); }
        else                         { text.TextAlignment(wux::TextAlignment::Left); }
        return true;
    }
    if (name == L"AccessKey") { if (ui) { ui.AccessKey(std::wstring(value)); return true; } return false; }

    // ---- attached -----------------------------------------------------
    if (name == L"Canvas.ZIndex") {
        if (!ui) { return false; }
        wuxc::Canvas::SetZIndex(ui, static_cast<int32_t>(_wtof(std::wstring(value).c_str())));
        return true;
    }
    if (name == L"Grid.Row") {
        if (!fe) { return false; }
        wuxc::Grid::SetRow(fe, static_cast<int32_t>(_wtof(std::wstring(value).c_str())));
        return true;
    }
    if (name == L"Grid.Column") {
        if (!fe) { return false; }
        wuxc::Grid::SetColumn(fe, static_cast<int32_t>(_wtof(std::wstring(value).c_str())));
        return true;
    }

    // ActualWidth is read-only by definition - it is a measured result, not a
    // setting. It appears in the profile as an expression input, never as a
    // target, so accept and ignore rather than logging a false gap.
    if (name == L"ActualWidth") { return true; }

    // ---- grid layout --------------------------------------------------
    //
    // ColumnDefinitions restructures a Grid rather than setting a simple value,
    // and XamlReader cannot hand back a ColumnDefinitionCollection, so the
    // widths are parsed out and the collection is rebuilt by hand.
    //
    // This one rule is what makes the taskbar float. It gives the container
    // columns [*, Auto, Auto, *]: the dock and tray size to their content and
    // the star columns either side absorb the remaining width. Without it the
    // dock's column is star-sized and stretches edge to edge no matter what
    // Width=Auto says on the frame itself.
    if (name == L"ColumnDefinitions") {
        if (!grid) { return false; }

        std::vector<wux::GridLength> widths;
        size_t pos = 0;
        while ((pos = value.find(L"<ColumnDefinition", pos)) != std::wstring_view::npos) {
            const size_t end = value.find(L'>', pos);
            if (end == std::wstring_view::npos) { break; }

            const std::wstring w = AttrValue(value.substr(pos, end - pos), L"Width");
            wux::GridLength gl{};
            if (w == L"Auto") {
                gl.GridUnitType = wux::GridUnitType::Auto;
                gl.Value = 1.0;
            } else if (w.empty() || w == L"*") {
                gl.GridUnitType = wux::GridUnitType::Star;
                gl.Value = 1.0;
            } else if (w.back() == L'*') {
                gl.GridUnitType = wux::GridUnitType::Star;
                gl.Value = _wtof(w.substr(0, w.size() - 1).c_str());
                if (gl.Value <= 0) { gl.Value = 1.0; }
            } else {
                gl.GridUnitType = wux::GridUnitType::Pixel;
                gl.Value = _wtof(w.c_str());
            }
            widths.push_back(gl);
            pos = end + 1;
        }
        if (widths.empty()) { return false; }

        // Update widths IN PLACE rather than Clear() + rebuild.
        //
        // Clearing the collection detaches columns that existing children are
        // already assigned to via Grid.Column. Those children end up referring
        // to slots that no longer mean what they did, and the taskbar's app
        // buttons collapse - measured: 33 items became 16, and every pinned icon
        // vanished. Existing ColumnDefinition objects are therefore reused, and
        // the collection is only grown if the profile asks for more columns than
        // the Grid currently has. It is never shrunk.
        // DIAGNOSTIC: report the Grid's existing columns before touching them.
        try {
            auto existing = grid.ColumnDefinitions();
            std::wstring desc;
            for (uint32_t i = 0; i < existing.Size(); ++i) {
                const auto gl = existing.GetAt(i).Width();
                wchar_t b[32] = {};
                if (gl.GridUnitType == wux::GridUnitType::Auto)      { swprintf_s(b, L"Auto"); }
                else if (gl.GridUnitType == wux::GridUnitType::Star) { swprintf_s(b, L"%.0f*", gl.Value); }
                else                                                 { swprintf_s(b, L"%.0fpx", gl.Value); }
                if (i) { desc += L","; }
                desc += b;
            }
            g_lastGridColumns = L"[" + desc + L"] count=" + std::to_wstring(existing.Size());
        } catch (...) { g_lastGridColumns = L"(unreadable)"; }

        if (!g_applyColumnDefinitions) { return false; }

        try {
            auto columns = grid.ColumnDefinitions();

            for (uint32_t i = 0; i < widths.size(); ++i) {
                if (i < columns.Size()) {
                    columns.GetAt(i).Width(widths[i]);      // reuse
                } else {
                    wuxc::ColumnDefinition cd;
                    cd.Width(widths[i]);
                    columns.Append(cd);                     // grow only
                }
            }
            return true;
        } catch (...) {
            return false;
        }
    }

    return false;
}

}  // namespace MaxwellShell
