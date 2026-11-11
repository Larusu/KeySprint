final class Config {
    enum Mode { TIME, WORDS }

    static final int[] TIME_OPTIONS = {15, 30, 60, 120};
    static final int[] WORD_OPTIONS = {10, 25, 50, 75};
    static final int MAX_WORD_COUNT = 10000;

    Mode mode = Mode.TIME;
    int difficulty = 0;
    int timeLimit = 30;
    int wordCount = 25;

    static final class Editor {
        boolean editing;
        String input = "";
        String error = "";
    }

    void selectMode(int index) {
        mode = Mode.values()[index];
    }

    void selectDifficulty(int index) {
        difficulty = index;
    }

    void selectPreset(int index) {
        if (mode == Mode.TIME) {
            timeLimit = TIME_OPTIONS[index];
        } else {
            wordCount = WORD_OPTIONS[index];
        }
    }

    boolean isWordPreset(int value) {
        for (int option : WORD_OPTIONS) {
            if (option == value) return true;
        }
        return false;
    }

    int presetIndex() {
        if (mode == Mode.TIME) {
            for (int i = 0; i < TIME_OPTIONS.length; i++) {
                if (TIME_OPTIONS[i] == timeLimit) return i;
            }
            return 0;
        }
        for (int i = 0; i < WORD_OPTIONS.length; i++) {
            if (WORD_OPTIONS[i] == wordCount) return i;
        }
        return WORD_OPTIONS.length;
    }

    String[] presetLabels() {
        if (mode == Mode.TIME) {
            String[] labels = new String[TIME_OPTIONS.length];
            for (int i = 0; i < TIME_OPTIONS.length; i++) {
                labels[i] = Integer.toString(TIME_OPTIONS[i]);
            }
            return labels;
        }
        String[] labels = new String[WORD_OPTIONS.length + 1];
        for (int i = 0; i < WORD_OPTIONS.length; i++) {
            labels[i] = Integer.toString(WORD_OPTIONS[i]);
        }
        labels[WORD_OPTIONS.length] =
                isWordPreset(wordCount) ? "custom" : Integer.toString(wordCount);
        return labels;
    }

    void openEditor(Editor editor) {
        if (mode != Mode.WORDS) return;
        editor.editing = true;
        editor.input = "";
        editor.error = "";
    }

    boolean handleEditorKey(int key, Editor editor) {
        if (key >= '0' && key <= '9') {
            if (editor.input.length() < 20) editor.input += (char) key;
            return false;
        }
        if (key == 127 || key == 8) {
            if (!editor.input.isEmpty()) {
                editor.input = editor.input.substring(0, editor.input.length() - 1);
            }
            return false;
        }
        if (key == 13 || key == 10) {
            return commitWordCount(editor);
        }
        return false;
    }

    boolean commitWordCount(Editor editor) {
        if (editor.input.isEmpty()) {
            editor.error = "Word count cannot be empty";
            return false;
        }

        int value = 0;
        for (int i = 0; i < editor.input.length(); i++) {
            char ch = editor.input.charAt(i);
            if (ch < '0' || ch > '9') {
                editor.error = "Word count must contain digits only";
                return false;
            }
            int digit = ch - '0';
            if (value > (MAX_WORD_COUNT - digit) / 10) {
                editor.error = "Word count must be between 1 and 10,000";
                return false;
            }
            value = value * 10 + digit;
        }
        if (value == 0) {
            editor.error = "Word count must be between 1 and 10,000";
            return false;
        }

        wordCount = value;
        editor.editing = false;
        editor.input = "";
        editor.error = "";
        return true;
    }
}
