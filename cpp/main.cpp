#include <thread>
#include <filesystem>
#include <iostream>
#include <iomanip>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include "config.h"
#include "game.h"
#include "terminal.h"
#include "scoring.h"
#include "render.h"
#include "word_bank.h"

namespace {

constexpr int FPS = 60;
constexpr std::chrono::milliseconds FRAME_TIME(1000 / FPS);

struct App {
    WordBank word_bank;
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
    app.config.word_list = get_word_list(app.word_bank, difficulty_key(app.config.difficulty));
    app.game = new_game(app.config);
    app.result.reset();
    // Keep the screen we're on: changing a selector is still a config preview.
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
    if (key == ctrl('f') && app.state != State::Config)
    {
        app.config_editor = ConfigEditor{};
        restart(app); // Abandon the old timer before showing the new preview.
        app.state = State::Config;
        return;
    }
    if (key == ctrl('n') && (app.state != State::Playing || !app.game.start))
    {
        if (!commit_word_count(app.config, app.config_editor))
            return;
        restart(app);
        app.state = State::Playing;
        return;
    }

    switch (app.state)
    {
        case State::Playing:
            if (key == KEY_ESC)
                restart(app);
            else if (key >= 0 && key < 256 && !is_over(app.game, app.config))
            {
                apply_key(app.game, static_cast<char>(key));
                top_up_words(app.game, app.config);
            }
            break;
        case State::Config:
        {
            if (app.config_editor.editing_count)
            {
                if (key == KEY_ESC)
                    app.config_editor = ConfigEditor{}; // Draft goes in the bin.
                else if (key == ctrl('f'))
                {
                    if (commit_word_count(app.config, app.config_editor))
                    {
                        restart(app);
                        app.state = State::Playing;
                    }
                }
                else if (key >= 0 && key < 256)
                {
                    handle_config_key(app.config, app.config_editor, static_cast<char>(key));
                    if (!app.config_editor.editing_count)
                        restart(app);
                }
                break;
            }
            if (key == '\t')
                app.config_focus = (app.config_focus + 1) % 3;
            else if (key == KEY_SHIFT_TAB)
                app.config_focus = (app.config_focus + 2) % 3;
            else if (key == KEY_LEFT || key == KEY_RIGHT)
            {
                adjust_config(app.config, app.config_focus, key == KEY_LEFT ? -1 : 1);
                restart(app);
            }
            else if (key == KEY_ESC || key == ctrl('f') || key == '\r' || key == '\n')
                app.state = State::Playing;
            else if (key >= 0 && key < 256)
            {
                const auto before = app.config.difficulty;
                handle_config_key(app.config, app.config_editor, static_cast<char>(key));
                if (before != app.config.difficulty)
                    restart(app);
            }
            break;
        }
        case State::Results:
            if (key == KEY_ESC)
            {
                restart(app);
                app.state = State::Playing;
            }
            break;
        case State::Exit:
            break;
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

    try
    {
        TerminalSession terminal;
        while (app.state != State::Exit)
        {
            const auto frame_end = Clock::now() + FRAME_TIME;
            handle_input(app);
            update(app);
            if (app.state != State::Exit)
                render(app.game, app.config, app.state, app.result.value_or(Result{}),
                       app.config_focus, app.config_editor);
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
