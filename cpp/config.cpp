#include "config.h"

namespace {

// move to the next/previous entry in an array
template <typename T, size_t N>
T step(const T (&values)[N], T current, int dir)
{
    int i = 0;
    for(int k = 0; k < int(N); k++)
        if(values[k] == current)
            i = k;
    i = (i + int(N) + dir) % int(N);
    return values[i];
}

}

void adjust_config(Config& c, size_t focus, int dir)
{
    switch(focus)
    {
        case 0: 
            c.mode       = step(ALL_MODES,        c.mode,       dir); 
            break;
        case 1:
            c.difficulty = step(ALL_DIFFICULTIES, c.difficulty, dir); 
            break;
        case 2:
            if(c.mode == Mode::Time)
                c.time_limit = step(TIME_OPTIONS, c.time_limit, dir);
            else
                c.word_count = step(WORD_OPTIONS, c.word_count, dir);
            break;
    }
}

const std::vector<std::string>& word_list(Difficulty d)
{
    static const std::vector<std::string> easy = {
        "cat", "dog", "sun", "map", "bag", "grab",
        "run", "book", "tree", "fish", "blue", "rain",
        "house", "apple", "water", "chair", "happy", "light",
        "mouse", "cloud", "bread", "green", "smile", "plant"
    };

    static const std::vector<std::string> medium = {
        "animal", "answer", "camera", "castle", "garden", "planet",
        "bottle", "window", "school", "friend", "market", "summer",
        "winter", "morning", "people", "family", "picture", "village",
        "kitchen", "forest", "bridge", "travel", "office", "number"
    };

    static const std::vector<std::string> hard = {
        "absolute", "activity", "balanced", "campaign",
        "capacity", "concrete", "constant", "creative", "database",
        "adventure", "beautiful", "challenge", "computer",
        "direction", "education", "environment", "important",
        "knowledge", "language", "management", "necessary",
        "operation", "performance", "production", "technology",
        "understand", "development", "experience", "information"
    };

    switch (d)
    {
        case Difficulty::Easy:   return easy;
        case Difficulty::Medium: return medium;
        case Difficulty::Hard:   return hard;
    }
    return easy;
}
