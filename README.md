# KeySprint

## Running Python Version

in-progress

## Running Java Version

Requires JDK 17 or newer. The JLine jar is bundled in `java/`.

### Build and run

Requires a JDK on your `PATH` (check with `javac -version`).

**Windows (PowerShell or cmd):**

```powershell
cd java
javac -cp ".;jline-3.26.3.jar" *.java
java -cp ".;jline-3.26.3.jar" KeySprint
```



**macOS / Linux:**

```bash
cd java
javac -cp ".:jline-3.26.3.jar" *.java
java -cp ".:jline-3.26.3.jar" KeySprint
```

The only difference is the classpath separator: `;` on Windows, `:` on
macOS/Linux. `--enable-native-access=ALL-UNNAMED` is required by JLine on
recent JDKs.

The Java version reads the shared word bank from `../data/words.json` when run
from `java/`, or `data/words.json` when run from the project root. Pass an
explicit path as the first argument if needed.

### Clean build files

From the project root:

```powershell
# Windows
Remove-Item java\*.class
```

```bash
# macOS / Linux
rm java/*.class
```

### Controls

- `←` / `→` move between the mode, difficulty, preset, and start cells.
- `Enter` selects a value or starts the game.
- Modes: Time (`15/30/60/120` seconds) and Words (`10/25/50/75`).
- In Words mode, the `custom` preset opens a 1–10,000 word-count editor.
- The timer starts on the first letter or digit you type.
- During a game, `Esc` returns to the menu; `Ctrl+C` exits.
- On the results screen, `Esc` returns to the menu and `Ctrl+C` exits; other
  keys do nothing.

## Running C++ version

From the project root, build without starting the game:

```bash
make -C cpp
```

Build and run:

```bash
make -C cpp run
```

Or run with an explicit word-bank path:

```bash
./cpp/keysprint data/words.json
```

On Windows, use `cpp/keysprint.exe` after building with a C++17 compiler and GNU Make.

Ctrl+F opens configuration. Tab and arrow keys select the mode, vocabulary,
and time or word-count preset. In Words mode, press `w` to enter a custom count.

See the [C++ usage guide](docs/cpp-word-bank-guide.md) for controls and checks.
Shared vocabulary attribution is in [data/SOURCES.md](data/SOURCES.md).

Clean build files:

```bash
make -C cpp clean
```
