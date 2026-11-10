#include <thread>
#include <unistd.h>
#include <iostream>
#include <iomanip>
#include <cstdlib>
#include <ctime>
#include "game.h"
#include "terminal.h"
#include "scoring.h"
#include "render.h"

namespace {

constexpr int FPS = 60;
constexpr std::chrono::milliseconds FRAME_TIME(1000 / FPS);

struct App {
    Config config;
    Game   game;
    State  state;
    std::optional<Result> result;
    size_t config_focus = 0; // 0 = mode, 1 = difficulty, 2 = time/words
};

void restart(App& app)
{
    app.game = new_game(app.config);
    app.result.reset();
}

void handle_key(App& app, int key)
{
    if(key == KEY_UNKNOWN) return;
    if(key == ctrl('c')) 
        app.state = State::Exit;

    switch(app.state)
    {
    case State::Playing:
    {
        if(key == KEY_ESC)
            restart(app);
        else if(key == ctrl('n') && !app.game.start)
            restart(app);
        else if(key == ctrl('f') && !app.game.start)
            app.state = app.state == State::Config ? State::Playing : State::Config;
        else if(key < 256) // prevent ctrl + key
            apply_key(app.game, static_cast<char>(key));
        break;
    }

    case State::Config:
    {
        bool changed = false;
        if(key == '\t')
            app.config_focus = (app.config_focus + 1) % 3;
        else if(key == KEY_SHIFT_TAB)
            app.config_focus = (app.config_focus + 2) % 3;
        else if(key == KEY_LEFT)
        {
            adjust_config(app.config, app.config_focus, -1);
            changed = true;
        }
        else if(key == KEY_RIGHT)
        {
            adjust_config(app.config, app.config_focus, +1);
            changed = true;
        }
        // escape, ctrl+f, enter
        else if(key == KEY_ESC || key == ctrl('f') || key == '\r' || key == '\n')
            app.state = State::Playing;

        if (changed)
            restart(app);
        break;
    }

    case State::Results:
    {
        if(key == ctrl('n') || key == KEY_ESC)
        {
            restart(app);
            app.state = State::Playing;
        }
        break;
    }

    case State::Exit:
        break;
    }
}

void handle_input(App& app)
{
    int key;
    while(app.state != State::Exit && (key = read_key()) != KEY_NONE)
        handle_key(app, key);
}

void update(App& app)
{
    if (app.state != State::Playing) return;
    top_up_words(app.game, app.config);
    if (is_over(app.game, app.config))
    {
        app.result = make_result(app.game, app.config);
        app.state  = State::Results;
    }
}

}

int main () 
{
    srand(time(0));
    termios orig = enable_raw_mode();

    Config config;
    App app = { config, new_game(config), State::Playing, {} };

    while(app.state != State::Exit)
    {
        const auto frame_end = Clock::now() + FRAME_TIME;
        handle_input(app);
        update(app);
        render(app.game, app.config, app.state, app.result.value_or(Result{}), app.config_focus);
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
