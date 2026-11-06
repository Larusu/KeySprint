#pragma once

#include <chrono>
#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

using Clock = std::chrono::steady_clock;

enum class Difficulty { Easy, Medium, Hard };
enum class State      { Playing, Results, Config, Exit };

constexpr std::string_view to_string(Difficulty d)
{
    switch(d)
    {
        case Difficulty::Easy:   return "Easy";
        case Difficulty::Medium: return "Medium";
        case Difficulty::Hard:   return "Hard";
    }
}

struct Config { // initial configuration
    std::vector<std::string> word_list; // entire dictionary
    size_t     word_count = 100;
    Difficulty difficulty = Difficulty::Easy;
    int        time_limit = 30;         // seconds
};

struct Stats {
    int correct   = 0; 
    int incorrect = 0;
    int extra     = 0;
    int missed    = 0;
};

struct Game {                       // mutable per game
    std::vector<std::string> words; // picked for this run
    std::vector<std::string> typed; // typed[i] = what the player typed for words[i]
    size_t cursor = 0;
    std::optional<Clock::time_point> start;
    Stats keystrokes;
};

std::vector<std::string>get_word_list();
std::vector<std::string>pick_random_words(const Config& config);
Game new_game(const Config& config);
void apply_key(Game& game, char ch);
bool is_over(const Game& game, const Config& config);
double time_left(const Game& game, const Config& config);
