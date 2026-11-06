#include "game.h"
#include <algorithm>
#include <cctype>
#include <cstdlib>

namespace {
    constexpr size_t MAX_EXTRA = 5; // limit extra letter
}

std::vector<std::string>get_word_list()
{
    // TODO: change after json file is implemented
    return {
        "Cat", "Dog", "Sun", "Map", "Bag", "Grab",
        "Animal", "Answer", "Camera", "Castle", "Garden", "Planet",
        "Absolute","Activity","Balanced","Campaign","Capacity","Concrete",
        "Constant", "Creative", "Database"
    };
}

std::vector<std::string>pick_random_words(const Config& c)
{
    std::vector<std::string> picked;
    if(c.word_list.empty()) 
        return picked;

    picked.reserve(c.word_count);
    for(size_t i = 0; i < c.word_count; i++)
        picked.push_back(c.word_list[static_cast<size_t>(rand()) % c.word_list.size()]);
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
