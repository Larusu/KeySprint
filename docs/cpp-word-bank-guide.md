# C++ word-bank usage

The C++ game loads the shared vocabulary from `data/words.json` once at startup.
The pools contain 933, 4,772, 9,474, and 24,165 words. Every practice word has
at least three lowercase letters. See [source and license notes](../data/SOURCES.md).

## Build and run

From the project root:

```bash
make -C cpp keysprint
./cpp/keysprint
```

An explicit data path is also supported:

```bash
./cpp/keysprint data/words.json
```

From inside `cpp`, run `./keysprint`. Without an argument, the program looks for
`data/words.json`, then `../data/words.json`. Startup reports invalid or missing
vocabulary before enabling terminal input mode.

The vendored parser is `cpp/include/nlohmann/json.hpp` (nlohmann/json 3.12.0).
The Makefile already includes its search path and rebuild dependency.

## Controls

| Key | Action |
| --- | --- |
| Ctrl+F | Open configuration from gameplay or results |
| 1 / 2 / 3 / 4 | Select English 1k / 5k / 10k / 25k outside count editing |
| w | Begin editing the session word count |
| Digits / Backspace | Edit the draft count |
| Enter | Commit the draft count |
| Ctrl+N | Start a new session from configuration or results |
| Ctrl+C | Exit |

The default settings are English 1k, 100 words, and a 30-second limit. The current
implementation ends a round when its words are completed or the timer expires;
it does not provide separate Time and Words modes. Configuration applies to the
next session started with Ctrl+N. Invalid empty, zero, and overflowing counts
retain the previous committed value and display an error.

## File responsibilities

- `word_bank.cpp`: parse and validate the shared JSON; return cached pools.
- `config.cpp`: select difficulty and edit the session count.
- `game.cpp`: sample session words without adjacent duplicates and process input.
- `main.cpp`: find the data, route controls, and start sessions.
- `render.cpp`: draw gameplay, configuration, and results.

## Current integration limits

The working branch is based on an older main. Current remote main already has
separate Time/Words modes, selector navigation, and scrolling; preserve those
features when integrating this implementation. Avoid copying the old C++ files
wholesale over current main. Large accepted counts also need an allocation bound
and exception-safe terminal restoration before the feature is ready to merge.

## Clean the build

```bash
make -C cpp clean
```
