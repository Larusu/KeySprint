# Build spec: KeySprint shared word bank and manual implementation handoff

Type: app

## Goal

Provide shared English JSON vocabulary and explicit C++ implementation instructions while leaving all application programming to the user.

## Success looks like

Read the same 4 ordered pools in C++, Python, and Java; deliver a guide covering the loader, sampling, dedicated configuration, and Ctrl+F; preserve all application and unrelated project files.

## Users

The requester and KeySprint contributors working on a terminal typing game.

## Must have

| # | Must have | Accept test |
|---|---|---|
| 1 | One shared UTF-8 data/words.json at the repository root, using standard JSON with schema_version 1, separate arrays under levels.english_1k, levels.english_5k, levels.english_10k, and levels.english_25k, actual counts, and source metadata | Parse the identical file using C++, Python, and Java readers without generating language-specific copies; verify schema_version is 1, all 4 named arrays contain the same ordered words in every reader, and every recorded count equals its array length. |
| 2 | Cumulative vocabulary: construct the original Google 1k and 5k prefixes, full Google base, and eligible wordfreq extension, then remove all words shorter than 3 without replacement | Compare the smaller pools with the corresponding prefixes of the next pool; compare every pool with its original construction filtered to words of at least 3 letters in source order. |
| 3 | Unique lowercase a-z vocabulary of at least 3 letters and a shared profanity-blocklist.json documenting exact whole-word filtering of the supplement | Check every practice word against ^[a-z]{3,}$, check within-pool uniqueness, and verify no supplemental entry appears in the delivered blocklist. |
| 4 | A documented C++ loader and sampler example with explicit setup, paths, commands, explanations, and error behavior | Extract the guide examples into temporary files outside the repository, compile with C++17 and warnings as errors, generate 100 words from each level, and verify membership, no adjacent repeats, and invalid-input failures. |
| 5 | A manual implementation plan for the dedicated configuration module, Ctrl+F screen, and new-session settings | Read the plan and guide and locate the exact module responsibilities, settings controls, Ctrl+F entry, Ctrl+N application of difficulty and count, positive-count validation, and future terminal acceptance checks. |
| 6 | Pinned provenance, upstream attribution, schema documentation, and actual counts of 933, 4772, 9474, and 24165 | Check source commits and SHA-256 values against the downloaded inputs, validate the data schema, and verify each recorded count and content digest. |
| 7 | Data and documentation updated without modifying application or unrelated project contents | Compare application and unrelated file SHA-256 digests with the pre-edit baseline and confirm only the authorized word-bank data and handoff documents changed. |

## Out of scope

- Application source edits, including new executable examples
- Makefile updates and dependency installation in the repository
- Implementing configuration interaction or keyboard shortcuts
- Python or Java gameplay feature work
- Scoring-formula redesign and saved-result persistence

## Constraints

- The user owns all code changes; place implementation examples in the guide only.
- Use one repository-root data/words.json unchanged across C++, Python, and Java.
- Use UTF-8 standard JSON, schema_version 1, and identical stable level keys with ordinary JSON value types.
- Preserve source order and original pool boundaries while removing every one- and two-letter practice word; do not replace removed entries.
- Append eligible wordfreq entries in source order and record actual counts; never invent words or duplicate-pad a level.
- Apply the delivered whole-word blocklist to new supplemental entries only; apply the minimum length of 3 to every vocabulary pool.
- Preserve java/main.java, .vscode, cpp/config.cpp, cpp/config.h, and every other pre-existing file.
- Temporary verification programs and libraries stay outside the repository and are not application integration.
- Document path resolution from the repository root and language subfolders without machine-specific paths in the proposed application.

## Reject if

- Any application source, build, configuration, or unrelated project file is modified.
- Instructions omit exact paths, setup, commands, or explanations.
- Vocabulary is not cumulative or any recorded count is incorrect.
- A new supplemental word matches the delivered blocklist.
- A language requires transformed vocabulary or reads different ordered word arrays.
- The documentation claims the Ctrl+F feature has already been implemented.

- A practice word has fewer than 3 letters.

## Open risks

- The nominal 1k, 5k, 10k, and 25k levels contain 933, 4772, 9474, and 24165 words after the approved minimum-length filter.
- Eight retained Google base entries overlap the current combined blocklist; the supplemental profanity filter is not applied retroactively to the base.
- The current game has three difficulty enum values; the user must implement the planned four-value replacement and update dependent references.

## Implementation assumptions

| # | I am assuming | Risk |
|---|---|---|
| 1 | Future sampling uses equal probabilities among eligible words, permits repeats, and excludes the immediately previous word. | low |
| 2 | Future settings changes take effect on a fresh Ctrl+N session; opening configuration does not replace current game words. | med |
| 3 | The proposed settings controls use keys 1-4 for difficulty, w for count editing, and Enter to commit a positive integer. | med |

The editing keys and new-session behavior above are documented implementation
defaults. Application coding belongs to the user.

## User-owned application requirements

The following features are specified for future implementation; they are not
implemented by this data-and-documentation delivery.

| Requirement | Future acceptance check |
| --- | --- |
| Move the existing configuration into `cpp/config.h` and `cpp/config.cpp` | One `Config` definition, four vocabulary choices, and settings input delegated to the dedicated module |
| Open configuration with Ctrl+F | From playing and results, Ctrl+F shows labeled difficulty and word-count controls |
| Apply settings on Ctrl+N | English 5k and 50 words produces a fresh 50-word session from that pool without adjacent repeats |
| Validate counts and data errors | Zero, nonnumeric, and overflowing counts are rejected; missing or malformed vocabulary is reported before raw terminal mode |

## Delivered files and next action

Read the [C++ guide](cpp-word-bank-guide.md), the
[manual implementation plan](superpowers/plans/2026-11-07-word-bank-handoff.md),
and the [source notes](../data/SOURCES.md). The shared data is
[words.json](../data/words.json), with its
[blocklist](../data/profanity-blocklist.json) and
[schema](../data/words.schema.json).

Next action for the user: move the existing settings definitions into the
dedicated configuration header, following Task 1 of the plan. Code examples in
the guide were checked separately; application integration remains pending.
