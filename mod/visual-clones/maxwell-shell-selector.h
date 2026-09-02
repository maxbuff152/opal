// =====================================================================
//  maxwell-shell-selector.h
//
//  Parses and matches the selector syntax the Opal shell profile uses.
//
//  This is deliberately NOT a general CSS engine. It implements exactly the
//  four features the live profile contains and nothing else:
//
//     Type#Name                    type and/or x:Name
//     A > B > C                    direct parent chain, up to 6 deep
//     Grid#ContainerGrid@Common    a visual state group to listen on
//     Button[Prop=Value]           one attribute predicate
//
//  Alternation (",") is resolved by the generator, which splits a multi-target
//  rule into separate single-path rules, so the runtime never sees a comma.
//
//  Matching runs right-to-left: the rightmost segment is tested against the
//  element that just appeared, then we walk up its parents. That order matters -
//  the rightmost segment is the most specific, so a non-match rejects the rule
//  immediately without ever touching the tree.
//
//  Written fresh for maxwell-shell.
// =====================================================================
#pragma once

#include <string>
#include <string_view>
#include <vector>

namespace MaxwellShell {

struct Segment {
    std::wstring type;        // empty = any
    std::wstring name;        // empty = any
    std::wstring stateGroup;  // empty = none
    std::wstring attrName;    // empty = no predicate
    std::wstring attrValue;
    bool         isRoot = false;  // ":root" - an anchor, not a type
};

// Splits on a delimiter that is not inside [ ] - the one attribute predicate in
// the profile contains a '.', and future ones could contain a '>'.
inline std::vector<std::wstring> SplitTopLevel(std::wstring_view s, wchar_t delim) {
    std::vector<std::wstring> out;
    int depth = 0;
    std::wstring cur;
    for (wchar_t c : s) {
        if (c == L'[') { ++depth; }
        else if (c == L']') { if (depth > 0) { --depth; } }
        if (c == delim && depth == 0) { out.push_back(cur); cur.clear(); continue; }
        cur.push_back(c);
    }
    out.push_back(cur);
    return out;
}

inline std::wstring Trim(std::wstring_view s) {
    size_t a = s.find_first_not_of(L" \t");
    if (a == std::wstring_view::npos) { return L""; }
    size_t b = s.find_last_not_of(L" \t");
    return std::wstring(s.substr(a, b - a + 1));
}

// Type#Name@StateGroup[Attr=Value] - every part optional except that at least
// one of type or name must be present, otherwise the segment matches anything
// and the rule would apply to the entire tree.
inline Segment ParseSegment(std::wstring_view raw) {
    Segment seg;
    std::wstring s = Trim(raw);

    // ":root" anchors the chain at the visual root. It is not a type name, and
    // treating it as one makes the whole selector unmatchable - which quietly
    // cost the taskbar its ColumnDefinitions rule, and with it the floating
    // dock. Matched as a wildcard: the segments to its right are specific
    // enough on their own.
    if (s == L":root") {
        seg.isRoot = true;
        return seg;
    }

    // attribute predicate first: it can contain # and @ inside the brackets
    const size_t lb = s.find(L'[');
    if (lb != std::wstring::npos) {
        const size_t rb = s.find(L']', lb);
        if (rb != std::wstring::npos) {
            const std::wstring inner = s.substr(lb + 1, rb - lb - 1);
            const size_t eq = inner.find(L'=');
            if (eq != std::wstring::npos) {
                seg.attrName  = Trim(inner.substr(0, eq));
                seg.attrValue = Trim(inner.substr(eq + 1));
            }
            s = s.substr(0, lb) + s.substr(rb + 1);
        }
    }

    const size_t at = s.find(L'@');
    if (at != std::wstring::npos) {
        seg.stateGroup = Trim(s.substr(at + 1));
        s = s.substr(0, at);
    }

    const size_t hash = s.find(L'#');
    if (hash != std::wstring::npos) {
        seg.name = Trim(s.substr(hash + 1));
        s = s.substr(0, hash);
    }

    seg.type = Trim(s);
    return seg;
}

inline std::vector<Segment> ParseSelector(std::wstring_view selector) {
    std::vector<Segment> out;
    for (const auto& part : SplitTopLevel(selector, L'>')) {
        const std::wstring t = Trim(part);
        if (!t.empty()) { out.push_back(ParseSegment(t)); }
    }
    return out;
}

// A XAML type may be reported fully qualified. The profile writes both forms -
// "Grid" and "Windows.UI.Xaml.Controls.Grid" refer to the same thing - so a bare
// selector type matches the last dotted component of the element's type.
inline bool TypeMatches(std::wstring_view selectorType, std::wstring_view elementType) {
    if (selectorType.empty()) { return true; }
    if (selectorType == elementType) { return true; }
    if (selectorType.find(L'.') != std::wstring_view::npos) { return false; }

    const size_t dot = elementType.find_last_of(L'.');
    if (dot == std::wstring_view::npos) { return false; }
    return elementType.substr(dot + 1) == selectorType;
}

}  // namespace MaxwellShell
