#pragma once

#include "game.h"
#include "scoring.h"

void render(const Game& game, const Config& config, const State& state,
            const Result& result, size_t config_focus);
