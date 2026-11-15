#include "config.h"

namespace {

// Next/previous preset. A custom count that isn't a preset jumps to either end
template <std::size_t N>
int step(const int (&values)[N], int current, int direction)
{
    for (std::size_t i = 0; i < N; ++i)
        if (values[i] == current)
            return values[(i + N + (direction < 0 ? N - 1 : 1)) % N];
    return direction < 0 ? values[N - 1] : values[0];
}

}

void adjust_config(Config& config, std::size_t focus, int direction)
{
    switch (focus)
    {
        case 0:  // only two modes, so either direction just toggles
            config.mode = config.mode == Mode::Time ? Mode::Words : Mode::Time;
            break;
        case 1:
        {
            const int n = int(DIFFICULTY_COUNT);
            config.difficulty = Difficulty((int(config.difficulty) + direction + n) % n);
            break;
        }
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
            if (editor.count_input.size() < 5)   // MAX_WORD_COUNT has 5 digits
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

    if (const int pick = ch - '1'; pick >= 0 && pick < int(DIFFICULTY_COUNT))
        config.difficulty = Difficulty(pick);
    else if ((ch == 'w' || ch == 'W') && config.mode == Mode::Words)
    {
        editor = {};
        editor.editing_count = true;
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

    const int value = std::stoi(editor.count_input);   // at most 5 digits, can't overflow
    if (value < 1 || value > MAX_WORD_COUNT)
    {
        editor.error = "Word count must be between 1 and 10,000";
        return false;
    }

    config.word_count = value;
    editor = {};
    return true;
}
