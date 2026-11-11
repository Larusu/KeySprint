#pragma once

#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

using WordBank = std::unordered_map<std::string, std::vector<std::string>>;

WordBank load_word_bank(const std::string& path);
const std::vector<std::string>& get_word_list(
    const WordBank& bank, std::string_view level);
