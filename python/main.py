from textual.app import App, ComposeResult
from textual.widgets import Header
from screens.game_screen import GameScreen

class KeySprintApp(App):
    ## Set ups the application and its widgets
    def on_mount(self) -> None:
        self.push_screen(GameScreen())


## Run the application
if __name__ == "__main__": ## check if the script is being run directly
    app = KeySprintApp()
    app.run()