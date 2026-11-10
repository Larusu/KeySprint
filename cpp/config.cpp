#include "config.h"

#include <limits>

void handle_config_key(Config& config, ConfigEditor& editor, char ch)
{
    // The count field owns the keys while it is active, so digits typed here
    // must not be mistaken for difficulty shortcuts.
    if (editor.editing_count)
    {
        if (ch >= '0' && ch <= '9')
        {
            if (editor.count_input.size() < 20) // keep far-future overflow cheap
                editor.count_input += ch;
        }
        else if (ch == 127 || ch == 8)
        {
            if (!editor.count_input.empty())
                editor.count_input.pop_back();
        }
        else if (ch == '\r' || ch == '\n')
        {
            commit_word_count(config, editor);
        }
        return;
    }

    switch (ch)
    {
        case '1': config.difficulty = Difficulty::English1k;  break;
        case '2': config.difficulty = Difficulty::English5k;  break;
        case '3': config.difficulty = Difficulty::English10k; break;
        case '4': config.difficulty = Difficulty::English25k; break;
        case 'w':
        case 'W':
            editor.editing_count = true;
            editor.count_input.clear();
            editor.error.clear();
            break;
        default:
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

    std::size_t value = 0;
    for (char ch : editor.count_input)
    {
        if (ch < '0' || ch > '9')
        {
            editor.error = "Word count must contain digits only";
            return false;
        }

        const std::size_t digit = static_cast<std::size_t>(ch - '0');
        if (value > (std::numeric_limits<std::size_t>::max() - digit) / 10)
        {
            editor.error = "Word count is too large";
            return false;
        }
        value = value * 10 + digit;
    }

    if (value == 0)
    {
        editor.error = "Word count must be greater than zero";
        return false;
    }

    config.word_count = value;
    editor.editing_count = false;
    editor.count_input.clear();
    editor.error.clear();
    return true;
}
