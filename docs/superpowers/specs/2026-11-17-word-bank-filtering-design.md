# Shared word-bank filtering design

> Superseded on 2026-11-17: the vocabulary was reviewed and cleaned directly; the blocklist workflow in this document was abandoned.

Status: superseded; the proposed blocklist implementation was not started.

## Intent

Remove words covered by the project's exclusion policy from the shared vocabulary used by Python, Java, and C++. Make later blocklist edits easy to apply and verify.

## Current behavior

Checked on November 17, 2026:

- All applications load `data/words.json`; their loaders do not apply `data/profanity-blocklist.json`.
- Pool sizes are 933, 4,772, 9,474, and 24,165. Exact matches against the current blocklist are respectively 0, 2, 8, and 8.
- The blocklist currently has 302 unique entries, and its recorded count and fingerprint match. An earlier local addition seen during the audit is no longer present; do not assume it persists.
- `pussies` appears in the 25k pool. The blocklist contains `pussy` but not that plural. Other missing variants exist.
- Existing provenance explicitly limits profanity filtering to the original supplement. Existing base words were exempted.
- Python caches loaded lists. Java and C++ validate recorded pool sizes, so vocabulary edits must update those sizes together.

## Approach and alternatives

The recommended first increment is shared-data cleanup with a reusable check command. It benefits all three implementations and keeps their current data contract. A Python-only runtime filter would protect one implementation; filtering in all three loaders would require more application changes and would still need a maintained exclusion list. Those are possible later safeguards.

This plan implements the first increment only. Its exact review additions are proposed below, not a claim that the entire vocabulary has been manually reviewed.

## Proposed behavior

- Use explicit, lowercase, whole-word exclusions across all four pools. Keep the existing exclusion entries.
- Add these project-reviewed exclusions as the proposed initial set: `pussies`, `assholes`, `bitching`, `fucks`, `shits`, `whores`, `retard`, `retarded`, and `niggers`. Record them separately in blocklist metadata as project additions, without attributing them to upstream sources.
- Preserve retained words' order, the prefix relationship between levels, and nominal labels. Remove without refilling or inventing words.
- Provide a read-only check command and an explicit write command using Python 3.10+ and the standard library.
- Validate both documents completely before writing. Missing or malformed input must produce an error and leave data untouched.
- Update current counts, fingerprints, current construction totals, and filtering scope. Preserve original source identifiers, licenses, and historical construction statistics.
- Run checks locally with no network access or additional packages.

## Completion criteria

- Every pool has zero exact matches against the updated blocklist, including the screenshot word and the other proposed additions.
- Each pool has at least two distinct words, contains only lowercase ASCII words of at least three letters, and retains its original relative order.
- Smaller pools remain prefixes of the next pool. Counts and fingerprints match the contents.
- Whole-word filtering retains `class`, `classic`, `assessment`, `analysis`, and `assistance`.
- Read-only checks never modify files. Repeating cleanup on the same clean inputs produces no byte changes.
- The real Python, Java, and C++ loaders accept the cleaned bank.
- Documentation explains how to apply future edits and that application restarts are required to reload cached/in-memory data.

## Limits

The check proves compliance with the explicit blocklist. It does not prove every other word is appropriate for every audience. New variants require review and explicit additions. Reversing a blocklist decision does not restore already removed vocabulary; restoration would need a deliberate change using version history or original sources.

Runtime filtering, automatic stemming, substring bans, replacement vocabulary, new external datasets, CI-provider setup, and changes to game behavior are outside this increment.
