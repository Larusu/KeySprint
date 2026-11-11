# C++ usage

The C++ version uses main's boxed layout, scrolling text, Time/Words selectors,
and character statistics. Shared vocabulary is loaded once from `data/words.json`.
Java and Python application files are unchanged relative to main.

## Build and run

From the project root:

```bash
make -C cpp
make -C cpp run
```

`make` builds only. `make run` starts the game. From inside `cpp`, omit `-C cpp`.
On Windows, build with a C++17 compiler and GNU Make; the target is `keysprint.exe`.
The native Windows terminal handling from main is retained.

You can also launch with an explicit data path:

```bash
./cpp/keysprint data/words.json
```

Without a path, the game tries `data/words.json`, then `../data/words.json`.
Invalid or missing data is reported before terminal input mode is enabled.
Each pool must have at least two distinct words so timed refills can avoid repeats.
The vendored nlohmann/json 3.12.0 header and its Makefile include path are included.

## Modes and vocabulary

The default is Time mode, 30 seconds, English 1k. The timer starts on the first
letter or digit. Time mode keeps adding words until the timer ends. Words mode
has no time limit and finishes when the last word is submitted or typed correctly.

Time presets are 15, 30, 60, and 120 seconds. Word-count presets are 10, 25, 50,
and 75, with 25 selected by default. Custom counts accept 1 through 10,000.

Vocabulary choices are English 1k, 5k, 10k, and 25k. The actual pool sizes are
933, 4,772, 9,474, and 24,165. Every word has at least three lowercase letters.
Larger pools include the smaller pools. Repeats are allowed, except consecutive
words, including the boundary between timed refills.
See [source and license notes](../data/SOURCES.md).

## Controls

| Key | Action |
| --- | --- |
| Ctrl+F | Open configuration from gameplay or results; abandon the old round |
| Tab / Shift+Tab | Move forward/back between the three configuration sections |
| Left / Right | Cycle the focused setting |
| 1 / 2 / 3 / 4 | Select English 1k / 5k / 10k / 25k in configuration |
| w | Begin a custom count in Words-mode configuration |
| Digits / Backspace | Edit the count draft; digits do not change difficulty here |
| Enter | Save a count draft; otherwise leave configuration |
| Esc | Cancel a count draft; otherwise leave configuration or restart gameplay/results |
| Ctrl+N | Save and start from configuration, restart from results, or reshuffle before typing |
| Ctrl+C | Exit from any screen |

Selector changes update the preview while configuration stays open. Enter,
Esc, or Ctrl+F returns to gameplay. During count editing, Ctrl+F saves the draft
and returns to gameplay; invalid drafts stay open with an error. Ctrl+N also
validates the draft before starting. Esc discards only the draft.

After a custom count, Right selects the first preset and Left selects the last.
The selected custom number is shown accurately in the bar. The terminal needs
at least 74 columns and 17 rows; a smaller window shows a resize message.

## Checks

Run the C++ regression suite from the project root:

```bash
make -C cpp test
```

On Linux/macOS, the terminal smoke tests also check real keyboard input,
configuration, results, layout boundaries, and terminal restoration:

```bash
python3 cpp/tests/terminal_smoke.py
```

Clean the game and test executables with `make -C cpp clean`.
