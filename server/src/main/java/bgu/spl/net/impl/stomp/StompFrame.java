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
        System.out.println("Parsing frame: " + frame); // Add this for debugging
        // Check if the frame ends with the null character
        //if (!frame.endsWith("\0")) {
        //return parse("ERROR\nmessage: Frame does not terminate with null character\n\n\u0000");
        //}

        // Remove the null character before parsing
        frame = frame.substring(0, frame.length() - 1);

        String[] lines = frame.split("\n"); // Split the frame into lines
        String command = lines[0];            // First line is the command
        Map<String, String> headers = new HashMap<>();
        int i = 1;

        // Parse headers until an empty line is encountered
        while (i < lines.length && !lines[i].isEmpty()) {
            String[] headerParts = lines[i].split(":");
            if (headerParts.length != 2) {
                return parse("ERROR\nmessage: Malformed header " + lines[i]+"\n\n\u0000");
            }
            headers.put(headerParts[0].trim(), headerParts[1].trim());
            i++;
        }

        i++; // Skip the empty line between headers and body

        // Parse the body
        StringBuilder body = new StringBuilder();
        while (i < lines.length) {
            body.append(lines[i]).append("\n");
            i++;
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
