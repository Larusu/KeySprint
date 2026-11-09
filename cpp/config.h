#pragma once

#include <string>
#include <vector>
#include <cstddef>
#include <string_view>

enum class Difficulty { Easy, Medium, Hard };
enum class Mode { Time, Words };

constexpr std::string_view to_string(Difficulty d)
{
    switch(d)
    {
        case Difficulty::Easy:   return "Easy";
        case Difficulty::Medium: return "Medium";
        case Difficulty::Hard:   return "Hard";
    }
}

constexpr std::string_view to_string(Mode m)
{
    switch(m)
    {
        case Mode::Time:  return "Time";
        case Mode::Words: return "Words";
    }
}

inline constexpr Mode       ALL_MODES[]        = { Mode::Time, Mode::Words };
inline constexpr Difficulty ALL_DIFFICULTIES[] = { Difficulty::Easy, Difficulty::Medium, Difficulty::Hard };
inline constexpr int        TIME_OPTIONS[]     = { 15, 30, 60, 120 }; // seconds
inline constexpr size_t     WORD_OPTIONS[]     = { 10, 25, 50, 100 };

struct Config {
    Mode       mode       = Mode::Time;
    Difficulty difficulty = Difficulty::Easy;
    int        time_limit = 30;   // Time mode
    size_t     word_count = 35;   // Words mode
};

const std::vector<std::string>& word_list(Difficulty d);
