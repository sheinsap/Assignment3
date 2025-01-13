package bgu.spl.net.srv;

import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.CopyOnWriteArraySet;

public class ConnectionsImpl<T> implements Connections<T> {

    private ConcurrentHashMap<Integer, ConnectionHandler<T>> connections = new ConcurrentHashMap<>();
    private final ConcurrentHashMap<String, CopyOnWriteArraySet<Integer>> topics = new ConcurrentHashMap<>();

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
        connections.remove(connectionId);
       topics.values().forEach(subscribers -> subscribers.remove(connectionId));
    };



}
