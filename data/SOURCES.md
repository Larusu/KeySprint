# Shared English word bank

All three KeySprint versions should read `data/words.json`. The data is UTF-8
standard JSON. Vocabulary is stored once in this shared folder; the four arrays
intentionally repeat their shared prefix so each level is easy to inspect.

| Level key | Nominal size | Actual unique words |
| --- | ---: | ---: |
| `english_1k` | 1,000 | 933 |
| `english_5k` | 5,000 | 4,772 |
| `english_10k` | 10,000 | 9,474 |
| `english_25k` | 25,000 | 24,165 |

The final count is 39,344 array entries across all four levels. There are 24,165
distinct vocabulary words. Every practice word has at least three letters.
The formatted `words.json` is 671,791 bytes.

## Construction

1. Start with the supplied Google no-swears list's 9,894 lowercase `a-z` words in
   their original order. Its original first 1,000 and 5,000 entries define the
   smaller pools before the length filter.
2. Build a 302-word whole-word exclusion list from lowercase `a-z` single-word
   LDNOOBW English entries and the 106 words removed between the original Google
   list and its no-swears variant. Combining the lists prevents reintroducing
   Google's removed entries through the supplement.
3. Inspect the wordfreq export's 25,000 entries in their source order. Exclude
   677 entries outside `a-z`, 96 blocklist matches, and 9,454 entries already
   present. Add the remaining 14,773 words after the Google prefix.
4. Remove every one- or two-letter word from each constructed pool. This removes
   67, 228, 420, and 502 entries respectively. Retain source order and do not
   refill the original boundaries: the 1k pool is the filtered original 1,000
   entries, rather than the first 1,000 entries remaining after filtering.
5. Record the actual counts. The final Google base contains 9,474 words and its
   supplement contains 14,691. No invented words, duplicate padding, or extra
   sources are used to reach a nominal target.

The resulting 25k pool is a cumulative combination of two sources; its entire
array is not a single globally frequency-ranked list. These are KeySprint pools,
not exact copies of Monkeytype's language files.

## Filtering scope

`profanity-blocklist.json` is reusable data with exact whole-word matching.
It documents the exclusion set used to prepare the supplemental vocabulary.
Generation checks every added word against it before writing the word bank.
The game does not need to repeat that filtering during play.

The Google no-swears base retains its source order after removing entries shorter
than three letters. The requested minimum length supersedes the earlier decision
to preserve every base entry. Eight retained base words match the current
combined blocklist; that newer profanity blocklist is not applied retroactively
to the base. The overlap is recorded in
`metadata.filtering.preserved_base_matches_current_blocklist`.

The minimum length applies to practice vocabulary. The 302-entry blocklist keeps
its original exclusion set, including any short entries, for reuse and auditing.

## Pinned sources and attribution

Full URLs, commits, raw-file SHA-256 digests, and source identifiers are embedded
in both data files. Every vocabulary array also has a digest in
`metadata.level_sha256`: hash its UTF-8 words in order, one per line, including
a newline after the last word.

| Contribution | Pinned source | Terms |
| --- | --- | --- |
| Google base and original-list exclusions | [first20hours/google-10000-english at d0736d4](https://github.com/first20hours/google-10000-english/tree/d0736d492489198e4f9d650c7ab4143bc14c1e9e) | [Preserved upstream notice](licenses/google-source-LICENSE.md) |
| Supplemental vocabulary | [aparrish/wordfreq-en-25000 at 9650fa6](https://github.com/aparrish/wordfreq-en-25000/tree/9650fa612e121beb6126b9b4d7344da287013c6e) | [CC BY-SA 4.0 attribution](licenses/wordfreq-DATA-LICENSE.md) |
| English exclusion entries | [LDNOOBW at 5faf2ba](https://github.com/LDNOOBW/List-of-Dirty-Naughty-Obscene-and-Otherwise-Bad-Words/tree/5faf2ba42d7b1c0977169ec3611df25a3c08eb13) | [CC BY 4.0 license](licenses/ldnoobw-LICENSE.txt) |

The Google source credits the Google Web Trillion Word Corpus, Thorsten Brants,
Alex Franz, the Linguistic Data Consortium, Peter Norvig, and cleanup by Josh
Kaufman. Its notice describes educational and personal/research use and its
upstream corpus terms. The wordfreq export credits Allison Parrish and wordfreq;
its supplemental data uses CC BY-SA 4.0. Each contribution keeps its own source
terms; this collection does not give the Google corpus a new blanket license.

## Data contract

`words.schema.json` describes the word-bank structure. Schema validation checks
the types, required keys, per-pool uniqueness, lowercase `a-z` pattern, and minimum
word length of three. Separately
check that `metadata.actual_counts[level]` equals the array length, that each
smaller pool is the next pool's prefix, and that the recorded hashes match.

Stable access in every language is `levels` followed by the same level key.
Level labels remain nominal; use the array length or `metadata.actual_counts`
when displaying how many usable words exist. Runtime difficulty and session
word-count settings belong in each implementation's configuration, outside this
shared vocabulary file.
