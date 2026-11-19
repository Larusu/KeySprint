#!/usr/bin/env python3
# Prepare and launch the existing KeySprint games without requiring Make.

from __future__ import annotations

import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import signal
import subprocess
import sys
import venv


ROOT = Path(__file__).resolve().parent

JDK_HELP = (
    "Install a JDK (not just a JRE), version 17 or newer:\n"
    "  https://adoptium.net/installation\n"
    "Make java available on PATH, or set JAVA_HOME to the JDK folder.\n"
    "If you change PATH, reopen this launcher; otherwise select Java again."
)
CPP_HELP = {
    "darwin": (
        "Install Apple's Command Line Tools: xcode-select --install\n"
        "Complete the installer, then select C++ again.\n"
        "https://developer.apple.com/documentation/xcode/installing-the-command-line-tools"
    ),
    "win32": (
        "Install MSYS2 and its native C++ compiler:\n"
        "  https://www.msys2.org/\n"
        "In the MSYS2 UCRT64 terminal: pacman -S mingw-w64-ucrt-x86_64-gcc\n"
        "Add C:\\msys64\\ucrt64\\bin (or your installation's ucrt64\\bin)\n"
        "to PATH, then reopen this launcher."
    ),
    "linux": (
        "Install g++ or clang++ with C++17 support using your system's package manager.\n"
        "For Ubuntu/Debian: sudo apt install g++\n"
        "For Fedora: sudo dnf install gcc-c++\n"
        "Then select C++ again."
    ),
}
PYTHON_HELP = (
    "Python setup needs Python 3.10+, venv, and pip.\n"
    "If venv/ensurepip is missing on Ubuntu/Debian, install python3-venv.\n"
    "Other systems: https://www.python.org/downloads/\n"
    "For download failures, check your internet connection and select Python again."
)


class SetupError(RuntimeError):
    """A recoverable setup problem to display above the language menu."""


# Check a tool and return its captured output, such as a compiler version.
# Report failed or timed-out checks with the matching installation instructions.
def probe(command: list[str], cwd: Path, help_text: str) -> str:
    try:
        result = subprocess.run(command, cwd=cwd, capture_output=True,
                                text=True, errors="replace", timeout=20)
    except (OSError, subprocess.TimeoutExpired) as error:
        raise SetupError(f"Cannot check {command[0]}: {error}\n{help_text}") from error
    output = (result.stdout or "") + (result.stderr or "")
    if result.returncode:
        raise SetupError(f"Tool check failed:\n{output.strip()}\n{help_text}")
    return output.strip()


# Run an installer or compiler with its output visible in the terminal.
# Stop preparation and report the problem if the command fails.
def run_checked(command: list[str], cwd: Path, help_text: str) -> None:
    try:
        result = subprocess.run(command, cwd=cwd)
    except OSError as error:
        raise SetupError(f"Could not start {command[0]}: {error}\n{help_text}") from error
    if result.returncode:
        raise SetupError(f"Setup stopped (exit code {result.returncode}).\n{help_text}")


class Launcher:
    # Locate the project and its dedicated build/launcher folder.
    # Generated files stay separate from the existing games and environments.
    def __init__(self, root: Path = ROOT):
        self.root = root.resolve()
        self.build = self.root / "build" / "launcher"

    # Create or reuse the launcher environment, repair missing pip, and install
    # Textual when the game imports fail. Return a command that uses this
    # environment directly, so the user never needs to activate it manually.
    def prepare_python(self) -> list[str]:
        folder = self.build / "venv"
        executable = folder / ("Scripts/python.exe" if os.name == "nt" else "bin/python")
        if not executable.is_file():
            print("Creating the launcher's Python environment...", flush=True)
            try:
                venv.create(folder, with_pip=True)
            except (OSError, subprocess.SubprocessError) as error:
                raise SetupError(f"Cannot create the Python environment: {error}\n{PYTHON_HELP}") from error
        check = [str(executable), "-c",
                 "import sys; assert sys.version_info >= (3, 10); "
                 "assert sys.prefix != sys.base_prefix; import screens.game_screen"]
        try:
            result = subprocess.run(check, cwd=self.root / "python",
                                    stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        except OSError as error:
            raise SetupError(
                f"The launcher environment is unusable: {error}\n"
                f"Remove only {folder} and select Python again.\n{PYTHON_HELP}"
            ) from error
        if result.returncode:
            print("Installing Python dependencies (internet needed on first use)...", flush=True)
            pip_check = subprocess.run([str(executable), "-m", "pip", "--version"],
                                       cwd=self.root, stdout=subprocess.DEVNULL,
                                       stderr=subprocess.DEVNULL)
            if pip_check.returncode:
                run_checked([str(executable), "-m", "ensurepip", "--upgrade"],
                            self.root, PYTHON_HELP)
            run_checked([str(executable), "-m", "pip", "install", "--disable-pip-version-check",
                         "textual"], self.root, PYTHON_HELP)
            run_checked(check, self.root / "python", PYTHON_HELP)
        else:
            print("Python environment is ready; reusing installed dependencies.", flush=True)
        return [str(executable), str(self.root / "python" / "main.py")]

    # Reuse a successful build while its command, compiler, inputs, and outputs
    # remain unchanged. Otherwise rebuild and save fresh content hashes only
    # after success, so failed or interrupted builds cannot be reused.
    def compile_cached(self, folder: Path, command: list[str], inputs: list[Path],
                       version: str, outputs: list[Path], help_text: str) -> None:
        digest = hashlib.sha256(json.dumps([command, version, sys.platform]).encode())
        for path in sorted(inputs):
            digest.update(str(path.relative_to(self.root)).encode())
            digest.update(b"\0")
            digest.update(path.read_bytes())
        signature = digest.hexdigest()
        stamp = folder / "build.json"
        try:
            cached = json.loads(stamp.read_text(encoding="utf-8"))
            recorded = cached["outputs"]
            if (cached["signature"] == signature and recorded
                    and all(path.name in recorded for path in outputs)
                    and all(Path(name).name == name and
                            hashlib.sha256((folder / name).read_bytes()).hexdigest() == value
                            for name, value in recorded.items())):
                print(f"{folder.name.title()} build is current; skipping compilation.", flush=True)
                return
        except (OSError, ValueError, KeyError, TypeError, AttributeError):
            pass
        folder.mkdir(parents=True, exist_ok=True)
        stamp.unlink(missing_ok=True)
        if folder.name == "java":
            for path in folder.glob("*.class"):
                path.unlink()
        print(f"Compiling {folder.name.title()}...", flush=True)
        run_checked(command, self.root, help_text)
        if not all(path.is_file() for path in outputs):
            raise SetupError("The compiler did not create the expected game files.\n" + help_text)
        built = sorted(folder.glob("*.class")) if folder.name == "java" else outputs
        stamp.write_text(json.dumps({"signature": signature, "outputs": {
            path.name: hashlib.sha256(path.read_bytes()).hexdigest() for path in built
        }}, indent=2), encoding="utf-8")

    # Find a JDK 17+ through JAVA_HOME or PATH and compile with its compiler
    # module and bundled JLine. Return the platform-appropriate game command.
    def prepare_java(self) -> list[str]:
        home = os.environ.get("JAVA_HOME")
        home_java = Path(home) / "bin" / ("java.exe" if os.name == "nt" else "java") if home else None
        java = str(home_java) if home_java and home_java.is_file() else shutil.which("java")
        if not java:
            raise SetupError("Java development tools were not found.\n" + JDK_HELP)
        compiler = [java, "-m", "jdk.compiler/com.sun.tools.javac.Main"]
        version = probe(compiler + ["-version"], self.root, JDK_HELP)
        match = re.search(r"javac\s+(\d+)", version)
        if not match or int(match.group(1)) < 17:
            raise SetupError(f"JDK 17 or newer is required. Found: {version}\n{JDK_HELP}")
        source_dir = self.root / "java"
        jar = source_dir / "jline-3.26.3.jar"
        sources = sorted(source_dir.glob("*.java"))
        if not jar.is_file() or not sources:
            raise SetupError("Java files or the bundled JLine jar are missing; restore the complete project folder.")
        folder = self.build / "java"
        command = compiler + ["-encoding", "UTF-8", "-cp", str(jar), "-d", str(folder)]
        command += [str(path) for path in sources]
        self.compile_cached(folder, command, sources + [jar], version,
                            [folder / f"{path.stem}.class" for path in sources],
                            "Check the compiler messages above.\n" + JDK_HELP)
        return [java, "--enable-native-access=ALL-UNNAMED", "-cp",
                os.pathsep.join([str(folder), str(jar)]), "KeySprint"]

    # Find g++ or clang++, compile the game with C++17 and thread support,
    # and include headers in rebuild checks. Return the compiled game command.
    def prepare_cpp(self) -> list[str]:
        help_text = CPP_HELP.get(sys.platform, CPP_HELP["linux"])
        compiler = shutil.which("g++") or shutil.which("clang++")
        if not compiler:
            raise SetupError("A C++ compiler was not found.\n" + help_text)
        version = probe([compiler, "--version"], self.root, help_text)
        source_dir = self.root / "cpp"
        sources = sorted(source_dir.glob("*.cpp"))
        if not sources or not (source_dir / "main.cpp").is_file():
            raise SetupError("C++ source files are missing; restore the complete project folder.") # :P
        if not all(path.is_file() for path in sources):
            raise SetupError("C++ source files are missing; restore the complete project folder.")
        headers = sorted(set(source_dir.rglob("*.h")) | set(source_dir.rglob("*.hpp")))
        folder = self.build / "cpp"
        executable = folder / ("keysprint.exe" if os.name == "nt" else "keysprint")
        command = [compiler, "-std=c++17", "-pthread", "-I", str(source_dir / "include")]
        command += [str(path) for path in sources] + ["-o", str(executable)]
        self.compile_cached(folder, command, sources + headers, version, [executable],
                            "Check the compiler messages above; a C++17 toolchain is required.\n" + help_text)
        return [str(executable)]


# Run the game in the same terminal while keeping the launcher alive when
# the game receives Ctrl+C. Use a caught signal handler rather than SIG_IGN
# so POSIX children can receive Ctrl+C, then restore terminal settings on exit.
def launch(command: list[str], cwd: Path) -> int:
    saved_terminal = None
    if os.name != "nt" and sys.stdin.isatty():
        import termios
        saved_terminal = termios.tcgetattr(sys.stdin)
    previous_handler = signal.signal(signal.SIGINT, lambda _signum, _frame: None)
    try:
        return subprocess.run(command, cwd=cwd).returncode
    finally:
        signal.signal(signal.SIGINT, previous_handler)
        if saved_terminal is not None:
            termios.tcsetattr(sys.stdin, termios.TCSAFLUSH, saved_terminal)
        if sys.stdout.isatty():
            print("\033[0m\033[?25h\033[?1049l\033[?2004l", end="", flush=True)


def clear_terminal() -> None:
    if sys.stdout.isatty():
        print("\033[2J\033[H", end="", flush=True)


# Show the language menu, prepare the selected game, and launch it.
# Return to the menu after the game or a setup error; exit on the quit option
# or when Ctrl+C/end of input is received at the menu.
def main() -> int:
    if sys.version_info < (3, 10):
        print("KeySprint's launcher requires Python 3.10 or newer.\n" + PYTHON_HELP)
        return 1
    app = Launcher()
    choices = {"1": (app.prepare_python, "python"),
               "2": (app.prepare_java, "java"), "3": (app.prepare_cpp, "cpp")}
    while True:
        print("\nKeySprint\n  1. Python\n  2. Java\n  3. C++\n  0. Exit", flush=True)
        try:
            choice = input("Choose a language: ").strip().lower()
        except (EOFError, KeyboardInterrupt):
            print()
            return 0
        try:
            if choice in ("0", "q", "quit", "exit"):
                return 0
            if choice not in choices:
                print("Enter 1, 2, 3, or 0.")
                continue
            if not (app.root / "data" / "words.json").is_file():
                raise SetupError("data/words.json is missing; restore the complete project folder.")
            prepare, folder = choices[choice]
            command = prepare()
            print("Starting the game. Exit the game to return to this menu.", flush=True)
            status = launch(command, app.root / folder)
            clear_terminal()
            if status not in (0, 130, -signal.SIGINT, 0xC000013A):
                print(f"The game closed with exit code {status}; review its messages above.")
        except KeyboardInterrupt:
            print("\nSetup cancelled. Choose a language to retry.")
        except (SetupError, OSError) as error:
            print(f"\n{error}\nChoose a language to retry, or 0 to exit.")


if __name__ == "__main__":
    raise SystemExit(main())
