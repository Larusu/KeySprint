#include "game.h"
#include <algorithm>
#include <cassert>
#include <cctype>
#include <random>
#include <stdexcept>

namespace {
constexpr size_t MAX_EXTRA = 5;  // limit extra letter
constexpr size_t LOOKAHEAD = 70; // Time mode: keep ahead of the cursor

// Append `count` random words, never the same word twice in a row
void add_words(Game& g, size_t count)
{
    assert(g.pool);
    const auto& pool = *g.pool;
    static std::mt19937 rng(std::random_device{}());

    g.words.reserve(g.words.size() + count);
    for (size_t i = 0; i < count; ++i)
    {
        const bool has_prev = !g.words.empty();
        size_t index = std::uniform_int_distribution<size_t>(
            0, pool.size() - (has_prev ? 2 : 1))(rng);
        if (has_prev && index >= g.last_index)
            ++index; // skip over the previous pick
        g.words.push_back(pool[index]);
        g.last_index = index;
    }
    g.typed.resize(g.words.size());
}

}

void apply_key(Game& g, char ch)
{
    if(!g.start)
    {
        if(!std::isalnum(static_cast<unsigned char>(ch))) return;
        g.start = Clock::now();
    }
    if(g.cursor >= g.words.size()) return;

    std::string& typed = g.typed[g.cursor];
    const std::string& target = g.words[g.cursor];

    if(ch == ' ')            { if(!typed.empty()) g.cursor++; return; }
    if(ch == 127 || ch == 8)
    {
        if(!typed.empty()) typed.pop_back();
        else if(g.cursor > 0 && g.typed[g.cursor - 1] != g.words[g.cursor - 1]) g.cursor--;
        return;
    }
    if(!std::isprint(static_cast<unsigned char>(ch)) || typed.size() >= target.size() + MAX_EXTRA)
        return;

    int& counter = typed.size() >= target.size() ? g.keystrokes.extra
                 : target[typed.size()] == ch     ? g.keystrokes.correct
                                                  : g.keystrokes.incorrect;
    counter++;
    typed += ch;
}

double elapsed(const Game& g) {
    if (!g.start) return 0.0;
    return std::chrono::duration<double>(Clock::now() - *g.start).count(); }

double time_left(const Game& g, const Config& c) 
{ return std::max(0.0, c.time_limit - elapsed(g)); }

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
    add_words(game, LOOKAHEAD);
}

Game new_game(const Config& config, const std::vector<std::string>& pool)
{
    if (pool.size() < 2)
        throw std::invalid_argument("At least two words are needed to prevent adjacent repeats");
    assert(config.mode != Mode::Words
           || (config.word_count >= 1 && config.word_count <= MAX_WORD_COUNT));

    Game game;
    game.pool = &pool;
    if (config.mode == Mode::Words)
        add_words(game, config.word_count);
    else
        top_up_words(game, config);      // empty game, so this fills the first batch
    return game;
}
