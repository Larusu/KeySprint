// Keep app routing under test without giving production code test-only hooks.
int keysprint_main(int argc, char** argv);
#define main keysprint_main
#include "../main.cpp"
#undef main

#include <algorithm>
#include <fstream>
#include <functional>
#include <nlohmann/json.hpp>

namespace {

void require(bool condition, const std::string& message)
{
    if (!condition)
        throw std::runtime_error(message);
}

template <typename F>
void rejects(F operation, const std::string& message)
{
    bool rejected = false;
    try { operation(); }
    catch (const std::exception&) { rejected = true; }
    require(rejected, message);
}

Config small_config(Mode mode)
{
    Config config;
    config.mode = mode;
    config.word_list = {"cat", "dog"};
    return config;
}

App small_app()
{
    App app;
    for (const auto difficulty : ALL_DIFFICULTIES)
        app.word_bank.emplace(std::string(difficulty_key(difficulty)),
                              std::vector<std::string>{"cat", "dog"});
    restart(app);
    return app;
}

void word_count_validation()
{
    Config config = small_config(Mode::Words);
    for (const std::string input : {"", "0", "10001", "99999999999999999999", "-1", "12x"})
    {
        ConfigEditor editor{true, input, {}};
        require(!commit_word_count(config, editor), "invalid count accepted: " + input);
        require(config.word_count == 25, "invalid draft changed the committed count");
        require(editor.editing_count && !editor.error.empty(), "error should keep editor open");
    }
    for (const int count : {1, 37, MAX_WORD_COUNT})
    {
        ConfigEditor editor{true, std::to_string(count), {}};
        require(commit_word_count(config, editor), "valid count rejected");
        require(config.word_count == count && !editor.editing_count, "count not committed");
    }
    ConfigEditor editor;
    config.mode = Mode::Time;
    handle_config_key(config, editor, 'w');
    require(!editor.editing_count, "time mode should not edit an unused word count");
    config.mode = Mode::Words;
    handle_config_key(config, editor, 'w');
    handle_config_key(config, editor, '4');
    require(config.difficulty == Difficulty::English1k, "count digit changed difficulty");
}

void selector_navigation()
{
    Config config;
    adjust_config(config, 0, 1);
    require(config.mode == Mode::Words, "mode selector failed");
    adjust_config(config, 1, -1);
    require(config.difficulty == Difficulty::English25k, "difficulty wrap failed");
    adjust_config(config, 1, 1);
    require(config.difficulty == Difficulty::English1k, "difficulty wrap back failed");
    config.word_count = 37;
    adjust_config(config, 2, 1);
    require(config.word_count == WORD_OPTIONS[0], "custom right should choose first preset");
    config.word_count = 37;
    adjust_config(config, 2, -1);
    require(config.word_count == WORD_OPTIONS[3], "custom left should choose last preset");
}

void sampling_and_refill()
{
    Config config = small_config(Mode::Words);
    config.word_count = MAX_WORD_COUNT;
    Game game = new_game(config);
    require(game.words.size() == MAX_WORD_COUNT, "full custom count was not generated");
    require(game.typed.size() == game.words.size(), "typed slots missing");
    require(std::adjacent_find(game.words.begin(), game.words.end()) == game.words.end(),
            "adjacent words repeated");
    config.word_count = 0;
    rejects([&] { new_game(config); }, "zero count should be rejected");
    config.word_count = MAX_WORD_COUNT + 1;
    rejects([&] { new_game(config); }, "oversized count should be rejected");
    config = small_config(Mode::Time);
    game = new_game(config);
    for (int batch = 0; batch < 100; ++batch)
    {
        const auto previous_size = game.words.size();
        game.cursor = previous_size - 1;
        top_up_words(game, config);
        require(game.words.size() > previous_size, "time mode did not refill");
        require(game.words[previous_size - 1] != game.words[previous_size],
                "repeat at refill boundary");
    }
    require(std::adjacent_find(game.words.begin(), game.words.end()) == game.words.end(),
            "refill introduced adjacent repeats");
    config.word_list = {"cat"};
    rejects([&] { new_game(config); }, "single-word pool should fail safely");
}

void mode_completion_and_scoring()
{
    Config config = small_config(Mode::Words);
    config.word_count = 1;
    Game game = new_game(config);
    game.start = Clock::now() - std::chrono::seconds(60);
    require(!is_over(game, config), "Words mode ended on the Time-mode limit");
    game.typed[0] = game.words[0];
    game.keystrokes.correct = 3;
    require(is_over(game, config), "correct last word did not finish Words mode");
    Result result = make_result(game, config);
    require(result.duration >= 60 && result.chars.correct == 3 && result.accuracy == 100,
            "Words-mode result lost elapsed time or character statistics");

    config.mode = Mode::Time;
    game = new_game(config);
    require(!is_over(game, config), "unstarted Time mode ended early");
    game.start = Clock::now() - std::chrono::seconds(31);
    game.typed[0] = "cx";
    game.cursor = 1;
    require(is_over(game, config), "Time mode ignored expiry");
    result = make_result(game, config);
    require(result.duration == 30, "timed result should stop at configured duration");
    require(result.chars.correct + result.chars.incorrect == 2 && result.chars.missed == 1,
            "character breakdown was lost in the merge");
}

void app_controls()
{
    App app = small_app();
    apply_key(app.game, 'c');
    handle_key(app, ctrl('f'));
    require(app.state == State::Config && !app.game.start, "Ctrl+F must open a fresh preview");
    handle_key(app, KEY_RIGHT);
    require(app.config.mode == Mode::Words && app.state == State::Config,
            "selector change closed the config screen");
    handle_key(app, KEY_SHIFT_TAB);
    require(app.config_focus == 2, "Shift+Tab wrap failed");
    handle_key(app, '\t');
    require(app.config_focus == 0, "Tab wrap failed");
    handle_key(app, '4');
    require(app.config.difficulty == Difficulty::English25k, "difficulty shortcut failed");
    handle_key(app, 'w');
    handle_key(app, '0');
    handle_key(app, ctrl('n'));
    require(app.state == State::Config && app.config_editor.editing_count,
            "Ctrl+N started a round with an invalid count");
    handle_key(app, KEY_ESC);
    require(app.state == State::Config && !app.config_editor.editing_count,
            "Esc must discard only the count draft");
    handle_key(app, 'w');
    handle_key(app, '1');
    handle_key(app, ctrl('n'));
    require(app.state == State::Playing && app.game.words.size() == 1,
            "Ctrl+N failed to commit and start a one-word round");
    for (const char ch : app.game.words[0])
        handle_key(app, ch);
    update(app);
    require(app.state == State::Results && app.result.has_value(), "results were not reached");
    handle_key(app, ctrl('f'));
    require(app.state == State::Config && !app.result, "Ctrl+F failed from results");
    handle_key(app, KEY_ESC);
    require(app.state == State::Playing, "Esc failed to leave config");
    handle_key(app, 'c');
    handle_key(app, KEY_ESC);
    require(!app.game.start, "Esc failed to restart gameplay");
    for (const auto state : {State::Playing, State::Config, State::Results})
    {
        app.state = state;
        handle_key(app, ctrl('c'));
        require(app.state == State::Exit, "Ctrl+C was not global");
    }
}

void loader_validation()
{
    const auto bank = load_word_bank("../data/words.json");
    require(bank.size() == 4, "real word bank did not load all four levels");
    for (const auto difficulty : ALL_DIFFICULTIES)
        require(get_word_list(bank, difficulty_key(difficulty)).size() > 2,
                "real vocabulary pool is missing");
    rejects([&] { get_word_list(bank, "missing"); }, "unknown difficulty should fail");
    rejects([] { load_word_bank("missing-keysprint-word-bank.json"); }, "missing file accepted");

    nlohmann::json fixture = {{"schema_version", 1}, {"language", "en"}};
    for (const auto difficulty : ALL_DIFFICULTIES)
    {
        const std::string key(difficulty_key(difficulty));
        fixture["levels"][key] = {"cat", "dog"};
        fixture["metadata"]["actual_counts"][key] = 2;
    }
    const auto path = std::filesystem::temp_directory_path()
        / ("keysprint-test-" + std::to_string(Clock::now().time_since_epoch().count()) + ".json");
    struct Cleanup {
        std::filesystem::path path;
        ~Cleanup() { std::error_code ignored; std::filesystem::remove(path, ignored); }
    } cleanup{path};
    auto load_fixture = [&](const nlohmann::json& document) {
        std::ofstream(path) << document;
        return load_word_bank(path.string());
    };
    require(load_fixture(fixture).size() == 4, "valid fixture rejected");
    for (const nlohmann::json& version : {nlohmann::json(-1), nlohmann::json(1.5),
                                       nlohmann::json(4294967297ULL)})
    {
        auto invalid = fixture;
        invalid["schema_version"] = version;
        rejects([&] { load_fixture(invalid); }, "invalid schema version accepted");
    }
    for (const auto& words : {std::vector<std::string>{"cat", "cat"},
                             std::vector<std::string>{"cat", "an"},
                             std::vector<std::string>{"Cat", "dog"}})
    {
        auto invalid = fixture;
        invalid["levels"]["english_1k"] = words;
        rejects([&] { load_fixture(invalid); }, "invalid vocabulary accepted");
    }
    auto singleton = fixture;
    singleton["levels"]["english_5k"] = {"cat"};
    singleton["metadata"]["actual_counts"]["english_5k"] = 1;
    rejects([&] { load_fixture(singleton); }, "singleton pool should fail at startup");
    auto invalid = fixture;
    invalid["metadata"]["actual_counts"]["english_1k"] = 3;
    rejects([&] { load_fixture(invalid); }, "inaccurate count accepted");
    std::ofstream(path) << "{ malformed";
    rejects([&] { load_word_bank(path.string()); }, "malformed JSON accepted");
}

}

int main()
{
    const std::pair<const char*, std::function<void()>> cases[] = {
        {"word count validation", word_count_validation},
        {"selector navigation", selector_navigation},
        {"sampling and refill boundaries", sampling_and_refill},
        {"mode completion and scoring", mode_completion_and_scoring},
        {"application controls", app_controls},
        {"word bank validation", loader_validation}
    };
    int failed = 0;
    for (const auto& test : cases)
    {
        try { test.second(); std::cout << "PASS: " << test.first << '\n'; }
        catch (const std::exception& error)
        {
            std::cerr << "FAIL: " << test.first << ": " << error.what() << '\n';
            ++failed;
        }
    }
    return failed == 0 ? 0 : 1;
}
