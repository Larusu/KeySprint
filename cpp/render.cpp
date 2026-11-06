#include <sstream>
#include <iomanip>
#include <iostream>
#include <algorithm>
#include <utility>
#include "render.h"
#include "terminal.h"

namespace {
constexpr int MAX_CHAR_PER_ROW = 50;

std::string colorize_word(const std::string& target, const std::string& typed, bool already_passed)
{
    std::string word;
    size_t max_len = std::max(target.size(), typed.size());

    for(size_t j = 0; j < max_len; j++)
    {
        const char* color;
        char shown;
        if(j >= target.size()) // extra char
        { 
            color = red();
            shown = typed[j];
        }
        else if(j < typed.size())
        {  // correct or incorrect
            color = (target[j] == typed[j]) ? green() : red(); 
            shown = target[j]; 
        }
        else
        {  // skipped or not typed yet
            color = (already_passed) ? red() : white(); 
            shown = target[j];
        }
        word += std::string(color) + shown + reset();
    }
    return word;
}

// Joins colored words into lines of at most MAX_CHAR_PER_ROW characters
std::vector<std::string> layout(const Game& game)
{
    std::vector<std::string> lines;
    std::string line;
    size_t line_width = 0;

    for(size_t i = 0; i < game.words.size(); i++)
    {
        const std::string& typed  = game.typed[i];
        const std::string& target = game.words[i];

        size_t needed = line_width == 0 ? target.size()
                                        : line_width + 1 + target.size();
        if (needed > MAX_CHAR_PER_ROW && !line.empty()) 
        {
            lines.push_back(line);
            line.clear();
            line_width = 0;
        }

        if (line_width > 0) { line += ' '; line_width++; }

        bool already_passed = i < game.cursor;
        line += colorize_word(target, typed, already_passed);

        line_width += std::max(target.size(), typed.size());
    }

    if(!line.empty())
        lines.push_back(line);

    return lines;
}

template <typename... Args>
void draw(std::stringstream& output, Position d, Args&&... args)
{
    output << move_cursor(d);
    output << std::fixed << std::setprecision(2);
    (output << ... << std::forward<Args>(args));
}

void draw_centered(std::stringstream& frame, int row, int center_col, std::string_view text)
{
    draw(frame, {row, center_col - int(text.size()) / 2}, text);
}

void draw_footer(std::stringstream& frame, Position size)
{
    const int row = size.row - 5;
    draw_centered(frame, row, size.col / 4,     "[Ctrl / C] Exit");
    draw_centered(frame, row, size.col / 2,     "[Ctrl / N] New Game");
    draw_centered(frame, row, size.col * 3 / 4, "[Ctrl / F] Config");
}

void draw_playing(std::stringstream& frame, const Game& g, const Config& c, Position size)
{
    draw(frame, {1, 1}, "Time: ", time_left(g, c));
    if (g.cursor < g.typed.size())
        draw(frame, {1, size.col / 2}, g.typed[g.cursor]);

    const int col = (size.col - MAX_CHAR_PER_ROW) / 2;
    const int row = size.row / 2 - 2;

    const std::vector<std::string> lines = layout(g);
    for (size_t i = 0; i < lines.size(); i++)
        draw(frame, {row + int(i), col}, lines[i]);
}

// WARNING: temporary screen
void draw_results(std::stringstream& frame, const Result& r, Position size)
{
    const int mid = size.row / 2;
    const int col = size.col / 2 - 8;
    draw(frame, {mid - 2, size.col / 2 - 6}, "Round over!");
    draw(frame, {mid,     col}, "WPM = ",        r.wpm);
    draw(frame, {mid + 1, col}, "Accuracy = ",   r.accuracy);
    draw(frame, {mid + 2, col}, "Duration = ",   r.duration);
    draw(frame, {mid + 3, col}, "Difficulty = ", to_string(r.difficulty));
    draw(frame, {mid + 4, col}, "Time = ",       r.date);
}

}

void render(const Game& g, const Config& c, const State& s, const Result& r)
{
    clear_screen();
    const Position size = get_term_size();
    std::stringstream frame;

    switch (s)
    {
        case State::Playing: draw_playing(frame, g, c, size);   break;
        case State::Results: draw_results(frame, r, size); break;
        case State::Config: //TODO: implement config 
        case State::Exit:    break;
    }
    draw_footer(frame, size);
    std::cout << frame.str() << std::flush;
}

