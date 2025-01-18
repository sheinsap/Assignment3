#include "../include/StompProtocol.h"
#include "../include/StompFrame.h"
#include "../include/SingletonCounter.h"
#include <atomic>
#include <map>
#include <string>

class StompProtocol {
private: bool terminate; 
std::atomic<int> nextId;
std::atomic<int> nextReceipt;    
std::map<std::string, int> channelSubscriptions;  

public:
     // Constructor
    StompProtocol() : terminate(false),nextId(1),nextReceipt(1) {}

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
            std::cout << response << std::endl;

        //STOMP frames from user
        } else if (frame.getCommand() == "CONNECT") {
            sendConnect(frame, nextReceipt);
        } else if (frame.getCommand() == "SEND") {
            sendSend(frame, nextReceipt);
        } else if (frame.getCommand() == "SUBSCRIBE") {
            sendSubscribe(frame, nextReceipt);
        } else if (frame.getCommand() == "UNSUBSCRIBE") {
            sendUnsubscribe(frame, nextReceipt);
        } else if (frame.getCommand() == "DISCONNECT") {
            sendDisconnect(frame, nextReceipt);           
        } else {
            std::cout << "Unknown command: " + frame.getCommand() << std::endl;
        }

        
    }

    // Check if the connection should terminate
    bool shouldTerminate() const {
        return terminate;
    }

private:
    void sendConnect(const StompFrame& frame, int receipt){
        nextReceipt++;
    }
    void sendSend(const StompFrame& frame, int receipt){

    }
    void sendSubscribe(const StompFrame& frame, int receipt){

    }
    void sendUnsubscribe(const StompFrame& frame, int receipt){

    }
    void sendDisconnect(const StompFrame& frame, int receipt){

    }


};