#pragma once
#include <cwctype>
#include <string>
#include <vector>

namespace auto_menu_ui_logic {

struct Candidate {
    std::wstring name;
    std::wstring text;
    std::wstring descendants;
    std::wstring ancestors;
};

enum class SelectionKind {
    None,
    Unique,
    Ambiguous,
};

struct Selection {
    SelectionKind kind = SelectionKind::None;
    int index = -1;
};

inline wchar_t FoldVietnamese(wchar_t c) {
    c = static_cast<wchar_t>(std::towlower(c));
    switch (c) {
        case L'à': case L'á': case L'ả': case L'ã': case L'ạ':
        case L'ă': case L'ằ': case L'ắ': case L'ẳ': case L'ẵ': case L'ặ':
        case L'â': case L'ầ': case L'ấ': case L'ẩ': case L'ẫ': case L'ậ': return L'a';
        case L'đ': return L'd';
        case L'è': case L'é': case L'ẻ': case L'ẽ': case L'ẹ':
        case L'ê': case L'ề': case L'ế': case L'ể': case L'ễ': case L'ệ': return L'e';
        case L'ì': case L'í': case L'ỉ': case L'ĩ': case L'ị': return L'i';
        case L'ò': case L'ó': case L'ỏ': case L'õ': case L'ọ':
        case L'ô': case L'ồ': case L'ố': case L'ổ': case L'ỗ': case L'ộ':
        case L'ơ': case L'ờ': case L'ớ': case L'ở': case L'ỡ': case L'ợ': return L'o';
        case L'ù': case L'ú': case L'ủ': case L'ũ': case L'ụ':
        case L'ư': case L'ừ': case L'ứ': case L'ử': case L'ữ': case L'ự': return L'u';
        case L'ỳ': case L'ý': case L'ỷ': case L'ỹ': case L'ỵ': return L'y';
        default: return c;
    }
}

inline std::wstring Key(const std::wstring& value) {
    std::wstring out;
    out.reserve(value.size());
    for (wchar_t c : value) {
        c = FoldVietnamese(c);
        if (std::iswalnum(c)) out.push_back(c);
    }
    return out;
}

inline bool InAutoFightUi(const Candidate& candidate) {
    return Key(candidate.ancestors).find(L"autofightui") != std::wstring::npos;
}

inline bool ContainsExactSegment(const std::wstring& value, const wchar_t* key) {
    std::size_t start = 0;
    while (start <= value.size()) {
        const std::size_t end = value.find(L'/', start);
        const std::wstring segment = value.substr(start, end == std::wstring::npos ? std::wstring::npos : end - start);
        if (Key(segment) == key) return true;
        if (end == std::wstring::npos) break;
        start = end + 1;
    }
    return false;
}

inline bool ExactLabel(const Candidate& candidate, const wchar_t* key) {
    return Key(candidate.text) == key || ContainsExactSegment(candidate.descendants, key);
}

inline Selection SelectByPredicate(const std::vector<Candidate>& candidates,
                                   bool (*predicate)(const Candidate&)) {
    Selection result{};
    int count = 0;
    for (std::size_t i = 0; i < candidates.size(); ++i) {
        if (!predicate(candidates[i])) continue;
        ++count;
        if (count == 1) {
            result.kind = SelectionKind::Unique;
            result.index = static_cast<int>(i);
        } else {
            result.kind = SelectionKind::Ambiguous;
            result.index = -1;
        }
    }
    return result;
}

inline bool IsHudAuto(const Candidate& candidate) {
    if (InAutoFightUi(candidate)) return false;
    return ExactLabel(candidate, L"auto");
}

inline bool IsSettingsChoice(const Candidate& candidate) {
    if (InAutoFightUi(candidate)) return false;
    return ExactLabel(candidate, L"thietlap");
}

inline bool IsAutoFightPanelProof(const Candidate& candidate) {
    return Key(candidate.name) == L"togglepickuptab" && InAutoFightUi(candidate);
}

inline Selection SelectHudAuto(const std::vector<Candidate>& candidates) {
    return SelectByPredicate(candidates, IsHudAuto);
}

inline Selection SelectSettingsChoice(const std::vector<Candidate>& candidates) {
    return SelectByPredicate(candidates, IsSettingsChoice);
}

inline Selection SelectAutoFightPanelProof(const std::vector<Candidate>& candidates) {
    return SelectByPredicate(candidates, IsAutoFightPanelProof);
}

} // namespace auto_menu_ui_logic
