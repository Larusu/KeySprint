"""Exercise the real terminal app through a POSIX pseudo-terminal."""
import fcntl
import os
from pathlib import Path
import pty
import re
import select
import struct
import subprocess
import termios
import time
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]

class TerminalSmoke(unittest.TestCase):
    def setUp(self):
        self.master, self.slave = pty.openpty()
        fcntl.ioctl(self.slave, termios.TIOCSWINSZ, struct.pack("HHHH", 30, 100, 0, 0))
        self.original = termios.tcgetattr(self.slave)
        self.app = subprocess.Popen([str(ROOT / "cpp/keysprint")], cwd=ROOT,
            stdin=self.slave, stdout=self.slave, stderr=self.slave)
        deadline = time.monotonic() + 3
        while "╭" not in self.screen():
            if self.app.poll() is not None or time.monotonic() > deadline:
                self.fail("terminal app did not finish startup")

    def tearDown(self):
        if self.app.poll() is None:
            os.write(self.master, b"\x03")
            try:
                self.app.wait(timeout=3)
            except subprocess.TimeoutExpired:
                self.app.kill()
                self.app.wait()
        os.close(self.master)
        os.close(self.slave)

    def screen(self, keys=b""):
        if keys:
            os.write(self.master, keys)
        output = b""
        deadline = time.monotonic() + 0.18
        while time.monotonic() < deadline:
            if select.select([self.master], [], [], 0.03)[0]:
                output += os.read(self.master, 262144)
        frames = output.decode("utf-8", errors="replace").split("\x1b[2J")
        frame = frames[-2] if len(frames) > 2 else frames[-1]
        self.frame = frame
        return re.sub(r"\x1b\[[0-?]*[ -/]*[@-~]", "", frame)

    def grid(self, rows=30, cols=100):
        """Replay cursor moves so overlapping labels cannot hide in a text dump."""
        cells = [[" "] * cols for _ in range(rows)]
        row = col = 0
        pieces = re.split(r"(\x1b\[[0-?]*[ -/]*[@-~])", self.frame)
        for piece in pieces:
            move = re.fullmatch(r"\x1b\[(\d+);(\d+)H", piece)
            if move:
                row, col = int(move[1]) - 1, int(move[2]) - 1
            elif piece.startswith("\x1b["):
                continue
            else:
                for char in piece:
                    if 0 <= row < rows and 0 <= col < cols:
                        cells[row][col] = char
                    col += 1
        return ["".join(line) for line in cells]

    def assert_box_boundaries(self):
        grid = self.grid()
        for line in grid:
            if "╭" in line or "╰" in line:
                self.assertEqual(line[13], "╭" if "╭" in line else "╰")
                self.assertEqual(line[86], "╮" if "╭" in line else "╯")
            self.assertFalse(line[:13].strip(), "drawing escaped the left side")
            self.assertFalse(line[87:].strip(), "drawing escaped the right side")
        bar = next(line for line in grid if "Time" in line and "25k" in line)
        self.assertEqual(bar.count("│"), 8, "a selector label overwrote its border")
        return grid

    def test_large_custom_count_fits_the_layout(self):
        self.screen()
        self.screen(b"\x06\x1b[Cw10000\r")
        self.assertIn("0/10000", self.screen(b"\x0e"))
        grid = self.assert_box_boundaries()
        self.assertTrue(any("10000" in line and "25k" in line for line in grid))

    def test_timed_refills_and_small_window(self):
        self.screen()
        playing = self.screen(b"zzz " * 90)
        self.assertNotIn("accuracy", playing, "Time mode ended after the initial batch")
        self.assertEqual(self.app.poll(), None)
        fcntl.ioctl(self.slave, termios.TIOCSWINSZ, struct.pack("HHHH", 10, 40, 0, 0))
        self.assertIn("74 columns and 17 rows", self.screen())
        fcntl.ioctl(self.slave, termios.TIOCSWINSZ, struct.pack("HHHH", 17, 74, 0, 0))
        self.assertNotIn("Resize terminal", self.screen())
        fcntl.ioctl(self.slave, termios.TIOCSWINSZ, struct.pack("HHHH", 30, 100, 0, 0))
        self.assertIn("left/right", self.screen(b"\x06"))
        self.assert_box_boundaries()

    def test_startup_errors_do_not_change_terminal(self):
        self.screen(b"\x03")
        self.app.wait(timeout=3)
        for cwd in (ROOT, ROOT / "cpp"):
            result = subprocess.run([str(ROOT / "cpp/keysprint"), "missing-word-bank.json"],
                                    cwd=cwd, capture_output=True, timeout=3)
            self.assertEqual(result.returncode, 1)
            self.assertIn(b"Startup error", result.stderr)
            self.assertNotIn(b"\x1b[", result.stdout)
        with tempfile.TemporaryDirectory() as folder:
            path = Path(folder) / "words.json"
            path.write_text("{ malformed")
            result = subprocess.run([str(ROOT / "cpp/keysprint"), str(path)],
                                    cwd=ROOT, capture_output=True, timeout=3)
            self.assertEqual(result.returncode, 1)
            self.assertIn(b"Startup error", result.stderr)
            self.assertNotIn(b"\x1b[", result.stdout)

    def test_main_layout_and_custom_word_round(self):
        initial = self.screen()
        self.assertIn("Words", initial, "main's Time/Words selector must be preserved")
        self.assertIn("25k", initial)
        self.assertIn("left/right", self.screen(b"\x06"))
        self.screen(b"\x1b[C")  # Time -> Words
        self.assertIn("Word count", self.screen(b"w"))
        self.assertIn("between 1 and 10,000", self.screen(b"0\r"))
        self.assertIn("Word count", self.screen(b"\x0e"))
        self.screen(b"\x7f1\r")
        self.assertIn("0/1", self.screen(b"\x0e"))
        results = self.screen(b"zzz ")
        self.assertIn("accuracy", results)
        self.assertIn("missed", results)
        self.assertIn("left/right", self.screen(b"\x06"))
        self.screen(b"\x03")
        self.assertEqual(self.app.wait(timeout=3), 0)
        self.assertEqual(termios.tcgetattr(self.slave), self.original)

if __name__ == "__main__":
    unittest.main()
