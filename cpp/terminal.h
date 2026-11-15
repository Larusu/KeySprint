#pragma once

#include <string>
#ifdef _WIN32
struct TerminalState { 
    unsigned long in_mode; 
    unsigned long out_mode; 
    unsigned int out_cp; 
};
#else
#include <termios.h>
struct TerminalState { 
    termios orig; 
};
#endif

struct Position { 
    int row; 
    int col; 
};

std::string move_cursor(Position pos);
const char* green();
const char* red();
const char* white();
const char* reset();
const char* dim();
const char* highlight();
const char* erase_screen();   // escape code only; clear_screen() below prints it
const char* sync_begin();     // start/end of one atomic screen update (ignored by
const char* sync_end();       // terminals that don't know it)
constexpr char ctrl(char c) { return c & 0x1F; }

constexpr int KEY_NONE      = -1;   // nothing to read
constexpr int KEY_UNKNOWN   = -2;   // dont care
constexpr int KEY_ESC       = 256;
constexpr int KEY_UP        = 257;
constexpr int KEY_DOWN      = 258;
constexpr int KEY_RIGHT     = 259;
constexpr int KEY_LEFT      = 260;
constexpr int KEY_SHIFT_TAB = 266;

void show_cursor();
void hide_cursor();
void enter_alternate_screen();
void exit_alternate_screen();
void clear_screen();
Position get_term_size();
TerminalState enable_raw_mode();
void restore_terminal(const TerminalState& orig);
int read_key();
void write_out(const std::string& data); // Sends the whole string to the terminal in one go
