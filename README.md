# KeySprint

## Running Python Version

in-progress

## Running Java Version

Requires JDK 17 or newer. The JLine jar is bundled in `java/`.

### Build and run

With JDK 17 or newer and GNU Make on your `PATH`, run from the project root:

```bash
make -C java run
```

From inside `java/`, use `make run`. This compiles all Java source files and
adds the bundled JLine library when compiling and running. The Makefile uses
the compiler module from the JDK selected by `java`, so a separate `javac`
executable on `PATH` is not needed.

Build without starting the game, or use an explicit word-bank path:

```bash
make -C java
make -C java run ARGS="../data/words.json"
```

`java KeySprint.java` does not include JLine. On JDK 17–21, this command also
does not compile the other source files. VS Code's library settings apply
only inside the editor; terminal commands must include the dependencies too.

To build and run without Make, use the following commands with `javac` on
your `PATH`:

**Windows (PowerShell or cmd):**

```powershell
cd java
javac -cp ".;jline-3.26.3.jar" *.java
java --enable-native-access=ALL-UNNAMED -cp ".;jline-3.26.3.jar" KeySprint
```



**macOS / Linux:**

```bash
cd java
javac -cp ".:jline-3.26.3.jar" *.java
java --enable-native-access=ALL-UNNAMED -cp ".:jline-3.26.3.jar" KeySprint
```

The only difference is the classpath separator: `;` on Windows, `:` on
macOS/Linux. `--enable-native-access=ALL-UNNAMED` is required by JLine on
recent JDKs.

The Java version reads the shared word bank from `../data/words.json` when run
from `java/`, or `data/words.json` when run from the project root. Pass an
explicit path as the first argument if needed.

### Clean build files

With Make:

```bash
make -C java clean
```

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
- The box keeps its original size with four visible rows of wrapped words.
  As you type, the text scrolls to follow the active line, keeping one previous
  line visible. The minimum terminal size is 78 columns by 11 rows for every
  word count.
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
./cpp/build/keysprint data/words.json
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
