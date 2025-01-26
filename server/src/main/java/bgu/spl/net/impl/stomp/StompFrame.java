package bgu.spl.net.impl.stomp;

import java.util.HashMap;
import java.util.Map;

public class StompFrame {
     private String command;
    private Map<String, String> headers;
    private String body;

    public StompFrame(String command, Map<String, String> headers, String body) {
        this.command = command;
        this.headers = headers;
        this.body = body;
    }

    public String getCommand() {
        return command;
    }

    public String getHeader(String key) {
        return headers.get(key);
    }

    public String getBody() {
        return body;
    }

    public static StompFrame parse(String frame) {
      
        String[] lines = frame.split("\n"); // Split the frame into lines
        String command = lines[0];            // First line is the command
        Map<String, String> headers = new HashMap<>();
        StringBuilder body = new StringBuilder();
        boolean inBody = false;
        
        for (int i = 1; i < lines.length; i++) {
            String line = lines[i];
            if (!inBody) {
                if (line.isEmpty()) {
                    // Blank line indicates the start of the body
                    inBody = true;
                } else {
                    String[] headerParts = lines[i].split(":");
                    if (headerParts.length == 2) {
                        headers.put(headerParts[0].trim(), headerParts[1].trim());
                    } else {
                        System.out.println("Malformed header: " + line);
                    }
                }
            }
            else{
                // Handle embedded frames by unescaping \0
                body.append(line.replace("\\0", "\0")).append("\n");
            }
        }

        // Return a new StompFrame
        return new StompFrame(command, headers, body.toString().trim());
    }
    public String toRawFrame() {
        StringBuilder rawFrame = new StringBuilder();

        // Append the command
        rawFrame.append(command).append("\n");

        // Append each header in the format: key:value
        for (Map.Entry<String, String> entry : headers.entrySet()) {
            rawFrame.append(entry.getKey()).append(":").append(entry.getValue()).append("\n");
        }

        // Add an empty line to separate headers from the body
        rawFrame.append("\n");

        // Append the body, if it exists
        if (body != null && !body.isEmpty()) {
            rawFrame.append(body);
        }

        // Append the null character to terminate the frame
        rawFrame.append("\0");

        return rawFrame.toString();
    }



}
