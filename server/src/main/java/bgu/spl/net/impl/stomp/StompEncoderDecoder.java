package bgu.spl.net.impl.stomp;

import java.nio.charset.StandardCharsets;
import java.util.ArrayList;
import java.util.List;

import bgu.spl.net.api.MessageEncoderDecoder;

public class StompEncoderDecoder implements MessageEncoderDecoder<StompFrame> {
    private final List<Byte> buffer = new ArrayList<>(); // Accumulate incoming bytes

    @Override
    public StompFrame decodeNextByte(byte nextByte) {
        // Check if the frame is complete (null character \u0000 marks the end)
        if (nextByte == '\0') {
            byte[] frameBytes = new byte[buffer.size()];
            for (int i = 0; i < buffer.size(); i++) {
                frameBytes[i] = buffer.get(i);
            }
            buffer.clear(); // Reset the buffer after processing

            String frameString = new String(frameBytes); // Convert bytes to string
            return StompFrame.parse(frameString); // Parse the raw frame into a StompFrame object
        }

        buffer.add(nextByte); // Accumulate byte
        return null; // Frame is not complete yet
    }

    @Override
    public byte[] encode(StompFrame message) {
        // Convert the StompFrame into a raw STOMP string
        String rawFrame = message.toRawFrame();

        // Convert the raw string into bytes
        return (rawFrame + "\u0000").getBytes(StandardCharsets.UTF_8);
    }

}
