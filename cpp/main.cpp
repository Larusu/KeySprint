#include <thread>
#include <filesystem>
#include <iostream>
#include <iomanip>
#include <optional>
#include <stdexcept>
#include <string>
#include "config.h"
#include "game.h"
#include "history.h"
#include "terminal.h"
#include "scoring.h"
#include "render.h"
#include "word_bank.h"

namespace {

constexpr int FPS = 60;
constexpr std::chrono::milliseconds FRAME_TIME(1000 / FPS);
constexpr size_t CONFIG_ROWS = 3; // mode, difficulty, time/words
const Result NO_RESULT{};         // passed to render before a game ends

struct App {
    WordBank word_bank;
    HistoryStore history_store;
    Config config;
    Game game;
    State state = State::Playing;
    std::optional<Result> result;
    size_t config_focus = 0;
    ConfigEditor config_editor;
};

// The terminal gets its coat back, even if a round throws on the way out.
class TerminalSession {
public:
    TerminalSession() : saved(enable_raw_mode()) {}
    ~TerminalSession() { restore_terminal(saved); }
    TerminalSession(const TerminalSession&) = delete;
    TerminalSession& operator=(const TerminalSession&) = delete;
private:
    TerminalState saved;
};

std::string find_word_bank(int argc, char** argv)
{
    if (argc > 2)
        throw std::runtime_error("Usage: keysprint [path/to/words.json]");
    if (argc == 2)
        return argv[1];
    for (const char* candidate : {"data/words.json", "../data/words.json"})
        if (std::filesystem::is_regular_file(candidate))
            return candidate;
    throw std::runtime_error(
        "Cannot find the word bank. Tried data/words.json and ../data/words.json");
}

void restart(App& app)
{
    app.game = new_game(app.config,
               get_word_list(app.word_bank, difficulty_info(app.config.difficulty).key));
    app.result.reset();
}

void start(App& app)
{
    restart(app);
    app.state = State::Playing;
}

void handle_playing(App& app, int key, bool is_char)
{
    if (key == KEY_ESC)
        start(app);
    else if (is_char && !is_over(app.game, app.config))
        apply_key(app.game, static_cast<char>(key));
}

void handle_results(App& app, int key)
{
    if (key == KEY_ESC)
        start(app);
}

void handle_config(App& app, int key, bool is_char)
{
    Config& config = app.config;
    ConfigEditor& editor = app.config_editor;

    if (editor.editing_count)
    {
        if (key == KEY_ESC)
            editor = {};                                  // draft goes in the bin
        else if (key == ctrl('f'))
        {
            if (commit_word_count(config, editor))
                start(app);
        }
        else if (is_char)
        {
            handle_config_key(config, editor, static_cast<char>(key));
            if (!editor.editing_count)                    // Enter committed the count
                restart(app);
        }
        return;
    }

    if (key == '\t')
        app.config_focus = (app.config_focus + 1) % CONFIG_ROWS;
    else if (key == KEY_SHIFT_TAB)
        app.config_focus = (app.config_focus + CONFIG_ROWS - 1) % CONFIG_ROWS;
    else if (key == KEY_LEFT || key == KEY_RIGHT)
    {
        adjust_config(config, app.config_focus, key == KEY_LEFT ? -1 : 1);
        restart(app);
    }
    else if (key == KEY_ESC || key == ctrl('f') || key == '\r' || key == '\n')
        app.state = State::Playing;
    else if (is_char)
    {
        const auto before = config.difficulty;
        handle_config_key(config, editor, static_cast<char>(key));
        if (before != config.difficulty)
            restart(app);
    }
}

void handle_key(App& app, int key)
{
    if (key == KEY_UNKNOWN || key == KEY_NONE)
        return;
    if (key == ctrl('c'))
    {
        app.state = State::Exit;
        return;
    }

    const bool in_game = app.state == State::Playing && app.game.start.has_value();
    if (!in_game)
    {
        if (key == ctrl('f') && app.state != State::Config)
        {
            app.config_editor = {};
            restart(app); // abandon the old timer before showing the preview
            app.state = State::Config;
            return;
        }
        if (key == ctrl('n'))
        {
            if (commit_word_count(app.config, app.config_editor))
                start(app);
            return;
        }
    }

    const bool is_char = key >= 0 && key < 256;
    switch (app.state)
    {
        case State::Playing: handle_playing(app, key, is_char); break;
        case State::Config:  handle_config(app, key, is_char);  break;
        case State::Results: handle_results(app, key);          break;
        case State::Exit:    break;
    }
}

void handle_input(App& app)
{
    int key;
    while (app.state != State::Exit && (key = read_key()) != KEY_NONE)
        handle_key(app, key);
}

void update(App& app)
{
    if (app.state != State::Playing)
        return;
    top_up_words(app.game, app.config);
    if (is_over(app.game, app.config))
    {
        app.result = make_result(app.game, app.config);
        app.state = State::Results;
    }
}

}

int main(int argc, char** argv)
{
    App app;
    try
    { // Parse and prepare the first round before touching the terminal.
        app.word_bank = load_word_bank(find_word_bank(argc, argv));
        restart(app);
    }
    catch (const std::exception& error)
    {
        std::cerr << "Startup error: " << error.what() << '\n';
        return 1;
    }
    std::string history_path = "data/history.json";
    for (const char* c : {"data/history.json", "../data/history.json"})
        if (std::filesystem::is_regular_file(c)) { history_path = c; break; }
    app.history_store = load_history(history_path);

    try
    {
        TerminalSession terminal;
        while (app.state != State::Exit)
        {
            const auto frame_end = Clock::now() + FRAME_TIME;
            handle_input(app);
            update(app);
            if (app.state != State::Exit)
            {
                const Result& result = app.result ? *app.result : NO_RESULT;
                render(app.game, app.config, app.state, result, app.config_focus, app.config_editor);
            }
            std::this_thread::sleep_until(frame_end);
        }
    }
    catch (const std::exception& error)
    {
        std::cerr << "Game error: " << error.what() << '\n';
        return 1;
    }

    if (app.result)
        std::cout << std::fixed << std::setprecision(0)
                  << "WPM: " << app.result->wpm << '\n'
                  << "Accuracy: " << app.result->accuracy << "%\n";
    else
        std::cout << "Thank you for playing!\n";
    return 0;
}
