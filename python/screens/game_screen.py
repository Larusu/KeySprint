from textual.app import App, ComposeResult
from textual.screen import Screen
from textual.widgets import Static
from textual.containers import Horizontal, Vertical

class GameScreen(Screen):
    CSS_PATH = "../styles/app.tcss"

    def compose(self) -> ComposeResult:

        with Vertical(id="game-layout"):
            with Horizontal(id="top-controls"):
                yield Static("Difficulty", id="difficulty")
                yield Static("Timer", id="timer")

            yield Static("the quick brown fox jumps over the lazy dog", 
                        id="typing-area")

            with Horizontal(id="bottom-controls"):
                yield Static("Restart", id="restart")
                yield Static("ESC to Exit", id="exit")