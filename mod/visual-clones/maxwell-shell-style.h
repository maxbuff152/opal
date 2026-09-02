// =====================================================================
//  maxwell-shell-style.h
//
//  Applies properties by letting XAML resolve them, instead of by hand.
//
//  WHY THIS REPLACED 24 HAND-WRITTEN SETTERS
//
//  The first applier had an explicit typed branch per property - Background,
//  Margin, CornerRadius and so on - because C++/WinRT exposes no way to look a
//  DependencyProperty up by name. That works until it doesn't:
//
//    * every value needed its own parser (Thickness, CornerRadius, GridLength,
//      FontWeight, brushes), and each parser was a chance to be subtly wrong;
//    * ColumnDefinitions cannot be expressed that way at all - mutating a Grid's
//      live collection collapsed the taskbar's pinned buttons, measured at 33
//      items dropping to 16;
//    * any property the profile gained later silently did nothing.
//
//  XAML already solves all of this. Wrapping the setters in a Style inside a
//  ResourceDictionary and parsing it makes the XAML parser resolve each property
//  name to a real DependencyProperty and convert each value string to the right
//  type. Reading Setter.Property() and Setter.Value() back out then gives a
//  typed pair that can go straight to SetValue.
//
//  This is the mechanism the Windhawk styler uses, reimplemented here. The
//  approach - build a Style from markup, read its setters - is a documented XAML
//  pattern, not a copied implementation.
//
//  Written fresh for maxwell-shell.
// =====================================================================
#pragma once

#include <string>
#include <string_view>
#include <vector>

#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.UI.Xaml.h>
#include <winrt/Windows.UI.Xaml.Controls.h>
#include <winrt/Windows.UI.Xaml.Markup.h>
#include <winrt/Windows.UI.Xaml.Media.h>

namespace MaxwellShell {

namespace wux  = winrt::Windows::UI::Xaml;
namespace wuxc = winrt::Windows::UI::Xaml::Controls;
namespace wuxm = winrt::Windows::UI::Xaml::Media;

// XML attribute values must be escaped or a colour like "#08090D" inside a
// nested brush breaks the surrounding document.
inline std::wstring EscapeXmlAttribute(std::wstring_view s) {
    std::wstring out;
    out.reserve(s.size() + 16);
    for (wchar_t c : s) {
        switch (c) {
            case L'&':  out += L"&amp;";  break;
            case L'"':  out += L"&quot;"; break;
            case L'<':  out += L"&lt;";   break;
            case L'>':  out += L"&gt;";   break;
            default:    out += c;         break;
        }
    }
    return out;
}

// One property assignment, already constant-resolved.
struct StyleSetter {
    std::wstring property;
    std::wstring value;
    bool         isXaml = false;   // ':=' - value is markup, not a literal
};

// Builds <ResourceDictionary><Style TargetType="type">...</Style></...> and
// returns the parsed Style. Throws if XAML cannot resolve the type.
inline wux::Style BuildStyle(std::wstring_view type,
                             const std::vector<StyleSetter>& setters) {
    std::wstring xaml =
        LR"(<ResourceDictionary
    xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation"
    xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml"
    xmlns:muxc="using:Microsoft.UI.Xaml.Controls")";

    // A dotted type name is a namespace + type, which needs its own xmlns.
    const size_t dot = type.rfind(L'.');
    if (dot != std::wstring_view::npos) {
        xaml += L"\n    xmlns:mshell=\"using:";
        xaml += EscapeXmlAttribute(type.substr(0, dot));
        xaml += L"\">\n    <Style TargetType=\"mshell:";
        xaml += EscapeXmlAttribute(type.substr(dot + 1));
        xaml += L"\">\n";
    } else {
        xaml += L">\n    <Style TargetType=\"";
        xaml += EscapeXmlAttribute(type);
        xaml += L"\">\n";
    }

    for (const auto& s : setters) {
        xaml += L"        <Setter Property=\"";
        xaml += EscapeXmlAttribute(s.property);
        xaml += L"\"";
        if (s.isXaml) {
            // Markup values become element content so the parser builds the
            // object graph - brushes, transforms, definition collections.
            xaml += L">\n            <Setter.Value>\n";
            xaml += s.value;
            xaml += L"\n            </Setter.Value>\n        </Setter>\n";
        } else {
            xaml += L" Value=\"";
            xaml += EscapeXmlAttribute(s.value);
            xaml += L"\" />\n";
        }
    }

    xaml += L"    </Style>\n</ResourceDictionary>";

    auto dictionary = wux::Markup::XamlReader::Load(xaml).as<wux::ResourceDictionary>();
    auto [key, value] = dictionary.First().Current();
    return value.as<wux::Style>();
}

// Types like Taskbar.TaskbarFrame are not resolvable as a XAML TargetType. The
// fallback still resolves every property the base type declares, which is most
// of the profile.
inline wux::Style BuildStyleWithFallback(std::wstring_view type,
                                         std::wstring_view fallbackType,
                                         const std::vector<StyleSetter>& setters) {
    try {
        return BuildStyle(type, setters);
    } catch (...) {
        if (fallbackType.empty() || fallbackType == type) { return nullptr; }
        try { return BuildStyle(fallbackType, setters); }
        catch (...) { return nullptr; }
    }
}

// Definition collections hold DependencyObjects that the layout engine writes
// measured sizes back into. A Style is parsed once and reused, so handing the
// same collection to two Grids - two taskbars on one UI thread - would let one
// monitor's column sizes leak into the other's. Each element gets its own copy.
inline winrt::Windows::Foundation::IInspectable ClonePerElement(
        const winrt::Windows::Foundation::IInspectable& value) {
    if (auto columns = value.try_as<wuxc::ColumnDefinitionCollection>()) {
        wuxc::Grid owner;
        auto copy = owner.ColumnDefinitions();
        for (auto const& c : columns) {
            wuxc::ColumnDefinition d;
            d.Width(c.Width());
            d.MinWidth(c.MinWidth());
            d.MaxWidth(c.MaxWidth());
            copy.Append(d);
        }
        return copy;
    }
    if (auto rows = value.try_as<wuxc::RowDefinitionCollection>()) {
        wuxc::Grid owner;
        auto copy = owner.RowDefinitions();
        for (auto const& r : rows) {
            wuxc::RowDefinition d;
            d.Height(r.Height());
            d.MinHeight(r.MinHeight());
            d.MaxHeight(r.MaxHeight());
            copy.Append(d);
        }
        return copy;
    }
    return value;
}

// Applies a parsed Style's setters to one element. Returns how many landed.
inline int ApplyStyle(const wux::DependencyObject& element, const wux::Style& style) {
    if (!element || !style) { return 0; }

    int applied = 0;
    for (auto const& base : style.Setters()) {
        auto setter = base.try_as<wux::Setter>();
        if (!setter) { continue; }

        auto property = setter.Property();
        if (!property) { continue; }

        try {
            auto value = ClonePerElement(setter.Value());

            // FontWeight comes back boxed as int, and SetValue rejects it with
            // "no such interface". Re-box it as the struct XAML expects.
            if (property == wuxc::TextBlock::FontWeightProperty() ||
                property == wuxc::Control::FontWeightProperty() ||
                property == wuxc::ContentPresenter::FontWeightProperty()) {
                if (auto asInt = value.try_as<int>()) {
                    value = winrt::box_value(winrt::Windows::UI::Text::FontWeight{
                        static_cast<uint16_t>(*asInt)});
                }
            }

            element.SetValue(property, value);
            ++applied;
        } catch (...) {
            // One bad setter must not abandon the rest of the rule.
        }
    }
    return applied;
}

}  // namespace MaxwellShell
