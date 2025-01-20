#include "../include/StompProtocol.h"
#include "../include/StompFrame.h"
#include "../include/SingletonCounter.h"
#include <mutex>
#include <atomic>
#include <map>
#include <set>
#include <string>

class StompProtocol {
private: bool terminate;
ConnectionHandler& connectionHandler;    
std::map<std::string, std::string> channelSubscriptions; //key:id value:channel
std::set<std::string> waitingReceipt; //key: receipts
std::mutex mutex;




public:
     // Constructor
    StompProtocol(ConnectionHandler& handler) : connectionHandler(handler),terminate(false)   {}

     void process(const StompFrame& frame) {
        std::string response;
        response = frame.toRawFrame();
        std::cout << response << std::endl;
        //STOMP frames from server
        if (frame.getCommand() == "CONNECTED") {
            response = frame.toRawFrame();
            std::cout << response << std::endl;
        } else if (frame.getCommand() == "MESSAGE") {
            response = frame.toRawFrame();
            std::cout << response << std::endl;
        } else if (frame.getCommand() == "ERROR") {
            response = frame.toRawFrame();
            std::cout << response << std::endl;
        } else if (frame.getCommand() == "RECEIPT") {
            response = frame.toRawFrame();
            waitingReceipt.erase(frame.getHeader("receipt"));
            std::cout << response << std::endl;

        //STOMP frames from user
        } else if (frame.getCommand() == "CONNECT") {
            sendConnect(frame);
        } else if (frame.getCommand() == "SEND") {
            sendSend(frame); //see function
        } else if (frame.getCommand() == "SUBSCRIBE") {
            sendSubscribe(frame);
        } else if (frame.getCommand() == "UNSUBSCRIBE") {
            sendUnsubscribe(frame);
        } else if (frame.getCommand() == "DISCONNECT") {
            sendDisconnect(frame);           
        } else {
            std::cout << "Unknown command: " + frame.getCommand() << std::endl;
        }

        
    }

    bool shouldTerminate() const {
        return terminate;
    }

private:
    void sendConnect(const StompFrame& frame){
        std::string rawFrame = frame.toRawFrame();
        connectionHandler.sendLine(rawFrame);
    }

    void sendSend(const StompFrame& frame){
        // frame.getHeader("receipt");
        // std::string rawFrame = frame.toRawFrame();
        // connectionHandler.sendLine(rawFrame);
        //(!!!) - the frame should look: SEND/ndestination:/topic/a/n/nHello topic a/n^@

    }

    void sendSubscribe(const StompFrame& frame){
        channelSubscriptions[frame.getHeader("id")] = frame.getHeader("destination:/");
        //(!!!)what about the case of many receipts per id's 
        waitingReceipt.insert(frame.getHeader("receipt"));
        std::string rawFrame = frame.toRawFrame();
        connectionHandler.sendLine(rawFrame);
    }

    void sendUnsubscribe(const StompFrame& frame){
        waitingReceipt.insert(frame.getHeader("receipt"));
        channelSubscriptions.erase(frame.getHeader("id"));
        std::string rawFrame = frame.toRawFrame();
        connectionHandler.sendLine(rawFrame);

    }

    void sendDisconnect(const StompFrame& frame){
        waitingReceipt.insert(frame.getHeader("receipt"));
        std::string rawFrame = frame.toRawFrame();
        connectionHandler.sendLine(rawFrame);
    }


};