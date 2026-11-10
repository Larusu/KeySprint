#include <sstream>
#include <iomanip>
#include <iostream>
#include <algorithm>
#include <utility>
#include "render.h"
#include "terminal.h"

namespace {
constexpr int MAX_CHAR_PER_ROW = 50;

template <typename... Args>
void draw(std::stringstream& output, Position d, Args&&... args)
{
    output << move_cursor(d);
    output << std::fixed << std::setprecision(2);
    (output << ... << std::forward<Args>(args));
}

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

std::string repeat(const char* s, int n)
{
    std::string str;
    for(int i = 0; i < n; i++)
    {
        str += s;
    }
    return str;
}

void draw_box(std::stringstream& frame, int row, int col, int width, int height)
{
    draw(frame, {row, col}, "╭", repeat("─", width - 2), "╮");
    for(int r = 1; r < height - 1; r++)
    {
        draw(frame, {row + r, col},             "│");
        draw(frame, {row + r, col + width - 1}, "│");
    }
    draw(frame, {row + height - 1, col}, "╰", repeat("─", width - 2), "╯");
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

void draw_config(std::stringstream& frame, const Config& c,
                 const ConfigEditor& editor, Position size)
{
    const int mid = size.row / 2;
    const int col = size.col / 2 - 18;

    draw_centered(frame, mid - 7, size.col / 2, "Configuration");
    draw(frame, {mid - 4, col}, "Difficulty:  ", to_string(c.difficulty));
    draw(frame, {mid - 3, col}, "Word count:  ", c.word_count);

    if (editor.editing_count)
        draw(frame, {mid - 2, col}, "Editing:     ",
             editor.count_input.empty() ? "_" : editor.count_input);
    else
        draw(frame, {mid - 2, col}, "Press w to edit the word count");

    if (!editor.error.empty())
        draw(frame, {mid - 1, col}, red(), editor.error, reset());

    draw(frame, {mid + 1, col}, "[1-4] English 1k / 5k / 10k / 25k");
    draw(frame, {mid + 2, col}, "[w] Edit count   [Enter] Commit count");
    draw(frame, {mid + 3, col}, "[Ctrl/N] Start   [Ctrl/C] Exit");
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

void render(const Game& g, const Config& c, const State& s, const Result& r,
            const ConfigEditor& editor)
{
    clear_screen();
    const Position size = get_term_size();
    std::stringstream frame;

    draw_box(frame, 5, 10, 40, 8);

    switch (s)
    {
        case State::Playing: draw_playing(frame, g, c, size);   break;
        case State::Results: draw_results(frame, r, size); break;
        case State::Config: draw_config(frame, c, editor, size); break;
        case State::Exit:   break;
    }
    draw_footer(frame, size);
    std::cout << frame.str() << std::flush;
}

