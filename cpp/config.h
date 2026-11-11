#pragma once

#include <cstddef>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

enum class Difficulty { English1k, English5k, English10k, English25k };
enum class Mode { Time, Words };

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

constexpr std::string_view to_string(Mode mode)
{
    return mode == Mode::Time ? "Time" : "Words";
}

inline constexpr Mode ALL_MODES[] = { Mode::Time, Mode::Words };
inline constexpr Difficulty ALL_DIFFICULTIES[] = {
    Difficulty::English1k, Difficulty::English5k,
    Difficulty::English10k, Difficulty::English25k
};
inline constexpr int TIME_OPTIONS[] = { 15, 30, 60, 120 };
inline constexpr int WORD_OPTIONS[] = { 10, 25, 50, 75 };
inline constexpr int MAX_WORD_COUNT = 10000;

struct Config {
    Mode mode = Mode::Time;
    Difficulty difficulty = Difficulty::English1k;
    int time_limit = 30; // Time mode only
    int word_count = 25; // Words mode only
    std::vector<std::string> word_list; // selected pool, copied from the startup cache
};

struct ConfigEditor {
    bool editing_count = false;
    std::string count_input;
    std::string error;
};

void adjust_config(Config& config, std::size_t focus, int direction);
void handle_config_key(Config& config, ConfigEditor& editor, char ch);
bool commit_word_count(Config& config, ConfigEditor& editor);
