package bgu.spl.net.impl.stomp;

import bgu.spl.net.srv.Server;
import bgu.spl.net.impl.stomp.StompProtocol;
import bgu.spl.net.impl.stomp.StompEncoderDecoder;

public class StompServer {

    public static void main(String[] args) {
        int port = Integer.parseInt(args[0]);
        String serverType = args[1];

        if ("tpc".equals(serverType)) {
            Server.threadPerClient(
                port,
                () -> new StompProtocol(), // Protocol factory
                () -> new StompEncoderDecoder()  // Encoder/Decoder factory
            ).serve();
        } else if ("reactor".equals(serverType)) {
            Server.reactor(
                Runtime.getRuntime().availableProcessors(),
                port,
                () -> new StompProtocol(), // Protocol factory
                () -> new StompEncoderDecoder()  // Encoder/Decoder factory
            ).serve();
        }
        else{
            System.out.println("Invalid server type");
        }
    }
}
