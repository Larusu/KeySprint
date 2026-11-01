#include <cassert>
#include <cstdlib>
#include <iostream>
#include <string>
#include <sys/ioctl.h>
#include <unistd.h>
#include <termios.h>
#include <vector>
#include <ctime>

#define MAX_CHAR_PER_ROW 50

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

// Returns the terminal size
Dimension get_term_size() // NOTE: terminal top-right starts at 1, 1
{
    struct winsize w;
    if(ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == 0)
        return {w.ws_row, w.ws_col};
    std::cerr << "ERROR: Cannot get terminal size. \n";
    return {1, 1};
}  

std::vector<std::string> random_words(int count = 20)
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
        picked_words.push_back(words[rand() % words.size()]);
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

// TODO: read input one keypress at a time (termios)
void validate_input()
{
    // check per character input
}

// TODO: restore the terminal, print the final score, and exit
void game_over()
{
    show_cursor(); clear(); 
    move_cursor(Dimension{1, 1});
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
    srand(time(0));
    enter_alternate_screen();
    print_screen();
    show_cursor();
    // TODO: call exit_alternate_screen() once the game loop is in place
    return 0;
}
