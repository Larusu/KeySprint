#include <cmath>
#include <sstream>
#include <iomanip>
#include <iostream>
#include <algorithm>
#include <string>
#include <utility>
#include <vector>
#include <string_view>
#include "terminal.h"
#include "render.h"

namespace {

struct Layout {
   std::vector<std::string> lines;
   size_t cursor_line = 0;
};

// main box
constexpr int BOX_WIDTH     = 74;
constexpr int VISIBLE_LINES = 6;
constexpr int PADDING_X     = 2;
constexpr int PADDING_Y     = 1;
constexpr int BOX_HEIGHT    = VISIBLE_LINES + 2 + 2 * PADDING_Y;
constexpr int BAR_HEIGHT    = 3;
constexpr int TOTAL_HEIGHT  = BAR_HEIGHT + BOX_HEIGHT + 2;

constexpr int SELECTOR_GAP      = 1;
// fixed weight so the bar doesn't resize when the mode changes
constexpr int OPTION_WEIGHT     = 3 * 4;

// results screen
constexpr int RESULT_ROWS         = 6;
constexpr int RESULT_BOX_HEIGHT   = RESULT_ROWS + 2 + 2 * PADDING_Y;
constexpr int RESULT_TOTAL_HEIGHT = 1 + RESULT_BOX_HEIGHT + 2; // header + box + blank + footer
static_assert(RESULT_ROWS <= VISIBLE_LINES, "results don't fit in the box");

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

Layout layout(const Game& game, size_t max_width)
{
    Layout out;
    std::string line;
    size_t line_width = 0;

    for(size_t i = 0; i < game.words.size(); i++)
    {
        const std::string& typed  = game.typed[i];
        const std::string& target = game.words[i];
        const size_t word_width = std::max(target.size(), typed.size());

        size_t needed = line_width == 0 ? word_width
                                        : line_width + 1 + word_width;
        if (needed > max_width && !line.empty())
        {
            out.lines.push_back(line);
            if (i > game.cursor && out.lines.size() > out.cursor_line + VISIBLE_LINES)
                return out;
            line.clear();
            line_width = 0;
        }

        if (line_width > 0) { line += ' '; line_width++; }

        if(i == game.cursor)
            out.cursor_line = out.lines.size();

        bool already_passed = i < game.cursor;
        line += colorize_word(target, typed, already_passed);
        line_width += word_width;
    }

    if(!line.empty())
        out.lines.push_back(line);

    if(game.cursor >= game.words.size() && !out.lines.empty())
        out.cursor_line = out.lines.size() - 1;

    return out;
}

Position box_origin(Position size)
{   // block top includes bar + box + blank + footer
    const int block_top = std::max(1, (size.row - TOTAL_HEIGHT) / 2 + 1);
    return {
        block_top + BAR_HEIGHT,
        std::max(1, (size.col - BOX_WIDTH)  / 2 + 1)
    };
}

// ---------- bar ----------
struct Selector {
    std::vector<std::string> options;
    size_t selected;
    int weight = 0; // 0 = compute from options
};

int counter_width(const Config& c)
{
    // Leave elbow room for 10000/10000 without squeezing into the next box.
    return c.mode == Mode::Time ? 9
        : std::max(9, 2 * int(std::to_string(c.word_count).size()) + 5);
}

void draw_counter(std::stringstream& frame, Position box, const Game& g, const Config& c)
{
    const int bar_row = box.row - BAR_HEIGHT;
    const int width = counter_width(c);
    draw_box(frame, bar_row, box.col, width, BAR_HEIGHT);

    std::string text;
    if(c.mode == Mode::Time)
        text = std::to_string(int(std::ceil(time_left(g, c))));
    else
        text = std::to_string(g.cursor) + "/" + std::to_string(g.words.size());
    const int pad = std::max(0, (width - 2 - int(text.size())) / 2);
    draw(frame, {bar_row + 1, box.col + 1 + pad}, text);
}

template <typename T, size_t N>
size_t fill_options(const T (&values)[N], T current, std::vector<std::string>& out)
{
    size_t selected = 0;
    for(size_t i = 0; i < N; i++)
    {
        out.push_back(std::to_string(values[i]));
        if(values[i] == current)
            selected = i;
    }
    return selected;
}

std::vector<Selector> make_selectors(const Config& c)
{
    std::vector<std::string> options_list;
    size_t selected_index;

    if(c.mode == Mode::Words)
        selected_index = fill_options(WORD_OPTIONS, static_cast<int>(c.word_count), options_list);
    else
        selected_index = fill_options(TIME_OPTIONS, c.time_limit, options_list);

    if (c.mode == Mode::Words
        && std::find(std::begin(WORD_OPTIONS), std::end(WORD_OPTIONS), c.word_count)
            == std::end(WORD_OPTIONS))
    {
        options_list = { std::to_string(c.word_count) };
        selected_index = 0; // The custom number gets its own honest label.
    }

    return {
        { {"Time", "Words"}, static_cast<size_t>(c.mode) },
        { {"1k", "5k", "10k", "25k"}, static_cast<size_t>(c.difficulty) },
        { options_list, selected_index, OPTION_WEIGHT }
    };
}

void draw_selector(std::stringstream& frame, int row, int col, int width,
        const Selector& sel, bool focused)
{
    draw_box(frame, row, col, width, BAR_HEIGHT);

    const int inner = width - 2; // - 2 = border
    const int slot  = inner / int(sel.options.size());

    std::string content;
    for(size_t i = 0; i < sel.options.size(); i++)
    {
        const std::string& option = sel.options[i];
        const int text_w    = int(option.size());
        const int pad_left  = std::max(0, (slot - text_w) / 2);
        const int pad_right = std::max(0, slot - text_w - pad_left);

        const char* color = dim();
        if (i == sel.selected)
            color = focused ? highlight() : green();
        content += std::string(pad_left, ' ');
        content += std::string(color) + option + reset();
        content += std::string(pad_right, ' ');
    }
    draw(frame, {row + 1, col + 1}, content);
}

int selector_weight(const Selector& s)
{
    if(s.weight > 0)
        return s.weight;

    int longest = 0;
    for(const std::string& c : s.options)
        longest = std::max(longest, int(c.size()));
    return std::max(1, longest * int(s.options.size()));
}

void draw_selectors(std::stringstream& frame, Position box,
        const std::vector<Selector>& selectors, int counter_box_width, int focus = -1
){
    int n = int(selectors.size());
    if(n == 0)
        return;

    const int space = BOX_WIDTH - counter_box_width - SELECTOR_GAP * n;

    // how much room each selector wants
    std::vector<int> weight(n);
    int total = 0;
    for(int i = 0; i < n; i++)
    {
        weight[i] = selector_weight(selectors[i]);
        total += weight[i];
    }

    // Give every label its minimum width first; share the leftover room.
    std::vector<int> widths(n);
    int minimum = 0;
    for (int i = 0; i < n; ++i)
    {
        int longest = 0;
        for (const auto& option : selectors[i].options)
            longest = std::max(longest, int(option.size()));
        widths[i] = 2 + longest * int(selectors[i].options.size());
        minimum += widths[i];
    }
    const int extra = std::max(0, space - minimum);
    int used = minimum;
    for (int i = 0; i < n; ++i)
    {
        const int share = extra * weight[i] / total;
        widths[i] += share;
        used += share;
    }
    widths[n - 1] += space - used;

    int col = box.col + counter_box_width + SELECTOR_GAP;
    int row = box.row - BAR_HEIGHT;
    for(int i = 0; i < n; i++)
    {
        draw_selector(frame, row, col, widths[i], selectors[i], i == focus);
        col += widths[i] + SELECTOR_GAP;
    }
}

void draw_text(std::stringstream& frame, Position box, const Game& g)
{
    const int text_width = BOX_WIDTH - 2 - 2 * PADDING_X;
    const Layout lay     = layout(g, size_t(text_width));

    // scroll: keep the current line on the 2nd visible row, but never
    // scroll past the end of the text
    size_t first = lay.cursor_line > 0 ? lay.cursor_line - 1 : 0;
    if(lay.lines.size() > size_t(VISIBLE_LINES))
        first = std::min(first, lay.lines.size() - size_t(VISIBLE_LINES));
    else
        first = 0;

    const int text_row   = box.row + 1 + PADDING_Y;
    const int text_col   = box.col + 1 + PADDING_X;
    for(int i = 0; i < VISIBLE_LINES; i++)
    {
        const size_t idx = first + size_t(i);
        if(idx >= lay.lines.size())
            break;
        draw(frame, {text_row + i, text_col}, lay.lines[idx]);
    }
}

void draw_playing(std::stringstream& frame, const Game& g, const Config& c, Position size)
{
    const Position box = box_origin(size);
    draw_box(frame, box.row, box.col, BOX_WIDTH, BOX_HEIGHT);

    const std::vector<Selector> selectors = make_selectors(c);
    draw_counter(frame, box, g, c);

    // disappear when game start
    if(!g.start.has_value())
        draw_selectors(frame, box, selectors, counter_width(c));

    draw_text(frame, box, g);
}

void draw_config(std::stringstream& frame, const Game& g, const Config& c,
                 Position size, size_t focus, const ConfigEditor& editor)
{
    const Position box = box_origin(size);
    draw_box(frame, box.row, box.col, BOX_WIDTH, BOX_HEIGHT);
    draw_counter(frame, box, g, c);
    draw_selectors(frame, box, make_selectors(c), counter_width(c), int(focus));
    if (editor.editing_count)
    {
        const int row = box.row + 1 + PADDING_Y;
        const int col = box.col + 1 + PADDING_X;
        draw(frame, {row, col}, "Word count: ", green(),
             editor.count_input.empty() ? "_" : editor.count_input + "_", reset());
        draw(frame, {row + 1, col}, dim(), "Choose 1 to 10,000 words", reset());
        if (!editor.error.empty())
            draw(frame, {row + 3, col}, red(), editor.error, reset());
        draw(frame, {row + 5, col}, "Enter: save   Esc: cancel   Ctrl+N: save and start");
    }
    else
        draw_text(frame, box, g);
}

// ---------- footer ----------
void draw_ctrl(std::stringstream& frame, int row, int center_col,
               std::string_view key, std::string_view label)
{
    const int width = int(key.size()) + 1 + int(label.size());
    draw(frame, {row, center_col - width / 2}, green(), key, reset(), ' ', label);
}

void draw_footer(std::stringstream& frame, Position box, const Game& g, const Config& c, const State& s)
{
    const int row = box.row + BOX_HEIGHT + 1; // 2 rows below the box

    if(s == State::Playing && !g.start.has_value())
    {
        draw_ctrl(frame, row, box.col + BOX_WIDTH * 1 / 6, "ctrl+n", "new game");
        draw_ctrl(frame, row, box.col + BOX_WIDTH * 3 / 6, "ctrl+f", "config");
        draw_ctrl(frame, row, box.col + BOX_WIDTH * 5 / 6, "ctrl+c", "exit");
    }
    else if(s == State::Playing && g.start.has_value())
    {
        draw_ctrl(frame, row, box.col + BOX_WIDTH * 1 / 6, "esc", "restart");
        draw_ctrl(frame, row, box.col + BOX_WIDTH * 3 / 6, "ctrl+f", "config");
        draw_ctrl(frame, row, box.col + BOX_WIDTH * 5 / 6, "ctrl+c", "exit");
    }
    else if(s == State::Config)
    {
        draw_ctrl(frame, row, box.col + BOX_WIDTH * 1 / 6, "tab",  "new section");
        draw_ctrl(frame, row, box.col + BOX_WIDTH * 3 / 6, "</>",  "left/right");
        draw_ctrl(frame, row, box.col + BOX_WIDTH * 5 / 6, "esc",  "back");
        draw_ctrl(frame, row + 1, box.col + BOX_WIDTH / 2, "1-4",
                  c.mode == Mode::Words ? "difficulty   w: word count   Ctrl+N: start"
                                       : "difficulty   Ctrl+N: start");
    }
    else if(s == State::Results)
    {
        draw_ctrl(frame, row, box.col + BOX_WIDTH * 1 / 4, "ctrl+n", "new game");
        draw_ctrl(frame, row, box.col + BOX_WIDTH * 3 / 4, "ctrl+c", "exit");
    }
}

// ---------- result ----------
void draw_stat(std::stringstream& frame, int row, int mid_col,
               std::string_view label, const std::string& value)
{
    draw(frame, {row, mid_col - int(label.size()) - 1}, label, ':');
    draw(frame, {row, mid_col + 2}, green(), value, reset());
}

void draw_results(std::stringstream& frame, const Result& r, Position size)
{
    const Position box = box_origin(size);
    draw_box(frame, box.row, box.col, BOX_WIDTH, BOX_HEIGHT);

    // header
    const std::string header = std::string(to_string(r.difficulty)) + "      "
                             + std::to_string(std::lround(r.duration)) + "s      "
                             + r.date.substr(0, 10);
    draw(frame, {box.row - BAR_HEIGHT + 1,
                 box.col + (BOX_WIDTH - int(header.size())) / 2}, header);

    const int mid = box.col + BOX_WIDTH / 2;
    int row = box.row + 1 + PADDING_Y;
    draw_stat(frame, row++, mid, "wpm",       std::to_string(std::lround(r.wpm)));
    draw_stat(frame, row++, mid, "accuracy",  std::to_string(std::lround(r.accuracy)) + "%");
    draw_stat(frame, row++, mid, "correct",   std::to_string(r.chars.correct));
    draw_stat(frame, row++, mid, "incorrect", std::to_string(r.chars.incorrect));
    draw_stat(frame, row++, mid, "extra",     std::to_string(r.chars.extra));
    draw_stat(frame, row++, mid, "missed",    std::to_string(r.chars.missed));
}

}

void render(const Game& g, const Config& c, const State& s, const Result& r, size_t focus, const ConfigEditor& editor)
{
    clear_screen();
    const Position size = get_term_size();
    if (size.col < BOX_WIDTH || size.row < TOTAL_HEIGHT + 2)
    {
        std::cout << move_cursor({1, 1})
                  << "Resize terminal to at least 74 columns and 17 rows."
                  << "\nCtrl+C exits." << std::flush;
        return;
    }
    const Position box  = box_origin(size);
    std::stringstream frame;

    switch (s)
    {
        case State::Playing: draw_playing(frame, g, c, size);       break;
        case State::Results: draw_results(frame, r, size);          break;
        case State::Config:  draw_config(frame, g, c, size, focus, editor); break;
        case State::Exit:    break;
    }
    draw_footer(frame, box, g, c, s);
    std::cout << frame.str() << std::flush;
}

