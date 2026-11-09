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
};

void handle_key(App& app, char ch)
{
    if (ch == ctrl('c')) 
        app.state = State::Exit; 
    else if(ch == ctrl('n') && app.state != State::Playing)
    { // play again
        app.state = State::Playing; 
        app.game = new_game(app.config); 
        app.result.reset();
    }
    else if(ch == ctrl('f') && app.state != State::Config)
        app.state = State::Config;
    else if(app.state == State::Playing)
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
        render(app.game, app.config, app.state, app.result.value_or(Result{}));
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
