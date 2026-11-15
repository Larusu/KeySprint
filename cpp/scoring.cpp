#include "scoring.h"
#include <algorithm>
#include <cassert>
#include <ctime>

namespace {

constexpr double CHARS_PER_WORD  = 5.0;
constexpr double SECONDS_PER_MIN = 60.0;

Stats compute_stats(const Game& game)
{
    Stats s{};
    for (size_t i = 0; i < game.words.size(); i++)
    {
        const std::string& target = game.words[i];
        const std::string& typed  = game.typed[i];

        for (size_t j = 0; j < typed.size(); j++)
        {
            if (j >= target.size())            s.extra++;
            else if (target[j] == typed[j])    s.correct++;
            else                               s.incorrect++;
        }
        if (i < game.cursor && typed.size() < target.size())
            s.missed += target.size() - typed.size();
    }
    return s;
}

// only fully correct words count, plus the space that committed each
size_t wpm_chars(const Game& game)
{
    size_t n = 0;
    for (size_t i = 0; i < game.words.size(); i++)
    {
        if (game.typed[i] != game.words[i]) continue;
        n += game.words[i].size();
        if (i < game.cursor) n++;
    }
    return n;
}

std::string now_iso8601()
{
    std::time_t t = std::time(nullptr);
    std::tm local{};
#ifdef _WIN32
    localtime_s(&local, &t);
#else
    localtime_r(&t, &local);
#endif
    char buf[20]; // "YYYY-MM-DDTHH:MM:SS" + '\0'
    std::strftime(buf, sizeof buf, "%Y-%m-%dT%H:%M:%S", &local);
    return buf;
}

}

Result make_result(const Game& g, const Config& c)
{
    assert(is_over(g, c));

    Result r{};
    r.chars      = compute_stats(g);
    r.difficulty = c.difficulty;
    r.date       = now_iso8601();

    r.duration = elapsed(g);
    if (c.mode == Mode::Time)
        r.duration = std::min(r.duration, double(c.time_limit));

    const int total = g.keystrokes.correct + g.keystrokes.incorrect + g.keystrokes.extra;
    r.accuracy = total ? 100.0 * g.keystrokes.correct / total : 0.0;

    r.wpm = r.duration > 0.0
          ? wpm_chars(g) / CHARS_PER_WORD / (r.duration / SECONDS_PER_MIN)
          : 0.0;

    return r;
}
