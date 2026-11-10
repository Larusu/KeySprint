#include "game.h"
#include <algorithm>
#include <cctype>
#include <random>
#include <stdexcept>

namespace {
    constexpr size_t MAX_EXTRA = 5; // limit extra letter
}

std::vector<std::string> pick_random_words(const Config& config)
{
    const auto pool_size = config.word_list.size();
    if (config.word_count == 0)
        throw std::invalid_argument("Word count must be positive");
    if (pool_size == 0)
        throw std::invalid_argument("The selected word pool is empty");
    if (pool_size == 1 && config.word_count > 1)
        throw std::invalid_argument(
            "At least two words are needed to prevent adjacent repeats");

    // One dice roller for the app: seed once so restarting a round does not
    // reset the sequence.
    static std::mt19937 generator(std::random_device{}());

    std::vector<std::string> picked;
    picked.reserve(config.word_count);
    std::size_t previous = pool_size;

    for (std::size_t i = 0; i < config.word_count; ++i)
    {
        std::size_t index;
        if (i == 0)
        {
            index = std::uniform_int_distribution<std::size_t>(
                0, pool_size - 1)(generator);
        }
        else
        {
            // Draw among every index except the previous one.
            index = std::uniform_int_distribution<std::size_t>(
                0, pool_size - 2)(generator);
            if (index >= previous)
                ++index;
        }
        picked.push_back(config.word_list[index]);
        previous = index;
    }
    return picked;
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

double time_left(const Game& g, const Config& c)
{
    if(!g.start)
        return c.time_limit;
    double elapsed = std::chrono::duration<double>(Clock::now() - *g.start).count();
    return std::max(0.0, c.time_limit - elapsed);
}

bool is_over(const Game& game, const Config& config)
{
    if (game.cursor >= game.words.size())
        return true;                          // passed the last word
    if (time_left(game, config) <= 0.0)
        return true;                          // out of time

    // last word typed correctly
    const size_t last = game.words.size() - 1;
    return game.cursor == last && game.typed[last] == game.words[last];
}

Game new_game(const Config& config) 
{
    Game game;
    game.words = pick_random_words(config);
    game.typed.resize(game.words.size());
    return game;
}
