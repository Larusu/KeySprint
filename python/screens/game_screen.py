from textual.app import App, ComposeResult
from textual.screen import Screen
from textual.widgets import Static, Input
from textual.containers import Horizontal, Vertical
from textual import events

from logic.word_generator import generate_words
from logic.history import HistoryStore, HISTORY_PATH
from logic.typing_engine import eval_text
from logic.timer import Timer
from logic.calculations import calculate_extra_chars

from screens.results_screen import ResultsScreen

from rich.text import Text

TIMER_PRESETS = (15, 30, 60, 120)
WORD_PRESETS = (10, 25, 50, 75)


class GameScreen(Screen):
    CSS_PATH = "../styles/app.tcss"
    AUTO_FOCUS = ""
    
    def compose(self) -> ComposeResult:
        
        # CONFIGURATION PANEL
        with Horizontal(id="config-panel-wrapper"):
            with Vertical(id="config-panel"):
                yield Static("GAME CONFIGURATION", id="config-title")
        
                # Difficulty
                yield Static("DIFFICULTY", classes="config-label")
        
                with Horizontal(classes="config-options"):
                    yield Static("1k", id="config-difficulty1", classes="config-option")
                    yield Static("5k", id="config-difficulty2", classes="config-option")
                    yield Static("10k", id="config-difficulty3", classes="config-option")
                    yield Static("25k", id="config-difficulty4", classes="config-option")
        
                # Game limit type
                yield Static("GAME LIMIT", classes="config-label")
        
                with Horizontal(classes="config-options"):
                    yield Static("TIMER", id="limit-timer", classes="config-option selected-option")
                    yield Static("WORDS", id="limit-words", classes="config-option")
        
                # Timer options
                yield Static("TIME LIMIT", classes="config-label")
        
                with Horizontal(id="config-timer-options", classes="config-options"):
                    yield Static("15", id="config-timer1", classes="config-option")
                    yield Static("30", id="config-timer2", classes="config-option selected-option")
                    yield Static("60", id="config-timer3", classes="config-option")
                    yield Static("120", id="config-timer4", classes="config-option")
        
                # Word count options
                yield Static("WORD COUNT", classes="config-label")
        
                with Horizontal(id="word-options", classes="config-options"):
                    yield Static("10", id="word-option1", classes="config-option")
                    yield Static("25", id="word-option2", classes="config-option")
                    yield Static("50", id="word-option3", classes="config-option")
                    yield Static("75", id="word-option4", classes="config-option")
                    yield Static("CUSTOM", id="word-option-custom", classes="config-option")
        
                yield Input(
                    placeholder="Enter 1–10,000 words",
                    id="custom-word-count",
                    type="integer"
                )
        
                yield Static("SAVE SETTINGS", id="save-config")

        with Vertical(id="game-layout"):

            # TOP CONTROLS
            with Horizontal(id="top-controls-wrapper"):
                yield Static("30", id="selected-time")

                with Horizontal(id="setting-box"):
                    yield Static("Timer", id="timer-mode")
                    yield Static("Words", id="words-mode")

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
                    yield Static("Custom", id="timer-option-custom")

            # TYPING AREA
            with Vertical(id="typing-wrapper"):
                with Horizontal(id="typing-container"):
                    yield Static(self.target_text, id="typing-area")

            # HISTORY PANEL
            with Horizontal(id="history-panel"):
                yield Static(
                    Text("Best WPM:", style="#753FEB"),
                    id="best-wpm"
                )
                yield Static(
                    Text("Accuracy:", style="#753FEB"),
                    id="accuracy"
                )
                yield Static(
                    Text("Attempts:", style="#753FEB"),
                    id="attempts"
                )

            # BOTTOM CONTROLS
            with Horizontal(id="bottom-controls"):
                yield Static(
                    Text("ctrl + r", style="#753FEB") + Text(" - Restart", style="#FFFFFF"),
                    id="restart"
                )
                yield Static(
                    Text("esc", style="#753FEB") + Text(" - Exit", style="#FFFFFF"),
                    id="exit"
                )
                yield Static(
                    Text("ctrl + f", style="#753FEB") + Text(" - Custom", style="#FFFFFF"),
                    id="custom"
                )

    def __init__(self):
        super().__init__()
        self.typed_text = ""
        self.selected_difficulty = "1k"
        self.selected_time = 30
        self.timer = Timer(self.selected_time)

        self.selected_mode = "timer"
        self.selected_word_count = 25
        self.history = HistoryStore.load(HISTORY_PATH)

        # Temporary settings being edited in the configuration panel
        self.config_difficulty = self.selected_difficulty
        self.config_mode = self.selected_mode
        self.config_time = self.selected_time
        self.config_word_count = 25
        self.config_custom_words = False

        self.char_results = []
        self.char_all_results = []

        self.extra_char_count = 0
        self.extra_char_all_count = 0

        self.completed_words = 0
        self.batch_start_count = 0

        self.remaining_words = []
        self.missed_characters = {}

        generated_words = generate_words(self.selected_difficulty)
        self.target_text = " ".join(generated_words)

        self.config_open = False

    def on_mount(self):
        # Highlight the default difficulty
        difficulty_options = {
            "1k": "difficulty-option1",
            "5k": "difficulty-option2",
            "10k": "difficulty-option3",
            "25k": "difficulty-option4"
        }

        self.update_selected_option(
            list(difficulty_options.values()),
            difficulty_options[self.selected_difficulty]
        )

        # Highlight the default mode
        self.update_selected_option(
            ["timer-mode", "words-mode"],
            "timer-mode"
        )
        self.update_top_limit_options()
        self.update_history_panel()

    def delete_last_char(self):
        if self.typed_text.endswith(" "):
            self.completed_words -= 1
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

        if character == " " and self.typed_text.endswith(" "):
            return

        if character == " " and self.typed_text == "":
            return

        if character == " ":
            word_index = self.completed_words - self.batch_start_count

            typed_word = self.typed_text.split(" ")[-1]
            target_word = self.target_text.split(" ")[word_index]

            remaining_word = target_word[len(typed_word):]

            if remaining_word:
                self.missed_characters[word_index] = remaining_word

            self.completed_words += 1

        self.typed_text += character
            
        update_text = self.query_one("#typing-area", Static)
        update_text.update(self.typed_text)

        if self.selected_mode == "words":
            update_text = self.query_one("#selected-time", Static)
            display_text = f"{self.completed_words}/{self.selected_word_count} words"
            update_text.update(display_text)

        self.check_results()
        self.format_target_text()

        if self.selected_mode == "words" and not self.remaining_words:
            completed_in_batch = self.completed_words - self.batch_start_count
            batch_word_count = len(self.target_text.split())

            typed_last_word = self.typed_text.split(" ")[-1]
            target_last_word = self.target_text.split()[-1]

            last_word_finished = len(typed_last_word) >= len(target_last_word)

            if (
                completed_in_batch >= batch_word_count
                or (
                    completed_in_batch == batch_word_count - 1
                    and last_word_finished
                )
            ):
                if completed_in_batch == batch_word_count - 1:
                    self.completed_words += 1

                self.char_all_results.extend(self.char_results)
                self.extra_char_all_count += self.extra_char_count

                self.countdown_interval.stop()

                self.show_results(self.timer.elapsed_time)
                return

        self.next_set_words()

    def toggle_config(self):
        self.config_open = not self.config_open

        game_layout = self.query_one("#game-layout")
        config_wrapper = self.query_one("#config-panel-wrapper")

        if self.config_open:
            game_layout.styles.display = "none"
            config_wrapper.styles.display = "block"
        else:
            config_wrapper.styles.display = "none"
            game_layout.styles.display = "block"
            self.set_focus(None)

    ## function that listens for keyboard events
    def on_key(self, event: events.Key) -> None:
        if event.key == "ctrl+f":
            event.prevent_default()
            event.stop()
            self.reset_game()
            self.toggle_config()
            return
        
        elif event.key == "ctrl+r":
            event.prevent_default()
            event.stop()
            self.reset_game()

        elif event.key == "escape":
            event.prevent_default()
            event.stop()
            self.app.exit()

        elif event.key in {"tab", "shift+tab", "enter", "ctrl+enter", "shift+enter"}:
            event.prevent_default()
            event.stop()

        elif self.timer.is_finished == True:
            event.prevent_default()
            event.stop()

        elif event.key == "backspace":
            self.delete_last_char()

        elif event.is_printable and event.character is not None:
            ## checks if a typing key was pressed
            self.insert_text(event.character)

    def check_results(self):
        self.char_results = eval_text(self.typed_text, self.target_text)
        self.extra_char_count = calculate_extra_chars(self.typed_text, self.target_text)

    def format_target_text(self):
        formatted_text = Text()
        result_index = 0

        target_words = self.target_text.split(" ")
        typed_words = self.typed_text.split(" ")

        for index, target_word in enumerate(target_words):

            if index < len(typed_words):
                typed_word = typed_words[index]

                for char_index, character in enumerate(typed_word):
                    if result_index < len(self.char_results):
                        if self.char_results[result_index]:
                            formatted_text.append(character, style="white")
                        else:
                            formatted_text.append(character, style="red")

                        result_index += 1

                # Preserve the untyped portion of the word
                remaining_word = target_word[len(typed_word):]

                if index in self.missed_characters:
                    formatted_text.append(remaining_word, style="red")
                else:
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
        
        if self.selected_mode == "timer":
            completed_in_batch = self.completed_words - self.batch_start_count
            batch_word_count = len(self.target_text.split())

            typed_last_word = self.typed_text.split(" ")[-1]
            target_last_word = self.target_text.split()[-1]

            last_word_finished = len(typed_last_word) >= len(target_last_word)

            if (
                completed_in_batch >= batch_word_count
                or (
                    completed_in_batch == batch_word_count - 1
                    and last_word_finished
                )
            ):
                if completed_in_batch == batch_word_count - 1:
                    self.completed_words += 1

                self.batch_start_count = self.completed_words

                self.char_all_results.extend(self.char_results)
                self.extra_char_all_count += self.extra_char_count

                ## resets for the current round
                generated_words = generate_words(self.selected_difficulty)
                self.target_text = " ".join(generated_words)
                self.typed_text = ""
                self.char_results = []
                self.format_target_text()
                self.extra_char_count = 0
                self.missed_characters = {}

        elif self.selected_mode == "words":
            completed_in_batch = self.completed_words - self.batch_start_count

            if completed_in_batch >= len(self.target_text.split()) and self.remaining_words:
                self.char_all_results.extend(self.char_results)
                self.extra_char_all_count += self.extra_char_count

                next_words = self.remaining_words[:30]
                self.remaining_words = self.remaining_words[30:]
                self.target_text = " ".join(next_words)
                self.batch_start_count = self.completed_words

                self.typed_text = ""
                
                self.char_results = []
                self.extra_char_count = 0
                self.format_target_text()  

    def reset_game(self):
        if hasattr(self, "countdown_interval"):
            self.countdown_interval.stop()
            del self.countdown_interval

        if self.selected_mode == "words":
            generated_words = generate_words(self.selected_difficulty, count=self.selected_word_count)
            self.target_text = " ".join(generated_words[:30])
            self.remaining_words = generated_words[30:]
        else:
            generated_words = generate_words(self.selected_difficulty)
            self.target_text = " ".join(generated_words)
            self.remaining_words = []

        self.typed_text = ""
        self.completed_words = 0
        self.batch_start_count = 0
        self.char_results = []
        self.char_all_results = []
        self.extra_char_count = 0
        self.extra_char_all_count = 0
        self.missed_characters = {}

        self.timer.reset_timer(self.selected_time)

        if self.selected_mode == "timer":
            display_text = f"{self.selected_time}"
        else:
            display_text = f"0/{self.selected_word_count} words"

        self.query_one("#selected-time", Static).update(display_text)
        self.format_target_text()
        self.update_history_panel()

    def update_history_panel(self):
        best = self.history.best_for(
            self.selected_mode,
            self.selected_difficulty,
            self.selected_time,
            self.selected_word_count,
        )
        self.query_one("#best-wpm", Static).update(
            Text("Best WPM:", style="#753FEB")
            + Text(f" {best['wpm']}", style="#FFFFFF")
        )
        self.query_one("#accuracy", Static).update(
            Text("Accuracy:", style="#753FEB")
            + Text(f" {int(best['accuracy'] + 0.5)}%", style="#FFFFFF")
        )
        self.query_one("#attempts", Static).update(
            Text("Attempts:", style="#753FEB")
            + Text(f" {self.history.attempts}", style="#FFFFFF")
        )

    def update_top_limit_options(self):
        if self.selected_mode == "timer":
            presets = TIMER_PRESETS
            selected_value = self.selected_time
            self.query_one("#timer-option-custom", Static).styles.display = "none"
        else:
            presets = WORD_PRESETS
            selected_value = self.selected_word_count
            self.query_one("#timer-option-custom", Static).styles.display = "block"

        option_ids = []
        selected_id = None
        for index, value in enumerate(presets, start=1):
            option_id = f"timer-option{index}"
            self.query_one(f"#{option_id}", Static).update(str(value))
            option_ids.append(option_id)
            if value == selected_value:
                selected_id = option_id

        if self.selected_mode == "words" and self.config_custom_words:
            selected_id = "timer-option-custom"
        self.update_selected_option(
            option_ids + ["timer-option-custom"],
            selected_id,
        )

    def show_results(self, duration):
        results = ResultsScreen(
            self.char_all_results,
            self.extra_char_all_count,
            duration,
        )
        try:
            self.history.record_result(
                self.selected_mode,
                self.selected_difficulty,
                self.selected_time,
                self.selected_word_count,
                results.wpm,
                results.accuracy,
            )
        except OSError as error:
            self.notify(f"Unable to save history: {error}", severity="error")
        self.update_history_panel()
        self.app.push_screen(results)

    def update_selected_option(self, option_ids, selected_id):
        for option_id in option_ids:
            widget = self.query_one("#" + option_id, Static)

            if option_id == selected_id:
                widget.add_class("selected-option")
            else:
                widget.remove_class("selected-option")


    def save_config(self):
        if self.config_mode == "words":
            if self.config_custom_words:
                custom_input = self.query_one("#custom-word-count", Input)

                try:
                    word_count = int(custom_input.value)
                except ValueError:
                    self.notify("Please enter a valid word count.")
                    return

                if not 1 <= word_count <= 10000:
                    self.notify("Word count must be between 1 and 10,000.")
                    return

                self.selected_word_count = word_count
            else:
                self.selected_word_count = self.config_word_count

        self.selected_difficulty = self.config_difficulty
        self.selected_mode = self.config_mode
        self.selected_time = self.config_time

        top_difficulty_options = {
            "1k": "difficulty-option1",
            "5k": "difficulty-option2",
            "10k": "difficulty-option3",
            "25k": "difficulty-option4",
        }
        self.update_selected_option(
            list(top_difficulty_options.values()),
            top_difficulty_options[self.selected_difficulty],
        )
        self.update_top_limit_options()
        self.reset_game()
        self.toggle_config()
        
    def on_click(self, event: events.Click) -> None:
        clicked_id = event.widget.id

        # Top controls: Custom word count
        if clicked_id == "timer-option-custom":

            self.selected_mode = "words"
            self.config_mode = "words"
            self.config_custom_words = True
            self.update_top_limit_options()

            # Highlight Words
            self.update_selected_option(
                ["timer-mode", "words-mode"],
                "words-mode"
            )

            # Highlight Custom
            self.update_selected_option(
                [
                    "timer-option1",
                    "timer-option2",
                    "timer-option3",
                    "timer-option4",
                    "timer-option-custom"
                ],
                "timer-option-custom"
            )

            # Open configuration panel
            if not self.config_open:
                self.toggle_config()

            # Switch configuration to Words
            self.update_selected_option(
                ["limit-timer", "limit-words"],
                "limit-words"
            )

            self.query_one("#config-timer-options").styles.display = "none"
            self.query_one("#word-options").styles.display = "block"

            # Select Custom inside the configuration panel too
            self.update_selected_option(
                [
                    "word-option1",
                    "word-option2",
                    "word-option3",
                    "word-option4",
                    "word-option-custom"
                ],
                "word-option-custom"
            )

            custom_input = self.query_one("#custom-word-count", Input)
            custom_input.styles.display = "block"
            custom_input.focus()

            return

        # Open configuration
        if clicked_id == "custom":
            self.toggle_config()
            return

        # Save configuration
        if clicked_id == "save-config":
            self.save_config()

            # Update the top Timer/Words selection
            selected_mode_id = (
                "timer-mode" if self.selected_mode == "timer"
                else "words-mode"
            )

            self.update_selected_option(
                ["timer-mode", "words-mode"],
                selected_mode_id
            )
            return

        # Configuration: difficulty
        difficulty_options = {
            "config-difficulty1": "1k",
            "config-difficulty2": "5k",
            "config-difficulty3": "10k",
            "config-difficulty4": "25k"
        }

        if clicked_id in difficulty_options:
            self.config_difficulty = difficulty_options[clicked_id]

            self.update_selected_option(
                list(difficulty_options.keys()), clicked_id
            )
            return

        # Configuration: game limit type
        if clicked_id in ("limit-timer", "limit-words"):
            self.config_mode = (
                "timer" if clicked_id == "limit-timer" else "words"
            )

            self.update_selected_option(
                ["limit-timer", "limit-words"], clicked_id
            )

            timer_options = self.query_one("#config-timer-options")
            word_options = self.query_one("#word-options")
            custom_input = self.query_one("#custom-word-count", Input)

            if self.config_mode == "timer":
                timer_options.styles.display = "block"
                word_options.styles.display = "none"
                custom_input.styles.display = "none"
                selected_id = next(
                    (
                        f"config-timer{index}"
                        for index, value in enumerate(TIMER_PRESETS, start=1)
                        if value == self.config_time
                    ),
                    None,
                )
                self.update_selected_option(
                    [f"config-timer{index}" for index in range(1, 5)],
                    selected_id,
                )
            else:
                timer_options.styles.display = "none"
                word_options.styles.display = "block"
                custom_input.styles.display = (
                    "block" if self.config_custom_words else "none"
                )
                selected_id = next(
                    (
                        f"word-option{index}"
                        for index, value in enumerate(WORD_PRESETS, start=1)
                        if value == self.config_word_count
                    ),
                    None,
                )
                if self.config_custom_words:
                    selected_id = "word-option-custom"
                self.update_selected_option(
                    [f"word-option{index}" for index in range(1, 5)]
                    + ["word-option-custom"],
                    selected_id,
                )

            return

        # Top controls: Timer / Words selection
        if clicked_id in ("timer-mode", "words-mode"):

            self.selected_mode = (
                "timer" if clicked_id == "timer-mode" else "words"
            )

            # Update selected mode color
            self.update_selected_option(
                ["timer-mode", "words-mode"],
                clicked_id
            )

            self.update_top_limit_options()
            self.reset_game()
            return

        # Configuration: timer duration
        config_timer_options = {
            "config-timer1": 15,
            "config-timer2": 30,
            "config-timer3": 60,
            "config-timer4": 120
        }

        if clicked_id in config_timer_options:
            self.config_time = config_timer_options[clicked_id]

            self.update_selected_option(
                list(config_timer_options.keys()), clicked_id
            )
            return

        # Configuration: preset word count
        config_word_options = {
            "word-option1": 10,
            "word-option2": 25,
            "word-option3": 50,
            "word-option4": 75
        }

        if clicked_id in config_word_options:
            self.config_word_count = config_word_options[clicked_id]
            self.config_custom_words = False

            self.update_selected_option(
                list(config_word_options.keys()) + ["word-option-custom"],
                clicked_id
            )

            self.query_one("#custom-word-count", Input).styles.display = "none"
            return

        # Configuration: custom word count
        if clicked_id == "word-option-custom":
            self.config_custom_words = True

            self.update_selected_option(
                list(config_word_options.keys()) + ["word-option-custom"],
                clicked_id
            )

            custom_input = self.query_one("#custom-word-count", Input)
            custom_input.styles.display = "block"
            custom_input.focus()
            return

        # Restart
        if clicked_id == "restart":
            self.reset_game()
            return

        # Exit
        if clicked_id == "exit":
            self.app.exit()
            return

        # Top difficulty controls
        top_difficulty_options = {
            "difficulty-option1": "1k",
            "difficulty-option2": "5k",
            "difficulty-option3": "10k",
            "difficulty-option4": "25k"
        }

        if clicked_id in top_difficulty_options:
            self.selected_difficulty = top_difficulty_options[clicked_id]

            self.update_selected_option(
                list(top_difficulty_options.keys()), clicked_id
            )

            self.reset_game()
            return

        # Top timer / word count options
        presets = TIMER_PRESETS if self.selected_mode == "timer" else WORD_PRESETS
        top_timer_options = {
            f"timer-option{index}": value
            for index, value in enumerate(presets, start=1)
        }

        if clicked_id in top_timer_options:
            selected_value = top_timer_options[clicked_id]

            if self.selected_mode == "timer":
                self.selected_time = selected_value
            else:
                self.selected_word_count = selected_value
                self.config_custom_words = False

            self.update_top_limit_options()
            self.reset_game()
            return

    def update_timer(self):
        if self.selected_mode == "words":
            self.timer.elapsed_time += 1

        else:
            self.timer.tick()

            timer = self.query_one("#selected-time", Static)
            timer.update(str(self.timer.remaining_time))

            if self.timer.is_finished == True:
                self.countdown_interval.stop()
                self.char_all_results.extend(self.char_results)
                self.extra_char_all_count += self.extra_char_count

                self.char_results = []
                self.extra_char_count = 0

                self.show_results(self.timer.duration)