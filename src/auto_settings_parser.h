#pragma once
#include <string>
#include <vector>

namespace autosettings {

enum class GroupKind { AutoTrain, PickItem, Utilities, Regene, Pet, AutoPk, FuBen };
enum class FieldType { Text, Boolean, Number };

struct Field {
    std::wstring name;
    std::wstring raw;
    FieldType type = FieldType::Text;
};

struct Group {
    GroupKind kind{};
    std::wstring name;
    std::wstring raw;
    std::vector<Field> fields;
    std::vector<std::wstring> extras;
    bool truncated = false;
};

struct Document {
    std::wstring raw;
    std::wstring version;
    bool versionMatches = false;
    bool schemaMismatch = false;
    std::vector<Group> groups;
    std::vector<std::wstring> topLevelExtras;
};

Document Parse(const std::wstring& raw);
const Group* FindGroup(const Document& document, GroupKind kind);
std::wstring DisplayValue(const Field& field);
const wchar_t* GroupDisplayName(GroupKind kind);

} // namespace autosettings
