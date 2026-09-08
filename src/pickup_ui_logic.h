#pragma once
#include <cwctype>
#include <string>
#include <vector>

namespace pickup_ui_logic {

struct Candidate {
    std::wstring name;
    std::wstring text;
    std::wstring descendants;
    std::wstring ancestors;
    int selected = -1;
};

enum class SelectionKind {
    None,
    Unique,
    Ambiguous,
};

struct Selection {
    SelectionKind kind = SelectionKind::None;
    int index = -1;
    int score = 0;
};

inline std::wstring CompactAsciiKey(const std::wstring& value) {
    std::wstring out;
    out.reserve(value.size());
    for (wchar_t ch : value) {
        if (ch == L'_' || ch == L'-' || ch == L' ' || ch == L'/' || ch == L'.') continue;
        if (ch >= L'A' && ch <= L'Z') ch = static_cast<wchar_t>(ch - L'A' + L'a');
        out.push_back(ch);
    }
    return out;
}

inline bool HasPickupInternalToken(const std::wstring& value) {
    const std::wstring key = CompactAsciiKey(value);
    return key.find(L"pickitem") != std::wstring::npos ||
           key.find(L"pickup") != std::wstring::npos;
}

inline bool HasVietnamesePickupLabel(const std::wstring& value) {
    return value.find(L"Nhặt vật phẩm") != std::wstring::npos ||
           value.find(L"nhặt vật phẩm") != std::wstring::npos;
}

inline int ScorePickupCandidate(const Candidate& candidate) {
    int score = 0;
    if (HasVietnamesePickupLabel(candidate.text)) score += 140;
    if (HasVietnamesePickupLabel(candidate.descendants)) score += 120;
    if (HasPickupInternalToken(candidate.name)) score += 100;
    if (HasPickupInternalToken(candidate.text)) score += 80;
    if (HasPickupInternalToken(candidate.descendants)) score += 60;
    // Ancestor PickUp/AutoFight context is corroborating evidence only. It must
    // never make an unrelated toggle a candidate by itself.
    if (score > 0 && HasPickupInternalToken(candidate.ancestors)) score += 20;
    return score;
}

inline Selection SelectPickupToggle(const std::vector<Candidate>& candidates) {
    Selection result{};
    int positiveCount = 0;
    for (std::size_t i = 0; i < candidates.size(); ++i) {
        const int score = ScorePickupCandidate(candidates[i]);
        if (score <= 0) continue;
        ++positiveCount;
        if (positiveCount == 1) {
            result.kind = SelectionKind::Unique;
            result.index = static_cast<int>(i);
            result.score = score;
        } else {
            result.kind = SelectionKind::Ambiguous;
            result.index = -1;
            if (score > result.score) result.score = score;
        }
    }
    if (positiveCount == 0) result = {};
    return result;
}

} // namespace pickup_ui_logic
