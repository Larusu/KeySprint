import random
import json
from functools import lru_cache #Caches the loaded word bank, so no need to keep parsing the json file each time generate_words is called
from pathlib import Path

# parents[2] goes up to the KeySprint project folder:
# word_generator.py -> logic -> python -> KeySprint.
# The / operator then adds data/words.json to that path.
WORD_BANK_PATH = Path(__file__).resolve().parents[2] / "data" / "words.json"

LEVEL_KEYS = {
    "1k": "english_1k",
    "5k": "english_5k",
    "10k": "english_10k",
    "25k": "english_25k",
}

# Loads the word bank from the JSON file and caches it
@lru_cache(maxsize=1)
def load_word_bank():
    try:
        # Open the file as UTF-8 text.
        # "with" automatically closes the file after we're done.
        with WORD_BANK_PATH.open(encoding="utf-8") as file:
            # Convert the JSON content into Python dictionaries and lists.
            document = json.load(file)
    # Handle file errors, such as a missing file or insufficient permission.
    except OSError as error:
        # Include the expected path so it's easier to find the problem.
        # "from error" keeps the original exception available for debugging.
        raise RuntimeError(f"Cannot open word bank: {WORD_BANK_PATH}") from error

    # Handle a file whose contents aren't valid JSON.
    except json.JSONDecodeError as error:
        # Explain which file has the JSON formatting problem.
        raise RuntimeError(f"Malformed word bank: {WORD_BANK_PATH}") from error

    # Check that the JSON document itself is a dictionary.
    # This avoids calling .get() on a list or another unexpected value.
    if not isinstance(document, dict):
        raise ValueError("Word bank must be a JSON object")

    # Check that this is the expected version and English-language data.
    # .get() returns None if either key is missing.
    if document.get("schema_version") != 1 or document.get("language") != "en":
        raise ValueError("Expected a version 1 English word bank")

    # Get the dictionary containing the four word pools.
    levels = document.get("levels")

    # Stop with a clear error if "levels" is missing or isn't a dictionary.
    if not isinstance(levels, dict):
        raise ValueError("Word bank is missing its levels")

    # Check each JSON key listed in LEVEL_KEYS.
    for key in LEVEL_KEYS.values():
        # Get that difficulty's list of words.
        words = levels.get(key)

        # Check that the pool is a list and contains at least one word.
        if not isinstance(words, list) or not words:
            raise ValueError(f"Missing or empty word pool: {key}")

    # Give the validated pools back to the caller.
    return levels

def generate_words(difficulty, count=30):
    try:
        level_key = LEVEL_KEYS[difficulty]
    
    # If an invalid difficulty is provided, raise a ValueError with a clear message.
    except KeyError as error:
        raise ValueError(f"Invalid difficulty level: {difficulty}") from error
    
    # Load the pools, select the requested one, and choose count words.
    # random.choices selects with replacement, so words can repeat.
    return random.choices(load_word_bank()[level_key], k=count)