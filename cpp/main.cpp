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
#define FPS              60

struct Dimension{
    int row;
    int col;
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
constexpr char ctrl(char c) { return c & 0x1F; }

// Returns the terminal size
Dimension get_term_size() // NOTE: terminal top-right starts at 1, 1
{
    struct winsize w;
    if(ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == 0)
        return {w.ws_row, w.ws_col};
    std::cerr << "ERROR: Cannot get terminal size. \n";
    return {1, 1};
}  

std::vector<std::string> random_words(size_t count = 20)
{
    std::vector<std::string> words = {
        "Cat", "Dog", "Sun", "Map", "Bag", "Grab",
        "Animal", "Answer", "Camera", "Castle", "Garden", "Planet",
        "Absolute","Activity","Balanced","Campaign","Capacity","Concrete",
        "Constant", "Creative", "Database"
    };

    std::vector<std::string> picked_words;
    for(size_t i = 0; i < count; i++)
    {
        size_t rand_num = rand() % words.size();
        picked_words.push_back(words[rand_num]);
    }
    return picked_words;
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
    std::vector<std::string> random = random_words();
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

void initialize_game(const termios& orig) {
    srand(time(0));

    // turn off canonical mode and echo
    termios copy = orig;
    copy.c_lflag &= ~(ICANON | ECHO);
    copy.c_cc[VMIN]  = 0;
    copy.c_cc[VTIME] = 0;   // no timeout
    tcsetattr(STDIN_FILENO, TCSANOW, &copy);

    // use separate terminal for the game
    enter_alternate_screen();
    hide_cursor();
    clear();
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

// TODO: render each frame:
//   - hide cursor and clear the screen
//   - draw the word block (max MAX_CHAR_PER_ROW chars per row)
//   - highlight typed characters (green = correct, red = wrong)
//   - cap the frame rate
void draw()
{
    hide_cursor(); clear();
}

// TODO: initialize game state (score, terminal size, words)
//       then run the draw/input loop.
void game()
{
}

int main () {
    termios orig_term;
    tcgetattr(STDIN_FILENO, &orig_term);

    Dimension term_size = get_term_size();

    initialize_game(orig_term);
    print_screen();

    auto start = std::chrono::steady_clock::now();

    while (true) {
        char character = 0;
        ssize_t n = read(STDIN_FILENO, &character, 1);

        if(n > 0){
            if (character == 'q') break;
        }

        move_cursor(Dimension{1,1});
        std::cout << std::fixed << std::setprecision(2);
        std::cout << "Time: " << stopwatch(start) << " s" << std::flush;

        move_cursor(Dimension{1, term_size.col - 8});
        std::cout << "Input: " << character << std::flush; // flush is important
        std::this_thread::sleep_for(std::chrono::milliseconds(FPS));
    }
    game_over(orig_term);
    return 0;
}
