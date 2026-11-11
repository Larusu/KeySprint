import java.io.IOException;
import java.math.BigInteger;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.ArrayList;
import java.util.LinkedHashMap;
import java.util.LinkedHashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;

final class WordBank {
    static final String[] KEYS = {
        "english_1k", "english_5k", "english_10k", "english_25k"
    };
    static final String[] LABELS = {"1k", "5k", "10k", "25k"};

    private WordBank() {
    }

    static Map<String, List<String>> load(String path) throws IOException {
        final String text;
        try {
            text = Files.readString(Path.of(path), StandardCharsets.UTF_8);
        } catch (IOException e) {
            throw new IOException("Cannot open word bank: " + path, e);
        }

        final Object document;
        try {
            document = Json.parse(text);
        } catch (IllegalArgumentException e) {
            throw new IllegalArgumentException("Malformed word bank: " + e.getMessage(), e);
        }

        Map<String, Object> root = asObject(document, "Word bank must be a JSON object");

        Object version = root.get("schema_version");
        if (!(version instanceof BigInteger) || !version.equals(BigInteger.ONE)) {
            throw new IllegalArgumentException("Unsupported word-bank schema version");
        }
        if (!"en".equals(root.get("language"))) {
            throw new IllegalArgumentException("Expected an English word bank");
        }

        Map<String, Object> levels = asObject(
                root.get("levels"), "Expected the four English vocabulary pools");
        if (levels.size() != 4) {
            throw new IllegalArgumentException("Expected the four English vocabulary pools");
        }

        Map<String, Object> metadata = asObject(
                root.get("metadata"), "Missing word-bank metadata");
        Map<String, Object> counts = asObject(
                metadata.get("actual_counts"), "Missing word-bank counts");

        Map<String, List<String>> bank = new LinkedHashMap<>();
        for (String key : KEYS) {
            List<Object> raw = asArray(
                    levels.get(key), "Expected an array of words: " + key);
            if (raw.size() < 2) {
                throw new IllegalArgumentException(
                        "Word pool needs at least two words: " + key);
            }

            BigInteger count = asNonNegativeInteger(
                    counts.get(key), "Expected a non-negative integer count: " + key);
            if (!count.equals(BigInteger.valueOf(raw.size()))) {
                throw new IllegalArgumentException("Incorrect word count: " + key);
            }

            Set<String> unique = new LinkedHashSet<>();
            List<String> words = new ArrayList<>(raw.size());
            for (Object item : raw) {
                if (!(item instanceof String)) {
                    throw new IllegalArgumentException(
                            "Invalid or duplicate word in " + key);
                }
                String word = (String) item;
                if (!isLettersOnly(word) || !unique.add(word)) {
                    throw new IllegalArgumentException(
                            "Invalid or duplicate word in " + key);
                }
                words.add(word);
            }
            bank.put(key, words);
        }
        return bank;
    }

    static List<String> getWordList(Map<String, List<String>> bank, String level) {
        List<String> words = bank.get(level);
        if (words == null) {
            throw new IllegalArgumentException("Unknown word-bank level: " + level);
        }
        return words;
    }

    private static boolean isLettersOnly(String word) {
        if (word.length() < 3) return false;
        for (int i = 0; i < word.length(); i++) {
            char c = word.charAt(i);
            if (c < 'a' || c > 'z') return false;
        }
        return true;
    }

    @SuppressWarnings("unchecked")
    private static Map<String, Object> asObject(Object value, String message) {
        if (!(value instanceof Map)) throw new IllegalArgumentException(message);
        return (Map<String, Object>) value;
    }

    @SuppressWarnings("unchecked")
    private static List<Object> asArray(Object value, String message) {
        if (!(value instanceof List)) throw new IllegalArgumentException(message);
        return (List<Object>) value;
    }

    private static BigInteger asNonNegativeInteger(Object value, String message) {
        if (!(value instanceof BigInteger)) throw new IllegalArgumentException(message);
        BigInteger number = (BigInteger) value;
        if (number.signum() < 0) throw new IllegalArgumentException(message);
        return number;
    }
}
