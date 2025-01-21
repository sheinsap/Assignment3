#include "../include/StompProtocol.h"
#include "../include/StompFrame.h"
#include "../include/SingletonCounter.h"
#include "../include/event.h"
#include <mutex>
#include <atomic>
#include <map>
#include <set>
#include <string>

class StompProtocol {
private: bool terminate;
ConnectionHandler& connectionHandler;
std::string loggedUser;    
std::map<std::string, std::string> channelSubscriptions; //key:id value:channel
std::map<std::string, std::string> waitingReceipt; //key: receipts value:command

std::mutex mutex;




public:
     // Constructor
    StompProtocol(ConnectionHandler& handler) : connectionHandler(handler),terminate(false),loggedUser("")   {}

    void processFromServer(const std::string& input) {
        StompFrame frame = parseFromServer(input);
        std::string act;
        // act = frame.toRawFrame();
        // std::cout << act << std::endl;
        //STOMP frames from server
        if (frame.getCommand() == "CONNECTED") {
            act = frame.toRawFrame();
            std::cout << act << std::endl;
        } else if (frame.getCommand() == "MESSAGE") {
            act = frame.toRawFrame();
            std::cout << act << std::endl;
        } else if (frame.getCommand() == "ERROR") {
            act = frame.toRawFrame();
            std::cout << act << std::endl;
        } else if (frame.getCommand() == "RECEIPT") {
            act = frame.toRawFrame();
            if(waitingReceipt[frame.getHeader("receipt")]=="DISCONNECT"){
                connectionHandler.close();
            }
            waitingReceipt.erase(frame.getHeader("receipt"));
            std::cout << act << std::endl;
        }
    }

    //parsing from user
    void processFromUser(const std::string& input) {
        //Command word
        std::istringstream stream(input);
        std::string command;
        stream >> command;

        //login {accept-version} {host} {login} {passcode}- CONNECT
        if (command == "login") {
            std::string hostPort, username, password;
            stream >> hostPort >> username >> password;
            StompFrame frame = StompFrame(
                "CONNECT",
                {
                    {"accept-version:", "1.2"},
                    {"host", "stomp.cs.bgu.ac.il"},
                    {"login", username},
                    {"passcode", password}
                },
                "");
            sendConnect(frame);
        }

        //join {channel_name} - SUBSCRIBE 
        if (command == "join") {
            std::string channel_name;
            stream >> channel_name ;
            StompFrame frame = StompFrame(
                "SUBSCRIBE",
                {
                    {"destination:/", channel_name},
                    {"id", std::to_string(SingletonCounter::getInstance().getNextId())},
                    {"receipt", std::to_string(SingletonCounter::getInstance().getNextReceipt())}
                },
                "");
            sendSubscribe(frame);
        }

        //exit {channel_name}- UNSUBSCRIBE
        if (command == "exit") {
            std::string channel_name;
            stream >> channel_name ;
            StompFrame frame = StompFrame(
                "UNSUBSCRIBE",
                {
                    {"destination:/", channel_name},
                    {"id", std::to_string(SingletonCounter::getInstance().getNextId())},
                    {"receipt", std::to_string(SingletonCounter::getInstance().getNextReceipt())}
                },
                "");
            sendUnsubscribe(frame);
        }

        //report {eventsPath} - SEND multiple times
        //(!!!) need to implement event parsing... 
        if (command == "report") {

            //parse events 
            std::string eventsPath;
            stream >> eventsPath ;
            names_and_events eventsData=parseEventsFile(eventsPath);
            const std::string& channel_name = eventsData.channel_name;
            std::vector<Event> events = eventsData.events;

            //Sort events
            std::sort(events.begin(), events.end(), [](const Event& a, const Event& b) {
            return a.get_date_time() < b.get_date_time();
            });

            //SEND frames
            for (Event& event : events) {
                event.setEventOwnerUser(loggedUser);
                sendEvent(event);
            }
            //need to return names and events
            //loop of adding each event a user + sending send's and adding to a field + ordering by date

        }

        
        ////(!!!) need to implement summery logic... 
        ////summary - 
        //    if (command == "summary") {
            // std::string eventPath;
            // stream >> eventPath ;
            // return StompFrame(
            //     "UNSUBSCRIBE",
            //     {
            //         {"destination:/", channel_name},
            //         {"id", std::to_string(SingletonCounter::getInstance().getNextId())},
            //         {"receipt", std::to_string(SingletonCounter::getInstance().getNextReceipt())}
            //     },
            //     "");
        // }

        //(!!!) Once the client receives the RECEIPT frame, it should close the socket
        //(!!!) and await further user commands.

        //logut - DISCONNECT
        if (command == "logout") {
            StompFrame frame = StompFrame(
                "DISCONNECT",
                {
                    {"receipt", std::to_string(SingletonCounter::getInstance().getNextReceipt())}
                },
                "");
            sendDisconnect(frame);
        }
        //  else {
        //     std::cout << "Unknown command: " + frame.getCommand() << std::endl;
        // }
        
    }

  

    bool shouldTerminate() const {
        return terminate;
    }
private:
    void sendConnect(const StompFrame& frame){
        loggedUser=frame.getHeader("user");
        std::string rawFrame = frame.toRawFrame();
        connectionHandler.sendLine(rawFrame);
    }

    void sendEvent(Event& event){
        // frame.getHeader("receipt");

        StompFrame frame = StompFrame(
                "SEND",
                {
                    {"destination:/", event.get_channel_name()},
                    {"user", event.getEventOwnerUser()},
                    {"city", event.get_city()},
                    {"event name", event.get_name()},
                    //(!!!) need to change from hashmap to string
                    //   {"general information", event.get_general_information()},
                    {"description", event.get_description()}
                  
                },
                "");
        std::string rawFrame = frame.toRawFrame();
        connectionHandler.sendLine(rawFrame);
        //(!!!) - the frame should look: SEND/ndestination:/topic/a/n/nHello topic a/n^@

    }

    void sendSubscribe(const StompFrame& frame){
        channelSubscriptions[frame.getHeader("id")] = frame.getHeader("destination:/");
        //(!!!)what about the case of many receipts per id's 
        waitingReceipt.insert(frame.getHeader("receipt"),frame.getCommand());
        std::string rawFrame = frame.toRawFrame();
        connectionHandler.sendLine(rawFrame);
    }

    void sendUnsubscribe(const StompFrame& frame){
        waitingReceipt.insert(frame.getHeader("receipt"),frame.getCommand());
        channelSubscriptions.erase(frame.getHeader("id"));
        std::string rawFrame = frame.toRawFrame();
        connectionHandler.sendLine(rawFrame);

    }

    void sendDisconnect(const StompFrame& frame){
        waitingReceipt.insert(frame.getHeader("receipt"),frame.getCommand());
        //(!!!)does it wait to receipt?
        loggedUser="";
        std::string rawFrame = frame.toRawFrame();
        connectionHandler.sendLine(rawFrame);
        //delete event in field? 
    }


};