from textual.app import App, ComposeResult
from textual.screen import Screen
from textual.widgets import Static
from textual.containers import Horizontal, Vertical
from textual import events

from logic.word_generator import generate_words
from logic.typing_engine import eval_text

class GameScreen(Screen):
    CSS_PATH = "../styles/app.tcss"

    def compose(self) -> ComposeResult:

        with Vertical(id="game-layout"):
            with Horizontal(id="top-controls"):
                yield Static("Difficulty", id="difficulty")
                yield Static("Timer", id="timer")

            yield Static(self.target_text, id="typing-area")

            with Horizontal(id="bottom-controls"):
                yield Static("Restart", id="restart")
                yield Static("ESC to Exit", id="exit")

    def __init__(self):
        super().__init__()
        self.typed_text = ""

        generated_words = generate_words("5k")
        self.target_text = " ".join(generated_words)

    def delete_last_char(self):
        self.typed_text = self.typed_text[:-1]
        update_text = self.query_one("#typing-area", Static)
        update_text.update(self.typed_text)

    ## function to insert text in typed_text and updates the typing area
    def insert_text(self, character):
        self.typed_text += character
        update_text = self.query_one("#typing-area", Static)
        update_text.update(self.typed_text)

    ## function that listens for keyboard events
    def on_key(self, event: events.Key) -> None:
        if event.key == "backspace":
            self.delete_last_char()
        elif event.character is not None: ## checls if a key is pressed
            self.insert_text(event.character)