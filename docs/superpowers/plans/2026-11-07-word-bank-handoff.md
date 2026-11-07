# Shared word bank C++ implementation plan

**Implementation owner:** Jeff. This is a manual handoff. The delivered data and
documentation do not implement these application changes.

**Goal:** Read the shared vocabulary, edit difficulty and session length in a
dedicated configuration module, and open that configuration with Ctrl+F.

**Architecture:** A word-bank module parses JSON once into four cached vectors.
The configuration module owns the selected difficulty and count. The application
routes shortcuts and starts fresh games from the chosen vector; rendering draws
the active view.

**Tech stack:** Existing C++17 terminal application, Make, nlohmann/json 3.12.0.

**Spec:** [Shared word-bank spec](../../word-bank-spec.md).

## Global constraints

- One shared `data/words.json` unchanged across C++, Python, and Java.
- Level keys: `english_1k`, `english_5k`, `english_10k`, `english_25k`.
- Actual pool sizes: 933, 4,772, 9,474, 24,165. All practice words have at least
  three lowercase letters; the original level boundaries are retained without
  replacing filtered words.
- Keep the initial defaults of 100 session words and 30 seconds; use English 1k
  as the initial vocabulary.
- Random selection allows repeats while preventing consecutive identical words.
- Apply settings on Ctrl+N; Ctrl+F opens configuration from playing or results.
- The user performs all source, dependency, and Makefile changes.

## Review focus

- Starting from the root or `cpp` must find the shared file; missing data must
  leave the terminal usable.
- Malformed JSON, missing keys, wrong counts, and empty arrays need clear errors.
- Zero-count, empty, and single-word sampling inputs must fail or finish without
  an infinite retry loop.
- A count being edited must not silently start a session when empty, invalid,
  zero, or too large for `std::size_t`.
- Digits must edit the count while its field is active; configuration keys must
  not become typed game characters.

## Task 1: Move configuration and implement settings editing

**Files to modify:** `cpp/config.h`, `cpp/config.cpp`, `cpp/game.h`,
`cpp/scoring.h`, plus any remaining old difficulty references.

**Interfaces produced:**

- `Difficulty { English1k, English5k, English10k, English25k }`.
- `difficulty_key(Difficulty) -> std::string_view` and
  `to_string(Difficulty) -> std::string_view`.
- `Config` with the current `word_list`, `word_count`, `difficulty`, and
  `time_limit` fields; use the example in guide section 5.
- `ConfigEditor` with `bool editing_count = false`, `std::string count_input`,
  and `std::string error`.
- `void handle_config_key(Config&, ConfigEditor&, char)`.
- `bool commit_word_count(Config&, ConfigEditor&)`: return true when no edit is
  pending or a positive in-range integer is committed; otherwise retain the
  editor, set an error, and return false.

- [ ] Move the existing definitions into `config.h`; remove their old copies
  from `game.h` and include `config.h` there. Keep `State` in `game.h`.
- [ ] Replace all old `Easy`, `Medium`, and `Hard` enumerators, including the
  result default. Confirm result labels still use `to_string()`.
- [ ] Implement keys 1-4 for difficulty and `w` for count editing. While editing
  the count, digits append to the buffer and Backspace removes its last digit.
- [ ] On Enter, validate all characters before conversion. Reject empty input,
  zero, signs, nondigits, and overflow; accept 50 and 100. Store a clear error
  while retaining the previous valid committed count.
- [ ] Check that typing `1` in the active count field changes the buffer and
  leaves the selected difficulty unchanged. Check invalid input does not reset
  the previous valid setting.
- [ ] Compile the module with the following command. Expect a successful compile.

```bash
g++ -std=c++17 -Wall -Wextra -Werror -Icpp -c cpp/config.cpp \
  -o /tmp/keysprint-config.o
```

## Task 2: Implement cached loading and sampling

**Files to create:** `cpp/word_bank.h`, `cpp/word_bank.cpp`, and the downloaded
`cpp/include/nlohmann/json.hpp`.

**Files to modify:** the old loader declaration in `cpp/game.h` and its
definition and sampling function in `cpp/game.cpp`.

**Interfaces produced:**

- `WordBank = std::unordered_map<std::string, std::vector<std::string>>`.
- `WordBank load_word_bank(const std::string& path)`.
- `const std::vector<std::string>& get_word_list(const WordBank&, std::string_view)`.
- Retain `std::vector<std::string> pick_random_words(const Config&)`.

- [ ] Add the pinned single header using guide section 4.
- [ ] Implement the loader and lookup using guide section 6. Verify schema
  version 1, language `en`, all four arrays, nonempty vectors, matching counts,
  lowercase `a-z`, a minimum length of three letters, and unique words.
- [ ] Remove the old zero-argument hardcoded `get_word_list()` and its declaration.
- [ ] Implement guide section 7's sampler with a generator seeded once and an
  index draw that excludes the previous index.
- [ ] Compile and run the isolated smoke program with the exact commands in
  guide section 8. Generate 100 words for each level and check pool membership
  and absence of adjacent duplicates.
- [ ] Check a missing file, invalid JSON, missing level, wrong count, empty
  array, a one- or two-letter vocabulary entry, and unknown requested level.
  Expect a useful error and nonzero exit.
- [ ] Check sampler inputs of zero requested words and an empty pool. Check a
  one-word pool with a request of 1 succeeds, and a request of 2 fails promptly.

## Task 3: Wire the application and render the Ctrl+F screen

**Files to modify:** `cpp/main.cpp`, `cpp/render.h`, `cpp/render.cpp`, and
`cpp/Makefile`.

**Interfaces consumed:** Task 1's configuration and editor functions; Task 2's
cached bank and lookup. Extend `render()` with a final
`const ConfigEditor&` parameter so it can show the count draft and error.

- [ ] Load the bank before `enable_raw_mode()` and catch startup errors. Resolve
  an explicit data path first, otherwise try the two documented launch paths.
- [ ] Keep `WordBank` and `ConfigEditor` in `App`. Assign the initial pool using
  `get_word_list(bank, difficulty_key(config.difficulty))`, then create the game.
- [ ] Keep Ctrl+C global. On Ctrl+F, enter `State::Config` and initialize its
  editor. While that state is active, delegate ordinary input to
  `handle_config_key()` instead of `apply_key()`.
- [ ] On Ctrl+N from configuration, call `commit_word_count()` first. Remain in
  configuration on failure; on success, assign the selected cached pool, create
  a fresh game, clear `app.result`, and switch to `State::Playing`.
- [ ] Replace the renderer's blank `State::Config` branch with a settings view
  showing current level, committed count, draft/error when present, and controls.
- [ ] Update the renderer's declaration, definition, and call consistently.
  Add both new modules and headers to the Makefile, add `-Iinclude`, and depend
  on the nlohmann header. Remove obsolete `srand()` setup once no `rand()` remains.
- [ ] Run `make -C cpp keysprint`. Expect the complete application to compile.
- [ ] Launch from the root and from `cpp`; check data is found. Temporarily
  supply a nonexistent path and confirm the terminal stays usable after failure.
- [ ] From gameplay, press Ctrl+F, select English 5k, enter 50 words, then press
  Ctrl+N. Expect a new 50-word session from that pool without adjacent repeats.
- [ ] Repeat from results. Check Ctrl+C exits from configuration. Check a pending
  invalid or overflowing count keeps the settings view open and shows its error.

## Handoff state

The shared JSON and documentation are delivered. Every task checkbox above is
pending user implementation. Next action: Task 1, moving the existing settings
definitions into the dedicated configuration header and implementing its editor.
