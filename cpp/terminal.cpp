#include <iostream>
#include <string>
#include <unistd.h>
#include <sys/ioctl.h>
#include "terminal.h"
#include <poll.h>

namespace {
bool read_byte(char& c) { return read(STDIN_FILENO, &c, 1) > 0; }

}

void show_cursor()             { std::cout << "\033[?25h";   }
void hide_cursor()             { std::cout << "\033[?25l";   }
void enter_alternate_screen()  { std::cout << "\033[?1049h"; }
void exit_alternate_screen()   { std::cout << "\033[?1049l"; }
void clear_screen()            { std::cout << "\033[2J";     }
const char* green()            { return "\033[32m";          }
const char* red()              { return "\033[31m";          }
const char* white()            { return "\033[37m";          }
const char* reset()            { return "\033[0m";           }
const char* dim()              { return "\033[2m";           }
const char* highlight()        { return "\x1b[37;42m";       }

std::string move_cursor(Position pos)
{ 
    return "\033[" + std::to_string(pos.row) + ";" + std::to_string(pos.col) + "H"; 
}

// Returns the terminal size
Position get_term_size() // NOTE: terminal origin starts at 1, 1
{
    struct winsize w;
    if(ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == 0)
        return {w.ws_row, w.ws_col};
    std::cerr << "ERROR: Cannot get terminal size. \n";
    return {1, 1};
}  

void restore_terminal(const termios& terminal)
{
    show_cursor(); 
    clear_screen(); 
    tcsetattr(STDIN_FILENO, TCSANOW, &terminal);
    exit_alternate_screen();
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
    clear_screen();

    return orig;
}

int read_key()
{
    char ch;
    if(!read_byte(ch))
        return KEY_NONE;
    if(ch != '\033')
        return static_cast<unsigned char>(ch);

    if(!read_byte(ch)) // got ESC, is anything after it?
        return KEY_ESC; // if no, then base ESC

    if(ch != '[' && ch != 'O')
        return KEY_UNKNOWN; // alt+key, ignore

    // read until the final byte
    while(read_byte(ch))
    {
        if(ch >= '@' && ch <= '~')
        {
            switch(ch)
            {
                case 'A': return KEY_UP;
                case 'B': return KEY_DOWN;
                case 'C': return KEY_RIGHT;
                case 'D': return KEY_LEFT;
                case 'Z': return KEY_SHIFT_TAB;
                default:  return KEY_UNKNOWN;
            }
        }
    }
    return KEY_UNKNOWN;
}
