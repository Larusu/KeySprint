#include <sstream>
#include <iomanip>
#include <iostream>
#include <algorithm>
#include <utility>
#include "render.h"
#include "terminal.h"

namespace {

constexpr int BOX_WIDTH     = 72;
constexpr int VISIBLE_LINES = 8;
constexpr int PADDING_X     = 2;
constexpr int PADDING_Y     = 1;
constexpr int BOX_HEIGHT    = VISIBLE_LINES + 2 + 2 * PADDING_Y;

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

std::vector<std::string> layout(const Game& game, size_t max_width)
{
    std::vector<std::string> lines;
    std::string line;
    size_t line_width = 0;

    for(size_t i = 0; i < game.words.size(); i++)
    {
        const std::string& typed  = game.typed[i];
        const std::string& target = game.words[i];
        size_t word_width = std::max(target.size(), typed.size());

        size_t needed = line_width == 0 ? word_width
                                        : line_width + 1 + word_width;
        if (needed > max_width && !line.empty()) 
        {
            lines.push_back(line);
            line.clear();
            line_width = 0;
        }

        if (line_width > 0) { line += ' '; line_width++; }

        bool already_passed = i < game.cursor;
        line += colorize_word(target, typed, already_passed);

        line_width += word_width;
    }

    if(!line.empty())
        lines.push_back(line);

    return lines;
}

Position box_origin(Position size)
{   // + 1 because terminal coordinates is 1 based
    return {
        std::max(1, (size.row - BOX_HEIGHT) / 2 + 1),
        std::max(1, (size.col - BOX_WIDTH)  / 2 + 1)
    };
}

void draw_playing(std::stringstream& frame, const Game& g, const Config& c, Position size)
{
    draw(frame, {1, 1}, "Time: ", time_left(g, c));
    if (g.cursor < g.typed.size())
        draw(frame, {1, size.col / 2}, g.typed[g.cursor]);

    const Position box   = box_origin(size);
    const int text_row   = box.row + 1 + PADDING_Y;
    const int text_col   = box.col + 1 + PADDING_X;
    const int text_width = BOX_WIDTH - 2 - 2 * PADDING_X;

    draw_box(frame, box.row, box.col, BOX_WIDTH, BOX_HEIGHT);

    const std::vector<std::string> lines = layout(g, text_width);
    const size_t line_shown = std::min(lines.size(), size_t(VISIBLE_LINES));
    for (size_t i = 0; i < line_shown; i++)
        draw(frame, {text_row + int(i), text_col}, lines[i]);
}

// ---------- footer ----------
void draw_ctrl(std::stringstream& frame, int row, int center_col,
               std::string_view key, std::string_view label)
{
    const int width = int(key.size()) + 1 + int(label.size());
    draw(frame, {row, center_col - width / 2}, green(), key, reset(), ' ', label);
}

void draw_footer(std::stringstream& frame, Position box)
{
    const int row = box.row + BOX_HEIGHT + 1; // 2 rows below the box
    draw_ctrl(frame, row, box.col + BOX_WIDTH * 1 / 6, "ctrl+n", "new game");
    draw_ctrl(frame, row, box.col + BOX_WIDTH * 3 / 6, "ctrl+f", "config");
    draw_ctrl(frame, row, box.col + BOX_WIDTH * 5 / 6, "ctrl+c", "exit");
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
    const Position box  = box_origin(size);
    std::stringstream frame;

    switch (s)
    {
        case State::Playing: draw_playing(frame, g, c, size);   break;
        case State::Results: draw_results(frame, r, size); break;
        case State::Config: //TODO: implement config 
        case State::Exit:    break;
    }
    draw_footer(frame, box);
    std::cout << frame.str() << std::flush;
}

