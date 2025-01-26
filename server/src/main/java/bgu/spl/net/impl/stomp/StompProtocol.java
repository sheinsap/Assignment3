package bgu.spl.net.impl.stomp;

import java.net.SocketOption;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.CopyOnWriteArraySet;
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
                sendError("Unknown command: " + frame.getCommand(), frame, "");
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
    private void sendError(String errorMessage, StompFrame frame, String description) {
        // Construct the headers manually
        Map<String, String> headers = new HashMap<>();
        headers.put("message", errorMessage);

        // Prepare the body with the embedded frame
        String embeddedFrame = frame.toRawFrame();
        if (embeddedFrame.endsWith("\0")) {
            embeddedFrame = embeddedFrame.substring(0, embeddedFrame.length() - 1); // Remove the \0
        }
        // Escape \0 characters inside the embedded frame for readability
        embeddedFrame = embeddedFrame.replace("\0", "\\0");

        String errorBody = "The message: \n-----\n" + embeddedFrame + "----- \n" + description;

        // Create the ERROR frame
        StompFrame errorFrame = new StompFrame("ERROR", headers, errorBody);

        // Send the ERROR frame
        connections.send(connectionId, errorFrame);
        shouldTerminate = true; // Mark connection for termination
    }


    // Helper method to send a RECEIPT frame
    private void sendReceipt(String receiptId) {
        StompFrame receiptFrame = StompFrame.parse("RECEIPT\nreceipt-id:" + receiptId + "\n\n\0");
        connections.send(connectionId, receiptFrame);
    }

    private void handleConnect(StompFrame frame) {
        String version = frame.getHeader("accept-version");
        String host = frame.getHeader("host");
        String username = frame.getHeader("login");
        String password = frame.getHeader("passcode");
        if(version == null || host == null || username == null || password == null){
            sendError("Missing a required header", frame, "");
            return;
        }
        // Check that the version is "1.2"
        if (!"1.2".equals(version)) {
            sendError("Unsupported STOMP version", frame, "Version is not 1.2");
            return;
        }
        // Check that the host is "stomp.cs.bgu.ac.il"
        if (!"stomp.cs.bgu.ac.il".equals(host)) {
            sendError("Invalid host", frame, "Host is not stomp.cs.bgu.ac.il");
            return;
        }
        // Check if the client is already logged in
        if (connections.isLoggedIn(connectionId)) {
            sendError("The client is already logged in, log out before trying again", frame, "Client is already logged in");
            return;
        }

        // Check user 
        if (connections.userExists(username)) {
            if (connections.isUserLoggedIn(username)) {
                sendError("User already logged in", frame, "User " + username + " is logged in somewhere else");
                return;
            }

            if (!connections.isPasswordCorrect(username, password)) {
                sendError("Wrong password", frame, "User " + username + "'s password is different than what you inserted");
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
            sendError("SEND frame must include a destination header", frame, "");
            return;
        }

        String body = frame.getBody();

        // Extract the first line from the body (the user)
        String user = body.split("\n", 2)[0];

        // Check if there are any subscribers for the destination
        CopyOnWriteArraySet<Integer> subscribers = connections.getSubscribers(destination);
        if (subscribers == null || subscribers.isEmpty()) {
            sendError("No subscribers found for destination: " + destination, frame, "The destination has no active subscribers.");
            return;
        }

        // Verify the sender's subscription
        String senderSubscriptionId = connections.getSubscriptionId(connectionId, destination);
        if (senderSubscriptionId == null) {
            sendError("Sender is not subscribed to destination: " + destination, frame, "You must subscribe to the destination before sending messages.");
            return;
        }

        // Broadcast the message to all subscribers
        for (int subscriberId : subscribers) {
            int messageId = messageIdCounter.getAndIncrement(); // Generate a unique message ID
            String subscriptionId = connections.getSubscriptionId(subscriberId, destination);
           
            StompFrame messageFrame = StompFrame.parse("MESSAGE\nsubscription:" + subscriptionId + "\nmessage-id:" + messageId +
            "\ndestination:" + destination + "\n" + user + "\n\n" + body + "\0");
    
            connections.send(subscriberId, messageFrame); 
        }
    }

    private void handleSubscribe(StompFrame frame) {
        
        String topic = frame.getHeader("destination");
        String subscriptionId = frame.getHeader("id");
        String receipt = frame.getHeader("receipt");

        if (topic == null || subscriptionId == null || receipt == null) {
            sendError("Missing a required header", frame, "");
            return;
        }
        connections.subscribeClient(connectionId, topic, subscriptionId);
        System.out.println("Connection " + connectionId + " subscribed to topic " + topic);

    }

    private void handleUnsubscribe(StompFrame frame) {
        
        String subscriptionId = frame.getHeader("id");
        String receipt = frame.getHeader("receipt");
        if (subscriptionId == null || receipt == null) {
            sendError("Missing a required header", frame, "");
            return;
        }
        // Check if the subscription exists
        String topic = connections.getTopicBySubscriptionId(connectionId, subscriptionId);
        if (topic == null) {
            sendError("Subscription does not exist or does not match", frame, "");
            return;
        }
        connections.unsubscribeClient(connectionId, subscriptionId);
        System.out.println("Connection " + connectionId + " unsubscribed from topic " + topic + " with subscription ID " + subscriptionId);

    }

    private void handleDisconnect(StompFrame frame) {
        if (!connections.isLoggedIn(connectionId)) {
            System.out.println("Disconnect attempted for an already disconnected connection: " + connectionId);
            return; // Avoid processing disconnect twice
        }
        shouldTerminate = true; // Mark connection for termination
    }

    private void handleError(StompFrame errorFrame) {
        
        connections.send(connectionId, errorFrame);
    }

}
