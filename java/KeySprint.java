import java.io.IOException;
import java.io.PrintWriter;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.Locale;
import java.util.Map;
import java.util.Random;
import org.jline.terminal.Attributes;
import org.jline.terminal.Size;
import org.jline.terminal.Terminal;
import org.jline.terminal.TerminalBuilder;
import org.jline.utils.NonBlockingReader;

public class KeySprint {
    private Terminal terminal;
    private Attributes original;
    private NonBlockingReader reader;
    private PrintWriter out;

    private static final int ROW_TOP = 1;
    private static final int ROW_HEAD = 2;
    private static final int ROW_SEP = 3;
    private static final int ROW_BODY = 4;
    private static final int BASE_BODY_ROWS = 4;
    private static final int C1 = 18; // mode/counter
    private static final int C2 = 23; // difficulty
    private static final int C3 = 23; // preset
    private static final int C4 = 9; // start
    private static final int INNER_WIDTH = C1 + C2 + C3 + C4 + 3;
    private static final int BOX_WIDTH = INNER_WIDTH + 2;
    private static final int WORD_WRAP = 50;
    private static final int CONTENT_OFFSET = (INNER_WIDTH - WORD_WRAP) / 2 + 1;

    private static final int PREVIEW_COUNT = 10;
    private static final int LOOKAHEAD = 50;
    private static final int MAX_EXTRA = 5;
    private static final char SKIP = '\0';
    private static final int TICK_MS = 100;
    private static final int ESCAPE_MS = 30;
    private static final String MENU_FOOT =
            "ctrl+c - exit     \u2190 \u2192 - move     enter - select/start";
    private static final String PLAY_FOOT = "esc - restart     ctrl+c - exit";
    private static final String RESULTS_FOOT = "ctrl+c - exit     esc - return to menu";
    private static final String EDIT_FOOT = "Enter: save     Esc: cancel";

    // Key codes outside Unicode range.
    private static final int KEY_ESC = 0x110000;
    private static final int KEY_LEFT = 0x110001;
    private static final int KEY_RIGHT = 0x110002;
    private static final int KEY_IGNORE = 0x110003;

    private enum State { MENU, PLAYING, RESULTS }

    private Map<String, List<String>> wordBank;
    private final Random rng = new Random();

    private final Config config = new Config();
    private final Config.Editor editor = new Config.Editor();

    // 0-1 mode, 2-5 difficulty, 6+ preset, last start.
    private int sel;
    private boolean headerDirty;

    private State state = State.MENU;
    private List<String> picked;
    private final List<String> typedWords = new ArrayList<>();
    private int cursor;
    private long startNanos; // 0 = not started.
    private long endNanos;
    private int correct;
    private int incorrect;
    private int missed;
    private int extra;
    private boolean completed;
    private boolean running = true;

    private int termWidth;
    private int termHeight;
    private int boxRow;
    private int boxCol;
    private int bodyRows = BASE_BODY_ROWS;
    private boolean waitingForSize;
    private boolean timerPaused;
    private long pauseStartNanos;
    private int[] prevBodyLen = new int[BASE_BODY_ROWS];
    private String lastC1 = "";

    // Computed by drawBody.
    private int[] wordLine = new int[0];
    private int[] wordCol = new int[0];
    private final List<String> bodyLines = new ArrayList<>();
    private final List<Integer> bodyWidths = new ArrayList<>();
    private int firstLine;
    private int totalLines;
    private int cursorLine;
    private int lastLine;
    private int lastLineWidth;

    public static void main(String[] args) {
        KeySprint app = new KeySprint();
        try {
            app.wordBank = WordBank.load(findWordBank(args));
        } catch (Exception e) {
            System.err.println("Startup error: " + e.getMessage());
            System.exit(1);
        }
        try {
            app.run();
        } catch (Exception e) {
            System.err.println("Game error: " + e.getMessage());
            System.exit(1);
        }
    }

    private static String findWordBank(String[] args) {
        if (args.length > 1) {
            throw new IllegalArgumentException(
                    "Usage: KeySprint [path/to/words.json]");
        }
        if (args.length == 1) {
            return args[0];
        }
        for (String candidate : new String[] {"data/words.json", "../data/words.json"}) {
            if (Files.isRegularFile(Path.of(candidate))) {
                return candidate;
            }
        }
        throw new IllegalArgumentException(
                "Cannot find the word bank. Tried data/words.json and ../data/words.json");
    }

    private void run() throws Exception {
        terminal = TerminalBuilder.builder().system(true).build();
        original = terminal.enterRawMode();
        Attributes raw = terminal.getAttributes();
        raw.setLocalFlag(Attributes.LocalFlag.ISIG, false); // Receive Ctrl+C as a key.
        terminal.setAttributes(raw);
        reader = terminal.reader();
        out = terminal.writer();

        try {
            setup();
            loop();
        } finally {
            if (timerPaused) {
                startNanos += System.nanoTime() - pauseStartNanos;
                timerPaused = false;
            }
            cleanup();
        }

        if (waitingForSize) {
            System.out.println("Terminal was too small (needed "
                    + BOX_WIDTH + " x " + requiredHeight() + ").");
        }
        printSummary();
    }

    private void move(int row, int col) {
        out.print("\033[" + row + ";" + col + "H");
    }

    private void setup() {
        sel = 0;
        refreshPreview();

        out.print("\033[0m");
        out.print("\033[?1049h"); // Alternate screen.
        out.print("\033[2J\033[H"); // Clear.
        out.print("\033[6 q"); // Steady cursor.
        out.print("\033[?25l"); // Hide cursor.

        updateSize();
    }

    private int screenRow(int relativeRow) {
        return boxRow + relativeRow - 1;
    }

    private int rowFoot() {
        return ROW_BODY + bodyRows + 2;
    }

    private int blockHeight() {
        return rowFoot();
    }

    private int requiredHeight() {
        return rowFoot() + 1;
    }

    private void positionBox() {
        boxCol = Math.max(1, (termWidth - BOX_WIDTH) / 2 + 1);
        boxRow = Math.max(1, (termHeight - blockHeight()) / 2 + 1);
    }

    private boolean sizeOk() {
        return termWidth >= BOX_WIDTH && termHeight >= requiredHeight();
    }

    private void setBodyRows(int rows) {
        rows = Math.max(1, rows);
        if (rows == bodyRows && prevBodyLen.length == rows) return;
        bodyRows = rows;
        prevBodyLen = new int[rows];
    }

    private int wrappedLineCount() {
        int line = 0;
        int lineW = 0;
        for (int i = 0; i < picked.size(); i++) {
            int wlen = Math.max(picked.get(i).length(), typedWords.get(i).length());
            if (lineW > 0 && lineW + 1 + wlen > WORD_WRAP) {
                line++;
                lineW = 0;
            }
            if (lineW > 0) lineW++;
            lineW += wlen;
        }
        return Math.max(1, line + 1);
    }

    private void updateSize() {
        Size size = terminal.getSize();
        int w = Math.max(1, size.getColumns());
        int h = Math.max(1, size.getRows());
        if (w == termWidth && h == termHeight) return;
        termWidth = w;
        termHeight = h;
        relayout();
    }

    private void relayout() {
        if (!sizeOk()) {
            if (!waitingForSize) {
                waitingForSize = true;
                if (state == State.PLAYING && startNanos > 0) {
                    timerPaused = true;
                    pauseStartNanos = System.nanoTime();
                }
            }
            drawTooSmall();
            return;
        }

        if (waitingForSize) {
            waitingForSize = false;
            if (timerPaused) {
                startNanos += System.nanoTime() - pauseStartNanos;
                timerPaused = false;
            }
        }
        positionBox();
        redrawAll();
    }

    private void drawTooSmall() {
        int w = Math.max(1, termWidth);
        int h = Math.max(1, termHeight);
        String[] lines = {
            "Terminal too small",
            "Need at least " + BOX_WIDTH + " x " + requiredHeight(),
            "Current: " + termWidth + " x " + termHeight,
            "Resize to continue"
        };
        out.print("\033[2J\033[H");
        out.print("\033[?25l");
        int start = Math.max(1, (h - lines.length) / 2 + 1);
        for (int i = 0; i < lines.length; i++) {
            int row = start + i;
            if (row > h) break;
            String text = lines[i];
            if (text.length() > w) text = text.substring(0, w);
            int col = Math.max(1, (w - text.length()) / 2 + 1);
            move(row, col);
            out.print(text);
        }
        out.flush();
    }

    private void redrawAll() {
        out.print("\033[2J\033[H");
        out.print(state == State.PLAYING ? "\033[?25h" : "\033[?25l");
        lastC1 = "";
        headerDirty = false;
        Arrays.fill(prevBodyLen, 0);
        drawBox();
        drawCounter();
        drawHeaderCells();
        if (state == State.RESULTS) {
            drawResults(); // Also draws footer.
        } else {
            drawBody();
            drawFooter();
        }
        positionCursor();
        out.flush();
    }

    private void drawBox() {
        move(screenRow(ROW_TOP), boxCol);
        out.print("\u250c" + "\u2500".repeat(C1) + "\u252c"
                + "\u2500".repeat(C2) + "\u252c" + "\u2500".repeat(C3)
                + "\u252c" + "\u2500".repeat(C4) + "\u2510");

        move(screenRow(ROW_HEAD), boxCol);
        out.print("\u2502" + " ".repeat(C1) + "\u2502"
                + " ".repeat(C2) + "\u2502" + " ".repeat(C3)
                + "\u2502" + " ".repeat(C4) + "\u2502");

        move(screenRow(ROW_SEP), boxCol);
        out.print("\u251c" + "\u2500".repeat(C1) + "\u2534"
                + "\u2500".repeat(C2) + "\u2534" + "\u2500".repeat(C3)
                + "\u2534" + "\u2500".repeat(C4) + "\u2524");

        for (int r = 0; r < bodyRows; r++) {
            move(screenRow(ROW_BODY + r), boxCol);
            out.print("\u2502" + " ".repeat(INNER_WIDTH) + "\u2502");
        }

        move(screenRow(rowFoot() - 2), boxCol);
        out.print("\u2514" + "\u2500".repeat(INNER_WIDTH) + "\u2518");
    }

    private int presetCount() {
        return config.mode == Config.Mode.WORDS ? 5 : 4;
    }

    private int startIndex() {
        return 6 + presetCount();
    }

    private int menuItemCount() {
        return startIndex() + 1;
    }

    private void drawCounter() {
        String text;
        if (state == State.MENU) {
            int focus = !editor.editing && sel < 2 ? sel : -1;
            text = cellChips(new String[] {"Time", "Words"},
                    config.mode.ordinal(), focus, C1);
        } else {
            text = config.mode == Config.Mode.TIME
                    ? " Time " + (long) Math.ceil(timeLeft())
                    : " Words " + cursor + "/" + picked.size();
            text = padPlain(text, C1);
        }
        if (text.equals(lastC1)) return;
        lastC1 = text;
        move(screenRow(ROW_HEAD), boxCol + 1);
        out.print(text);
    }

    private String padPlain(String text, int width) {
        if (text.length() >= width) return text.substring(0, width);
        return text + " ".repeat(width - text.length());
    }

    private void drawHeaderCells() {
        boolean inMenu = state == State.MENU && !editor.editing;
        int diffFocus = inMenu && sel >= 2 && sel < 6 ? sel - 2 : -1;
        int start = startIndex();
        int presetFocus = inMenu && sel >= 6 && sel < start ? sel - 6 : -1;
        int startFocus = inMenu && sel == start ? 0 : -1;

        move(screenRow(ROW_HEAD), boxCol + 1 + C1 + 1);
        out.print(cellChips(WordBank.LABELS, config.difficulty, diffFocus, C2));
        move(screenRow(ROW_HEAD), boxCol + 1 + C1 + 1 + C2 + 1);
        out.print(cellChips(config.presetLabels(), config.presetIndex(), presetFocus, C3));
        move(screenRow(ROW_HEAD), boxCol + 1 + C1 + 1 + C2 + 1 + C3 + 1);
        out.print(cellChips(new String[] {"start"}, -1, startFocus, C4));
    }

    private String cellChips(String[] chips, int selected, int focused, int width) {
        StringBuilder sb = new StringBuilder(" ");
        int vis = 1;
        for (int i = 0; i < chips.length; i++) {
            if (i > 0) {
                sb.append(' ');
                vis++;
            }
            if (i == focused) sb.append("\033[1;33m");
            else if (i == selected) sb.append("\033[33m");
            sb.append(chips[i]);
            if (i == focused || i == selected) sb.append("\033[0m");
            vis += chips[i].length();
        }
        while (vis < width) {
            sb.append(' ');
            vis++;
        }
        return sb.toString();
    }

    private void drawBody() {
        if (state == State.RESULTS) return;
        if (state == State.MENU && editor.editing) {
            drawEditor();
            return;
        }

        computeLayout();
        for (int r = 0; r < bodyRows; r++) {
            move(screenRow(ROW_BODY + r), boxCol + CONTENT_OFFSET);
            String s = r < bodyLines.size() ? bodyLines.get(r) : "";
            int w = r < bodyWidths.size() ? bodyWidths.get(r) : 0;
            out.print(s);
            if (w < prevBodyLen[r]) { // Erase leftover characters.
                out.print(" ".repeat(prevBodyLen[r] - w));
            }
            prevBodyLen[r] = Math.max(prevBodyLen[r], w);
        }
    }

    private void drawEditor() {
        String[] lines = {
            "Word count: " + (editor.input.isEmpty() ? "_" : editor.input + "_"),
            "Choose 1 to 10,000 words",
            editor.error,
            "Enter: save     Esc: cancel"
        };
        for (int r = 0; r < bodyRows; r++) {
            move(screenRow(ROW_BODY + r), boxCol + 1);
            out.print(" ".repeat(INNER_WIDTH));
            String line = r < lines.length ? lines[r] : "";
            if (!line.isEmpty()) {
                move(screenRow(ROW_BODY + r), boxCol + CONTENT_OFFSET);
                if (r == 2) out.print("\033[31m" + line + "\033[0m");
                else out.print(line);
            }
            prevBodyLen[r] = line.length();
        }
    }

    private void drawFooter() {
        String f;
        if (state == State.MENU && editor.editing) {
            f = EDIT_FOOT;
        } else if (state == State.MENU) {
            f = MENU_FOOT;
        } else if (state == State.PLAYING) {
            f = PLAY_FOOT;
        } else {
            f = RESULTS_FOOT;
        }
        if (f.length() > BOX_WIDTH) f = f.substring(0, BOX_WIDTH);
        int row = screenRow(rowFoot());
        move(row, 1);
        out.print("\033[2K");
        int col = boxCol + Math.max(0, (BOX_WIDTH - f.length()) / 2);
        move(row, col);
        out.print(f);
    }

    private String statCell(String label, String value) {
        return String.format(Locale.ROOT, "%-10s%8s", label, value);
    }

    private void drawResults() {
        out.print("\033[?25l");
        String[] lines = {
            statCell("Time", String.format(Locale.ROOT, "%.1fs", resultSeconds()))
                    + "    " + statCell("WPM", String.format(Locale.ROOT, "%.1f", wpm())),
            statCell("Accuracy", String.format(Locale.ROOT, "%.1f%%", accuracy()))
                    + "    " + " ".repeat(18),
            statCell("Correct", Integer.toString(correct))
                    + "    " + statCell("Incorrect", Integer.toString(incorrect)),
            statCell("Missed", Integer.toString(missed))
                    + "    " + statCell("Extra", Integer.toString(extra))
        };
        int blockWidth = 0;
        for (String s : lines) blockWidth = Math.max(blockWidth, s.length());
        blockWidth = Math.min(blockWidth, WORD_WRAP);
        int pad = Math.max(0, (WORD_WRAP - blockWidth) / 2);
        for (int r = 0; r < bodyRows; r++) {
            move(screenRow(ROW_BODY + r), boxCol + 1);
            out.print(" ".repeat(INNER_WIDTH));
            if (r < lines.length) {
                String s = lines[r];
                if (s.length() > WORD_WRAP) s = s.substring(0, WORD_WRAP);
                String line = " ".repeat(pad) + s;
                if (line.length() < WORD_WRAP) {
                    line += " ".repeat(WORD_WRAP - line.length());
                }
                move(screenRow(ROW_BODY + r), boxCol + CONTENT_OFFSET);
                out.print(line);
            }
            prevBodyLen[r] = WORD_WRAP;
        }
        drawFooter();
    }

    private void ensureLayoutCapacity(int size) {
        if (wordLine.length >= size) return;
        int length = Math.max(size, wordLine.length * 2 + 16);
        wordLine = Arrays.copyOf(wordLine, length);
        wordCol = Arrays.copyOf(wordCol, length);
    }

    // Wrap words and record cursor positions.
    private void computeLayout() {
        ensureLayoutCapacity(picked.size());
        bodyLines.clear();
        bodyWidths.clear();

        int line = 0;
        int lineW = 0;
        for (int i = 0; i < picked.size(); i++) {
            String target = picked.get(i);
            String typed = typedWords.get(i);
            int wlen = Math.max(target.length(), typed.length());
            if (lineW > 0 && lineW + 1 + wlen > WORD_WRAP) {
                line++;
                lineW = 0;
            }
            if (lineW > 0) lineW++;
            wordLine[i] = line;
            wordCol[i] = lineW;
            lineW += wlen;
            lastLine = line;
            lastLineWidth = lineW;
        }
        totalLines = line + 1;

        if (cursor < picked.size()) {
            cursorLine = wordLine[cursor];
            firstLine = cursorLine > 0 ? cursorLine - 1 : 0;
        } else {
            cursorLine = totalLines - 1;
            firstLine = Math.max(0, totalLines - bodyRows);
        }
        int maxFirst = Math.max(0, totalLines - bodyRows);
        firstLine = Math.max(0, Math.min(firstLine, maxFirst));

        renderLines();
    }

    private void renderLines() {
        int line = 0;
        int lineW = 0;
        StringBuilder sb = new StringBuilder();

        for (int i = 0; i < picked.size(); i++) {
            String target = picked.get(i);
            String typed = typedWords.get(i);
            int wlen = Math.max(target.length(), typed.length());
            if (lineW > 0 && lineW + 1 + wlen > WORD_WRAP) {
                if (line >= firstLine && line < firstLine + bodyRows) {
                    bodyLines.add(sb.toString());
                    bodyWidths.add(lineW);
                }
                line++;
                lineW = 0;
                sb.setLength(0);
            }
            if (line >= firstLine + bodyRows) break;

            if (lineW > 0) {
                lineW++;
                if (line >= firstLine) sb.append(' ');
            }
            if (line >= firstLine) appendWord(sb, i, wlen);
            lineW += wlen;
        }

        if (line >= firstLine && line < firstLine + bodyRows && lineW > 0) {
            bodyLines.add(sb.toString());
            bodyWidths.add(lineW);
        }
        while (bodyLines.size() < bodyRows) {
            bodyLines.add("");
            bodyWidths.add(0);
        }
    }

    private void appendWord(StringBuilder sb, int index, int wlen) {
        String target = picked.get(index);
        String typed = typedWords.get(index);
        for (int j = 0; j < wlen; j++) {
            if (j >= target.length()) { // Extra character.
                sb.append("\033[31m").append(typed.charAt(j)).append("\033[0m");
            } else if (j < typed.length()) {
                boolean ok = target.charAt(j) == typed.charAt(j);
                sb.append(ok ? "\033[32m" : "\033[31m")
                    .append(target.charAt(j)).append("\033[0m");
            } else { // Skipped (red) or untyped (white).
                sb.append(index < cursor ? "\033[31m" : "\033[37m")
                    .append(target.charAt(j)).append("\033[0m");
            }
        }
    }

    private void positionCursor() {
        if (state != State.PLAYING) return;
        int line;
        int col;
        if (cursor < picked.size()) {
            line = wordLine[cursor];
            col = wordCol[cursor] + typedWords.get(cursor).length();
        } else { // Finished: past last word.
            line = lastLine;
            col = lastLineWidth;
        }
        int screenLine = line - firstLine;
        if (screenLine < 0) screenLine = 0;
        if (screenLine >= bodyRows) screenLine = bodyRows - 1;
        move(screenRow(ROW_BODY + screenLine), boxCol + CONTENT_OFFSET + col);
    }

    private void loop() throws Exception {
        while (running) {
            updateSize();

            int key = readKey();
            if (key == -1) break; // Input closed.

            boolean dirty = key != NonBlockingReader.READ_EXPIRED;
            if (dirty) {
                if (key == 3) { // Ctrl+C.
                    running = false;
                    break;
                }
                if (waitingForSize) {
                    if (key == KEY_ESC && state == State.PLAYING) {
                        abortGame();
                    }
                    continue;
                }
                handleKey(key);
            }
            if (waitingForSize) continue;

            if (state == State.PLAYING) topUpWords();

            if (state == State.PLAYING && config.mode == Config.Mode.WORDS) {
                int lines = wrappedLineCount();
                if (lines > bodyRows) {
                    setBodyRows(lines);
                    relayout();
                    if (waitingForSize) continue;
                }
            }

            if (dirty) {
                if (headerDirty) {
                    drawHeaderCells();
                    headerDirty = false;
                }
                drawBody();
                drawFooter();
            }
            drawCounter();
            positionCursor();
            out.flush();

            if (state == State.PLAYING && isOver()) {
                endGame();
            }
        }
    }

    // Read one key; distinguish arrows from bare Esc.
    private int readKey() throws IOException {
        int key = reader.read(TICK_MS);
        if (key != 27) return key; // -2 tick, -1 EOF, or normal key.

        int b1 = reader.read(ESCAPE_MS);
        if (b1 == NonBlockingReader.READ_EXPIRED || b1 == -1) return KEY_ESC;
        if (b1 == 'O') {
            int b2 = reader.read(ESCAPE_MS);
            if (b2 == 'D') return KEY_LEFT;
            if (b2 == 'C') return KEY_RIGHT;
            return KEY_IGNORE;
        }
        if (b1 != '[') return KEY_IGNORE;
        int b2 = reader.read(ESCAPE_MS);
        if (b2 == 'D') return KEY_LEFT;
        if (b2 == 'C') return KEY_RIGHT;
        // Drain unknown escape sequence.
        while (b2 != NonBlockingReader.READ_EXPIRED && b2 != -1
                && !(b2 >= 0x40 && b2 <= 0x7E)) {
            b2 = reader.read(ESCAPE_MS);
        }
        return KEY_IGNORE;
    }

    private void handleKey(int key) {
        if (key == 3) { // Ctrl+C.
            running = false;
            return;
        }

        if (state == State.MENU) {
            handleMenuKey(key);
        } else if (state == State.PLAYING) {
            handlePlayKey(key);
        } else if (key == KEY_ESC) {
            returnToMenu();
        }
    }

    private void handleMenuKey(int key) {
        if (editor.editing) {
            if (key == KEY_ESC) {
                cancelEditor();
                return;
            }
            if ((key >= '0' && key <= '9') || key == 127 || key == 8
                    || key == 13 || key == 10) {
                if (config.handleEditorKey(key, editor)) {
                    sel = config.presetIndex();
                    headerDirty = true;
                }
            }
            return;
        }

        if (key == KEY_ESC) {
            running = false;
            return;
        }
        if (key == KEY_LEFT) {
            moveSel(-1);
            return;
        }
        if (key == KEY_RIGHT) {
            moveSel(1);
            return;
        }
        if (key >= '1' && key <= '4') {
            config.selectDifficulty(key - '1');
            refreshPreview();
            headerDirty = true;
            return;
        }
        if (key == 13 || key == 10) {
            activateMenuItem();
        }
    }

    private void handlePlayKey(int key) {
        if (key == KEY_ESC) {
            abortGame();
            return;
        }
        if (key == KEY_LEFT || key == KEY_RIGHT || key == KEY_IGNORE) return;

        boolean alnum = (key >= '0' && key <= '9')
                || (key >= 'A' && key <= 'Z')
                || (key >= 'a' && key <= 'z');
        if (startNanos == 0) {
            if (!alnum) return; // Start timer on first letter/digit.
            startNanos = System.nanoTime();
        }
        if (cursor >= picked.size()) return;

        if (key == ' ') {
            pressSpace();
        } else if (key == 127 || key == 8) {
            backspace();
        } else if (key >= 32 && key <= 126) {
            typeChar((char) key);
        }
    }

    private void activateMenuItem() {
        int start = startIndex();
        if (sel < 2) {
            config.selectMode(sel);
            if (sel >= menuItemCount()) sel = menuItemCount() - 1;
            headerDirty = true;
        } else if (sel < 6) {
            config.selectDifficulty(sel - 2);
            refreshPreview();
            headerDirty = true;
        } else if (sel < start) {
            int preset = sel - 6;
            if (config.mode == Config.Mode.WORDS && preset == 4) {
                config.openEditor(editor);
                headerDirty = true;
            } else {
                config.selectPreset(preset);
                headerDirty = true;
            }
        } else {
            startGame();
        }
    }

    private void cancelEditor() {
        editor.editing = false;
        editor.input = "";
        editor.error = "";
        headerDirty = true;
    }

    private void moveSel(int dir) {
        int count = menuItemCount();
        sel = (sel + dir + count) % count;
        headerDirty = true;
    }

    private void startGame() {
        state = State.PLAYING;
        editor.editing = false;
        editor.input = "";
        editor.error = "";
        correct = 0;
        incorrect = 0;
        missed = 0;
        extra = 0;
        cursor = 0;
        startNanos = 0;
        endNanos = 0;
        completed = false;

        int count = config.mode == Config.Mode.WORDS
                ? config.wordCount
                : (config.difficulty == 0 ? 80 : 70);
        picked = pickWords(count, null);
        resetTyped();
        if (config.mode == Config.Mode.WORDS) {
            setBodyRows(Math.max(BASE_BODY_ROWS, wrappedLineCount()));
        } else {
            setBodyRows(BASE_BODY_ROWS);
        }
        headerDirty = true;
        out.print("\033[?25h"); // Show cursor.
        relayout();
    }

    private void abortGame() {
        startNanos = 0;
        endNanos = 0;
        completed = false;
        timerPaused = false;
        pauseStartNanos = 0;
        returnToMenu();
    }

    private void returnToMenu() {
        state = State.MENU;
        setBodyRows(BASE_BODY_ROWS);
        cancelEditor();
        refreshPreview();
        out.print("\033[?25l");
        relayout();
    }

    private void refreshPreview() {
        picked = pickWords(PREVIEW_COUNT, null);
        resetTyped();
    }

    private void resetTyped() {
        typedWords.clear();
        for (int i = 0; i < picked.size(); i++) typedWords.add("");
        cursor = 0;
    }

    private void topUpWords() {
        if (config.mode != Config.Mode.TIME) return;
        int ahead = config.difficulty == 0 ? LOOKAHEAD + 20 : LOOKAHEAD;
        if (picked.size() - cursor >= ahead) return;

        String previous = picked.isEmpty() ? null : picked.get(picked.size() - 1);
        List<String> more = pickWords(LOOKAHEAD, previous);
        picked.addAll(more);
        for (int i = 0; i < more.size(); i++) typedWords.add("");
    }

    private boolean isOver() {
        if (state != State.PLAYING) return false;
        if (config.mode == Config.Mode.TIME) {
            return startNanos > 0 && timeLeft() <= 0;
        }
        if (cursor >= picked.size()) return true;
        int last = picked.size() - 1;
        return cursor == last && typedWords.get(last).equals(picked.get(last));
    }

    private void endGame() {
        if (config.mode == Config.Mode.TIME) countCurrentWordAsMissed();
        completed = true;
        endNanos = System.nanoTime();
        state = State.RESULTS;
        setBodyRows(BASE_BODY_ROWS);
        relayout();
    }

    private void pressSpace() {
        String target = picked.get(cursor);
        String typed = typedWords.get(cursor);
        if (typed.length() < target.length()) {
            missed++;
            typedWords.set(cursor, typed + SKIP);
        } else {
            cursor++;
        }
    }

    private void countCurrentWordAsMissed() {
        if (cursor >= picked.size()) return;
        String target = picked.get(cursor);
        String typed = typedWords.get(cursor);
        missed += Math.max(0, target.length() - typed.length());
    }

    private void typeChar(char c) {
        String target = picked.get(cursor);
        String typed = typedWords.get(cursor);
        if (typed.length() >= target.length() + MAX_EXTRA) return;

        if (typed.length() >= target.length()) {
            extra++;
        } else if (target.charAt(typed.length()) == c) {
            correct++;
        } else {
            incorrect++;
        }
        typedWords.set(cursor, typed + c);
    }

    private void backspace() {
        String target = picked.get(cursor);
        String typed = typedWords.get(cursor);

        if (!typed.isEmpty()) {
            int i = typed.length() - 1;
            char last = typed.charAt(i);
            if (last == SKIP) {
                missed--;
            } else if (i >= target.length()) {
                extra--;
            } else if (target.charAt(i) == last) {
                correct--;
            } else {
                incorrect--;
            }
            typedWords.set(cursor, typed.substring(0, i));
            return;
        }

        if (cursor > 0) {
            // Don't cross a fully correct word.
            if (typedWords.get(cursor - 1).equals(picked.get(cursor - 1))) return;
            cursor--;
        }
    }

    private double timeLeft() {
        if (startNanos == 0) return config.timeLimit;
        double elapsed = (System.nanoTime() - startNanos) / 1_000_000_000.0;
        return Math.max(0.0, config.timeLimit - elapsed);
    }

    private List<String> pickWords(int count, String previousWord) {
        List<String> pool = WordBank.getWordList(wordBank, WordBank.KEYS[config.difficulty]);
        List<String> chosen = new ArrayList<>(count);
        int previous = previousWord == null ? -1 : pool.indexOf(previousWord);
        if (previous < 0) previous = -1;

        for (int i = 0; i < count; i++) {
            boolean hasPrevious = previous >= 0;
            int index = rng.nextInt(pool.size() - (hasPrevious ? 1 : 0));
            if (hasPrevious && index >= previous) {
                index++;
            }
            chosen.add(pool.get(index));
            previous = index;
        }
        return chosen;
    }

    private void cleanup() throws IOException {
        out.print("\033[0m"); // Reset colors.
        out.print("\033[0 q"); // Reset cursor style.
        out.print("\033[?25h"); // Show cursor.
        out.print("\033[?1049l"); // Leave alternate screen.
        out.flush();
        terminal.setAttributes(original);
        terminal.close();
    }

    private double elapsedSeconds() {
        if (startNanos == 0) return 0.0;
        long end = endNanos == 0 ? System.nanoTime() : endNanos;
        return (end - startNanos) / 1_000_000_000.0;
    }

    private double resultSeconds() {
        double elapsed = elapsedSeconds();
        if (config.mode == Config.Mode.TIME) {
            return Math.min(elapsed, config.timeLimit);
        }
        return elapsed;
    }

    private double wpm() {
        double elapsed = resultSeconds();
        if (elapsed <= 0) return 0.0;
        return (correct / 5.0) / (elapsed / 60.0);
    }

    private double accuracy() {
        int typedCount = correct + incorrect + extra;
        return typedCount == 0 ? 0.0 : 100.0 * correct / typedCount;
    }

    private void printSummary() {
        if (startNanos == 0) {
            System.out.println("Game not started.");
            return;
        }
        double elapsed = resultSeconds();

        System.out.println("Time: " + String.format(Locale.ROOT, "%.1f", elapsed) + "s");
        System.out.println("Correct: " + correct + "   Incorrect: " + incorrect
                + "   Missed: " + missed + "   Extra: " + extra);
        System.out.println(String.format(Locale.ROOT,
                "WPM: %.1f   Accuracy: %.1f%%", wpm(), accuracy()));
    }
}
