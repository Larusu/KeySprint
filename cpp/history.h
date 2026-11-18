#pragma once

#include "config.h"
#include "scoring.h"
#include <unordered_map>
#include <string>
#include <string_view>

struct History { // per config record
    double wpm      = 0.0;
    double accuracy = 0.0;
};

using HistoryBank = std::unordered_map<std::string, History>;

struct HistoryStore {
    int attempts     = 0;  // shared across all configs
    std::string path = ""; // json path
    HistoryBank bank;
};

inline std::string history_key(const Config& c);
HistoryStore load_history(const std::string& path);
void record_result(HistoryStore& s, const Config& c, const Result& r);
History best_for(const HistoryStore& s, const Config& c);
