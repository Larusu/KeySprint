#pragma once

#include <string>
#include <termios.h>

struct Position { 
    int row; 
    int col; 
};
constexpr char ctrl(char c) { return c & 0x1F; }

std::string move_cursor(Position pos);
void show_cursor();
void hide_cursor();
void enter_alternate_screen();
void exit_alternate_screen();
void clear_screen();
const char* green();
const char* red();
const char* white();
const char* reset();
const char* dim();
const char* highlight();
Position get_term_size();
termios enable_raw_mode();
void restore_terminal(const termios& orig);
