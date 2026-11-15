from textual.app import App, ComposeResult
from textual.screen import Screen
from textual.widgets import Static
from textual.containers import Horizontal, Vertical
from textual import events

from logic.word_generator import generate_words
from logic.typing_engine import eval_text
from logic.timer import Timer

class GameScreen(Screen):
    CSS_PATH = "../styles/app.tcss"

    def compose(self) -> ComposeResult:

        with Vertical(id="game-layout"):
            with Horizontal(id="top-controls-wrapper"):
                yield Static("30", id="selected-time")

                with Horizontal(id="setting-box"):
                    yield Static("Difficulty", id="difficulty")
                    yield Static("Timer", id="timer")

                with Horizontal(id="difficulty-options"):   
                    yield Static("1k", id="difficulty-option1")
                    yield Static("5k", id="difficulty-option2")
                    yield Static("10k", id="difficulty-option3")
                    yield Static("25k", id="difficulty-option4")

                with Horizontal(id="timer-options"):  
                    yield Static("15", id="timer-option1")
                    yield Static("30", id="timer-option2")
                    yield Static("60", id="timer-option3")
                    yield Static("120", id="timer-option4")

            with Vertical(id="typing-wrapper"):
                with Horizontal(id="typing-container"):
                    yield Static(self.target_text, id="typing-area")

            with Horizontal(id="bottom-controls"):
                yield Static("Restart", id="restart")
                yield Static("ESC to Exit", id="exit")

    def __init__(self):
        super().__init__()
        self.typed_text = ""
        self.selected_difficulty = "1k"
        self.selected_time = 30
        self.timer = Timer(self.selected_time)

        self.char_results = []
        self.char_all_results = []

        self.extra_char_count = 0

        generated_words = generate_words(self.selected_difficulty)
        self.target_text = " ".join(generated_words)

    def delete_last_char(self):
        self.typed_text = self.typed_text[:-1]
        update_text = self.query_one("#typing-area", Static)
        update_text.update(self.typed_text)
        self.check_results()
        self.format_target_text()
        self.next_set_words()

    ## function to insert text in typed_text and updates the typing area
    def insert_text(self, character):
        if not self.timer.is_running and not self.timer.is_finished:
            self.timer.start_timer()
            self.countdown_interval = self.set_interval(1, self.update_timer)

        self.typed_text += character
        update_text = self.query_one("#typing-area", Static)
        update_text.update(self.typed_text)

        self.check_results()
        self.format_target_text()
        self.next_set_words()

    ## function that listens for keyboard events
    def on_key(self, event: events.Key) -> None:
        if self.timer.is_finished == True:
            event.prevent_default()
            event.stop()
        else:
            if event.key == "backspace":
                self.delete_last_char()
            elif event.character is not None: ## checks if a key is pressed
                self.insert_text(event.character)

    def check_results(self):
        self.char_results = eval_text(self.typed_text, self.target_text)

        current_extra_char = max(0, len(self.typed_text) - len(self.target_text))
        self.extra_char_count = current_extra_char

    def format_target_text(self):
        formatted_text = ""
        result_index = 0

        target_words = self.target_text.split(" ")
        typed_words = self.typed_text.split(" ")

        for index, target_word in enumerate(target_words):

            if index < len(typed_words):
                typed_word = typed_words[index]

                for char_index, character in enumerate(typed_word):
                    if result_index < len(self.char_results):
                        if self.char_results[result_index]:
                            formatted_text += f"[white]{character}[/white]"
                        else:
                            formatted_text += f"[red]{character}[/red]"

                        result_index += 1

                # Preserve the untyped portion of the word
                remaining_word = target_word[len(typed_word):]
                formatted_text += remaining_word

            else:
                # Display words that haven't been reached yet
                formatted_text += target_word

            # Restore spaces between words
            if index < len(target_words) - 1:
                formatted_text += " "
                if index < len(typed_words) - 1:
                    result_index += 1

        update_text = self.query_one("#typing-area", Static)
        update_text.update(formatted_text)

    ## function to refresh and generate new set of 30 words in the same round
    def next_set_words(self):
        if len(self.target_text) == len(self.typed_text):
            self.char_all_results.extend(self.char_results)

            ## resets for the current round
            generated_words = generate_words(self.selected_difficulty)
            self.target_text = " ".join(generated_words)
            self.typed_text = ""
            self.char_results = []
            self.format_target_text()

    def on_click(self, event: events.Click) -> None:
        selected_difficulty_option = event.widget.id

        difficulty_options = {
            "difficulty-option1": "1k",
            "difficulty-option2": "5k",
            "difficulty-option3": "10k",
            "difficulty-option4": "25k"
        }

        difficulty = difficulty_options.get(selected_difficulty_option)

        if difficulty:
            self.selected_difficulty = difficulty
            generated_words = generate_words(difficulty)
            self.target_text = " ".join(generated_words)
            self.typed_text = ""
            self.char_results = []
            self.format_target_text()

        for options_id in difficulty_options:
            widget = self.query_one("#" + options_id, Static)

            if options_id == selected_difficulty_option:
                widget.styles.color = "#753FEB"
            else:
                widget.styles.color = "#FFFFFF"


        ## timer
        selected_timer_option = event.widget.id

        timer_options = {
            "timer-option1": 15,
            "timer-option2": 30,
            "timer-option3": 60,
            "timer-option4": 120
        }

        timer = timer_options.get(selected_timer_option)

        if timer:
            self.selected_time = timer

            update_text = self.query_one("#selected-time", Static)
            update_text.update(str(timer))

            self.timer.reset_timer(timer)

        for options_id in timer_options:
            widget = self.query_one("#" + options_id, Static)

            if options_id == selected_timer_option:
                widget.styles.color = "#753FEB"
            else:
                widget.styles.color = "#FFFFFF"

    def update_timer(self):
        self.timer.tick()

        timer = self.query_one("#selected-time", Static)
        timer.update(str(self.timer.remaining_time))

        if self.timer.is_finished == True:
            self.countdown_interval.stop()
            self.char_all_results.extend(self.char_results)
            