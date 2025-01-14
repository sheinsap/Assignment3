package bgu.spl.net.impl.stomp;

import bgu.spl.net.srv.Server;

public class StompServer {

    public static void main(String[] args) {
        int port = Integer.parseInt(args[0]);
        String serverType = args[1];

        if ("tpc".equals(serverType)) {
            Server.threadPerClient(
                port,
                () -> new StompProtocol(), // Protocol factory
                () -> new StompEncoderDecoder()         // Encoder/Decoder factory
            ).serve();
        } else if ("reactor".equals(serverType)) {
            // Reactor server setup (future step)
        }
    }
}
