#include "Identifier.h"

std::unordered_map<std::string, std::pair<std::string, size_t>> Identifier::interned_names_with_hash;

void Identifier::update(const std::string &name)
{
  if (name.empty()) {
    static std::string empty_name;
    static size_t empty_hash = std::hash<std::string>{}(empty_name);

    interned_name_ptr = &empty_name;
    hash = empty_hash;
    return;
  }

  is_config_variable_ = name[0] == '$' && name != "$children";

  auto it = interned_names_with_hash.find(name);
  if (it == interned_names_with_hash.end()) {
    it = interned_names_with_hash.emplace(name, std::make_pair(name, std::hash<std::string>{}(name))).first;
  }

  interned_name_ptr = &it->second.first;
  hash = it->second.second;
}