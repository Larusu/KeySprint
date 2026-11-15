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
            yield Static("", id="results") ## for debugging

            with Horizontal(id="bottom-controls"):
                yield Static("Restart", id="restart")
                yield Static("ESC to Exit", id="exit")

    def __init__(self):
        super().__init__()
        self.typed_text = ""

        generated_words = generate_words("5k")
        self.target_text = " ".join(generated_words)

        self.char_results = []

    def delete_last_char(self):
        self.typed_text = self.typed_text[:-1]
        update_text = self.query_one("#typing-area", Static)
        update_text.update(self.typed_text)
        self.check_results()
        self.format_target_text()

    ## function to insert text in typed_text and updates the typing area
    def insert_text(self, character):
        self.typed_text += character
        update_text = self.query_one("#typing-area", Static)
        update_text.update(self.typed_text)
        self.check_results()
        self.format_target_text()

    ## function that listens for keyboard events
    def on_key(self, event: events.Key) -> None:
        if event.key == "backspace":
            self.delete_last_char()
        elif event.character is not None: ## checks if a key is pressed
            self.insert_text(event.character)

    def check_results(self):
        self.char_results = eval_text(self.typed_text, self.target_text)
        update_text = self.query_one("#results", Static)
        update_text.update(str(self.char_results))

    def format_target_text(self):
        formatted_text = ""

        for index, character in enumerate(self.target_text):
            if  index < len(self.char_results):
                if self.char_results[index]:
                    formatted_text += f"[white]{character}[/white]"
                else:
                    formatted_text += f"[red]{character}[/red]"
            else:
                formatted_text += character

        update_text = self.query_one("#typing-area", Static)
        update_text.update(formatted_text)
