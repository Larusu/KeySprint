# Word-bank delivery checks

Checked on 2026-11-07. This report covers the delivered data and documentation.
Application integration belongs to the user.

| Check | Result | Evidence |
| --- | --- | --- |
| Schema and JSON format | Pass | Draft 2020-12 schema validated with jsonschema 4.23.0; the word bank satisfies it; one- and two-letter test entries are rejected |
| Counts, valid words, and cumulative pools | Pass | 933 / 4,772 / 9,474 / 24,165; every word is lowercase `a-z` with at least three letters, each pool is unique, and each smaller pool is the next pool's prefix |
| Sources and filtering | Pass | Original source metadata is retained; the 9,474 remaining Google entries keep their order; all 14,691 retained supplemental words pass the 302-word whole-word blocklist |
| Cross-language reads | Pass | Python's `json`, C++ nlohmann/json 3.12.0, and Java Gson 2.11.0 parse identical complete contents for the word bank, blocklist, and schema |
| C++ guide examples | Pass | Extracted into temporary files and compiled with C++17, `-O2 -Wall -Wextra -Werror`; 100 generated words per level stay in their pool without adjacent repeats |
| Invalid inputs and sampler edge cases | Pass | 20 file/schema/input errors, including one- and two-letter words, report diagnostics and exit 1; direct checks cover zero count, empty pool, a one-word pool, and a two-word pool |
| Existing project contents | Pass | All 22 application and unrelated files match their pre-edit SHA-256 digests; only the authorized word-bank data and handoff documentation changed |

## Actual data sizes

`data/words.json` contains 39,344 entries across its four arrays, representing
24,165 distinct words. Its formatted size is 671,791 bytes. Counts remain below
the nominal labels because filtering removes short words without replacing them.
The source notes record the eight retained base words that overlap the
current combined exclusion list; the new-word filter applies to the supplement.

The length edit removed 67, 228, 420, and 502 entries from the original four
pools. Each updated pool is exactly its previous pool with words shorter than
three removed. No new words were added and source order was retained.

## Loading measurement

The documented C++ loader was measured over 25 warm-cache reads:

| Measurement | Time |
| --- | ---: |
| Median | 13.24 ms |
| Minimum | 11.71 ms |
| Maximum | 20.78 ms |

This includes file reading, JSON parsing, validation, and converting all four
arrays into cached vectors. It used the local G++ compiler with C++17 and `-O2`.
It does not measure cold storage reads, a debug build, or the game's frame rate.

## Deferred application checks

The game has not been rebuilt or modified by this delivery. Its current Ctrl+F
branch and hardcoded vocabulary remain as they were.

After implementing the [manual plan](superpowers/plans/2026-11-07-word-bank-handoff.md),
check Ctrl+F from gameplay and results, difficulty selection, positive-count
validation, Ctrl+N application of the settings, and Ctrl+C exit. These checks
are pending user implementation, so this report does not mark them as passing.

The next action is Task 1 of that plan: move the existing configuration definition
into the dedicated module and implement its editor.
