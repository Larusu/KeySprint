#include "main.h"
#include <cctype>
#include <sstream>

int main () 
{
    srand(time(0));
    termios orig = enable_raw_mode();

    Config config = { get_word_list(), 20, Difficulty::Easy, 30 };
    Game game = new_game(config);
    State state = State::Playing;

    while(state != State::Exit)
    { run(config, game, state); }

    game_over(game, orig);
    std::cout << "Thank you for playing!";
    return 0;
}

// ANSI Escape helpers
std::string move_cursor(Position d)
{ return "\033[" + std::to_string(d.row) + ";" + std::to_string(d.col) + "H"; }
void show_cursor()             { std::cout << "\033[?25h";   }
void hide_cursor()             { std::cout << "\033[?25l";   }
void enter_alternate_screen()  { std::cout << "\033[?1049h"; }
void exit_alternate_screen()   { std::cout << "\033[?1049l"; }
void clear()                   { std::cout << "\033[2J";     }
const char* green()            { return "\033[32m"; }
const char* red()              { return "\033[31m"; }
const char* white()            { return "\033[37m"; }
const char* reset()            { return "\033[0m";  }
int calculateWPM() { return }

// Returns the terminal size
Position get_term_size() // NOTE: terminal top-right starts at 1, 1
{
    struct winsize w;
    if(ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == 0)
        return {w.ws_row, w.ws_col};
    std::cerr << "ERROR: Cannot get terminal size. \n";
    return {1, 1};
}  

std::vector<std::string>get_word_list()
{
    // TODO: change after json file is implemented
    return {
        "Cat", "Dog", "Sun", "Map", "Bag", "Grab",
        "Animal", "Answer", "Camera", "Castle", "Garden", "Planet",
        "Absolute","Activity","Balanced","Campaign","Capacity","Concrete",
        "Constant", "Creative", "Database"
    };
}

std::vector<std::string>pick_random_words(const Config& c)
{
    std::vector<std::string> picked;
    if(c.word_list.empty()) 
        return picked;

    picked.reserve(c.word_count);
    for(size_t i = 0; i < c.word_count; i++)
        picked.push_back(c.word_list[static_cast<size_t>(rand()) % c.word_list.size()]);
    return picked;
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

        int max_len = std::max(target.size(), typed.size());
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
                color = (i < game.cursor) ? red() : white(); 
                shown = target[j];
            }

            line += std::string(color) + shown + reset();
        }
        line_width += max_len;
    }

    if(!line.empty())
        lines.push_back(line);

    return lines;
}

termios enable_raw_mode()
{
    termios orig;
    tcgetattr(STDIN_FILENO, &orig);

    // turn off canonical mode and echo
    termios copy = orig;
    copy.c_lflag &= ~(ICANON | ECHO | ISIG);
    copy.c_cc[VMIN]  = 0;
    copy.c_cc[VTIME] = 0;   // no timeout
    tcsetattr(STDIN_FILENO, TCSANOW, &copy);

    // use separate terminal for the game
    enter_alternate_screen();
    hide_cursor();
    clear();

    return orig;
}

Game new_game(const Config& config) 
{
    return Game {
        pick_random_words(config),                   // words
        std::vector<std::string>(config.word_count), // typed
        0,                                           // cursor
        {},
        {}
    };
}

template <typename... Args>
void draw(std::stringstream& output, Position d, Args&&... args)
{
    output << move_cursor(d);
    output << std::fixed << std::setprecision(2);
    (output << ... << std::forward<Args>(args));
}

void render(const Game& g, const Config& c, const State& state)
{
    clear();
    
    Position size = get_term_size();
    std::stringstream frame;

    if(state == State::Playing)
    {
        draw(frame, {1, 1}, "Time: ", time_left(g, c));
        if(g.typed.size() > g.cursor)
            draw(frame, {1, size.col / 2}, g.typed[g.cursor]);

        std::vector<std::string> split = layout(g);
        for (size_t i = 0; i < split.size(); i++) {
            int col = (size.col - MAX_CHAR_PER_ROW) / 2;
            int row = size.row / 2 - 2;
            draw(frame, {row + int(i), col}, split[i]);
        }
    }
    else if(state == State::Results)
    {
        // WARNING: Temporary draw
        draw(frame, {size.row / 2 - 2, size.col / 2 - 6}, "Round over!");
        draw(frame, {size.row / 2,     size.col / 2 - 8}, "Correct = ",   g.stats.correct);
        draw(frame, {size.row / 2 + 1, size.col / 2 - 8}, "Incorrect = ", g.stats.incorrect);
        draw(frame, {size.row / 2 + 2, size.col / 2 - 8}, "Missed = ",    g.stats.missed);
        draw(frame, {size.row / 2 + 3, size.col / 2 - 8}, "Extra = ",     g.stats.extra);
    }

    std::cout << frame.str() << std::flush;

}

bool is_over(const Game& g, const Config& c) 
{ return g.cursor >= g.words.size() || time_left(g, c) <= 0.0; }

double time_left(const Game& g, const Config& c)
{
    if(!g.start)
        return c.time_limit;
    double elapsed = std::chrono::duration<double>(Clock::now() - *g.start).count();
    return std::max(0.0, c.time_limit - elapsed);
}

// TODO: print the final score
void game_over(const Game& game, const termios& terminal)
{
    show_cursor(); 
    clear(); 
    tcsetattr(STDIN_FILENO, TCSANOW, &terminal);
    exit_alternate_screen();
}

void key_handler(Game& g, char ch)
{
    if (!g.start) // if game hasnt started yet
        return;

    // ignore any key press and input after last word
    if (!g.start || g.cursor >= g.words.size()) 
        return;

    std::string& typed = g.typed[g.cursor];
    const std::string& target = g.words[g.cursor];

    if(ch == ' ')
    {
        if(typed.empty()) // prevent spam
            return;
        if(typed.size() < target.size())
            g.stats.missed += target.size() - typed.size();
        g.cursor++;
        return;
    }
    else if(ch == 127 || ch == 8) // backspace
    {
        if(!typed.empty())
            typed.pop_back(); 
        else if(g.cursor > 0 && g.typed[g.cursor - 1] == g.words[g.cursor - 1])
            return;
        else if(g.cursor > 0)
            g.cursor--; // cannot backspace if the previous word is correct
        return;
    }

    // limit number of typed key
    if(typed.size() > target.size() + MAX_EXTRA)
        return;

    if (typed.size() >= target.size())
        g.stats.extra++;
    else if (target[typed.size()] == ch)
        g.stats.correct++;
    else
        g.stats.incorrect++;

    typed += ch;
}

void run(const Config& config, Game& game, State& state)
{
    auto next_frame = Clock::now();

    // 1. input: drain everything typed since the last frame
    char ch;
    while (read(STDIN_FILENO, &ch, 1) > 0)
    {
        if (ch == ctrl('c')) { state = State::Exit; return; }
        
        if(state == State::Playing)
        {
            if (!game.start && std::isalnum(static_cast<unsigned char>(ch)))
                game.start = Clock::now();
        }

        key_handler(game, ch);
    }
    
    if(state == State::Playing && is_over(game, config)) 
        state = State::Results;
    
    render(game, config, state);

    next_frame += FRAME_TIME;
    std::this_thread::sleep_until(next_frame);
}
