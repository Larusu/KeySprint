import json
import math
from pathlib import Path

from logic.word_generator import LEVEL_KEYS, WORD_BANK_PATH


HISTORY_PATH = WORD_BANK_PATH.with_name("history.json")


def history_key(mode, difficulty, time_limit, word_count):
    try:
        difficulty_key = LEVEL_KEYS[difficulty]
    except KeyError as error:
        raise ValueError(f"Invalid difficulty level: {difficulty}") from error

    if mode == "timer":
        return f"{difficulty_key}/time/{time_limit}"
    if mode == "words":
        return f"{difficulty_key}/words/{word_count}"
    raise ValueError(f"Invalid game mode: {mode}")


class HistoryStore:
    def __init__(self, path):
        self.path = Path(path)
        self.attempts = 0
        self.records = {}

    @classmethod
    def load(cls, path=HISTORY_PATH):
        store = cls(path)
        try:
            contents = store.path.read_text(encoding="utf-8")
        except FileNotFoundError:
            return store
        except OSError as error:
            raise OSError(f"Cannot read history file: {store.path}") from error

        try:
            document = json.loads(contents)
        except json.JSONDecodeError as error:
            raise ValueError(f"Malformed history file: {store.path}") from error

        if not isinstance(document, dict):
            raise ValueError("History must be a JSON object")

        attempts = document.get("attempts", 0)
        if isinstance(attempts, bool) or not isinstance(attempts, int):
            raise ValueError("Invalid history attempts")
        store.attempts = attempts
        if attempts <= 0:
            return store

        records = document.get("records")
        if isinstance(records, dict):
            for key, values in records.items():
                if not isinstance(values, dict):
                    continue
                wpm = values.get("wpm", 0)
                accuracy = values.get("accuracy", 0.0)
                if isinstance(wpm, bool) or not isinstance(wpm, int):
                    raise ValueError(f"Invalid WPM in history record: {key}")
                if (isinstance(accuracy, bool)
                        or not isinstance(accuracy, (int, float))
                        or not math.isfinite(accuracy)):
                    raise ValueError(f"Invalid accuracy in history record: {key}")
                store.records[key] = {
                    "wpm": wpm,
                    "accuracy": float(accuracy),
                }
        return store

    def best_for(self, mode, difficulty, time_limit, word_count):
        key = history_key(mode, difficulty, time_limit, word_count)
        return self.records.get(key, {"wpm": 0, "accuracy": 0.0})

    def record_result(self, mode, difficulty, time_limit, word_count, wpm, accuracy):
        if not math.isfinite(wpm) or wpm < 0:
            raise ValueError("WPM must be a finite, non-negative number")
        if not math.isfinite(accuracy) or not 0 <= accuracy <= 100:
            raise ValueError("Accuracy must be between 0 and 100")

        key = history_key(mode, difficulty, time_limit, word_count)
        previous = self.best_for(mode, difficulty, time_limit, word_count)
        records = self.records.copy()
        records[key] = {
            "wpm": max(previous["wpm"], math.floor(wpm + 0.5)),
            "accuracy": max(previous["accuracy"], accuracy),
        }
        attempts = self.attempts + 1
        document = {"attempts": attempts, "records": records}

        try:
            self.path.write_text(
                json.dumps(document, indent=3) + "\n",
                encoding="utf-8",
            )
        except OSError as error:
            raise OSError(f"Cannot write history file: {self.path}") from error

        self.records = records
        self.attempts = attempts
