import java.io.IOException;
import java.math.BigInteger;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.LinkedHashMap;
import java.util.Map;

final class HistoryStore {
    static final class Best {
        final int wpm;
        final double accuracy;

        Best(int wpm, double accuracy) {
            this.wpm = wpm;
            this.accuracy = accuracy;
        }
    }

    private static final class Record {
        int wpm;
        double accuracy;
    }

    private final Path path;
    private final Map<String, Record> records = new LinkedHashMap<>();
    private int attempts;

    private HistoryStore(Path path) {
        this.path = path;
    }

    static HistoryStore load(Path path) throws IOException {
        HistoryStore store = new HistoryStore(path);
        if (Files.notExists(path)) return store;

        final Object document;
        try {
            document = Json.parse(Files.readString(path, StandardCharsets.UTF_8));
        } catch (IllegalArgumentException e) {
            throw new IllegalArgumentException(
                    "Malformed history file: " + e.getMessage(), e);
        } catch (IOException e) {
            throw new IOException("Cannot read history file: " + path, e);
        }

        Map<?, ?> root = asObject(document, "History must be a JSON object");
        if (root.containsKey("attempts")) {
            store.attempts = asInteger(root.get("attempts"), "attempts");
        }
        if (store.attempts <= 0) return store;

        Object rawRecords = root.get("records");
        if (rawRecords instanceof Map<?, ?>) {
            Map<?, ?> history = (Map<?, ?>) rawRecords;
            for (Map.Entry<?, ?> entry : history.entrySet()) {
                if (!(entry.getKey() instanceof String)
                        || !(entry.getValue() instanceof Map<?, ?>)) {
                    continue;
                }
                Map<?, ?> values = (Map<?, ?>) entry.getValue();
                Record record = new Record();
                if (values.containsKey("wpm")) {
                    record.wpm = asInteger(values.get("wpm"), "wpm");
                }
                if (values.containsKey("accuracy")) {
                    record.accuracy = asNumber(values.get("accuracy"), "accuracy");
                }
                store.records.put((String) entry.getKey(), record);
            }
        }
        return store;
    }

    Best bestFor(Config config) {
        Record record = records.get(keyFor(config));
        return record == null ? new Best(0, 0.0)
                : new Best(record.wpm, record.accuracy);
    }

    int attempts() {
        return attempts;
    }

    void recordResult(Config config, double wpm, double accuracy) throws IOException {
        attempts++;
        Record record = records.computeIfAbsent(keyFor(config), key -> new Record());
        record.wpm = Math.max(record.wpm, (int) Math.round(wpm));
        record.accuracy = Math.max(record.accuracy, accuracy);
        save();
    }

    private String keyFor(Config config) {
        String key = WordBank.KEYS[config.difficulty];
        if (config.mode == Config.Mode.TIME) {
            return key + "/time/" + config.timeLimit;
        }
        return key + "/words/" + config.wordCount;
    }

    private void save() throws IOException {
        StringBuilder json = new StringBuilder();
        json.append("{\n  \"attempts\": ").append(attempts)
                .append(",\n  \"records\": {");
        boolean first = true;
        for (Map.Entry<String, Record> entry : records.entrySet()) {
            if (!first) json.append(',');
            json.append("\n    \"").append(entry.getKey()).append("\": { \"wpm\": ")
                    .append(entry.getValue().wpm).append(", \"accuracy\": ")
                    .append(Double.toString(entry.getValue().accuracy)).append(" }");
            first = false;
        }
        json.append("\n  }\n}\n");

        try {
            Files.writeString(path, json.toString(), StandardCharsets.UTF_8);
        } catch (IOException e) {
            throw new IOException("Failed writing history file: " + path, e);
        }
    }

    private static Map<?, ?> asObject(Object value, String message) {
        if (!(value instanceof Map<?, ?>)) {
            throw new IllegalArgumentException(message);
        }
        return (Map<?, ?>) value;
    }

    private static int asInteger(Object value, String field) {
        if (!(value instanceof BigInteger)) {
            throw new IllegalArgumentException("Invalid history " + field);
        }
        try {
            return ((BigInteger) value).intValueExact();
        } catch (ArithmeticException e) {
            throw new IllegalArgumentException("Invalid history " + field, e);
        }
    }

    private static double asNumber(Object value, String field) {
        if (!(value instanceof Number)) {
            throw new IllegalArgumentException("Invalid history " + field);
        }
        double number = ((Number) value).doubleValue();
        if (!Double.isFinite(number)) {
            throw new IllegalArgumentException("Invalid history " + field);
        }
        return number;
    }
}
