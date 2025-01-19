package bgu.spl.net.impl.stomp;

import java.net.SocketOption;
import java.util.Map;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.atomic.AtomicInteger;

import bgu.spl.net.api.StompMessagingProtocol;
import bgu.spl.net.srv.Connections;
import bgu.spl.net.srv.ConnectionsImpl;

public class StompProtocol implements StompMessagingProtocol<StompFrame>{

    private int connectionId;                
    private ConnectionsImpl<StompFrame> connections;  
    private boolean shouldTerminate = false;  
    private static final AtomicInteger messageIdCounter = new AtomicInteger(1); // For unique message IDs
   

    public void start(int connectionId, Connections<StompFrame> connections){
        this.connectionId = connectionId;
        this.connections = (ConnectionsImpl<StompFrame>) connections;
      
    }
    
    public void process(StompFrame frame){

        switch (frame.getCommand()) {
            case "CONNECT":
                handleConnect(frame);
                break;
            case "SEND":
               handleSend(frame);
               break;
            case "SUBSCRIBE":
                handleSubscribe(frame);
                break;
            case "UNSUBSCRIBE":
                handleUnsubscribe(frame);
                break;
            case "DISCONNECT":
                handleDisconnect(frame);
                break;
            case "ERROR":
                handleError(frame);
                break;
            default:
                sendError("Unknown command: " + frame.getCommand());
         }
        // Handle receipt header if present
        if (frame.getHeader("receipt") != null) {
            sendReceipt(frame.getHeader("receipt"));
        }
    }
	
    public boolean shouldTerminate(){
        return shouldTerminate;
    }

    // Helper method to send an ERROR frame
    private void sendError(String errorMessage) {
        StompFrame errorFrame = StompFrame.parse("ERROR\nmessage: " + errorMessage + "\n\n\u0000");
        connections.send(connectionId, errorFrame);
    }

    // Helper method to send a RECEIPT frame
    private void sendReceipt(String receiptId) {
        StompFrame receiptFrame = StompFrame.parse("RECEIPT\nreceipt-id:" + receiptId + "\n\n\u0000");
        connections.send(connectionId, receiptFrame);
    }

    //(!!!) should add a check if login and passcode are uniqe?
    private void handleConnect(StompFrame frame) {
        String version = frame.getHeader("accept-version");
        String host = frame.getHeader("host");
        String username = frame.getHeader("login");
        String password = frame.getHeader("passcode");
        if(version == null || host == null || username == null || password == null){
            sendError("Missing a required header");
            return;
        }
        // Check that the version is "1.2"
        if (!"1.2".equals(version)) {
            sendError("Unsupported STOMP version");
            return;
        }
        // Check that the host is "stomp.cs.bgu.ac.il"
        if (!"stomp.cs.bgu.ac.il".equals(host)) {
            sendError("Invalid host");
            return;
        }
        // Check if the client is already logged in
        if (connections.isLoggedIn(connectionId)) {
            sendError("The client is already logged in, log out before trying again");
            return;
        }

        // Check user 
        if (connections.userExists(username)) {
            if (connections.isUserLoggedIn(username)) {
                sendError("User already logged in");
                return;
            }

            if (!connections.isPasswordCorrect(username, password)) {
                sendError("Wrong password");
                return;
            }
        } else {
            // New user
            connections.addUser(username, password);
            System.out.println("New user created: " + username);
    }

        // Log in the user
        connections.login(connectionId, username, password);


        StompFrame connectedFrame = StompFrame.parse("CONNECTED\nversion:1.2\n\n\0");
        connections.send(connectionId, connectedFrame);
      
    }

    private void handleSend(StompFrame frame) {
        
        String destination = frame.getHeader("destination");
        if (destination == null) {
            sendError("SEND frame must include a destination header");
            return;
        }

        String body = frame.getBody();
        int messageId = messageIdCounter.getAndIncrement(); // Generate a unique message ID

        // Broadcast the message to all subscribers
        for (int subscriberId : connections.getSubscribers(destination)) {
            String subscriptionId = connections.getSubscriptionId(subscriberId, destination);
            if (subscriptionId == null) {
                sendError("No subscription found for destination: " + destination);
                continue;
            
            }
            StompFrame messageFrame = StompFrame.parse("MESSAGE\nsubscription:" + subscriptionId + "\nmessage-id:" + messageId +
            "\ndestination:" + destination + "\n\n" + body + "\u0000");
    
            connections.send(subscriberId, messageFrame); 
        }
    }

    private void handleSubscribe(StompFrame frame) {
        
        String topic = frame.getHeader("destination");
        String subscriptionId = frame.getHeader("id");
        String receipt = frame.getHeader("receipt");

        if (topic == null || subscriptionId == null || receipt == null) {
            sendError("Missing a required header");
            return;
        }
        connections.subscribeClient(connectionId, topic, subscriptionId);
        System.out.println("Connection " + connectionId + " subscribed to topic " + topic);

    }

    private void handleUnsubscribe(StompFrame frame) {
        
        String subscriptionId = frame.getHeader("id");
        String receipt = frame.getHeader("receipt");
        if (subscriptionId == null || receipt == null) {
            sendError("Missing a required header");
            return;
        }
        // Check if the subscription exists
        String topic = connections.getTopicBySubscriptionId(connectionId, subscriptionId);
        if (topic == null) {
            sendError("Subscription does not exist or does not match");
            return;
        }
        connections.unsubscribeClient(connectionId, subscriptionId);
        System.out.println("Connection " + connectionId + " unsubscribed from topic " + topic + " with subscription ID " + subscriptionId);

    }

    private void handleDisconnect(StompFrame frame) {
        
        connections.disconnect(connectionId);
        // (!!!) not sure if it is the right logic
        shouldTerminate = true; // Mark connection for termination
    }

    private void handleError(StompFrame errorFrame) {
        
        connections.send(connectionId, errorFrame);
    }

}
