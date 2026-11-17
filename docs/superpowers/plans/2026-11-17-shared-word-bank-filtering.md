# Shared word-bank filtering implementation plan

> Superseded on 2026-11-17: the vocabulary was reviewed and cleaned directly; the blocklist workflow in this document was abandoned.

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Clean all shared vocabulary pools against an explicit exclusion list and make future cleanup repeatable and verifiable.

**Architecture:** Keep `data/words.json` as the shared application input. Add one standard-library Python utility for checking and cleaning the data, with exact whole-word matching and tests independent of the game UI. Application loaders continue consuming the existing version 1 document shape.

**Tech stack:** Python 3.10+, `json`, `hashlib`, `argparse`, `pathlib`, and `unittest`; existing Java and C++ loaders for compatibility checks.

**Spec:** [Shared word-bank filtering design](../specs/2026-11-17-word-bank-filtering-design.md).

## Global constraints

- Implement shared-data cleanup first. Python runtime filtering is a separate follow-up.
- Match whole lowercase words; preserve retained order and cumulative prefixes; remove without replacement.
- Preserve upstream source IDs, pinned commits, source fingerprints, licenses, and historical source/length-filter statistics.
- Use Python 3.10+ and the standard library, with no network requirement.
- Keep nominal level keys and `schema_version: 1`; recalculate actual pool sizes.
- Require at least two distinct words per pool for compatibility with Java and C++.
- Recheck `git status` before editing and preserve unrelated user changes. This planning pass started with a clean working tree.
- Keep implementation changes uncommitted unless the user authorizes commits.

## File map

- Create `scripts/word_bank.py`: pure validation/cleanup functions and command-line entry point.
- Create `scripts/tests/test_word_bank.py`: fixture-based tests and actual-data regression checks.
- Create `scripts/tests/WordBankSmoke.java`: a small standalone check using the real Java loader, compiled into a temporary directory.
- Modify `data/profanity-blocklist.json`: proposed additions and accurate project-specific metadata.
- Modify `data/words.json`: filtered pools and recalculated derived metadata.
- Modify `data/SOURCES.md`, `data/licenses/wordfreq-DATA-LICENSE.md`, `docs/cpp-word-bank-guide.md`, and `README.md`: current counts, filtering scope, provenance, and maintenance commands where relevant.
- Keep application source, JSON schema, and upstream license notices unchanged for this increment. These are scope boundaries, not instructions to ignore a discovered compatibility problem.

## Review focus

- A variant absent from the exclusion list remains eligible: test explicit inclusion of the reported variants and document the coverage limit.
- Harmless words contain blocked substrings: test exact matching with `class`, `analysis`, and the other spec fixtures.
- Hand-edited metadata is stale: checks fail; cleanup repairs derived fields without changing source history.
- Input is missing, malformed, or would leave too few words: report an error before any replacement; test that original bytes remain intact.
- Execution happens from another directory or runs twice: use file-relative defaults and prove read-only checks and cleanup idempotence.

## Task 1: Add read-only data validation

**Files:** create `scripts/word_bank.py` and `scripts/tests/test_word_bank.py`.

**Interfaces:**

- `read_documents(data_dir: Path) -> tuple[dict, dict]`: return word bank followed by blocklist; errors identify the failing path.
- `check_documents(bank: dict, blocklist: dict) -> list[str]`: return deterministic diagnostic strings; never mutate inputs.
- CLI: `python3 scripts/word_bank.py --check`; default to check mode. `--data-dir PATH` overrides the directory; default to the repository's `data` folder relative to the script.
- Exit 0 for valid data, 1 for invalid/unreadable data, and argparse's exit 2 for invalid arguments. Check mode never repairs files.

- [ ] Write failing unit tests with small valid cumulative pools and blocklist fixtures. Assert valid inputs produce `[]` and a blocked word in any one of the four pools produces a diagnostic naming both pool and word.
- [ ] Add assertions for missing files, malformed JSON, wrong document types, unsupported versions/language/match mode, missing/extra level keys, invalid vocabulary items, duplicate words, and singleton/empty pools. Reject booleans where integer metadata is required.
- [ ] Add tests for mismatched actual counts, pool fingerprints, blocklist count/fingerprint, and broken prefix relationships. Hash UTF-8 words in array order, one per line, with a final newline. If `metadata.filtering.blocklist_sha256` is present, require it to match the current blocklist fingerprint; test that a mixed old/new file pair is rejected even if its pools have no blocked overlap.
- [ ] Add tests proving check mode preserves input objects and file bytes, and defaults work when launched from a temporary working directory.
- [ ] Run `python3 -m unittest discover -s scripts/tests -p 'test_word_bank.py' -v`; confirm failures arise because the utility is missing before implementing it.
- [ ] Implement the interfaces. Validate shapes before calculating sets/hashes; report content problems without an unhandled traceback. Validate the current schema's required data fields and bounds needed by these checks; do not claim this replaces a general JSON Schema engine.
- [ ] Run the same unit tests and confirm they pass. Run `python3 scripts/word_bank.py --check` against the current bank; expect exit 1 listing known overlaps. That real-data failure is expected until Task 2.
- [ ] Review the diff: validation must not rewrite either data file.

## Task 2: Implement cleanup and apply the exclusion policy

**Files:** extend the utility/tests; modify both data JSON files.

**Interfaces:**

- Consumes Task 1's readers/checker.
- `clean_documents(bank: dict, blocklist: dict, *, updated_on: str) -> tuple[dict, dict]`: return new cleaned documents; leave inputs unchanged. Reject invalid structural content, duplicates, or invalid words; repair stale derived counts/hashes and remove blocked vocabulary.
- CLI: `python3 scripts/word_bank.py --write`. `--check` and `--write` are mutually exclusive. Print removed words/counts per pool and whether files changed.

- [ ] Write failing fixture tests showing removal from every level, including original base words. Assert the result preserves the order of survivors and cumulative prefixes, without padding.
- [ ] Use a synthetic blocked word `ass` with allowed words `class`, `classic`, `assessment`, `analysis`, and `assistance`; assert all allowed words survive. Separately test a listed plural is excluded and an unlisted variant is not automatically inferred.
- [ ] Test repair of stale derived metadata, preservation of source/history fields, no input mutation, minimum two surviving words, and no writes for invalid/missing input. Repeating cleanup on clean data must produce identical bytes, including on a later date.
- [ ] Run the unit tests to confirm the new tests fail, then implement cleanup. Use a set for membership and a list comprehension per pool; do not sort vocabulary or expand word stems automatically.
- [ ] Before any file replacement, validate the complete cleaned pair with `check_documents`. Serialize and stage both outputs in their own data directory, then replace only after preparation succeeds. Each replacement is atomic; document that two separate files cannot be replaced as one atomic transaction. A write error must exit nonzero, and a subsequent check must detect an inconsistent pair.
- [ ] Add the nine exact proposed exclusions listed in the spec. Merge them with existing entries, keep the blocklist sorted/unique, and record them in `metadata.project_additions`. Update its construction description without changing upstream contribution counts or attributing project additions to upstream sources.
- [ ] Recalculate blocklist `count` and `metadata.word_sha256`; set `applied_to` to all four vocabulary pools. In the bank, recalculate `metadata.actual_counts`, `metadata.level_sha256`, `construction.base_count` (10k size), and `construction.supplement_count` (25k size minus 10k size).
- [ ] Set `metadata.filtering.profanity_scope` to all four pools and `preserved_base_matches_current_blocklist` to zero. Add `blocklist_sha256` and `whole_bank_updated_on` to filtering metadata. Update the date only when policy/content changes, so a no-op cleanup leaves bytes unchanged.
- [ ] Preserve historical `*_before_length_filter`, `removed_for_length`, original preparation date, upstream source hashes, and original exclusion statistics. Do not rewrite them as though the new whole-bank filter was part of the original build.
- [ ] Run the unit tests, then `python3 scripts/word_bank.py --check` to preview failures and `python3 scripts/word_bank.py --write` to clean. Run `--check` again; require exit 0. Run `--write` a second time; require a no-change report and identical bytes.
- [ ] Add actual-data regression tests: every pool has zero blocklist overlap; none contains any proposed exclusion; the harmless fixtures remain present in 25k; actual counts/hashes/prefixes pass. Run the full utility suite again.
- [ ] Review the data diff: only intended exclusions disappear, retained word order is unchanged, and all source and license identifiers remain intact. Report resulting counts; do not assume exact nominal sizes.

## Task 3: Verify consumers and document maintenance

**Files:** create `scripts/tests/WordBankSmoke.java`; modify the documentation in the file map.

**Interfaces:**

- Python uses existing `logic.word_generator.load_word_bank()` and `generate_words(difficulty, count=30)`.
- Java smoke entry point: `public static void main(String[] args) throws Exception`, where the two arguments are the word-bank path and blocklist path. It uses existing `WordBank.load(String)`, `WordBank.getWordList(...)`, and `Json.parse(String)`; it must throw on any blocked overlap or missing level.
- C++ uses existing `loader_validation()` through `make -C cpp test`.

- [ ] Add a Python actual-loader test to `scripts/tests/test_word_bank.py`. Clear its cache before loading; verify all four pools have zero blocked overlap, then verify requests of 30 and 10,000 return that many entries from the selected clean pool. Use deterministic full-pool checks to prove exclusions; random sampling alone is insufficient.
- [ ] Add the Java smoke program with assertions for all four levels, zero blocked overlaps, and the minimum pool size. Keep it outside `java/` so the existing Makefile does not package test helpers with the game.
- [ ] Run `python3 -m unittest discover -s scripts/tests -p 'test_word_bank.py' -v` and `make -C cpp test`; require all tests to pass. The C++ suite already invokes its real loader on the shared bank and validates counts.
- [ ] Compile the Java smoke program with the real loader and parser into a fresh temporary output directory using the installed JDK compiler. Run it against absolute paths to both shared JSON files; require exit 0. Then run `make -C java` to verify the application still builds. Record the exact commands and results in the implementation handoff.
- [ ] Update `data/SOURCES.md` with measured final pool sizes, total entries, distinct-word count, byte size, whole-bank filtering scope, and a clearly dated account of the additional cleanup. Preserve original construction history as history.
- [ ] Update the wordfreq modification description and the C++ guide's actual pool sizes. Preserve the source license terms and original license notices.
- [ ] Add maintenance instructions to `README.md`: edit the explicit blocklist; run `python3 scripts/word_bank.py --check`; apply with `--write`; run `--check` and the utility tests again; inspect the diff; restart applications. Explain that removed words are not restored by deleting a blocklist entry.
- [ ] Document the scope accurately: applications consume the cleaned bank; runtime blocklist enforcement and CI integration remain future work. Checks must be run when data changes, and coverage is limited to explicit exclusions.
- [ ] Run `git diff --check`, review changed paths and the complete diff, and report resulting counts, passed/failed checks, and any untested platform behavior. Do not commit or start the separate runtime-filtering work automatically.

## Execution handoff

Planning is complete; no product or data changes have been made by this task. The next action is Task 1's failing fixture tests, followed by its read-only checker. The proposed exclusion additions and this scope are reviewable before execution.

After this increment passes, a separate small plan can add blocklist loading/filtering to Python's cached loader, including errors for a missing or invalid blocklist and empty filtered pools. Java/C++ runtime enforcement should be considered alongside that follow-up if identical runtime behavior is required.
