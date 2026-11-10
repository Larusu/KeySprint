#include <thread>
#include <unistd.h>
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
    Config   config;
    Game     game;
    State    state;
    std::optional<Result> result;
    ConfigEditor config_editor;
};

std::string find_word_bank(int argc, char** argv)
{
    if (argc > 1)
        return argv[1];

    for (const char* candidate : {"data/words.json", "../data/words.json"})
        if (std::filesystem::is_regular_file(candidate))
            return candidate;

    throw std::runtime_error(
        "Cannot find the word bank. Tried data/words.json and ../data/words.json");
}

void start_game(App& app)
{
    app.config.word_list = get_word_list(
        app.word_bank, difficulty_key(app.config.difficulty));
    app.game = new_game(app.config);
    app.result.reset();
    app.state = State::Playing;
}

void handle_key(App& app, char ch)
{
    if (ch == ctrl('c'))
        app.state = State::Exit;
    else if (ch == ctrl('f') && app.state != State::Config)
    { // settings preview; the unfinished round is abandoned
        app.state = State::Config;
        app.config_editor = ConfigEditor{};
    }
    else if (ch == ctrl('n') && app.state != State::Playing)
    { // apply settings and play again
        if (app.state == State::Config
            && !commit_word_count(app.config, app.config_editor))
            return;
        start_game(app);
    }
    else if (app.state == State::Config)
        handle_config_key(app.config, app.config_editor, ch);
    else if (app.state == State::Playing)
        apply_key(app.game, ch);
}

void handle_input(App& app)
{
    char ch;
    while (app.state != State::Exit && read(STDIN_FILENO, &ch, 1) > 0)
        handle_key(app, ch);
}

void update(App& app)
{
    if (app.state == State::Playing && is_over(app.game, app.config))
    {
        app.result = make_result(app.game, app.config);
        app.state  = State::Results;
    }
}

}

int main(int argc, char** argv)
{
    WordBank bank;
    Config config;
    try
    { // load and validate before touching the terminal
        const std::string path = find_word_bank(argc, argv);
        bank = load_word_bank(path);
        config.word_list = get_word_list(bank, difficulty_key(config.difficulty));
    }
    catch (const std::exception& error)
    {
        std::cerr << "Startup error: " << error.what() << '\n';
        return 1;
    }

    termios orig = enable_raw_mode();

    App app = { std::move(bank), config, new_game(config), State::Playing,
                std::nullopt, ConfigEditor{} };

    while(app.state != State::Exit)
    {
        const auto frame_end = Clock::now() + FRAME_TIME;
        handle_input(app);
        update(app);
        render(app.game, app.config, app.state, app.result.value_or(Result{}),
               app.config_editor);
        std::this_thread::sleep_until(frame_end);
    }

    restore_terminal(orig);

    // print goodbye message :'(
    if (app.result)
    {
        std::cout << std::fixed << std::setprecision(0)
                  << "WPM: " << app.result->wpm << '\n'
                  << "Accuracy: " << app.result->accuracy << "%\n";
    }
    else
    {
        std::cout << "Thank you for playing!\n";
    }
    return 0;
}
