# KeySprint

## Running Python Version

in-progress

## Running Java Version

in-progress

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
