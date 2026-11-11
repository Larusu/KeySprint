#pragma once

#include <chrono>
#include <optional>
#include "config.h"

using Clock = std::chrono::steady_clock;

enum class State      { Playing, Results, Config, Exit };

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

void top_up_words(Game& game, const Config& config);
Game new_game(const Config& config);
void apply_key(Game& game, char ch);
bool is_over(const Game& game, const Config& config);
double elapsed(const Game& g);
double time_left(const Game& game, const Config& config);
