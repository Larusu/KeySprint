#include "word_bank.h"

#include <algorithm>
#include <fstream>
#include <stdexcept>
#include <unordered_set>
#include <utility>
#include <nlohmann/json.hpp>

namespace {

std::size_t as_count(const nlohmann::json& value)
{
    if (!value.is_number_integer() || value.get<long long>() < 0)
        throw std::runtime_error("Expected a non-negative integer count");
    return static_cast<std::size_t>(value.get<long long>());
}

}

WordBank load_word_bank(const std::string& path)
{
    std::ifstream file(path);
    if (!file)
        throw std::runtime_error("Cannot open word bank: " + path);

    const auto document = nlohmann::json::parse(file);

    const auto& version = document.at("schema_version");
    if (!version.is_number_integer() || version != 1)
        throw std::runtime_error("Unsupported word-bank schema version");
    if (document.at("language") != "en")
        throw std::runtime_error("Expected an English word bank");

    const auto& levels = document.at("levels");
    if (!levels.is_object() || levels.size() != 4)
        throw std::runtime_error("Expected the four English vocabulary pools");

    WordBank bank;
    for (const char* key : {"english_1k", "english_5k",
                            "english_10k", "english_25k"})
    {
        auto words = levels.at(key).get<std::vector<std::string>>();
        if (words.size() < 2)
            throw std::runtime_error(
                std::string("Word pool needs at least two words: ") + key);

        const auto& count = document.at("metadata").at("actual_counts").at(key);
        if (as_count(count) != words.size())
            throw std::runtime_error(std::string("Incorrect word count: ") + key);

        std::unordered_set<std::string> unique;
        for (const auto& word : words)
        {
            const bool letters_only = word.size() >= 3 && std::all_of(
                word.begin(), word.end(), [](unsigned char ch) {
                    return ch >= 'a' && ch <= 'z';
                });
            if (!letters_only || !unique.insert(word).second)
                throw std::runtime_error(
                    std::string("Invalid or duplicate word in ") + key);
        }
        bank.emplace(key, std::move(words));
    }
    return bank;
}

const std::vector<std::string>& get_word_list(
    const WordBank& bank, std::string_view level)
{
    const auto found = bank.find(std::string(level));
    if (found == bank.end())
        throw std::runtime_error("Unknown word-bank level: " + std::string(level));
    return found->second;
}
