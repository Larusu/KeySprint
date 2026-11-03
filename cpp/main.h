#pragma once

#include <cassert>
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <optional>
#include <string>
#include <sys/ioctl.h>
#include <thread>
#include <unistd.h>
#include <termios.h>
#include <vector>
#include <ctime>
#include <iomanip>
#include <utility>
#include <algorithm>

#define MAX_CHAR_PER_ROW 50
constexpr int FPS = 60;
constexpr std::chrono::milliseconds FRAME_TIME(1000 / FPS);
constexpr char ctrl(char c) { return c & 0x1F; }
using Clock = std::chrono::steady_clock;

enum class Difficulty { Easy, Medium, Hard };

struct Dimension {
    int row;
    int col;
};

struct Config {
    std::vector<std::string> word_list; // entire dictionary
    size_t        word_count;           // max word count. no limit if time-based
    Difficulty difficulty;              // 0 easy, 1 medium, 2 hard
    int        time_limit;              // seconds - max of 120 seconds
};

struct Stats {
    int correct   = 0; 
    int incorrect = 0;
    int extra     = 0;
    int missed    = 0;
};

struct Game {                       // mutable per game
    std::vector<std::string> words; // picked for this run
    std::vector<std::string> typed; // typed[i] = what the player typed for words[i]
    size_t cursor = 0;
    std::optional<Clock::time_point> start;
    Stats stats;
};

struct Result {             // for JSON
    double      wpm;
    double      accuracy;   // percent (probably)
    double      duration;   // seconds
    Difficulty  difficulty;
    std::string date; 
};

struct Player {             // persisted profile
    int attempts, best_WPM, best_accuracy;
    std::vector<Result> history;
};

// ANSI Escape helpers
void move_cursor(Dimension d);
void show_cursor();
void hide_cursor();
void enter_alternate_screen();
void exit_alternate_screen();
void clear();
const char* green();
const char* red();
const char* reset();
Dimension get_term_size();
std::vector<std::string>get_word_list();
std::vector<std::string>pick_random_words(const Config& c);
std::vector<std::string> split_words(
        const std::vector<std::string>& picked_words);
termios enable_raw_mode();
Game new_game(const Config& config);
template <typename... Args>
void draw(Dimension d, Args&&... args);
void screen(const Game& g, const Config& c);
double time_left(const Game& g, const Config& c);
void game_over(const Game& game, const termios& terminal);
void key_handler(Game& g, char input);
