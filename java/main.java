import java.io.PrintWriter;
import java.util.List;
import org.jline.terminal.Attributes;
import org.jline.terminal.Terminal;
import org.jline.terminal.TerminalBuilder;
import org.jline.utils.NonBlockingReader;

public class main {
    public static void main(String[] args) throws Exception {
        Terminal terminal = TerminalBuilder.builder().system(true).build();
        Attributes original = terminal.enterRawMode();
        NonBlockingReader reader = terminal.reader();
        PrintWriter out = terminal.writer();
        StringBuilder typed = new StringBuilder();

        List<String> words = List.of("Jhervis", "Nicole", "Jennica", "Jenoel", "Kate", "Gian", "Jack", "Jake");
        String text = String.join(" ", words);
        
        int correct = 0;
        int mistakes = 0;

        try { 
            out.print("\033[0m");               // reset color for assurance haha
            out.print("\033[3J\033[2J\033[H");  // clear terminal for redraw
            out.print("\033[3;1H");             // go to row 3, column 1
            out.print(text);             
            out.print("\033[5;1H");             // go to row 3, column 1
            out.print("Correct: " + correct);
            out.print("\033[6;1H");             // go to row 4, column 1
            out.print("Mistakes: " + mistakes);
            out.print("\033[3;1H"); 
            out.print("\033[6 q");

            int textIndex = 0;
            int typedIndex = 0;

            while (true) {

                int tinype = reader.read();
                typed.append((char) tinype);

                if (tinype >= 32 && tinype <= 127) {
                    if (typed.charAt(textIndex) == 32) {
                        textIndex++;
                        typedIndex++;
                    }
                    else if (typed.charAt(typedIndex) == text.charAt(textIndex)) {
                        out.print("\033[32m" + (char) text.charAt(textIndex));
                        correct++;
                        textIndex++;
                        out.print("\033[0m");
                    } else {
                        out.print("\033[31m" + (char) text.charAt(textIndex));
                        mistakes++;
                        textIndex++;
                        out.print("\033[0m");
                    }

                } else if (tinype == 127 || tinype == 8) {
                    typedIndex--;
                    out.print("\033[1D" + "\033[37m" + (char) text.charAt(textIndex) + "\033[1C");
                } 

                if (tinype == 113) {
                    break;
                }

                out.print("\033[5;1H");                                 // go back to row 3, column 1
                out.print("\033[2K");                                   // clear the line
                out.print("Correct: " + correct);
                out.print("\033[6;1H");                                 // go to row 4, column 1
                out.print("\033[2K");
                out.print("Mistakes: " + mistakes);
                out.print("\033[3;" + (textIndex + 1) + 'H');           // go to the text line 
                out.print("\033[?6 q");                                 // show cursor
            }

        } finally {
            terminal.setAttributes(original);
            terminal.close();
        }
    }

        // out.print("\033[?25h");   // show cursor
        // out.print("\033[?25l");   // hide cursor
}
    
    
