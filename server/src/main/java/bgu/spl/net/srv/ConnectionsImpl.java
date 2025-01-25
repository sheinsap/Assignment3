package bgu.spl.net.srv;

import java.util.Map;
import java.util.Set;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.CopyOnWriteArraySet;
import java.util.concurrent.locks.ReentrantLock;

public class ConnectionsImpl<T> implements Connections<T> {

    private ConcurrentHashMap<Integer, ConnectionHandler<T>> connections = new ConcurrentHashMap<>();
    private final ConcurrentHashMap<String, CopyOnWriteArraySet<Integer>> topics = new ConcurrentHashMap<>();
    private final ConcurrentHashMap<String, String> users = new ConcurrentHashMap<>(); // username -> password
    private final ConcurrentHashMap<Integer, String> loggedInUsers = new ConcurrentHashMap<>(); // connectionId -> username
    private final ConcurrentHashMap<Integer, ConcurrentHashMap<String, String>> clientSubscriptions = new ConcurrentHashMap<>(); // connectionId -> {topic -> subscriptionId}
    private final ReentrantLock lock = new ReentrantLock();

    public boolean send(int connectionId, T msg){
        ConnectionHandler<T> handler = connections.get(connectionId);
        if (handler != null) {
            handler.send(msg);
            return true;
        }
        return false;
    }

    public void send(String channel, T msg){
        CopyOnWriteArraySet<Integer> subscribers = topics.get(channel);
        if (subscribers != null) {
            for (Integer connectionId : subscribers) {
                send(connectionId, msg);
            }
        }

    }

    public void disconnect(int connectionId){
        lock.lock();
        try {
            // Check if the connection is already removed
            if (!connections.containsKey(connectionId)) {
                System.out.println("Disconnect attempted for non-existent connection: " + connectionId);
                return; // Already disconnected
            }

            // Remove the user-specific subscriptions
            clientSubscriptions.remove(connectionId);

            // Remove the user from logged-in users
            loggedInUsers.remove(connectionId);
        } finally {
            lock.unlock();
        }
        // Remove the connection from the global map
        connections.remove(connectionId);

        // Remove the user from subscriptions
        topics.values().forEach(subscribers -> subscribers.remove(connectionId));
        System.out.println("Connection " + connectionId + " disconnected.");
    }
    
    public void addConnection(int connectionId, ConnectionHandler<T> handler) {
        connections.put(connectionId, handler);
    }

    public void subscribe(String channel, int connectionId) {
        topics.putIfAbsent(channel, new CopyOnWriteArraySet<>());
        topics.get(channel).add(connectionId);
    }

    public void unsubscribe(String channel, int connectionId) {
        CopyOnWriteArraySet<Integer> subscribers = topics.get(channel);
        if (subscribers != null) {
            subscribers.remove(connectionId);
        }
    }

    public CopyOnWriteArraySet<Integer> getSubscribers(String channel) {
        return topics.getOrDefault(channel, new CopyOnWriteArraySet<>());
    }
    
    public boolean login(int connectionId, String username, String password) {
        lock.lock();
        try{
            if (users.containsKey(username)) {
                if (!users.get(username).equals(password)) {
                    return false; // Invalid password
                }
            } else {
                users.put(username, password); // Register new user
            }

            loggedInUsers.put(connectionId, username); // log in an existing user
            return true;
        } finally {
            lock.unlock();
        }
    }

    public boolean isLoggedIn(int connectionId) {
        return loggedInUsers.containsKey(connectionId);
    }

    // Subscription management
    public void subscribeClient(int connectionId, String topic, String subscriptionId) {
        
        clientSubscriptions.putIfAbsent(connectionId, new ConcurrentHashMap<>());
        clientSubscriptions.get(connectionId).put(topic, subscriptionId);
        subscribe(topic, connectionId);
    }

    public void unsubscribeClient(int connectionId, String subscriptionId) {
        lock.lock();
        try{
            Map<String, String> subscriptions = clientSubscriptions.get(connectionId);
            if (subscriptions != null) {
                String topic = null;

                // Find the topic based on subscription ID
                for (Map.Entry<String, String> entry : subscriptions.entrySet()) {
                    if (entry.getValue().equals(subscriptionId)) {
                        topic = entry.getKey();
                        break;
                    }
                }

                if (topic != null) {
                    subscriptions.remove(topic);
                    unsubscribe(topic, connectionId);
                }
            }
        }
        finally {
            lock.unlock();
        }
    }
    public boolean userExists(String username) {
        return users.containsKey(username);
    }
    public boolean isUserLoggedIn(String username) {
        return loggedInUsers.containsValue(username);
    }
    public boolean isPasswordCorrect(String username, String password) {
        return users.get(username).equals(password);
    }
    public void addUser(String username, String password) {
        users.putIfAbsent(username, password);
    }
    public String getSubscriptionId(int connectionId, String destination) {
        Map<String, String> subscriptions = clientSubscriptions.get(connectionId);
        if (subscriptions != null) {
            return subscriptions.get(destination); // Return the subscription ID for the destination
        }
        return null; // No subscription found
    }
    public String getTopicBySubscriptionId(int connectionId, String subscriptionId) {
        Map<String, String> subscriptions = clientSubscriptions.get(connectionId);
        if (subscriptions != null) {
            for (Map.Entry<String, String> entry : subscriptions.entrySet()) {
                if (entry.getValue().equals(subscriptionId)) {
                    return entry.getKey(); // Return the topic associated with the subscription ID
                }
            }
        }
        return null; // Subscription ID not found
    }
};


