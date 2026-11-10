#pragma once

#include <cstddef>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

enum class Difficulty { English1k, English5k, English10k, English25k };

constexpr std::string_view difficulty_key(Difficulty difficulty)
{
    switch (difficulty)
    {
        case Difficulty::English1k:  return "english_1k";
        case Difficulty::English5k:  return "english_5k";
        case Difficulty::English10k: return "english_10k";
        case Difficulty::English25k: return "english_25k";
    }
    throw std::invalid_argument("Unknown difficulty");
}

constexpr std::string_view to_string(Difficulty difficulty)
{
    switch (difficulty)
    {
        case Difficulty::English1k:  return "English 1k";
        case Difficulty::English5k:  return "English 5k";
        case Difficulty::English10k: return "English 10k";
        case Difficulty::English25k: return "English 25k";
    }
    return "Unknown";
}

struct Config { // initial configuration
    std::vector<std::string> word_list; // selected vocabulary pool
    std::size_t  word_count = 100;      // session length
    Difficulty   difficulty = Difficulty::English1k;
    int          time_limit = 30;       // seconds
};

struct ConfigEditor {
    bool        editing_count = false;
    std::string count_input; // draft text while editing
    std::string error;
};

void handle_config_key(Config& config, ConfigEditor& editor, char ch);
bool commit_word_count(Config& config, ConfigEditor& editor);
