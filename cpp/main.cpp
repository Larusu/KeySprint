#include <cassert>
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <string>
#include <sys/ioctl.h>
#include <thread>
#include <unistd.h>
#include <termios.h>
#include <vector>
#include <ctime>
#include <iomanip>

#define MAX_CHAR_PER_ROW 50
constexpr int FPS = 60;
constexpr std::chrono::milliseconds FRAME_TIME(1000 / FPS);
constexpr char ctrl(char c) { return c & 0x1F; }

enum class Difficulty { Easy, Medium, Hard };

struct Dimension {
    int row;
    int col;
};

struct Game {
    std::vector<std::string> word_list; // entire dictionary
    int word_count;                     // max word count. no limit if time-based
    Difficulty difficulty;              // 0 easy, 1 medium, 2 hard
    uint8_t time;                       // seconds - max of 120 seconds
};

struct Stats {
    int correct; 
    int incorrect;
    int extra;
    int missed;
};

struct Player {
    int attempts;
    int best_WPM;
    int best_accuracy;
};

// ANSI Escape helpers
void move_cursor(Dimension d)  { std::cout << "\033["<<d.row<<";"<<d.col<<"H"; }
void show_cursor()             { std::cout << "\033[?25h";                     }
void hide_cursor()             { std::cout << "\033[?25l";                     }
void enter_alternate_screen()  { std::cout << "\033[?1049h";                   }
void exit_alternate_screen()   { std::cout << "\033[?1049l";                   }
void clear()                   { std::cout << "\033[2J";                       }
const char* green()            { return "\033[32m"; }
const char* red()              { return "\033[31m"; }
const char* reset()            { return "\033[0m";  }

// Returns the terminal size
Dimension get_term_size() // NOTE: terminal top-right starts at 1, 1
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

std::vector<std::string>pick_random_words(
        const std::vector<std::string>& word_list,
        size_t word_count = 20
){
    std::vector<std::string> picked;
    if(word_list.empty()) 
        return picked;

    picked.reserve(word_count);
    for(size_t i = 0; i < word_count; i++)
        picked.push_back(word_list[rand() % word_list.size()]);
    return picked;
}

// Joins words into lines of at most MAX_CHAR_PER_ROW characters
std::vector<std::string> split_words(
        const std::vector<std::string>& picked_words
){
    std::vector<std::string> split_word;

    size_t char_count = 0;
    std::string new_word = "";

    for(const std::string& word : picked_words)
    {
        size_t space = split_word.empty() ? 0 : 1;

        if(word.length() + char_count + space > MAX_CHAR_PER_ROW)
        {
            split_word.push_back(new_word);
            char_count = 0;
            new_word = "";
        }

        if(!new_word.empty())
            new_word += " ";

        char_count += word.length() + space;
        new_word += word;
    }

    if (!new_word.empty()) {
        split_word.push_back(new_word);
    }

    return split_word;
}

// Temporary: renders the word block centered on screen
// TODO: fold into draw() once the game loop exists
void print_screen()
{
    std::vector<std::string> random = pick_random_words(get_word_list(), 50);
    std::vector<std::string> split = split_words(random);
    Dimension d = get_term_size();

    int start_row = (d.row - split.size()) / 2;
    int terminal_width = d.col;

    d.row = (d.row - split.size()) / 2;
    for(size_t i = 0; i < split.size(); i++)
    {
        d.col = (terminal_width - split[i].size()) / 2;
        d.row = start_row + i;
        move_cursor(d);
        std::cout << split[i];
    }

    std::cout << std::flush;
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

    return orig;
}

Game new_game() {
    srand(time(0));

    // use separate terminal for the game
    enter_alternate_screen();
    hide_cursor();
    clear();

    // load new game 
    std::vector<std::string> words = get_word_list();
    Game game = {
        pick_random_words(words, 50), // word_list
        200,                          // word_count
        Difficulty::Easy,             // difficulty duh
        30,                           // time - seconds
    };

    return game;
}

void draw(std::string str, Dimension d)
{
    clear();
    move_cursor(d);
    std::cout << std::fixed << std::setprecision(2);
    std::cout << str;
}

double stopwatch(std::chrono::steady_clock::time_point start)
{
    auto now = std::chrono::steady_clock::now();
    return std::chrono::duration<double>(now - start).count();
}

// TODO: read input one keypress at a time (termios)
void validate_input()
{
    // check per character input
}

// TODO: print the final score
void game_over(const termios& terminal)
{
    show_cursor(); 
    clear(); 
    tcsetattr(STDIN_FILENO, TCSANOW, &terminal);
    exit_alternate_screen();
}


// TODO: initialize game state (score, terminal size, words)
//       then run the draw/input loop.
void game()
{
}

int main () {
    termios orig = enable_raw_mode();

    Game game = new_game();
    Dimension term_size = get_term_size();
    auto start = std::chrono::steady_clock::now();

    auto next_frame = std::chrono::steady_clock::now();

    while (true) {
        char ch = 0;
        ssize_t n = read(STDIN_FILENO, &ch, 1);

        if(n > 0){
            if (ch == ctrl('c')) break;
        }

        draw("Time: " + std::to_string(stopwatch(start)), Dimension{1, 1});

        next_frame += FRAME_TIME;
        move_cursor(Dimension{1, term_size.col - 8});
        std::cout << "Input: " << ch << std::flush; // flush is important
        std::this_thread::sleep_until(next_frame);
    }

    game_over(orig);
    std::cout << "Thank you for playing!";
    return 0;
}
