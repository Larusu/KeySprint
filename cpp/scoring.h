#pragma once

#include <string>
#include "game.h"

struct Result {             // for JSON
    double      wpm        = 0.0;
    double      accuracy   = 0.0; // percent
    double      duration   = 0.0; // seconds
    Difficulty  difficulty = Difficulty::Easy;
    std::string date       = ""; 
};

Result make_result(const Game& game, const Config& config);
