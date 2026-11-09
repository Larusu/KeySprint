#include "scoring.h"
#include <iomanip>
#include <cassert>
#include <algorithm>
#include <ctime>
#include <sstream>

namespace {

Stats compute_stats(const Game& game)
{
    Stats s{};

    for(size_t i = 0; i < game.words.size(); i++)
    {
        const std::string& target = game.words[i];
        const std::string& typed  = game.typed[i];

        for(size_t j = 0; j < typed.size(); j++)
        {
            if (j >= target.size())
                s.extra++;
            else if (target[j] == typed[j])
                s.correct++;
            else
                s.incorrect++;
        }
        if(i < game.cursor && typed.size() < target.size())
            s.missed += target.size() - typed.size();
    }
    return s;
}

}

Result make_result(const Game& g, const Config& c)
{
    assert(is_over(g, c));
    if(!is_over(g, c))
        return {};

    Result result{};
    Stats totals = compute_stats(g);

    // calculate duration
    double duration = elapsed(g);
    if (c.mode == Mode::Time)
        duration = std::min(duration, double(c.time_limit));
    result.duration = duration;

    // calculate accuracy
    int overall = g.keystrokes.correct + g.keystrokes.incorrect + g.keystrokes.extra;
    result.accuracy = overall != 0 ? double(g.keystrokes.correct) / overall * 100
                                   : 0.0;

    // calculate WPM
    constexpr double CHARS_PER_WORD   = 5.0;
    constexpr double SECONDS_PER_MIN  = 60.0;
    double wpm = 0.0;
    if(duration != 0.0)
        wpm = (double(totals.correct) / CHARS_PER_WORD) / (duration / SECONDS_PER_MIN);
    result.wpm = wpm;

    // difficulty
    result.difficulty = c.difficulty;

    // date and time 
    auto now = std::chrono::system_clock::now();
    std::time_t current_time = std::chrono::system_clock::to_time_t(now);
    std::tm local_time;
    std::ostringstream oss;
    localtime_r(&current_time, &local_time);
    oss << std::put_time(&local_time, "%Y-%m-%dT%H:%M:%S");
    result.date = oss.str();

    return result;
}

