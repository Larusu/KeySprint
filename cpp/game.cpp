#include "game.h"
#include <algorithm>
#include <cctype>
#include <random>
#include <stdexcept>
#include <string_view>

namespace {
constexpr size_t MAX_EXTRA = 5; // limit extra letter
constexpr size_t LOOKAHEAD = 50; // Time mode: keep ahead of the cursor

std::vector<std::string> pick_random_words(const Config& config, std::size_t count,
                                           std::string_view previous_word = {})
{
    const auto& pool = config.word_list;
    if (pool.size() < 2)
        throw std::invalid_argument("At least two words are needed to prevent adjacent repeats");

    // One dice roller for the app. Refills remember the last roll too.
    static std::mt19937 generator(std::random_device{}());
    std::size_t previous = pool.size();
    if (!previous_word.empty())
        previous = static_cast<std::size_t>(
            std::find(pool.begin(), pool.end(), previous_word) - pool.begin());

    std::vector<std::string> picked;
    picked.reserve(count);
    for (std::size_t i = 0; i < count; ++i)
    {
        const bool has_previous = previous < pool.size();
        std::size_t index = std::uniform_int_distribution<std::size_t>(
            0, pool.size() - (has_previous ? 2 : 1))(generator);
        if (has_previous && index >= previous)
            ++index;
        picked.push_back(pool[index]);
        previous = index;
    }
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
    size_t ahead = config.difficulty == Difficulty::English1k ? LOOKAHEAD + 20 : LOOKAHEAD;
    if (game.words.size() - game.cursor >= ahead) return;

    const auto more = pick_random_words(config, LOOKAHEAD,
        game.words.empty() ? std::string_view{} : game.words.back());
    game.words.insert(game.words.end(), more.begin(), more.end());
    game.typed.resize(game.words.size()); // what word is must type be
}

Game new_game(const Config& config)
{
    if (config.mode == Mode::Words
        && (config.word_count < 1 || config.word_count > MAX_WORD_COUNT))
        throw std::invalid_argument("Word count must be between 1 and 10,000");
    Game game;
    const size_t word_size = config.difficulty == Difficulty::English1k ? 80 : 70;
    const size_t count = (config.mode == Mode::Words) ? config.word_count : word_size;
    game.words = pick_random_words(config, count);
    game.typed.resize(game.words.size());
    return game;
}
