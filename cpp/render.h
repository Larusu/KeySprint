#pragma once

#include "config.h"
#include "game.h"
#include "scoring.h"

void render(const Game& game, const Config& config, const State& state,
            const Result& result, const ConfigEditor& editor);
