# Using the shared word bank in C++

The JSON data is ready. The steps and code below are for you to implement.
Your existing C++, Python, Java, and Makefile files have not been edited.

## 1. Know which file does what

| File | Purpose |
| --- | --- |
| `data/words.json` | Shared vocabulary for all three languages |
| `data/profanity-blocklist.json` | The whole-word exclusions used when preparing the supplement |
| `data/words.schema.json` | Machine-readable structure of the word bank |
| `data/SOURCES.md` | Exact counts, construction, source versions, and attribution |
| `cpp/config.h` and `cpp/config.cpp` | Your future difficulty and word-count definitions and editing logic |
| `cpp/word_bank.h` and `cpp/word_bank.cpp` | Your future JSON loading and pool lookup |
| `cpp/game.cpp` | Your existing game logic and random word selection |

The actual pool sizes are 933, 4,772, 9,474, and 24,165. Every practice word has
at least three letters. Level names such as
`english_25k` are stable labels. Read the array's length when you need its exact
size. Difficulty chooses a pool; `word_count` chooses the length of one session.

## 2. Use the same JSON contract in every language

Read these paths in the parsed document:

- `schema_version`: integer `1`.
- `levels.english_1k`, `levels.english_5k`, `levels.english_10k`, or
  `levels.english_25k`: an array of lowercase word strings.
- `metadata.actual_counts.<level>`: that array's real size.

For example, selecting `levels.english_5k` and generating 50 session words means
choosing 50 words from its 4,772-word array. The shared file contains vocabulary;
it does not store the current player's selected settings.

The parser reads JSON punctuation and turns the text into objects and arrays.
The loader then converts those arrays into ordinary C++ vectors. The game works
with those vectors after loading.

Python can use its `json` module:

```python
import json
from pathlib import Path

path = Path(__file__).resolve().parent.parent / "data" / "words.json"
bank = json.loads(path.read_text(encoding="utf-8"))
words = bank["levels"]["english_5k"]
```

Java can read the same file with a JSON library. For example, with Gson already
on its classpath, use `JsonParser.parseReader(reader).getAsJsonObject()`, then
`getAsJsonObject("levels").getAsJsonArray("english_5k")`. Open the reader with
`Files.newBufferedReader(path, StandardCharsets.UTF_8)`. This is a data-compatibility
example; no Java dependency or gameplay feature was added to your project.

## 3. Resolve the file path deliberately

| Process working directory | Path supplied to the loader |
| --- | --- |
| KeySprint repository root | `data/words.json` |
| `cpp`, `python`, or `java` subfolder | `../data/words.json` |

Relative paths start from the process's working directory, not the `.cpp` file.
Your current `cpp/Makefile` launches `./keysprint` from the `cpp` directory when
you run `make` there, so that launch needs `../data/words.json`.

For the eventual application, accept an optional word-bank path argument. Use
that supplied path first; otherwise try `data/words.json` and then
`../data/words.json`. Report both attempted locations if neither exists. This
supports the two documented launch locations without a machine-specific path.

## 4. Add nlohmann/json yourself

From the KeySprint repository root, these commands would put the pinned single
header in your C++ folder:

```bash
mkdir -p cpp/include/nlohmann
curl --fail --location \
  https://raw.githubusercontent.com/nlohmann/json/v3.12.0/single_include/nlohmann/json.hpp \
  --output cpp/include/nlohmann/json.hpp
```

Then `#include <nlohmann/json.hpp>` finds it when the compiler receives
`-Iinclude` from inside `cpp`, or `-Icpp/include` from the repository root.
The library is header-only. It does not require a separate linking flag.
See the [official integration guide](https://json.nlohmann.me/integration/#header-only).

The header has not been installed or copied into your project by this handoff.

## 5. Move the existing configuration definition

When you implement this step, move `Difficulty`, its display function, and
`Config` out of `cpp/game.h` into your existing `cpp/config.h`. Replace the three
old difficulty values with the four vocabulary choices. Remove the old
definitions from `game.h` and have it include `config.h`; keeping both copies
would cause definition errors.

This is a complete proposed `config.h` for the data-loading example:

```cpp
#pragma once

#include <cstddef>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

enum class Difficulty { English1k, English5k, English10k, English25k };

constexpr std::string_view difficulty_key(Difficulty difficulty)
{
    switch (difficulty) {
        case Difficulty::English1k:  return "english_1k";
        case Difficulty::English5k:  return "english_5k";
        case Difficulty::English10k: return "english_10k";
        case Difficulty::English25k: return "english_25k";
    }
    throw std::invalid_argument("Unknown difficulty");
}

constexpr std::string_view to_string(Difficulty difficulty)
{
    switch (difficulty) {
        case Difficulty::English1k:  return "English 1k";
        case Difficulty::English5k:  return "English 5k";
        case Difficulty::English10k: return "English 10k";
        case Difficulty::English25k: return "English 25k";
    }
    return "Unknown";
}

struct Config {
    std::vector<std::string> word_list;
    std::size_t word_count = 100;
    Difficulty difficulty = Difficulty::English1k;
    int time_limit = 30;
};
```

`difficulty_key()` maps the C++ enum to the shared string key. Python and Java
can use different internal types while keeping these JSON keys identical.
`to_string()` supplies the label your results screen already displays. Update
any remaining `Difficulty::Easy`, `Medium`, and `Hard` references, including the
default result difficulty in `scoring.h`.

## 6. Put JSON reading in a word-bank module

The following blocks are a proposed new `cpp/word_bank.h` and
`cpp/word_bank.cpp`. They are documentation examples, not files already added
to your application.

`word_bank.h`:

```cpp
#pragma once

#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

using WordBank = std::unordered_map<std::string, std::vector<std::string>>;

WordBank load_word_bank(const std::string& path);
const std::vector<std::string>& get_word_list(
    const WordBank& bank, std::string_view level);
```

`word_bank.cpp`:

```cpp
#include "word_bank.h"

#include <algorithm>
#include <fstream>
#include <stdexcept>
#include <unordered_set>
#include <utility>
#include <nlohmann/json.hpp>

WordBank load_word_bank(const std::string& path)
{
    std::ifstream file(path);
    if (!file)
        throw std::runtime_error("Cannot open word bank: " + path);

    const auto document = nlohmann::json::parse(file);
    const auto& version = document.at("schema_version");
    if (!version.is_number_integer() || version.get<int>() != 1)
        throw std::runtime_error("Unsupported word-bank schema version");
    if (document.at("language") != "en")
        throw std::runtime_error("Expected an English word bank");

    const auto& levels = document.at("levels");
    if (!levels.is_object() || levels.size() != 4)
        throw std::runtime_error("Expected the four English vocabulary pools");

    WordBank bank;
    for (const char* key : {"english_1k", "english_5k",
                            "english_10k", "english_25k"}) {
        auto words = levels.at(key).get<std::vector<std::string>>();
        if (words.empty())
            throw std::runtime_error(std::string("Empty word pool: ") + key);

        const auto& count = document.at("metadata").at("actual_counts").at(key);
        if (!count.is_number_unsigned() || count.get<std::size_t>() != words.size())
            throw std::runtime_error(std::string("Incorrect word count: ") + key);

        std::unordered_set<std::string> unique;
        for (const auto& word : words) {
            const bool letters_only = word.size() >= 3 && std::all_of(
                word.begin(), word.end(), [](unsigned char ch) {
                    return ch >= 'a' && ch <= 'z';
                });
            if (!letters_only || !unique.insert(word).second)
                throw std::runtime_error(std::string("Invalid or duplicate word in ") + key);
        }
        bank.emplace(key, std::move(words));
    }
    return bank;
}

const std::vector<std::string>& get_word_list(
    const WordBank& bank, std::string_view level)
{
    const auto found = bank.find(std::string(level));
    if (found == bank.end())
        throw std::runtime_error("Unknown word-bank level: " + std::string(level));
    return found->second;
}
```

`std::ifstream` opens the file. `json::parse()` interprets its contents.
`.at()` requires a named field to exist. `.get<std::vector<std::string>>()`
converts an array of JSON strings into a C++ vector. The checks reject unsupported
versions, missing fields, empty lists, inaccurate counts, words shorter than three
letters, invalid word characters, and duplicates. A malformed JSON document raises
the parser's diagnostic.

The returned `WordBank` holds all four vectors. The temporary parsed JSON is
released when the loader returns. Keep the returned bank for the application
lifetime so changing difficulty selects an in-memory pool without rereading the
file. `config.word_list` can hold a copy of the selected vector to fit your
current `pick_random_words(const Config&)` interface.

Remove the old zero-argument `get_word_list()` declaration from `game.h` and its
hardcoded definition from `game.cpp` when you adopt this replacement. The new
declaration belongs to `word_bank.h`.

## 7. Replace random selection when you are ready

This proposed replacement keeps your existing function signature:

```cpp
#include "config.h"

#include <random>
#include <stdexcept>
#include <vector>

std::vector<std::string> pick_random_words(const Config& config)
{
    const auto pool_size = config.word_list.size();
    if (config.word_count == 0)
        throw std::invalid_argument("Word count must be positive");
    if (pool_size == 0)
        throw std::invalid_argument("The selected word pool is empty");
    if (pool_size == 1 && config.word_count > 1)
        throw std::invalid_argument("At least two words are needed to prevent adjacent repeats");

    static std::mt19937 generator(std::random_device{}());
    std::vector<std::string> picked;
    picked.reserve(config.word_count);
    std::size_t previous = pool_size;

    for (std::size_t i = 0; i < config.word_count; ++i) {
        std::size_t index;
        if (i == 0) {
            index = std::uniform_int_distribution<std::size_t>(0, pool_size - 1)(generator);
        } else {
            index = std::uniform_int_distribution<std::size_t>(0, pool_size - 2)(generator);
            if (index >= previous)
                ++index;
        }
        picked.push_back(config.word_list[index]);
        previous = index;
    }
    return picked;
}
```

The generator is seeded once. The first word can use any index. Each later draw
chooses uniformly among the remaining indexes and skips the previous one. This
allows a word to appear again later and prevents consecutive identical words.
There is no retry loop that could get stuck on a one-word pool. Your delivered
pools contain unique entries, so excluding the previous index excludes that word.

After replacing all `rand()` uses, remove the obsolete `srand(time(0))` setup in
`main.cpp`. Your current `new_game(config)` can continue to call
`pick_random_words(config)` and resize `game.typed` as it already does.

## 8. Run a separate loader check before wiring the game

Once you have created the proposed header and module files, save this small test
program outside the project as `/tmp/keysprint-word-demo.cpp`:

```cpp
#include "config.h"
#include "word_bank.h"

#include <algorithm>
#include <iostream>
#include <limits>
#include <stdexcept>

std::vector<std::string> pick_random_words(const Config& config);

int main(int argc, char** argv)
{
    try {
        const std::string path = argc > 1 ? argv[1] : "data/words.json";
        const std::string level = argc > 2 ? argv[2] : "english_1k";
        Config config;
        if (argc > 3) {
            const std::string text = argv[3];
            if (text.empty() || !std::all_of(text.begin(), text.end(),
                    [](unsigned char ch) { return ch >= '0' && ch <= '9'; }))
                throw std::invalid_argument("Word count must contain digits only");
            const auto count = std::stoull(text);
            if (count == 0 || count > std::numeric_limits<std::size_t>::max())
                throw std::invalid_argument("Word count is outside the supported range");
            config.word_count = static_cast<std::size_t>(count);
        }
        const WordBank bank = load_word_bank(path);
        config.word_list = get_word_list(bank, level);
        for (const auto& word : pick_random_words(config))
            std::cout << word << '\n';
    } catch (const std::exception& error) {
        std::cerr << "Word bank error: " << error.what() << '\n';
        return 1;
    }
}
```

The main function catches load and validation errors. It prints one generated
word per line, so `wc -l` can verify the requested session length.

For this isolated check, save the sampling example as
`/tmp/keysprint-random-words.cpp`. This avoids linking the old game implementation
while you are checking the new loader. From the repository root, run:

```bash
g++ -std=c++17 -O2 -Wall -Wextra -Werror \
  -Icpp -Icpp/include \
  /tmp/keysprint-word-demo.cpp /tmp/keysprint-random-words.cpp cpp/word_bank.cpp \
  -o /tmp/keysprint-word-demo
/tmp/keysprint-word-demo data/words.json english_5k 50
/tmp/keysprint-word-demo data/words.json english_5k 50 | wc -l
```

The final command must print `50`. Repeat using `english_1k`, `english_10k`, and
`english_25k`. A nonexistent file, an unknown level, a count of `0`, and a count
such as `abc` must print an error and exit with status `1`.

## 9. Implement the dedicated settings module and Ctrl+F view

The detailed [implementation plan](superpowers/plans/2026-11-07-word-bank-handoff.md)
lists the files, interfaces, and checks. Use the following behavior:

| Key | Behavior in the settings screen |
| --- | --- |
| `1`, `2`, `3`, `4` | Select English 1k, 5k, 10k, or 25k |
| `w` | Start editing the session word count |
| Digits and Backspace | Edit the count while its field is active |
| Enter | Commit a positive whole-number count |
| Ctrl+N | Validate any pending count, select the pool, and start a fresh session |
| Ctrl+C | Exit, preserving your existing shortcut |

These editing keys are proposed implementation defaults. The requested behavior
is Ctrl+F opening configuration, with working difficulty and word-count controls.

Keep a small `ConfigEditor` value for the count buffer and any error message.
Handle settings keystrokes in `config.cpp` while `State::Config` is active.
`render.cpp` remains responsible for drawing and should call a settings-view
function for that state instead of leaving the current TODO branch blank.

Your current `handle_key()` already switches to `State::Config` on Ctrl+F. Add
the settings input branch after the global shortcuts and before gameplay input.
Settings keys must not reach `apply_key()` while configuration is open.

When Ctrl+N starts the next game:

1. Validate or commit a pending count; stay in configuration and show an error
   when the count is empty, zero, nonnumeric, or outside `std::size_t` range.
2. Assign `app.config.word_list` from the cached bank using
   `difficulty_key(app.config.difficulty)`.
3. Call `new_game(app.config)`, clear the previous result, and enter
   `State::Playing`.

Opening settings does not replace the current game's words. The documented way
out of settings is a fresh Ctrl+N session; this handoff does not introduce resume
behavior or change timer calculations. Settings are for the current application
run; saving them across launches is separate work.

## 10. Update startup, rendering, and the Makefile yourself

- Load the bank once in `main.cpp`, before `enable_raw_mode()`. Catch failures and
  exit normally so a missing data file does not leave the terminal in raw mode.
- Keep the bank in `App`, select the initial English 1k vector, and create the
  initial game with the current defaults of 100 words and 30 seconds.
- Pass `ConfigEditor` to the renderer so the settings view can display draft
  input and validation errors. Keep `State` in `game.h`.
- Add `config.cpp` and `word_bank.cpp` to `SOURCES` in `cpp/Makefile`.
- Add `config.h` and `word_bank.h` to `HEADERS`. Add `-Iinclude` to `CXXFLAGS`.
- Keep `cpp/include/nlohmann/json.hpp` as a target dependency, so replacing the
  header causes a rebuild. Continue using C++17 and the existing warning flags.

After those changes, build from the repository root without automatically
launching the terminal game:

```bash
make -C cpp keysprint
```

Then launch it from the supported subfolder:

```bash
cd cpp
./keysprint
```

Press Ctrl+F. The settings screen must show the selected difficulty, word count,
and keyboard controls. Select English 5k, enter 50 words, and press Ctrl+N. Verify
the next session has 50 words from that pool. Repeat from the results screen.

The terminal application and its Ctrl+F behavior remain for you to implement and
test. Only the shared data and documentation are delivered by this handoff.

## Verified loading cost

The delivered word bank is 671,791 bytes, about 656 KiB. In a local test, the
documented C++ loader took a median of 13.2 ms across 25 warm-cache runs, ranging
from 11.7 to 20.8 ms. The build used C++17 with `-O2 -Wall -Wextra -Werror`.

That timing includes the cached file read, JSON parsing, validation, and conversion
of all four arrays into vectors. It measures this loader on this machine, rather
than a universal startup time. Keep the bank loaded to make it a one-time cost
per launch. [Verification details](word-bank-verification.md).
