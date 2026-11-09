#include "config.h"

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
