package bgu.spl.net.impl.stomp;

import java.net.SocketOption;
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
    private static final ConcurrentHashMap<Integer, ConcurrentHashMap<String, String>> subscriptions = new ConcurrentHashMap<>(); // A map of topics this client is subscribed to, along with their subscription IDs
    private static final ConcurrentHashMap<String, String> users = new ConcurrentHashMap<>(); // A map of users and their passwords
    private static final ConcurrentHashMap<Integer, String> loggedInUsers = new ConcurrentHashMap<>(); // A map of contains logged-in users (connectionId, username)

    public void start(int connectionId, Connections<StompFrame> connections){
        this.connectionId = connectionId;
        this.connections = (ConnectionsImpl<StompFrame>) connections;
        subscriptions.putIfAbsent(connectionId, new ConcurrentHashMap<>());
      
    }
    
    public void process(StompFrame frame){

        switch (frame.getCommand()) {
            case "CONNECT":
                handleConnect(frame);
            case "SEND":
               handleSend(frame);
            case "SUBSCRIBE":
                handleSubscribe(frame);
            case "UNSUBSCRIBE":
                handleUnsubscribe(frame);
            case "DISCONNECT":
                handleDisconnect(frame);
            case "ERROR":
                handleError(frame);
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
        // (!!!) needs to check if it is the right logic
        if (loggedInUsers.containsKey(connectionId)) {
            sendError("The client is already logged in, log out before trying again");
            return;
        }
        if (users.containsKey(username) && !users.get(username).equals(password)) {
            sendError("Wrong password");
            return;
        }
        if (!users.containsKey(username)) {
            users.put(username, password);
        }
        

        // If all checks pass, send the CONNECTED frame
        loggedInUsers.put(connectionId, username);
        StompFrame connectedFrame = StompFrame.parse("CONNECTED\nversion:1.2\n\n\u0000");
        connections.send(connectionId, connectedFrame);
    }

    private void handleSend(StompFrame frame) {
        // Logic for handling a SEND frame
        String destination = frame.getHeader("destination");
        if (destination == null) {
            sendError("SEND frame must include a destination header");
            return;
        }
        if (!connections.isSubscribed(destination, connectionId)) {
            sendError("Client not subscribed to the destination: " + destination);
            return;
        }

        String body = frame.getBody();
        int messageId = messageIdCounter.getAndIncrement(); // Generate a unique message ID

        // Broadcast the message to all subscribers
        for (int subscriberId : connections.getSubscribers(destination)) {
            String subscriptionId = subscriptions.get(subscriberId).get(destination);
            StompFrame messageFrame = StompFrame.parse("MESSAGE\nsubscription:" + subscriptionId + "\nmessage-id:" + messageId +
            "\ndestination:" + destination + "\n\n" + body + "\u0000");
    
            connections.send(subscriberId, messageFrame); 
        }
    }

    private void handleSubscribe(StompFrame frame) {
        
        String destination = frame.getHeader("destination");
        String id = frame.getHeader("id");

        if (destination == null || id == null) {
            sendError("Missing a required header");
            return;
        }

        subscriptions.get(connectionId).put(destination, id);
        connections.subscribe(destination, connectionId); // Add connectionId to the topic
    }

    private void handleUnsubscribe(StompFrame frame) {
    
        String id = frame.getHeader("id");
        if (id == null) {
            sendError("UNSUBSCRIBE frame must include an id header");
            return;
        }

        subscriptions.get(connectionId).entrySet().removeIf(entry -> entry.getValue().equals(id));
        connections.unsubscribe(id, connectionId);
    }

    private void handleDisconnect(StompFrame frame) {
        
        subscriptions.remove(connectionId);
        loggedInUsers.remove(connectionId);
        connections.disconnect(connectionId);
        shouldTerminate = true; // Mark connection for termination
    }

    private void handleError(StompFrame errorFrame) {
        
        connections.send(connectionId, errorFrame);
    }

}
