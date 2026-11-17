# Central launcher

Run this from the project folder:

```sh
python3 launcher.py
```

On Windows, use `py -3 launcher.py` or `python launcher.py`. Select `1` for
Python, `2` for Java, `3` for C++, or `0` to exit. The launcher prepares only
the version you select. Exit the game with its existing controls to return
to the language menu. Ctrl+C at the launcher menu closes the launcher.
Python keeps its Escape/Ctrl+Q exit controls; its Textual Ctrl+C action shows
a quit hint. Java and C++ keep their Ctrl+C exit controls.

## Double-click startup

- Open the `startup` folder to find all platform startup files.
- **Windows:** double-click `startup/Start-KeySprint.bat`.
- **macOS:** double-click `startup/Start-KeySprint.command` to open Terminal.
- **Linux:** double-click `startup/Start-KeySprint.desktop` and allow launching if
  your file manager asks. Alternatively, use `startup/Start-KeySprint.sh` and choose
  **Run in Terminal**. Desktop behavior depends on the file manager and an
  installed terminal application; a terminal command always remains available.

If executable permissions were lost when copying the project on macOS/Linux,
restore them once from the project folder:

```sh
chmod +x startup/Start-KeySprint.command startup/Start-KeySprint.sh startup/Start-KeySprint.desktop
```

Keep the entire project folder together, including the shared `data` folder,
the Java jar, and the startup helpers. The launcher locates the project from
its own file location, so it can also be started from another directory.
Run it in a real terminal; an editor's output-only panel cannot accept game input.

## Prerequisites

The launcher needs [Python 3.10 or newer](https://www.python.org/downloads/).
The other tools are checked only when their language is selected:

- **Python game:** the launcher creates `build/launcher/venv`, installs Textual
  and its dependencies, and reuses that environment afterward. There is no
  activation command. Internet access is needed for the first download.
  If Ubuntu/Debian reports missing `venv` or `ensurepip`, install `python3-venv`.
  See the [Python virtual environment documentation](https://docs.python.org/3/library/venv.html).
- **Java game:** install a **JDK 17+**, including the compiler module. The
  bundled JLine jar is used automatically. Make `java` available on `PATH`,
  or set `JAVA_HOME` to the JDK folder. The compiler comes from this same JDK;
  a separate `javac` command is unnecessary.
  [Temurin installation instructions](https://adoptium.net/installation).
- **C++ game:** install `g++` or `clang++` with C++17 support and make it
  available on `PATH`. On macOS, run `xcode-select --install` and complete
  [Apple's Command Line Tools installation](https://developer.apple.com/documentation/xcode/installing-the-command-line-tools).
  On Windows, follow the [MSYS2 instructions](https://www.msys2.org/), open
  its UCRT64 terminal, and run `pacman -S mingw-w64-ucrt-x86_64-gcc`. Add its
  `ucrt64\bin` folder to `PATH` so the compiler and runtime DLLs can be found.
  Ubuntu/Debian users can install `g++`; Fedora users can install `gcc-c++`.

Make is not required. The launcher does not install system-wide tools or
request administrator access. If a prerequisite is missing, it explains
what to install and returns to the menu. Reopen the launcher after changing
environment variables such as `PATH` or `JAVA_HOME`.

## Setup reuse and troubleshooting

Generated environments and compiled files are stored under the ignored
`build/launcher` folder. Existing game sources, Makefiles, shared word data,
and `python/.venv` are left alone. Java and C++ builds are reused until the
source files, headers, bundled library, compiler version, or build command
changes. Missing or modified compiled output also triggers rebuilding.

Setup and compilation errors remain visible above the menu. Correct the
problem and select the language again. A failed or interrupted build is
never recorded as successful.

When transferring the project to another computer, leave out `build/` and
any `.venv` folders. Let the launcher create fresh files on that computer;
virtual environments are local to the Python installation that created them.
The project folder must be writable. To reset launcher setup, delete only
`build/launcher` and select the desired language again.

## Checks

```sh
python3 -m unittest discover -s tests -v
```

Native Windows and macOS startup and gameplay still need checks on those
systems; Linux checks do not establish native behavior on either platform.

See [the verification report](launcher-checks.md) for the checks run on this change.
