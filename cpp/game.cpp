#include "game.h"
#include <algorithm>
#include <cctype>
#include <cstdlib>

namespace {
constexpr size_t MAX_EXTRA = 5; // limit extra letter
constexpr size_t LOOKAHEAD = 40; // Time mode: keep ahead of the cursor

std::vector<std::string>pick_random_words(Difficulty d, int count)
{
    const std::vector<std::string>& words = word_list(d);
    std::vector<std::string> picked;
    if(words.empty()) 
        return picked;

    picked.reserve(count);
    for(size_t i = 0; i < size_t(count); i++)
        picked.push_back(words[static_cast<size_t>(rand()) % words.size()]);
    return picked;
}

}

void apply_key(Game& g, char ch)
{
    if(!g.start)
    { // ignore keys until the first letter/digit
        if (!std::isalnum(static_cast<unsigned char>(ch)))
            return;
        g.start = Clock::now();
    }
    // ignore any key press and input after last word
    if (g.cursor >= g.words.size()) 
        return;

    std::string& typed = g.typed[g.cursor];
    const std::string& target = g.words[g.cursor];

    if(ch == ' ')
    {
        if(typed.empty()) // prevent spam
            return;
        g.cursor++;
        return;
    }
    else if(ch == 127 || ch == 8) // backspace
    {
        if(!typed.empty())
            typed.pop_back(); 
        else if(g.cursor > 0 && g.typed[g.cursor - 1] != g.words[g.cursor - 1])
            g.cursor--; // can only go to previous word that has mistake
        return;
    }

    if(!std::isprint(static_cast<unsigned char>(ch)))
        return;

    // limit number of typed key
    if(typed.size() >= target.size() + MAX_EXTRA)
        return;

    if (typed.size() >= target.size())
        g.keystrokes.extra++;
    else if (target[typed.size()] == ch)
        g.keystrokes.correct++;
    else
        g.keystrokes.incorrect++;

    typed += ch;
}

double elapsed(const Game& g)
{
    if (!g.start) return 0.0;
    return std::chrono::duration<double>(Clock::now() - *g.start).count();
}

double time_left(const Game& g, const Config& c)   // Time mode only
{
    return std::max(0.0, c.time_limit - elapsed(g));
}

bool is_over(const Game& game, const Config& config)
{
    switch (config.mode)
    {
        case Mode::Time:
            return time_left(game, config) <= 0.0;
        case Mode::Words:
        {
            if (game.cursor >= game.words.size()) return true;
            const size_t last = game.words.size() - 1;
            return game.cursor == last && game.typed[last] == game.words[last];
        }
    }
    return false;
}

void top_up_words(Game& game, const Config& config)
{
    if (config.mode != Mode::Time) return;
    if (game.words.size() - game.cursor >= LOOKAHEAD) return;

    const auto more = pick_random_words(config.difficulty, LOOKAHEAD);
    game.words.insert(game.words.end(), more.begin(), more.end());
    game.typed.resize(game.words.size()); // what word is must type be
}

Game new_game(const Config& config) 
{
    Game game;
    const size_t count = (config.mode == Mode::Words) ? config.word_count : 200;
    game.words = pick_random_words(config.difficulty, count);
    game.typed.resize(game.words.size());
    return game;
}
