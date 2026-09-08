#include "auto_settings_parser.h"
#include <array>

namespace autosettings {
namespace {

struct Def { const wchar_t* name; FieldType type; };

std::vector<std::wstring> Split(const std::wstring& s, wchar_t delim) {
    std::vector<std::wstring> out;
    std::size_t start = 0;
    for (;;) {
        const auto pos = s.find(delim, start);
        if (pos == std::wstring::npos) {
            out.emplace_back(s.substr(start));
            break;
        }
        out.emplace_back(s.substr(start, pos - start));
        start = pos + 1;
    }
    return out;
}

template <std::size_t N>
Group ParseFixed(GroupKind kind, const wchar_t* displayName, const std::wstring& raw,
                 const Def (&defs)[N], bool& mismatch) {
    Group g;
    g.kind = kind;
    g.name = displayName;
    g.raw = raw;
    const auto values = Split(raw, L'|');
    const std::size_t consumed = values.size() < N ? values.size() : N;
    for (std::size_t i = 0; i < consumed; ++i)
        g.fields.push_back({defs[i].name, values[i], defs[i].type});
    if (values.size() < N) {
        g.truncated = true;
        mismatch = true;
    }
    if (values.size() > N) {
        g.extras.assign(values.begin() + static_cast<std::ptrdiff_t>(N), values.end());
        mismatch = true;
    }
    return g;
}

constexpr Def kAutoTrain[] = {
    {L"IsAttackMonsterInList", FieldType::Boolean}, {L"AttackMonsterList", FieldType::Text},
    {L"IsLureModel", FieldType::Boolean}, {L"IsTrainInRanger", FieldType::Boolean},
    {L"RangerScan", FieldType::Number}, {L"AutoTrainSkillList", FieldType::Text},
    {L"UsingCombo", FieldType::Boolean}, {L"UsingF1Key", FieldType::Boolean},
    {L"GiveUpMonsterOutRanger", FieldType::Boolean},
};
constexpr Def kPickItem[] = {
    {L"IsOn", FieldType::Boolean}, {L"PickRanger", FieldType::Number},
    {L"IsFilterItem", FieldType::Boolean}, {L"FilterItemSettings", FieldType::Text},
    {L"AutoEatX2", FieldType::Boolean}, {L"AutoUsingItem", FieldType::Boolean},
    {L"UsingItemList", FieldType::Text}, {L"IsAutoDropItem", FieldType::Boolean},
    {L"DropItemSettings", FieldType::Text},
};
constexpr Def kUtilities[] = {
    {L"AutoAcceptInviteTeam", FieldType::Boolean}, {L"AutoRejectInviteTeam", FieldType::Boolean},
    {L"AutoRejectTrade", FieldType::Boolean}, {L"AutoLevelUp", FieldType::Boolean},
    {L"LevelUpSet", FieldType::Text}, {L"IsAutoBuff", FieldType::Boolean},
    {L"AutoBuffSkillList", FieldType::Text}, {L"ChatSelect", FieldType::Text},
    {L"ChatCostumeChannel", FieldType::Text}, {L"ChatSelectSend", FieldType::Boolean},
    {L"AutoRejectEmoji", FieldType::Boolean}, {L"AutoRejectMount", FieldType::Boolean},
    {L"RejectJoinGuild", FieldType::Boolean}, {L"RejectJoinAllies", FieldType::Boolean},
};
constexpr Def kRegene[] = {
    {L"AutoRegenHP", FieldType::Boolean}, {L"AutoRegenHPPercent", FieldType::Number},
    {L"AutoRegenMP", FieldType::Boolean}, {L"AutoRegenMPPercent", FieldType::Number},
    {L"AutoComeback", FieldType::Boolean}, {L"AutoRevival", FieldType::Boolean},
    {L"HPItemRegen", FieldType::Text}, {L"MPItemRegen", FieldType::Text},
    {L"IsNgaMy", FieldType::Boolean}, {L"NgaMyPercent", FieldType::Number},
    {L"PhatQuangPhoChieu", FieldType::Boolean}, {L"ThanhTamPhoThienChu", FieldType::Boolean},
    {L"KimChamDoKiep", FieldType::Boolean}, {L"CaiTuHoanSinh", FieldType::Boolean},
};
constexpr Def kPet[] = {
    {L"IsAutoCallPet", FieldType::Boolean}, {L"PetIDSelect", FieldType::Text},
    {L"BloodSacrifice", FieldType::Boolean}, {L"BloodSacrificeValue", FieldType::Number},
    {L"Dedication", FieldType::Boolean}, {L"DedicationValue", FieldType::Number},
    {L"AutoSkillPet", FieldType::Boolean}, {L"AutoCallBackPet", FieldType::Boolean},
    {L"CallBackPetNumber", FieldType::Number}, {L"AutoEat", FieldType::Boolean},
    {L"HpPercent", FieldType::Number}, {L"AutoInjoy", FieldType::Boolean},
    {L"InjoyValue", FieldType::Number}, {L"AttackModel", FieldType::Number},
    {L"IsAutoCallSprit", FieldType::Boolean}, {L"SpritIDSelect", FieldType::Text},
};
constexpr Def kAutoPk[] = {
    {L"AutoPkAgian", FieldType::Boolean}, {L"IsLowHpTarget", FieldType::Boolean},
    {L"IsFactionTarget", FieldType::Boolean}, {L"FactionID", FieldType::Text},
    {L"SkillPK", FieldType::Text}, {L"UsingCombo", FieldType::Boolean},
};
constexpr Def kFuBenBase[] = {
    {L"SelectedFuBen", FieldType::Text}, {L"AutoRepeat", FieldType::Boolean},
    {L"RepeatCount", FieldType::Number}, {L"FollowLeader", FieldType::Boolean},
    {L"AcceptFuBenInvite", FieldType::Boolean}, {L"AutoInviteMembers", FieldType::Boolean},
    {L"AutoRevive", FieldType::Boolean}, {L"DesiredMembers", FieldType::Number},
    {L"MinInviteLevel", FieldType::Number}, {L"ScheduleEnabled", FieldType::Boolean},
};

Group ParseFuBen(const std::wstring& raw, bool& mismatch) {
    Group g;
    g.kind = GroupKind::FuBen;
    g.name = L"Phụ bản / FUBEN";
    g.raw = raw;
    const auto values = Split(raw, L'|');
    constexpr std::size_t baseN = sizeof(kFuBenBase) / sizeof(kFuBenBase[0]);
    const std::size_t baseConsumed = values.size() < baseN ? values.size() : baseN;
    for (std::size_t i = 0; i < baseConsumed; ++i)
        g.fields.push_back({kFuBenBase[i].name, values[i], kFuBenBase[i].type});
    if (values.size() < baseN) {
        g.truncated = true;
        mismatch = true;
        return g;
    }
    std::size_t index = baseN;
    for (int schedule = 1; schedule <= 8 && index < values.size(); ++schedule) {
        const std::wstring prefix = L"Schedule" + std::to_wstring(schedule) + L".";
        static const Def sdefs[] = {
            {L"Enabled", FieldType::Boolean}, {L"Hour", FieldType::Number},
            {L"Minute", FieldType::Number}, {L"FuBen", FieldType::Text},
        };
        const std::size_t remaining = values.size() - index;
        const std::size_t take = remaining < 4 ? remaining : 4;
        for (std::size_t j = 0; j < take; ++j)
            g.fields.push_back({prefix + sdefs[j].name, values[index + j], sdefs[j].type});
        index += take;
        if (take < 4) {
            g.truncated = true;
            mismatch = true;
            break;
        }
    }
    if (index < values.size()) {
        g.extras.assign(values.begin() + static_cast<std::ptrdiff_t>(index), values.end());
        mismatch = true;
    }
    return g;
}

} // namespace

const wchar_t* GroupDisplayName(GroupKind kind) {
    switch (kind) {
        case GroupKind::AutoTrain: return L"Đánh quái / AUTOTRAIN";
        case GroupKind::PickItem: return L"Nhặt đồ / PICKITEM";
        case GroupKind::Utilities: return L"Tiện ích / UTILITIES";
        case GroupKind::Regene: return L"Hồi phục / REGENE";
        case GroupKind::Pet: return L"Pet / PET";
        case GroupKind::AutoPk: return L"PK / AUTOPK";
        case GroupKind::FuBen: return L"Phụ bản / FUBEN";
    }
    return L"?";
}

Document Parse(const std::wstring& raw) {
    Document d;
    d.raw = raw;
    const auto top = Split(raw, L'#');
    if (top.empty()) {
        d.schemaMismatch = true;
        return d;
    }
    d.version = top[0];
    d.versionMatches = d.version == L"4.1";
    if (!d.versionMatches) d.schemaMismatch = true;

    const std::array<GroupKind, 7> kinds = {GroupKind::AutoTrain, GroupKind::PickItem, GroupKind::Utilities,
        GroupKind::Regene, GroupKind::Pet, GroupKind::AutoPk, GroupKind::FuBen};
    for (std::size_t i = 0; i < kinds.size(); ++i) {
        const std::wstring segment = (i + 1 < top.size()) ? top[i + 1] : L"";
        if (i + 1 >= top.size()) d.schemaMismatch = true;
        switch (kinds[i]) {
            case GroupKind::AutoTrain: d.groups.push_back(ParseFixed(kinds[i], GroupDisplayName(kinds[i]), segment, kAutoTrain, d.schemaMismatch)); break;
            case GroupKind::PickItem: d.groups.push_back(ParseFixed(kinds[i], GroupDisplayName(kinds[i]), segment, kPickItem, d.schemaMismatch)); break;
            case GroupKind::Utilities: d.groups.push_back(ParseFixed(kinds[i], GroupDisplayName(kinds[i]), segment, kUtilities, d.schemaMismatch)); break;
            case GroupKind::Regene: d.groups.push_back(ParseFixed(kinds[i], GroupDisplayName(kinds[i]), segment, kRegene, d.schemaMismatch)); break;
            case GroupKind::Pet: d.groups.push_back(ParseFixed(kinds[i], GroupDisplayName(kinds[i]), segment, kPet, d.schemaMismatch)); break;
            case GroupKind::AutoPk: d.groups.push_back(ParseFixed(kinds[i], GroupDisplayName(kinds[i]), segment, kAutoPk, d.schemaMismatch)); break;
            case GroupKind::FuBen: d.groups.push_back(ParseFuBen(segment, d.schemaMismatch)); break;
        }
    }
    if (top.size() > 8) {
        d.topLevelExtras.assign(top.begin() + 8, top.end());
        d.schemaMismatch = true;
    }
    return d;
}

const Group* FindGroup(const Document& document, GroupKind kind) {
    for (const auto& group : document.groups)
        if (group.kind == kind) return &group;
    return nullptr;
}

std::wstring DisplayValue(const Field& field) {
    if (field.type != FieldType::Boolean) return field.raw;
    if (field.raw == L"1") return L"ON";
    if (field.raw == L"0") return L"OFF";
    return field.raw + L" (BOOL?)";
}

} // namespace autosettings
