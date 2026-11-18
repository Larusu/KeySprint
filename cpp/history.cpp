#include "history.h"
#include "include/nlohmann/json.hpp"
#include <fstream>

namespace {

void save_history(const HistoryStore& s)
{
    // file was already open in memory upon startup, no need to reopen
    nlohmann::json json;
    json["attempts"] = s.attempts;
    json["records"]  = nlohmann::json::object();
    for(const auto& [key, h] : s.bank)
        json["records"][key] = { { "wpm", h.wpm }, { "accuracy", h.accuracy } };
    std::ofstream(s.path) << json.dump(3);
}

}

inline std::string history_key(const Config& c)
{
    std::string key(difficulty_info(c.difficulty).key);
    if(c.mode == Mode::Time)
        return key + "/time/" + std::to_string(c.time_limit);
    return key + "/words/" + std::to_string(c.word_count);
}

HistoryStore load_history(const std::string& path)
{
    HistoryStore store{};
    store.path = path;

    std::ifstream file(path);
    if (!file)
        return store; // first run

    const auto json = nlohmann::json::parse(file);

    store.attempts = json.value("attempts", 0); 
    if(store.attempts <= 0) 
        return store;

    const auto it = json.find("records"); 
    if(it != json.end() && it->is_object())
    {
        for(const auto& [key, val] : it->items())
        {
            if(!val.is_object())
                continue;
            store.bank[key] = { 
                val.value("wpm",      0.0),
                val.value("accuracy", 0.0)
            };
        }
    }

    return store;
}

void record_result(HistoryStore& s, const Config& c, const Result& r)
{
    s.attempts++;
    History& h = s.bank[history_key(c)]; // entry inside the map (HistoryBank)
    h.wpm      = std::max(h.wpm, r.wpm); // the map now has this value
    h.accuracy = std::max(h.accuracy, r.accuracy);
    save_history(s);
}

History best_for(const HistoryStore& s, const Config& c)
{
    const auto it = s.bank.find(history_key(c));
    if (it == s.bank.end())
        return {};     // History{0.0, 0.0}
    return it->second; // a copy of the stored record
}
