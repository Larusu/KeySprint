#include "main.h"
#include <cctype>
#include <ctime>
#include <iomanip>
#include <sstream>

int main () 
{
    srand(time(0));
    termios orig = enable_raw_mode();

    Config config = { get_word_list(), 5, Difficulty::Easy, 10 };
    Game game = new_game(config);
    State state = State::Playing;
    Result result = {};

    while(state != State::Exit)
    { run(config, game, state, result); }

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

bool is_over(const Game& g, const Config& c) 
{
    size_t last = g.words.size() - 1;
    return g.cursor >= g.words.size() // passed the last word
        || (g.cursor == last && g.typed[last] == g.words[last]) // correct + last word
        || time_left(g, c) <= 0.0; // out of time dnkeew eht
}

double time_left(const Game& g, const Config& c)
{
    if(!g.start)
        return c.time_limit;
    double elapsed = std::chrono::duration<double>(Clock::now() - *g.start).count();
    return std::max(0.0, c.time_limit - elapsed);
}

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

// Returns the terminal size
Position get_term_size() // NOTE: terminal top-right starts at 1, 1
{
    struct winsize w;
    if(ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == 0)
        return {w.ws_row, w.ws_col};
    std::cerr << "ERROR: Cannot get terminal size. \n";
    return {1, 1};
}  

// TODO: print the final score
void game_over(const Game& game, const termios& terminal)
{
    show_cursor(); 
    clear(); 
    tcsetattr(STDIN_FILENO, TCSANOW, &terminal);
    exit_alternate_screen();
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

Result make_result(const Game& game, const Config& config)
{
    if(!is_over(game, config))
        return {};

    Result result{};
    Stats totals = compute_stats(game);

    // calculate duration
    double duration = 0.0;
    if(game.start)
    {
        double lap = std::chrono::duration<double>(Clock::now() - *game.start).count();
        duration = std::min(lap, static_cast<double>(config.time_limit));
    }
    result.duration = duration;

    // calculate accuracy
    int overall = game.stats.correct + game.stats.incorrect + game.stats.extra;
    result.accuracy = overall != 0 ? double(game.stats.correct) / overall * 100
                                   : 0.0;

    // calculate WPM
    constexpr double CHARS_PER_WORD   = 5.0;
    constexpr double SECONDS_PER_MIN  = 60.0;
    double wpm = 0.0;
    if(duration != 0.0)
        wpm = (double(totals.correct) / CHARS_PER_WORD) / (duration / SECONDS_PER_MIN);
    result.wpm = wpm;

    // difficulty
    result.difficulty = config.difficulty;

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

template <typename... Args>
void draw(std::stringstream& output, Position d, Args&&... args)
{
    output << move_cursor(d);
    output << std::fixed << std::setprecision(2);
    (output << ... << std::forward<Args>(args));
}

void render(const Game& g, const Config& c, const State& state, const Result& result)
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
        Stats final = compute_stats(g);
        // WARNING: Temporary draw
        draw(frame, {size.row / 2 - 2, size.col / 2 - 6}, "Round over!");
        draw(frame, {size.row / 2,     size.col / 2 - 8}, "WPM = ",        result.wpm);
        draw(frame, {size.row / 2 + 1, size.col / 2 - 8}, "Accuracy = ",   result.accuracy);
        draw(frame, {size.row / 2 + 2, size.col / 2 - 8}, "Duration = ",   result.duration);
        draw(frame, {size.row / 2 + 3, size.col / 2 - 8}, "Difficulty = ", to_string(result.difficulty));
        draw(frame, {size.row / 2 + 4, size.col / 2 - 8}, "Time = ", result.date);
    }
    
    Position below = {size.row - 5, size.col / 2};
    int one_fourth = below.col / 2;
    draw(frame, {below.row, below.col - one_fourth - 7}, "[Ctrl / C] Exit");
    draw(frame, {below.row, below.col - 9},              "[Ctrl / N] New Game");
    draw(frame, {below.row, below.col + one_fourth - 8}, "[Ctrl / F] Config");

    std::cout << frame.str() << std::flush;
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
    if(typed.size() >= target.size() + MAX_EXTRA)
        return;

    if (typed.size() >= target.size())
        g.stats.extra++;
    else if (target[typed.size()] == ch)
        g.stats.correct++;
    else
        g.stats.incorrect++;

    typed += ch;
}

void run(const Config& config, Game& game, State& state, Result& result)
{
    auto next_frame = Clock::now();

    char ch;
    while (read(STDIN_FILENO, &ch, 1) > 0)
    {
        if (ch == ctrl('c')) 
        { // exit game
            state = State::Exit; 
            return;
        }
        else if(ch == ctrl('n') && state != State::Playing) 
        { // play again
            state = State::Playing; 
            game = new_game(config); 
        }
        else if(ch == ctrl('f') && state != State::Config)
        { // change config
            state = State::Config;
        }
        
        if(state == State::Playing)
        {
            if (!game.start && std::isalnum(static_cast<unsigned char>(ch)))
                game.start = Clock::now();
            key_handler(game, ch);
        }
    }
    
    if(state == State::Playing && is_over(game, config)) 
    {
        result = make_result(game, config);
        state = State::Results;
    }
    
    render(game, config, state, result);

    next_frame += FRAME_TIME;
    std::this_thread::sleep_until(next_frame);
}
