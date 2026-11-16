from textual.app import App, ComposeResult
from textual.screen import Screen
from textual.widgets import Static
from textual.containers import Horizontal, Vertical
from textual import events

from logic.calculations import calculate_accuracy, calculate_correct_chars, calculate_incorrect_chars, calculate_wpm

class ResultsScreen(Screen):
    CSS_PATH = "../styles/app.tcss"

    def compose(self) -> ComposeResult:
        with Vertical(id="results-layout"):
            yield Static("TEST RESULTS", id="results-title")

            with Vertical(id="results-content"):
                with Horizontal(id="main-stats"):
                    with Vertical(classes="stat-card"):
                        yield Static("WPM", classes="stat-label")
                        yield Static(f"{self.wpm:.1f}", id="wpm-value", classes="stat-value")

                    with Vertical(classes="stat-card"):
                        yield Static("ACCURACY", classes="stat-label")
                        yield Static(f"{self.accuracy:.1f}%", id="accuracy-value", classes="stat-value")

                with Vertical(id="character-stats"):
                    with Horizontal(classes="result-row"):
                        yield Static("Correct Characters")
                        yield Static(str(self.correct_chars), id="correct-value")

                    with Horizontal(classes="result-row"):
                        yield Static("Incorrect Characters")
                        yield Static(str(self.incorrect_chars), id="incorrect-value")

                    with Horizontal(classes="result-row"):
                        yield Static("Extra Characters")
                        yield Static(str(self.extra_char_all_count), id="extra-value")

                with Horizontal(id="results-controls"):
                    yield Static("Restart", id="restart")
                    yield Static("ESC to Exit", id="exit")

    def __init__(self, total_char_result, total_extra_chars, total_duration):
        super().__init__()
        self.chars_all_result = total_char_result
        self.extra_char_all_count = total_extra_chars
        self.duration = total_duration

        self.correct_chars = calculate_correct_chars(self.chars_all_result)
        self.incorrect_chars = calculate_incorrect_chars(self.extra_char_all_count, self.chars_all_result)
        self.accuracy = calculate_accuracy(self.extra_char_all_count, self.incorrect_chars, self.correct_chars)
        self.wpm = calculate_wpm(self.correct_chars, self.duration)

    def on_click(self, event: events.Click) -> None:
        if event.widget.id == "restart":
            game_screen = self.app.screen_stack[-2]
            game_screen.reset_game()
            self.app.pop_screen()
    
        elif event.widget.id == "exit":
            self.app.exit()

    def on_key(self, event: events.Key) -> None:
        if event.key == "ctrl+r":
            game_screen = self.app.screen_stack[-2]
            game_screen.reset_game()
            self.app.pop_screen()
    
        elif event.key == "escape":
            event.prevent_default()
            event.stop()
            self.app.exit()