#include "../include/StompProtocol.h"
#include "../include/StompFrame.h"
#include "../include/SingletonCounter.h"
#include "../include/event.h"
#include <mutex>
#include <atomic>
#include <map>
#include <set>
#include <string>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <ctime>

class StompProtocol {
private: bool terminate;
ConnectionHandler& connectionHandler;
int reportsCount;
int activeCount;
int forcesArrivalCount;
std::string loggedUser;    
std::map<std::string, std::string> channelSubscriptions; //key:id value:channel
std::map<std::string, std::string> waitingReceipt; //key: receipts value:command
std::map<std::string, std::map<std::string, std::vector<Event>>> userEvents; //key: user value: Hash map(key:channel name, value: Events sorted by date)
std::mutex mutex;




public:
     // Constructor
    StompProtocol(ConnectionHandler& handler) : connectionHandler(handler),terminate(false),loggedUser(""),
    reportsCount(0),activeCount(0),forcesArrivalCount(0)   {}

    void processFromServer(const std::string& input) {
        StompFrame frame = frame.parseFromServer(input);
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
        if (command == "report") {

            //parse events 
            std::string eventsPath;
            stream >> eventsPath ;
            names_and_events eventsData=parseEventsFile(eventsPath);
            
            //Channel name + events vector 
            const std::string& channel_name = eventsData.channel_name;
            std::vector<Event> events = eventsData.events;

            //Add user to events and update counters
            for (Event& event : events) {
                event.setEventOwnerUser(loggedUser);
                reportsCount++;
                std::map<std::string, std::string> general_information = event.get_general_information();  
                std::string active = general_information["active"];
                std::string forces_arrival = general_information["forces_arrival_at_scene"];
                if(active=="true"){
                    activeCount++;
                }
                if(forces_arrival=="true"){
                    forcesArrivalCount++;
                }
            }

                // Add events to userEvents per user, per channel
            for (Event& event : events) {
                // Add the user if not already present
                if (userEvents.find(loggedUser) == userEvents.end()) {
                    userEvents[loggedUser] = std::map<std::string, std::vector<Event>>();
                }

                // Add the channel if not already present
                if (userEvents[loggedUser].find(channel_name) == userEvents[loggedUser].end()) {
                    userEvents[loggedUser][channel_name] = std::vector<Event>();
                }

                // Add the event to the appropriate user's channel
                userEvents[loggedUser][channel_name].push_back(event);
            }

            // Ensure all events are sorted by date per user, per channel
            for (auto& userPair : userEvents) {
                for (auto& channelPair : userPair.second) {
                    channelPair.second = sortEvents(channelPair.second);
                }
            }
              //send all the events
            for (Event& event : events){
                sendEvent(event);
            }
        }

        
        //summary {channel_name} {user} {file} 
           if (command == "summary") {
            std::string channel_name, user, file;
            stream >> channel_name >> user >> file ;

        }

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
        //(!!!) ?? frame.getHeader("receipt");
        StompFrame frame = StompFrame::parseEvent(event);
        std::string rawFrame = frame.toRawFrame();
        connectionHandler.sendLine(rawFrame);
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

    void sendSummary(const std::string& channel_name, const std::string& user, const std::string& file) {
        // Check if the user exists in userEvents
        auto userIt = userEvents.find(user);
        if (userIt == userEvents.end()) {
            std::cerr << "User '" << user << "' not found in userEvents." << std::endl;
            return;
        }

        // Check if the channel exists for the user
        auto channelIt = userIt->second.find(channel_name);
        if (channelIt == userIt->second.end()) {
            std::cerr << "Channel '" << channel_name << "' not found for user '" << user << "'." << std::endl;
            return;
        }

        // Retrieve events for the specified channel
        const std::vector<Event>& events = channelIt->second;


        // Prepare output
        std::ostringstream output;
        output << "Channel " << channel_name << "\n";
        output << "Stats:\n";

        // for (const auto& event : events) {
        //     reportsCount++;
        //     if (event.get_active()) {
        //         activeCount++;
        //     }
        //     if (event.get_forces_arrival_at_scene()) {
        //         forcesArrivalCount++;
        //     }
        // }

        // Add stats to output
        output << " Total: " << reportsCount << "\n";
        output << " active: " << activeCount << "\n";
        output << " forces arrival at scene: " << forcesArrivalCount << "\n";

        // Event Reports
        output << "Event Reports:\n";

        int reportIndex = 0;
        for (const auto& event : events) {
            std::string description = event.get_description();
            std::string summary = description.substr(0, 27);
            if (description.size() > 27) {
                summary += "...";
            }

            // Convert date time
            std::string dateTimeString = epochToDate(event.get_date_time());

            // Add event details to the output
            output << " Report_" << reportIndex++ << ":\n";
            output << "  city: " << event.get_city() << "\n";
            output << "  date time: " << dateTimeString << "\n";
            output << "  event name: " << event.get_name() << "\n";
            output << "  summary: " << summary << "\n";
        }

        // Write the output to the file
        // std::ofstream outputFile(file, std::ios::out); // std::ios::out ensures it opens for writing
        // if (!outputFile.is_open()) {
        std::ofstream outputFile(file);
        if (!outputFile) {
            std::cerr << "Failed to open file '" << file << "' for writing." << std::endl;
            return;
        }
        outputFile << output.str();
        outputFile.close();

        std::cout << "Summary written to file '" << file << "' successfully." << std::endl;
    }

    // Helper function to convert epoch time to a date-time string
    std::string epochToDate(time_t epoch) {
        std::tm* tm = std::localtime(&epoch);
        std::ostringstream dateStream;
        dateStream << std::put_time(tm, "%d/%m/%y %H:%M");
        return dateStream.str();
    }

    void sendDisconnect(const StompFrame& frame){
        waitingReceipt.insert(frame.getHeader("receipt"),frame.getCommand());
        //(!!!)does it wait to receipt?
        loggedUser="";
        std::string rawFrame = frame.toRawFrame();
        connectionHandler.sendLine(rawFrame);
        //delete event in field? 
    }



    std::vector<Event> sortEvents(std::vector<Event> events)
    {
        // Sort events first by date and then by name
        std::sort(events.begin(), events.end(), [](const Event& a, const Event& b) {
            if (a.get_date_time() == b.get_date_time()) {
                // If dates are equal, sort by name
                return a.get_name() < b.get_name();
            }
            // Otherwise, sort by date
            return a.get_date_time() < b.get_date_time();
        });

        return events;
    }

};