import java.math.BigInteger;
import java.util.ArrayList;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;

// Minimal JSON parser for the shared word bank. Returns Map, List, String,
// BigInteger (integral numbers), Double, Boolean, or null.
final class Json {
    private final String text;
    private int pos;

    private Json(String text) {
        this.text = text;
    }

    static Object parse(String text) {
        Json parser = new Json(text);
        parser.skipWhitespace();
        Object value = parser.parseValue();
        parser.skipWhitespace();
        if (parser.pos != text.length()) {
            throw parser.error("Unexpected trailing content");
        }
        return value;
    }

    private Object parseValue() {
        if (pos >= text.length()) throw error("Unexpected end of input");
        char c = text.charAt(pos);
        switch (c) {
            case '{':
                return parseObject();
            case '[':
                return parseArray();
            case '"':
                return parseString();
            case 't':
                expectLiteral("true");
                return Boolean.TRUE;
            case 'f':
                expectLiteral("false");
                return Boolean.FALSE;
            case 'n':
                expectLiteral("null");
                return null;
            default:
                if (c == '-' || (c >= '0' && c <= '9')) return parseNumber();
                throw error("Unexpected character '" + c + "'");
        }
    }

    private Map<String, Object> parseObject() {
        Map<String, Object> result = new LinkedHashMap<>();
        expect('{');
        skipWhitespace();
        if (peek() == '}') {
            pos++;
            return result;
        }
        while (true) {
            skipWhitespace();
            if (peek() != '"') throw error("Expected object key string");
            String key = parseString();
            skipWhitespace();
            expect(':');
            skipWhitespace();
            result.put(key, parseValue());
            skipWhitespace();
            char c = peek();
            if (c == ',') {
                pos++;
                continue;
            }
            if (c == '}') {
                pos++;
                return result;
            }
            throw error("Expected ',' or '}'");
        }
    }

    private List<Object> parseArray() {
        List<Object> result = new ArrayList<>();
        expect('[');
        skipWhitespace();
        if (peek() == ']') {
            pos++;
            return result;
        }
        while (true) {
            skipWhitespace();
            result.add(parseValue());
            skipWhitespace();
            char c = peek();
            if (c == ',') {
                pos++;
                continue;
            }
            if (c == ']') {
                pos++;
                return result;
            }
            throw error("Expected ',' or ']'");
        }
    }

    private String parseString() {
        expect('"');
        StringBuilder sb = new StringBuilder();
        while (true) {
            if (pos >= text.length()) throw error("Unterminated string");
            char c = text.charAt(pos++);
            if (c == '"') return sb.toString();
            if (c == '\\') {
                if (pos >= text.length()) throw error("Unterminated escape");
                char escape = text.charAt(pos++);
                switch (escape) {
                    case '"':
                        sb.append('"');
                        break;
                    case '\\':
                        sb.append('\\');
                        break;
                    case '/':
                        sb.append('/');
                        break;
                    case 'b':
                        sb.append('\b');
                        break;
                    case 'f':
                        sb.append('\f');
                        break;
                    case 'n':
                        sb.append('\n');
                        break;
                    case 'r':
                        sb.append('\r');
                        break;
                    case 't':
                        sb.append('\t');
                        break;
                    case 'u':
                        sb.append(parseUnicodeEscape());
                        break;
                    default:
                        throw error("Invalid escape '\\" + escape + "'");
                }
            } else {
                if (c < 0x20) throw error("Unescaped control character in string");
                sb.append(c);
            }
        }
    }

    private char parseUnicodeEscape() {
        if (pos + 4 > text.length()) throw error("Incomplete unicode escape");
        int value = 0;
        for (int i = 0; i < 4; i++) {
            char c = text.charAt(pos++);
            int digit = Character.digit(c, 16);
            if (digit < 0) throw error("Invalid unicode escape");
            value = value * 16 + digit;
        }
        return (char) value;
    }

    private Object parseNumber() {
        int start = pos;
        if (peek() == '-') pos++;
        if (pos >= text.length()) throw error("Incomplete number");
        if (peek() == '0') {
            pos++;
        } else if (peek() >= '1' && peek() <= '9') {
            while (pos < text.length() && isDigit(text.charAt(pos))) pos++;
        } else {
            throw error("Invalid number");
        }

        boolean floating = false;
        if (pos < text.length() && text.charAt(pos) == '.') {
            floating = true;
            pos++;
            if (pos >= text.length() || !isDigit(text.charAt(pos))) {
                throw error("Invalid fraction");
            }
            while (pos < text.length() && isDigit(text.charAt(pos))) pos++;
        }
        if (pos < text.length() && (text.charAt(pos) == 'e' || text.charAt(pos) == 'E')) {
            floating = true;
            pos++;
            if (pos < text.length() && (text.charAt(pos) == '+' || text.charAt(pos) == '-')) {
                pos++;
            }
            if (pos >= text.length() || !isDigit(text.charAt(pos))) {
                throw error("Invalid exponent");
            }
            while (pos < text.length() && isDigit(text.charAt(pos))) pos++;
        }

        String token = text.substring(start, pos);
        return floating ? (Object) Double.valueOf(token) : (Object) new BigInteger(token);
    }

    private void expectLiteral(String literal) {
        if (!text.startsWith(literal, pos)) throw error("Expected '" + literal + "'");
        pos += literal.length();
    }

    private void expect(char expected) {
        if (pos >= text.length() || text.charAt(pos) != expected) {
            throw error("Expected '" + expected + "'");
        }
        pos++;
    }

    private char peek() {
        if (pos >= text.length()) throw error("Unexpected end of input");
        return text.charAt(pos);
    }

    private void skipWhitespace() {
        while (pos < text.length()) {
            char c = text.charAt(pos);
            if (c == ' ' || c == '\t' || c == '\n' || c == '\r') {
                pos++;
            } else {
                break;
            }
        }
    }

    private static boolean isDigit(char c) {
        return c >= '0' && c <= '9';
    }

    private IllegalArgumentException error(String message) {
        return new IllegalArgumentException(message + " at position " + pos);
    }
}
