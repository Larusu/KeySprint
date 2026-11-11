#include "config.h"

namespace {

template <typename T, std::size_t N>
T step(const T (&values)[N], T current, int direction)
{
    for (std::size_t i = 0; i < N; ++i)
        if (values[i] == current)
            return values[(i + N + (direction < 0 ? N - 1 : 1)) % N];
    // A custom count is visiting from outside the preset carousel.
    return direction < 0 ? values[N - 1] : values[0];
}

}

void adjust_config(Config& config, std::size_t focus, int direction)
{
    switch (focus)
    {
        case 0: config.mode = step(ALL_MODES, config.mode, direction); break;
        case 1: config.difficulty = step(ALL_DIFFICULTIES, config.difficulty, direction); break;
        case 2:
            if (config.mode == Mode::Time)
                config.time_limit = step(TIME_OPTIONS, config.time_limit, direction);
            else
                config.word_count = step(WORD_OPTIONS, config.word_count, direction);
            break;
    }
}

void handle_config_key(Config& config, ConfigEditor& editor, char ch)
{
    if (editor.editing_count)
    {
        if (ch >= '0' && ch <= '9')
        {
            if (editor.count_input.size() < 20)
                editor.count_input += ch;
        }
        else if (ch == 127 || ch == 8)
        {
            if (!editor.count_input.empty())
                editor.count_input.pop_back();
        }
        else if (ch == '\r' || ch == '\n')
            commit_word_count(config, editor);
        return;
    }

    switch (ch)
    {
        case '1': config.difficulty = Difficulty::English1k; break;
        case '2': config.difficulty = Difficulty::English5k; break;
        case '3': config.difficulty = Difficulty::English10k; break;
        case '4': config.difficulty = Difficulty::English25k; break;
        case 'w':
        case 'W':
            if (config.mode == Mode::Words)
            {
                editor.editing_count = true;
                editor.count_input.clear();
                editor.error.clear();
            }
            break;
    }
}

bool commit_word_count(Config& config, ConfigEditor& editor)
{
    if (!editor.editing_count)
        return true;
    if (editor.count_input.empty())
    {
        editor.error = "Word count cannot be empty";
        return false;
    }

    int value = 0;
    for (char ch : editor.count_input)
    {
        if (ch < '0' || ch > '9')
        {
            editor.error = "Word count must contain digits only";
            return false;
        }
        // Stop before multiplying: even a mountain of digits cannot overflow.
        const int digit = ch - '0';
        if (value > (MAX_WORD_COUNT - digit) / 10)
        {
            editor.error = "Word count must be between 1 and 10,000";
            return false;
        }
        value = value * 10 + digit;
    }
    if (value == 0)
    {
        editor.error = "Word count must be between 1 and 10,000";
        return false;
    }

    config.word_count = value;
    editor = ConfigEditor{};
    return true;
}
