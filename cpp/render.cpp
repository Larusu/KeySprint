#include <algorithm>
#include <cmath>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>
#include "history.h"
#include "terminal.h"
#include "render.h"

namespace {

// main box
constexpr int BOX_WIDTH     = 74;
constexpr int VISIBLE_LINES = 6;
constexpr int PADDING_X     = 2;
constexpr int PADDING_Y     = 1;
constexpr int BOX_HEIGHT    = VISIBLE_LINES + 2 + 2 * PADDING_Y;
constexpr int BAR_HEIGHT    = 3;
constexpr int BEST_HEIGHT   = 3;
constexpr int TOTAL_HEIGHT  = BAR_HEIGHT + BEST_HEIGHT + BOX_HEIGHT + 2;

constexpr int SELECTOR_GAP      = 1;
// fixed weight so the bar doesn't resize when the mode changes
constexpr int OPTION_WEIGHT     = 3 * 4;

// smallest terminal we can draw in
constexpr int MIN_COLS = BOX_WIDTH;
constexpr int MIN_ROWS = TOTAL_HEIGHT + 2;

enum class Style : unsigned char { Normal, Green, Red, White, Dim, Highlight };

struct Cell {
    std::string glyph = " "; // one character, as UTF-8 (the box lines are 3 bytes)
    Style style = Style::Normal;
};

bool operator==(const Cell& a, const Cell& b)
{
    return a.style == b.style && a.glyph == b.glyph;
}

struct Screen {
    int rows = 0;
    int cols = 0;
    std::vector<Cell> back;   // being drawn this frame
    std::vector<Cell> front;  // what the terminal shows right now
    bool full_redraw = true;  // terminal content unknown: erase it and draw everything
};

// Call at the start of every frame: blank back buffer, (re)size it if needed.
void screen_begin(Screen& s, Position size)
{
    if(size.row != s.rows || size.col != s.cols)
    {
        s.rows = size.row;
        s.cols = size.col;
        s.back.assign(size_t(s.rows) * size_t(s.cols), Cell{});
        s.front = s.back;
        s.full_redraw = true; // after a resize the old content can't be trusted
    }
    else
    {
        std::fill(s.back.begin(), s.back.end(), Cell{});
    }
}

// Writes text starting at `at` (1-based, like move_cursor). One character takes
// one column; anything outside the screen is clipped.
void put(Screen& s, Position at, std::string_view text, Style style = Style::Normal)
{
    int col = at.col;
    for(size_t i = 0; i < text.size(); col++)
    {
        const unsigned char lead = static_cast<unsigned char>(text[i]);
        size_t len = 1; // UTF-8: the first byte tells how many bytes the character has
        if(lead >= 0xF0)      len = 4;
        else if(lead >= 0xE0) len = 3;
        else if(lead >= 0xC0) len = 2;
        len = std::min(len, text.size() - i);

        if(at.row >= 1 && at.row <= s.rows && col >= 1 && col <= s.cols)
        {
            Cell& cell = s.back[size_t(at.row - 1) * size_t(s.cols) + size_t(col - 1)];
            // a control character would move the cursor and wreck the picture
            cell.glyph = (lead < 0x20 || lead == 0x7F) ? "?" : std::string(text.substr(i, len));
            cell.style = style;
        }
        i += len;
    }
}

const char* style_code(Style style)
{
    switch(style)
    {
        case Style::Green:     return green();
        case Style::Red:       return red();
        case Style::White:     return white();
        case Style::Dim:       return dim();
        case Style::Highlight: return highlight();
        case Style::Normal:    break;
    }
    return reset();
}

// Escape codes that turn what the terminal shows into the back buffer
// (empty string = nothing changed). Afterwards `front` is up to date.
std::string screen_flush(Screen& s)
{
    std::string out;
    if(s.full_redraw)
    {
        out += erase_screen();
        std::fill(s.front.begin(), s.front.end(), Cell{}); // blank, so only non-blank cells get drawn
        s.full_redraw = false;
    }

    Style current = Style::Normal; // every frame ends with a reset, so we start from Normal
    int cur_row = -1;              // where the terminal cursor is, -1 = unknown
    int cur_col = -1;
    for(int r = 1; r <= s.rows; r++)
    {
        for(int c = 1; c <= s.cols; c++)
        {
            const size_t i = size_t(r - 1) * size_t(s.cols) + size_t(c - 1);
            const Cell& want = s.back[i];
            if(want == s.front[i])
                continue;

            // a run of changed cells in a row needs only one cursor move
            if(r != cur_row || c != cur_col)
                out += move_cursor({r, c});
            if(want.style != current)
            {   // styles add up, so always reset before choosing the next one
                out += reset();
                if(want.style != Style::Normal)
                    out += style_code(want.style);
                current = want.style;
            }
            out += want.glyph;

            cur_row = r;
            // after the last column the cursor position is ambiguous (pending wrap)
            cur_col = (c < s.cols) ? c + 1 : -1;
        }
    }
    if(current != Style::Normal)
        out += reset();

    s.front.swap(s.back); // the back buffer is blanked again at the start of the next frame
    if(out.empty())
        return out;
    return sync_begin() + out + sync_end();
}

// Kept between frames: the diff needs to know what the terminal shows.
Screen live_screen;
Position notice_size{0, 0}; // terminal size when the "too small" message was drawn, 0 = not shown

// ---------- text ----------

struct Line { size_t first, last; }; // words [first, last) share one line

struct Layout {
    std::vector<Line> lines;
    size_t cursor_line = 0;
};

char printable(char c)
{
    const unsigned char u = static_cast<unsigned char>(c);
    return (u >= 0x20 && u < 0x7F) ? c : '?';
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

void draw_box(Screen& screen, int row, int col, int width, int height)
{
    put(screen, {row, col}, "╭" + repeat("─", width - 2) + "╮");
    for(int r = 1; r < height - 1; r++)
    {
        put(screen, {row + r, col},             "│");
        put(screen, {row + r, col + width - 1}, "│");
    }
    put(screen, {row + height - 1, col}, "╰" + repeat("─", width - 2) + "╯");
}

size_t word_width(const Game& g, size_t i)
{ return std::max(g.words[i].size(), g.typed[i].size()); }

Layout layout(const Game& game, size_t max_width)
{
    Layout out;
    size_t first = 0;    // the line being built holds words [first, i)
    size_t width = 0;

    for (size_t i = 0; i < game.words.size(); i++)
    {
        const size_t w = word_width(game, i);
        if (i > first && width + 1 + w > max_width)      // doesn't fit: close the line
        {
            out.lines.push_back({first, i});
            if (i > game.cursor && out.lines.size() > out.cursor_line + VISIBLE_LINES)
                return out;                              // enough lines below the cursor
            first = i;
        }
        width = (i == first) ? w : width + 1 + w;
        if (i == game.cursor)
            out.cursor_line = out.lines.size();
    }
    if (first < game.words.size())
        out.lines.push_back({first, game.words.size()});
    if (game.cursor >= game.words.size() && !out.lines.empty())
        out.cursor_line = out.lines.size() - 1;
    return out;
}

// Draws one word at (row, col), clipped at end_col. Returns the column after it.
int draw_word(Screen& screen, int row, int col, int end_col,
              const std::string& target, const std::string& typed, bool passed)
{
    const size_t len = std::max(target.size(), typed.size());
    for (size_t j = 0; j < len && col < end_col; j++, col++)
    {
        Style style;
        if (j >= target.size())     style = Style::Red;                         // extra
        else if (j < typed.size())  style = target[j] == typed[j] ? Style::Green : Style::Red;
        else                        style = passed ? Style::Red : Style::White; // skipped / not typed yet

        const char shown = printable(j < target.size() ? target[j] : typed[j]);
        put(screen, {row, col}, std::string_view(&shown, 1), style);
    }
    return col;
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

void draw_counter(Screen& screen, Position box, const Game& g, const Config& c)
{
    const int bar_row = box.row - BAR_HEIGHT;
    const int width = counter_width(c);
    draw_box(screen, bar_row, box.col, width, BAR_HEIGHT);

    std::string text;
    if(c.mode == Mode::Time)
        text = std::to_string(int(std::ceil(time_left(g, c))));
    else
        text = std::to_string(g.cursor) + "/" + std::to_string(g.words.size());
    const int pad = std::max(0, (width - 2 - int(text.size())) / 2);
    put(screen, {bar_row + 1, box.col + 1 + pad}, text);
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

// Carousel of presets. A custom word count that isn't a preset is shown alone.
template <size_t N>
Selector preset_selector(const int (&values)[N], int current)
{
    Selector s{ {}, N, OPTION_WEIGHT };               // selected == N means "not found yet"
    for (size_t i = 0; i < N; i++)
    {
        s.options.push_back(std::to_string(values[i]));
        if (values[i] == current)
            s.selected = i;
    }
    if (s.selected == N)
        s = { { std::to_string(current) }, 0, OPTION_WEIGHT };
    return s;
}

std::vector<Selector> make_selectors(const Config& c)
{
    Selector difficulty{ {}, size_t(c.difficulty) };
    for (const DifficultyInfo& d : DIFFICULTIES)
        difficulty.options.emplace_back(d.label);

    return {
        { {"Time", "Words"}, size_t(c.mode) },
        difficulty,
        c.mode == Mode::Time ? preset_selector(TIME_OPTIONS, c.time_limit)
                             : preset_selector(WORD_OPTIONS, c.word_count),
    };
}

void draw_selector(Screen& screen, int row, int col, int width,
        const Selector& sel, bool focused)
{
    draw_box(screen, row, col, width, BAR_HEIGHT);

    const int inner = width - 2; // - 2 = border
    const int slot  = inner / int(sel.options.size());

    int x = col + 1;
    for(size_t i = 0; i < sel.options.size(); i++)
    {
        const std::string& option = sel.options[i];
        const int text_w    = int(option.size());
        const int pad_left  = std::max(0, (slot - text_w) / 2);
        const int pad_right = std::max(0, slot - text_w - pad_left);

        Style style = Style::Dim;
        if (i == sel.selected)
            style = focused ? Style::Highlight : Style::Green;
        put(screen, {row + 1, x + pad_left}, option, style);
        x += pad_left + text_w + pad_right;
    }
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

void draw_selectors(Screen& screen, Position box,
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
        draw_selector(screen, row, col, widths[i], selectors[i], i == focus);
        col += widths[i] + SELECTOR_GAP;
    }
}

void draw_text(Screen& screen, Position box, const Game& g)
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

    const int text_row = box.row + 1 + PADDING_Y;
    const int text_col = box.col + 1 + PADDING_X;
    for (int i = 0; i < VISIBLE_LINES && first + size_t(i) < lay.lines.size(); i++)
    {
        const Line& line = lay.lines[first + size_t(i)];
        int col = text_col;
        for (size_t w = line.first; w < line.last; w++)
            col = draw_word(screen, text_row + i, col, text_col + text_width,
                            g.words[w], g.typed[w], w < g.cursor) + 1;   // +1 = the space
    }
}

void draw_best(Screen& screen, Position box, const HistoryStore& hs, const Config& c)
{
    const int bar_row = box.row + BOX_HEIGHT;
    draw_box(screen, bar_row, box.col, BOX_WIDTH, BEST_HEIGHT);

    const History best = best_for(hs, c);
    const std::pair<std::string_view, std::string> stats[] = {
        {"Best WPM:", std::to_string(std::lround(best.wpm))},
        {"Accuracy:", std::to_string(std::lround(best.accuracy)) + "%"},
        {"Attempts:", std::to_string(hs.attempts)}
    };

    const int n = int(std::size(stats));
    const int row = bar_row + BEST_HEIGHT / 2;
    for(int i = 0; i < n; i++)
    {
        const auto& [label, value] = stats[i];
        const int width  = int(label.size()) + 1 + int(value.size());
        const int center = box.col + BOX_WIDTH * (2 * i + 1) / (2 * n);
        put(screen, {row, center - width / 2}, label, Style::Normal);
        put(screen, {row, center - width / 2 + int(label.size()) + 1}, value, Style::Green);
    }
}

void draw_playing(Screen& screen, const Game& g, const Config& c, Position size)
{
    const Position box = box_origin(size);
    draw_box(screen, box.row, box.col, BOX_WIDTH, BOX_HEIGHT);

    const std::vector<Selector> selectors = make_selectors(c);
    draw_counter(screen, box, g, c);

    // disappear when game start
    if(!g.start.has_value())
        draw_selectors(screen, box, selectors, counter_width(c));

    draw_text(screen, box, g);
}

void draw_config(Screen& screen, const Game& g, const Config& c,
                 Position size, size_t focus, const ConfigEditor& editor)
{
    const Position box = box_origin(size);
    draw_box(screen, box.row, box.col, BOX_WIDTH, BOX_HEIGHT);
    draw_counter(screen, box, g, c);
    draw_selectors(screen, box, make_selectors(c), counter_width(c), int(focus));
    if (editor.editing_count)
    {
        const std::string_view label = "Word count: ";
        const int row = box.row + 1 + PADDING_Y;
        const int col = box.col + 1 + PADDING_X;
        put(screen, {row, col}, label);
        put(screen, {row, col + int(label.size())},
            editor.count_input.empty() ? "_" : editor.count_input + "_", Style::Green);
        put(screen, {row + 1, col}, "Choose 1 to 10,000 words", Style::Dim);
        if (!editor.error.empty())
            put(screen, {row + 3, col}, editor.error, Style::Red);
        put(screen, {row + 5, col}, "Enter: save   Esc: cancel   Ctrl+N: save and start");
    }
    else
        draw_text(screen, box, g);
}

// ---------- footer ----------
struct Hint { std::string_view key, label; };

// Spaces the hints evenly: hint i of n is centered at (2i+1)/(2n) of the box width.
void draw_hints(Screen& screen, Position box, int row, std::initializer_list<Hint> hints)
{
    const int n = int(hints.size());
    int i = 0;
    for (const Hint& h : hints)
    {
        const int center = box.col + BOX_WIDTH * (2 * i++ + 1) / (2 * n);
        const int col    = center - (int(h.key.size()) + 1 + int(h.label.size())) / 2;
        put(screen, {row, col}, h.key, Style::Green);
        put(screen, {row, col + int(h.key.size()) + 1}, h.label);
    }
}

void draw_footer(Screen& screen, Position box, const Game& g, const Config& c, State s)
{
    const int row = box.row + BOX_HEIGHT + BEST_HEIGHT + 1;

    switch (s)
    {
        case State::Playing:
            if (g.start)
                draw_hints(screen, box, row - BEST_HEIGHT, {{"esc", "restart"}});
            else
                draw_hints(screen, box, row, {{"ctrl+n", "new game"}, 
                        {"ctrl+f", "config"}, {"ctrl+c", "exit"}});
            break;
        case State::Config:
        {
            draw_hints(screen, box, row, {{"tab", "new section"}, 
                    {"</>", "left/right"}, {"esc", "back"}});
            const int row2 = std::min(row + 2, screen.rows);
            if (c.mode == Mode::Words)
                draw_hints(screen, box, row2, {{"1-4", "difficulty"}, {"w", "word count"}});
            else
                draw_hints(screen, box, row2, {{"1-4", "difficulty"}});
            break;
        }
        case State::Results:
            draw_hints(screen, box, row, {{"ctrl+n", "new game"}, 
                                                        {"ctrl+c", "exit"}});
            break;
        case State::Exit:
            break;
    }
}
// ---------- result ----------
void draw_results(Screen& screen, const Result& r, const HistoryStore& hs, 
        const Config& c, Position size)
{
    const Position box = box_origin(size);
    draw_box(screen, box.row, box.col, BOX_WIDTH, BOX_HEIGHT);

    const std::string header = std::string(difficulty_info(r.difficulty).title) + "      "
                             + std::to_string(std::lround(r.duration)) + "s      "
                             + r.date.substr(0, 10);
    put(screen, {box.row - BAR_HEIGHT + 1,
                 box.col + (BOX_WIDTH - int(header.size())) / 2}, header);

    std::string crown = r.new_best ? " new best" : ""; // " ★"
    const std::pair<std::string_view, std::string> stats[] = {
        {"wpm:",       std::to_string(std::lround(r.wpm)) + crown},
        {"accuracy:",  std::to_string(std::lround(r.accuracy)) + "%"},
        {"correct:",   std::to_string(r.chars.correct)},
        {"incorrect:", std::to_string(r.chars.incorrect)},
        {"extra:",     std::to_string(r.chars.extra)},
        {"missed:",    std::to_string(r.chars.missed)},
    };

    const int mid = box.col + BOX_WIDTH / 2;
    int row = box.row + 1 + PADDING_Y;
    for (const auto& [label, value] : stats)
    {
        put(screen, {row, mid - int(label.size())}, label);   // the label includes its colon
        put(screen, {row, mid + 2}, value, Style::Green);
        row++;
    }
    draw_best(screen, box, hs, c);
}
}

void render(const Game& g, const Config& c, const State& s, const Result& r, 
        size_t focus, const ConfigEditor& editor, const HistoryStore& hs)
{
    const Position size = get_term_size();
    if (size.col < MIN_COLS || size.row < MIN_ROWS)
    {
        live_screen.full_redraw = true; // this message replaces whatever was on screen
        // Draw it once per size, not every frame: redrawing it would blink as well.
        if (size.row != notice_size.row || size.col != notice_size.col)
        {
            clear_screen();
            std::cout << move_cursor({1, 1})
                      << "Resize terminal to at least " << MIN_COLS << " columns and "
                      << MIN_ROWS << " rows."
                      << "\nCtrl+C exits." << std::flush;
            notice_size = size;
        }
        return;
    }
    notice_size = {0, 0};

    screen_begin(live_screen, size);
    const Position box = box_origin(size);

    switch (s)
    {
        case State::Playing: draw_playing(live_screen, g, c, size);       break;
        case State::Results: draw_results(live_screen, r, hs, c, size);   break;
        case State::Config:  draw_config(live_screen, g, c, size, focus, editor); break;
        case State::Exit:    break;
    }
    draw_footer(live_screen, box, g, c, s);

    if(!g.start.has_value())
        draw_best(live_screen, box, hs, c);

    const std::string update = screen_flush(live_screen);
    if (!update.empty())
        write_out(update);
}
