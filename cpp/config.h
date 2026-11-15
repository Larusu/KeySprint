#pragma once

#include <cstddef>
#include <iterator>
#include <string>
#include <string_view>

enum class Difficulty { English1k, English5k, English10k, English25k };
enum class Mode { Time, Words };

struct DifficultyInfo {
    std::string_view key;    // word bank / JSON key
    std::string_view title;  // "English 1k"
    std::string_view label;  // "1k", for the selector row
};

// Indexed by Difficulty, so the order must match the enum.
inline constexpr DifficultyInfo DIFFICULTIES[] = {
    { "english_1k",  "English 1k",  "1k"  },
    { "english_5k",  "English 5k",  "5k"  },
    { "english_10k", "English 10k", "10k" },
    { "english_25k", "English 25k", "25k" },
};
inline constexpr std::size_t DIFFICULTY_COUNT = std::size(DIFFICULTIES);
static_assert(DIFFICULTY_COUNT == std::size_t(Difficulty::English25k) + 1,
              "DIFFICULTIES must match the Difficulty enum");

constexpr const DifficultyInfo& difficulty_info(Difficulty d)
{
    return DIFFICULTIES[std::size_t(d)];
}

inline constexpr int TIME_OPTIONS[] = { 15, 30, 60, 120 };
inline constexpr int WORD_OPTIONS[] = { 10, 25, 50, 75 };
inline constexpr int MAX_WORD_COUNT = 10000;

struct Config {
    Mode       mode       = Mode::Time;
    Difficulty difficulty = Difficulty::English1k;
    int        time_limit = 30;  // Time mode only
    int        word_count = 25;  // Words mode only
};

struct ConfigEditor {
    bool        editing_count = false;
    std::string count_input;
    std::string error;
};

void adjust_config(Config& config, std::size_t focus, int direction);
void handle_config_key(Config& config, ConfigEditor& editor, char ch);
bool commit_word_count(Config& config, ConfigEditor& editor);
