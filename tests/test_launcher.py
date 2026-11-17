"""Launcher regressions; game sources are never edited by these tests."""

import contextlib
import importlib.util
import io
import os
from pathlib import Path
import signal
import subprocess
import sys
import tempfile
import unittest
from unittest.mock import patch


ROOT = Path(__file__).resolve().parents[1]
SPEC = importlib.util.spec_from_file_location("launcher", ROOT / "launcher.py")
launcher = importlib.util.module_from_spec(SPEC)
if SPEC.loader and (ROOT / "launcher.py").exists():
    SPEC.loader.exec_module(launcher)


class LauncherTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory(prefix="KeySprint test space ")
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        for folder in ("java", "cpp/include", "python", "data"):
            (self.root / folder).mkdir(parents=True)
        for name in ("KeySprint.java", "Config.java"):
            (self.root / "java" / name).write_text("// fixture", encoding="utf-8")
        (self.root / "java/jline-3.26.3.jar").write_bytes(b"fixture jar")
        for name in ("main.cpp", "terminal.cpp", "game.cpp", "scoring.cpp",
                     "render.cpp", "config.cpp", "word_bank.cpp", "game.h",
                     "include/json.hpp"):
            (self.root / "cpp" / name).write_text("// fixture", encoding="utf-8")
        (self.root / "python/main.py").write_text("print('game')", encoding="utf-8")
        (self.root / "data/words.json").write_text("{}", encoding="utf-8")
        self.app = launcher.Launcher(self.root)

    def java_tool(self, command, **kwargs):
        if "-version" in command:
            return subprocess.CompletedProcess(command, 0, "javac 21.0.1\n", "")
        self.assertIn("jdk.compiler/com.sun.tools.javac.Main", command)
        self.assertEqual(command[command.index("-encoding") + 1], "UTF-8")
        self.assertEqual(command[command.index("-cp") + 1],
                         str(self.root / "java/jline-3.26.3.jar"))
        self.java_compiles += 1
        output = Path(command[command.index("-d") + 1])
        for name in ("KeySprint.class", "Config.class", "KeySprint$State.class"):
            (output / name).write_bytes(b"compiled")
        return subprocess.CompletedProcess(command, 0)

    def test_java_reuses_build_but_rebuilds_changed_source_jar_or_missing_class(self):
        self.java_compiles = 0
        with patch.dict(os.environ, {}, clear=True), \
             patch.object(launcher.shutil, "which", return_value="/tools/java"), \
             patch.object(launcher.subprocess, "run", side_effect=self.java_tool):
            command = self.app.prepare_java()
            self.assertEqual(command[0], "/tools/java")
            self.assertEqual(command[-1], "KeySprint")
            self.app.prepare_java()
            self.assertEqual(self.java_compiles, 1)
            (self.root / "java/Config.java").write_text("// changed", encoding="utf-8")
            self.app.prepare_java()
            self.assertEqual(self.java_compiles, 2)
            (self.root / "java/jline-3.26.3.jar").write_bytes(b"changed jar")
            self.app.prepare_java()
            self.assertEqual(self.java_compiles, 3)
            (self.root / "build/launcher/java/KeySprint$State.class").unlink()
            self.app.prepare_java()
            self.assertEqual(self.java_compiles, 4)

    def test_failed_rebuild_does_not_certify_old_java_output(self):
        self.java_compiles = 0
        with patch.dict(os.environ, {}, clear=True), \
             patch.object(launcher.shutil, "which", return_value="/tools/java"), \
             patch.object(launcher.subprocess, "run", side_effect=self.java_tool):
            self.app.prepare_java()
            source = self.root / "java/Config.java"
            source.write_text("// broken change", encoding="utf-8")
            with patch.object(launcher.subprocess, "run", side_effect=[
                subprocess.CompletedProcess([], 0, "javac 21.0.1", ""),
                subprocess.CompletedProcess([], 1),
            ]):
                with self.assertRaises(launcher.SetupError):
                    self.app.prepare_java()
            source.write_text("// fixture", encoding="utf-8")
            self.app.prepare_java()
            self.assertEqual(self.java_compiles, 2)

    def test_missing_and_old_jdk_have_installation_help(self):
        with patch.dict(os.environ, {}, clear=True), \
             patch.object(launcher.shutil, "which", return_value=None):
            with self.assertRaisesRegex(launcher.SetupError, "https://adoptium.net"):
                self.app.prepare_java()
        with patch.object(launcher.shutil, "which", return_value="/tools/java"), \
             patch.object(launcher.subprocess, "run", return_value=
                          subprocess.CompletedProcess([], 0, "javac 11.0.2", "")):
            with self.assertRaisesRegex(launcher.SetupError, "17"):
                self.app.prepare_java()

    def test_cpp_rebuilds_for_local_and_vendored_headers_without_make(self):
        compiles = []

        def compiler(command, **kwargs):
            if "--version" in command:
                return subprocess.CompletedProcess(command, 0, "g++ 13.3", "")
            self.assertIn("-std=c++17", command)
            self.assertIn("-pthread", command)
            self.assertNotIn("make", command)
            compiles.append(command)
            Path(command[command.index("-o") + 1]).write_bytes(b"executable")
            return subprocess.CompletedProcess(command, 0)

        with patch.object(launcher.shutil, "which", return_value="/tools/g++"), \
             patch.object(launcher.subprocess, "run", side_effect=compiler):
            command = self.app.prepare_cpp()
            self.assertEqual(Path(command[0]).parent, self.root / "build/launcher/cpp")
            self.app.prepare_cpp()
            self.assertEqual(len(compiles), 1)
            for i, name in enumerate(("game.h", "include/json.hpp"), 2):
                (self.root / "cpp" / name).write_text("// changed", encoding="utf-8")
                self.app.prepare_cpp()
                self.assertEqual(len(compiles), i)

    def test_python_creates_owned_environment_and_installs_only_missing_dependencies(self):
        executable = self.root / "build/launcher/venv" / (
            "Scripts/python.exe" if os.name == "nt" else "bin/python")
        installed = False
        installations = []

        def create_environment(path, **kwargs):
            executable.parent.mkdir(parents=True)
            executable.touch()

        def python_process(command, **kwargs):
            nonlocal installed
            self.assertEqual(command[0], str(executable))
            if "pip" in command and "install" in command:
                installations.append(command)
                installed = True
                return subprocess.CompletedProcess(command, 0)
            if "ensurepip" in command:
                return subprocess.CompletedProcess(command, 0)
            return subprocess.CompletedProcess(command, 0 if installed else 1)

        with patch.object(launcher.venv, "create", side_effect=create_environment) as create, \
             patch.object(launcher.subprocess, "run", side_effect=python_process):
            command = self.app.prepare_python()
            self.app.prepare_python()
            self.assertEqual(create.call_count, 1)
            self.assertEqual(len(installations), 1)
            self.assertEqual(command, [str(executable), str(self.root / "python/main.py")])

    def test_menu_recovers_after_setup_error_and_invalid_choice(self):
        output = io.StringIO()
        with contextlib.redirect_stdout(output), \
             patch("builtins.input", side_effect=["bad", "2", "0"]), \
             patch.object(launcher.Launcher, "prepare_java", side_effect=
                          launcher.SetupError("Install the JDK, then retry.")), \
             patch.object(launcher, "launch") as launch:
            self.assertEqual(launcher.main(), 0)
        self.assertIn("Install the JDK", output.getvalue())
        self.assertGreaterEqual(output.getvalue().count("KeySprint"), 3)
        launch.assert_not_called()

    def test_retry_repairs_environment_left_without_pip(self):
        folder = self.root / "build/launcher/venv"
        launcher.venv.create(folder, with_pip=False)
        executable = folder / ("Scripts/python.exe" if os.name == "nt" else "bin/python")
        screens = self.root / "python/screens"
        screens.mkdir()
        (screens / "__init__.py").touch()
        (screens / "game_screen.py").write_text("import fixture_dependency", encoding="utf-8")
        real_run = subprocess.run

        def run_with_local_installer(command, **kwargs):
            if "pip" in command and "install" in command:
                ready = real_run([str(executable), "-m", "pip", "--version"],
                                 capture_output=True, text=True)
                self.assertEqual(ready.returncode, 0, "Retry did not restore missing pip")
                (self.root / "python/fixture_dependency.py").touch()
                return subprocess.CompletedProcess(command, 0)
            return real_run(command, **kwargs)

        with patch.object(launcher.subprocess, "run", side_effect=run_with_local_installer):
            self.assertEqual(self.app.prepare_python()[0], str(executable))

    def test_ctrl_c_at_menu_after_a_game_exits_launcher(self):
        with contextlib.redirect_stdout(io.StringIO()), \
             patch("builtins.input", side_effect=["2", KeyboardInterrupt]), \
             patch.object(launcher.Launcher, "prepare_java", return_value=["java", "KeySprint"]), \
             patch.object(launcher, "launch", return_value=0):
            self.assertEqual(launcher.main(), 0)

    @unittest.skipIf(os.name == "nt", "POSIX signal delivery test")
    def test_game_ctrl_c_does_not_kill_launcher(self):
        # A real child interrupts the foreground process group, as Ctrl+C does.
        script = (
            "import os,signal,sys; from pathlib import Path; "
            f"sys.path.insert(0, {str(ROOT)!r}); import launcher; "
            "code='import os,signal,time; os.killpg(os.getpgrp(),signal.SIGINT); time.sleep(.1)'; "
            "launcher.launch([sys.executable,'-c',code],Path.cwd()); print('MENU SURVIVED')"
        )
        result = subprocess.run([sys.executable, "-c", script], capture_output=True,
                                text=True, start_new_session=True, timeout=10)
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn("MENU SURVIVED", result.stdout)


if __name__ == "__main__":
    unittest.main()
