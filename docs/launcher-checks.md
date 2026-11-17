# Launcher verification

Checked on Linux on November 17, 2026, using Python 3.12.3, OpenJDK 21,
g++ 13.3, and Textual 8.2.8.

## Limitations and existing failures

- Native macOS and Windows startup and gameplay are **not checked**. The
  macOS shell helper was exercised on Linux, which does not verify Finder.
  Linux desktop command parsing was exercised with Gio without opening a
  graphical terminal; the file manager's trust dialog remains user-dependent.
- The existing `make -C cpp test` command **fails to compile** its game tests.
  They reference `Config.word_list`, `ALL_DIFFICULTIES`, `difficulty_key`, and
  an older `new_game(config)` signature absent from the current sources.
  None of those existing files was changed. The actual C++ game compiled and
  ran through the launcher.
- The interview spec's Ctrl+C wording does not describe a universal game
  shortcut. The existing Python game uses Escape/Ctrl+Q to quit; Textual's
  Ctrl+C action displays a quit hint. These controls were preserved.

## Acceptance results

| Check | Result | Evidence |
|---|---|---|
| M1: Language menu and return after game exit | Pass with existing controls | One native Linux terminal session selected Java, C++, then Python; each rendered and returned to the menu. Java/C++ exited with Ctrl+C; Python exited through its existing quit controls. Ctrl+C at the menu then exited with status 0. Invalid input also reprompted in the regression suite. |
| M2: Python environment and dependencies | Pass | A fresh launcher-owned environment was created and Textual installed. Later runs reported reuse. A real temporary environment without pip was repaired by ensurepip in the regression suite. |
| M3: Java without Make | Pass on Linux | A fresh copied project compiled using the JDK compiler module with no Make on PATH, including a project path containing spaces. Gameplay rendered in a separate native terminal check. |
| M4: C++ without Make | Pass on Linux | A fresh copied project compiled with g++ and no Make on PATH, including a project path containing spaces. Gameplay rendered in a separate native terminal check. |
| M5: Build reuse and invalidation | Pass | Real repeat builds skipped compilation. Temporary-fixture tests rebuilt after Java source/JLine changes, C++ local/vendored header changes, and a missing Java inner-class output. Failed rebuilds did not certify stale output. |
| M6: Setup errors and retry | Pass | Tests hid Java or supplied an old JDK, forced a failed compiler, exercised invalid menu input, and checked that setup errors returned to the menu without starting a game. A real download restriction produced installation guidance; retry with network access succeeded. |
| M7: Terminal and double-click entry points | Partial | POSIX startup helpers opened the menu from another directory and paths with spaces. The Linux desktop entry passed desktop-file-validate and native Gio command parsing; missing Python produced guidance and a pause. Native Windows and Finder launches are not checked. |
| M8: Existing game files unchanged | Pass | SHA-256 hashes of all 38 tracked Python/Java/C++/data files, including Makefiles, match the hashes recorded before implementation. |
| R1: Reject game changes | Pass: condition absent | Original file hashes match; tracked-file diff is empty. All deliverable changes are additions. |
| R2: Reject manual activation/build steps | Pass: condition absent | The native menu session launched all three games without a virtual-environment activation command or a build command. |
| S1: Open all three versions from one menu | Pass on Linux | The sequential native terminal check completed and returned to the same menu after each game. |
| O1: Gameplay, scoring, controls, visual changes excluded | Pass | No existing game files changed. |
| O2: System-wide toolchain provisioning excluded | Pass | Prerequisite errors provide instructions; the launcher does not invoke system package managers. |
| O3: GUI/executable packaging excluded | Pass | Delivered one standard-library terminal launcher with small platform entry files. |
| O4: Offline first setup excluded | Pass | Python's initial dependencies download through pip; subsequent valid setups are reused. |

## Regression checks

`python3 -m unittest discover -s tests -v` passed **9 tests**. Shell syntax,
Python compilation, desktop entry validation, and whitespace checks also passed.
A separate review found two recovery/bootstrap issues; both were fixed and
rechecked with no further important issue reported.

Assumptions still unconfirmed: the school computers' installed tools, project
folder write access there, and platform-specific permission/trust behavior.
The numbered menu and project-local build storage follow the approved spec.
