# C++ word-bank and configuration setup

The backslash is only necessary when a Makefile definition continues onto another
physical line. These two definitions mean the same thing:

```makefile
SOURCES = main.cpp game.cpp config.cpp
```

```makefile
SOURCES = main.cpp game.cpp \
          config.cpp
```

I would use single-line definitions while setting this up. The complete Makefile
in step 10 uses that form. If you use a continuation, put the backslash directly
before the newline, without trailing spaces.

This is a manual walkthrough. The shared data already exists; the C++ changes
below are for you to implement. The [companion guide](cpp-word-bank-guide.md)
contains the full configuration-header, loader, sampler, and isolated test examples.

## 1. Start in the project root and check the tools

The project root is the folder containing `cpp`, `python`, `java`, `data`, and
`docs`. On this computer:

```bash
cd /home/jeff/Documents/KeySprint
g++ --version
make --version
curl --version
```

All three tools are already available here. `g++` compiles C++, `make` runs your
build rules, and `curl` downloads the JSON library header.

The setup commands below start from this root unless a step explicitly says to
enter `cpp`.

## 2. Understand the files you are preparing

The intended layout is:

```text
KeySprint/
├── data/
│   ├── words.json
│   ├── profanity-blocklist.json
│   ├── words.schema.json
│   ├── SOURCES.md
│   └── licenses/
├── cpp/
│   ├── include/
│   │   └── nlohmann/
│   │       └── json.hpp
│   ├── config.h
│   ├── config.cpp
│   ├── word_bank.h
│   ├── word_bank.cpp
│   ├── game.h
│   ├── game.cpp
│   ├── main.cpp
│   ├── render.h
│   ├── render.cpp
│   ├── scoring.h
│   ├── scoring.cpp
│   ├── terminal.h
│   ├── terminal.cpp
│   └── Makefile
├── python/
└── java/
```

At the time of this walkthrough, the shared data files exist. `config.h` exists
with `#pragma once`, and `config.cpp` is empty. You still need to create
`word_bank.h`, `word_bank.cpp`, and download `json.hpp`.

| File | What belongs there |
| --- | --- |
| `data/words.json` | The vocabulary read by all three versions |
| `data/profanity-blocklist.json` | The exclusion data used to prepare supplemental vocabulary |
| `data/words.schema.json` | A description of the word-bank format for validation tools |
| `data/SOURCES.md` and `data/licenses/` | Source versions, filtering details, and attribution |
| `cpp/include/nlohmann/json.hpp` | The downloaded JSON parser library |
| `cpp/config.h` | Configuration types and declarations |
| `cpp/config.cpp` | Difficulty selection and word-count editing logic |
| `cpp/word_bank.h` | The word-bank type and loader/lookup declarations |
| `cpp/word_bank.cpp` | JSON parsing, validation, and conversion into vectors |

Only `words.json` is needed for normal vocabulary loading. The schema and
blocklist do not need to be loaded on every game or keystroke.

## 3. Keep the shared data at the root

Use the same `data/words.json` from C++, Python, and Java. Each language uses its
own parser to access the same strings and arrays.

| Level key | Actual words |
| --- | ---: |
| `english_1k` | 933 |
| `english_5k` | 4,772 |
| `english_10k` | 9,474 |
| `english_25k` | 24,165 |

All practice words contain at least three lowercase letters. The names are
nominal level labels; filtering reduced their actual sizes.

For English 5k, the lookup path is `levels` followed by `english_5k`.
`metadata.actual_counts.english_5k` records its actual pool size. A session count
of 50 means choosing 50 practice words from that 4,772-word pool.

Relative paths depend on the folder from which the application is running:

| Working directory | Data path |
| --- | --- |
| Repository root | `data/words.json` |
| `cpp` | `../data/words.json` |

Changing the data does not require recompiling C++. Restart the application to
reload its cached word bank.

## 4. Download the parser header

From the project root, run:

```bash
mkdir -p cpp/include/nlohmann
curl --fail --location --output cpp/include/nlohmann/json.hpp https://raw.githubusercontent.com/nlohmann/json/v3.12.0/single_include/nlohmann/json.hpp
```

`mkdir -p` creates the required folders. The second command saves the pinned
single-header library at the path shown in the layout.

Your word-bank implementation can then use:

```cpp
#include <nlohmann/json.hpp>
```

The Makefile's `-Iinclude` option tells the compiler to search `cpp/include`
when the build runs inside `cpp`. The library is header-only, so its implementation
comes through the include; no separate JSON-library linking flag is required.
See the [official header setup](https://json.nlohmann.me/integration/#header-only).

## 5. Fill in the dedicated configuration files

### `cpp/config.h`

Move the existing configuration definitions out of `game.h` into this header.
Use the complete header in [companion-guide section 5](cpp-word-bank-guide.md#5-move-the-existing-configuration-definition).

It defines:

- `Difficulty` with `English1k`, `English5k`, `English10k`, and `English25k`.
- `difficulty_key()` to map those enum values to shared JSON keys.
- `to_string()` to return labels for the results screen.
- `Config`, holding `word_list`, `word_count`, `difficulty`, and `time_limit`.

Keep the starting values at English 1k, 100 session words, and 30 seconds.

Also add the editor state and editing declarations to this header:

```cpp
struct ConfigEditor {
    bool editing_count = false;
    std::string count_input;
    std::string error;
};

void handle_config_key(Config& config, ConfigEditor& editor, char ch);
bool commit_word_count(Config& config, ConfigEditor& editor);
```

The editor stores text currently being typed into the settings field. Keeping
that draft separate lets the last valid count remain usable when input is invalid.

### `cpp/config.cpp`

Include `config.h` and implement the two editing functions declared above.

`handle_config_key()` should:

1. While count editing is active, append digits, handle Backspace, and use Enter
   to call `commit_word_count()`.
2. Otherwise, use keys `1` through `4` to select the difficulty.
3. Use `w` to begin editing the word count.

Check the active editing mode before interpreting digits as difficulty keys.
Typing `1` in the count field must append a digit to that field.

`commit_word_count()` should return true when no edit is pending or a positive
integer is committed. For empty input, zero, nondigits, or overflow, retain the
previous valid count, set `editor.error`, and return false.

## 6. Create the word-bank module

### `cpp/word_bank.h`

This header declares the public interface. Its complete contents are:

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

`WordBank` maps each level's string key to its vector of words.
`load_word_bank()` constructs that collection. `get_word_list()` retrieves the
selected vector by reference, avoiding a copy during lookup.

### `cpp/word_bank.cpp`

Include `word_bank.h` and `nlohmann/json.hpp`. Implement the declarations using
the complete body in [companion-guide section 6](cpp-word-bank-guide.md#6-put-json-reading-in-a-word-bank-module).

The loader's work is:

1. Open the requested file with `std::ifstream`.
2. Parse it with `nlohmann::json::parse()`.
3. Check schema version 1, language `en`, and all four level keys.
4. Convert each array into `std::vector<std::string>`.
5. Check nonempty pools, accurate counts, uniqueness, lowercase letters, and a
   minimum word length of three.
6. Return the bank containing all four vectors.

The parser understands JSON syntax. Your loader checks the structure and rules
KeySprint expects. The separate `.schema.json` file is not automatically run by
`nlohmann/json`.

## 7. Update the existing game definitions and sampling

### `cpp/game.h`

- Include `config.h`.
- Remove the old `Difficulty`, `to_string()`, and `Config` definitions after
  moving them. Each definition should exist in one place.
- Remove the old zero-argument `get_word_list()` declaration. The replacement
  is declared in `word_bank.h`.
- Keep `Clock`, `State`, `Stats`, `Game`, and the gameplay function declarations.

### `cpp/game.cpp`

Remove the hardcoded zero-argument `get_word_list()` definition.

Keep the existing `pick_random_words(const Config&)` signature. Replace its
`rand()` implementation with [companion-guide section 7's sampler](cpp-word-bank-guide.md#7-replace-random-selection-when-you-are-ready).
It uses `std::mt19937`, seeded once, and a uniform index draw that excludes the
previous index. This permits later repeats while preventing consecutive identical
words. Empty pools and impossible one-word-pool requests must fail clearly.

Your existing `new_game(config)` can continue to call this function and resize
the typed-word vector.

### `cpp/scoring.h`

Change the default result difficulty from `Difficulty::Easy` to
`Difficulty::English1k`. Search the C++ folder for remaining uses of `Easy`,
`Medium`, and `Hard` and update them to the new vocabulary choices.

The scoring formulas do not need to change for this setup.

## 8. Connect startup and keyboard handling in `main.cpp`

Include `config.h` and `word_bank.h` alongside your existing project headers.
Keep the loaded bank and the configuration editor in `App`:

```cpp
struct App {
    WordBank word_bank;
    Config config;
    Game game;
    State state;
    std::optional<Result> result;
    ConfigEditor config_editor;
};
```

Update the aggregate initialization to match this field order.

At startup:

1. To support an explicit data-path argument, use `int main(int argc, char** argv)`.
   When an argument is supplied, use `argv[1]` as the word-bank path.
2. Without an argument, check `data/words.json`, then `../data/words.json`.
   Select an existing regular file; report both locations if neither is found.
3. Load the bank once. Catch startup errors and print their messages before
   calling `enable_raw_mode()`.
4. Create the default `Config`, assign its word list from the selected cached
   pool, and create the first game.
5. Keep the bank in `App` for later difficulty changes.
6. Enter raw mode and run your existing input/update/render loop.

After adopting the new sampler, remove `srand(time(0))` and any unused old random
headers once no `rand()` calls remain.

In `handle_key()`, keep global shortcuts first. Ctrl+F enters `State::Config`
and starts with a fresh `ConfigEditor`. Add configuration input handling before
the gameplay input branch:

```cpp
else if (app.state == State::Config)
    handle_config_key(app.config, app.config_editor, ch);
else if (app.state == State::Playing)
    apply_key(app.game, ch);
```

When Ctrl+N is pressed from configuration, call `commit_word_count()` first.
Stay on the settings screen if it returns false. Once the settings are valid:

```cpp
app.config.word_list = get_word_list(
    app.word_bank, difficulty_key(app.config.difficulty));
app.game = new_game(app.config);
app.result.reset();
app.state = State::Playing;
```

Use the same pool assignment when Ctrl+N starts another game from results.
Opening configuration does not replace the current game's words; Ctrl+N starts
the fresh session that applies the settings. Keep Ctrl+C available in every state.

## 9. Draw the configuration screen

In `render.h` and `render.cpp`, add a final `const ConfigEditor&` parameter to
`render()`. Update the call in `main.cpp` to pass `app.config_editor`.

Inside `render.cpp`, create a settings-view helper and call it from
`case State::Config`. Replace the current blank TODO branch.

The view should show the selected difficulty, committed word count, count draft
when editing, any error message, and these controls:

| Key | Action |
| --- | --- |
| Ctrl+F | Open settings from gameplay or results |
| 1-4 | Choose the vocabulary level when not editing the count |
| w | Edit the word count |
| Digits / Backspace | Change the count draft |
| Enter | Commit a valid count |
| Ctrl+N | Validate pending input and start a fresh session |
| Ctrl+C | Exit |

Keep drawing in the rendering module and editing decisions in `config.cpp`.

## 10. Use this complete Makefile

Replace `cpp/Makefile` with the following when the listed source and header files
have been created and implemented. Each variable definition is on one line, so
there are no continuation backslashes.

```makefile
.SILENT:

CXX      = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -Wmissing-declarations -Iinclude
TARGET   = keysprint
SOURCES  = main.cpp terminal.cpp game.cpp scoring.cpp render.cpp config.cpp word_bank.cpp
HEADERS  = game.h terminal.h scoring.h render.h config.h word_bank.h include/nlohmann/json.hpp

all: $(TARGET) run

.PHONY: all run clean

$(TARGET): $(SOURCES) $(HEADERS)
	$(CXX) $(CXXFLAGS) $(SOURCES) -o $(TARGET)

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET)
```

The three recipe commands beginning with `$(CXX)`, `./`, and `rm` must start
with an actual Tab character. The variable definitions and target lines do not
start with tabs. [GNU make recipe syntax](https://www.gnu.org/s/make/manual/html_node/Recipe-Syntax.html)
describes that distinction.

| Part | Meaning |
| --- | --- |
| `.SILENT:` | Hide command echoing while still allowing their output and errors |
| `CXX` | Choose the C++ compiler |
| `-std=c++17` | Use C++17 |
| `-Wall -Wextra -Wmissing-declarations` | Preserve your existing warning options |
| `-Iinclude` | Find `nlohmann/json.hpp` under the `cpp/include` directory |
| `TARGET` | Name the output executable |
| `SOURCES` | List the `.cpp` files compiled into that executable |
| `HEADERS` | Make header changes trigger a rebuild |
| `all` | Build and run when you invoke plain `make` |
| `$(TARGET): ...` | Rebuild the executable when a listed dependency changes |
| Compiler recipe | Compile and link the listed source files into `keysprint` |
| `run: $(TARGET)` | Ensure the executable is built before running it |
| `clean` | Remove the built executable |
| `.PHONY` | Treat `all`, `run`, and `clean` as actions even if similarly named files exist |

Keep `words.json` in the shared data folder. It is read at runtime and does not
belong in `SOURCES` or `HEADERS`.

## 11. Build and run after implementing the modules

From the project root, compile without automatically opening the terminal game:

```bash
make -C cpp keysprint
```

`-C cpp` makes `make` use that directory, so paths such as `main.cpp` and
`include/nlohmann/json.hpp` are relative to `cpp`.

To build and run using the default target:

```bash
make -C cpp
```

To run through the explicit run target:

```bash
make -C cpp run
```

To remove the executable:

```bash
make -C cpp clean
```

After implementing the explicit data-path argument, you can also launch directly
from the repository root:

```bash
./cpp/keysprint data/words.json
```

## 12. Check the completed behavior

1. Build the full application successfully with `make -C cpp keysprint`.
2. Launch it from the repository root and from `cpp`; confirm both find the data.
3. From gameplay, press Ctrl+F. Confirm that settings and their controls appear.
4. Select English 5k, press `w`, enter `50`, and press Enter.
5. Press Ctrl+N. Confirm the fresh session contains 50 words from that pool.
6. Confirm every target word has at least three letters and adjacent target words
   differ.
7. Repeat the settings change from the results screen.
8. Try an empty count, zero, and an overflowing count. Confirm a useful error
   appears and no invalid game starts.
9. Try a missing or malformed data file. Confirm the startup error appears before
   raw mode is enabled, leaving the terminal usable.
10. Check Ctrl+C exits from gameplay, results, and configuration.

For the complete isolated loader test, use
[companion-guide section 8](cpp-word-bank-guide.md#8-run-a-separate-loader-check-before-wiring-the-game).
These application checks are for your implementation; saving this walkthrough
does not modify or complete the C++ application.
