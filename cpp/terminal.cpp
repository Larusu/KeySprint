#include <iostream>
#include <string>
#include "terminal.h"
#ifdef _WIN32
    #define WIN32_LEAN_AND_MEAN
    #define NOMINMAX
    #include <windows.h>
    #ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
        #define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004
    #endif
#else
    #include <unistd.h>
    #include <sys/ioctl.h>
#endif


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

// ================= Windows =================
#ifdef _WIN32

Position get_term_size() 
{
    CONSOLE_SCREEN_BUFFER_INFO info;
    if(GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &info))
    {   // srWindow = visible area (dwSize is the whole scrollback buffer)
        return { info.srWindow.Bottom - info.srWindow.Top  + 1,
                 info.srWindow.Right  - info.srWindow.Left + 1 };
    }
    std::cerr << "ERROR: Cannot get terminal size. \n";
    return {1, 1};
}

TerminalState enable_raw_mode()
{
    HANDLE in  = GetStdHandle(STD_INPUT_HANDLE);
    HANDLE out = GetStdHandle(STD_OUTPUT_HANDLE);

    TerminalState saved{};
    GetConsoleMode(in,  &saved.in_mode);
    GetConsoleMode(out, &saved.out_mode);
    saved.out_cp = GetConsoleOutputCP();

    // no line buffering, no echo, and Ctrl+C arrives as a key (3), not a signal.
    // quick-edit is off so clicking the window doesn't freeze the game.
    DWORD raw_in = (saved.in_mode & ~(ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT |
                                      ENABLE_PROCESSED_INPUT | ENABLE_QUICK_EDIT_MODE))
                   | ENABLE_EXTENDED_FLAGS;
    SetConsoleMode(in, raw_in);

    // make ANSI escapes work, and the box characters print as UTF-8
    SetConsoleMode(out, saved.out_mode | ENABLE_PROCESSED_OUTPUT
                                       | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
    SetConsoleOutputCP(CP_UTF8);

    enter_alternate_screen();
    hide_cursor();
    clear_screen();
    return saved;
}

void restore_terminal(const TerminalState& saved)
{
    show_cursor();
    clear_screen();
    exit_alternate_screen();
    std::cout << std::flush;

    SetConsoleMode(GetStdHandle(STD_INPUT_HANDLE),  saved.in_mode);
    SetConsoleMode(GetStdHandle(STD_OUTPUT_HANDLE), saved.out_mode);
    SetConsoleOutputCP(saved.out_cp);
}

// Reads console input events directly, so no escape-sequence parsing is needed.
int read_key()
{
    HANDLE in = GetStdHandle(STD_INPUT_HANDLE);

    DWORD pending = 0;
    while(GetNumberOfConsoleInputEvents(in, &pending) && pending > 0)
    {
        INPUT_RECORD rec;
        DWORD n = 0;
        if(!ReadConsoleInputA(in, &rec, 1, &n) || n == 0)
            break;

        // skip focus/resize/mouse events and key releases
        if(rec.EventType != KEY_EVENT || !rec.Event.KeyEvent.bKeyDown)
            continue;

        const KEY_EVENT_RECORD& k = rec.Event.KeyEvent;
        switch(k.wVirtualKeyCode)
        {
            case VK_ESCAPE: return KEY_ESC;
            case VK_UP:     return KEY_UP;
            case VK_DOWN:   return KEY_DOWN;
            case VK_LEFT:   return KEY_LEFT;
            case VK_RIGHT:  return KEY_RIGHT;
            case VK_TAB:
                return (k.dwControlKeyState & SHIFT_PRESSED) ? KEY_SHIFT_TAB : '\t';
        }
        if(k.uChar.AsciiChar != 0)   // 0 = shift/ctrl/alt pressed on their own
            return static_cast<unsigned char>(k.uChar.AsciiChar);
    }
    return KEY_NONE;
}

// ================= POSIX =================
#else

Position get_term_size()
{
    struct winsize w;
    if(ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == 0)
        return {w.ws_row, w.ws_col};
    std::cerr << "ERROR: Cannot get terminal size. \n";
    return {1, 1};
}  

void restore_terminal(const TerminalState& saved)
{
    show_cursor();
    clear_screen();
    tcsetattr(STDIN_FILENO, TCSANOW, &saved.orig);
    exit_alternate_screen();
}

TerminalState enable_raw_mode()
{
    TerminalState saved;
    tcgetattr(STDIN_FILENO, &saved.orig);

    termios copy = saved.orig;
    copy.c_lflag &= ~(ICANON | ECHO | ISIG);
    copy.c_cc[VMIN]  = 0;
    copy.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSANOW, &copy);

    enter_alternate_screen();
    hide_cursor();
    clear_screen();
    return saved;
}

namespace {
bool read_byte(char& c) { return read(STDIN_FILENO, &c, 1) > 0; }
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
#endif
