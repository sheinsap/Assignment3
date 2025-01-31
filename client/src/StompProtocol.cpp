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
#include "StompProtocol.h"


// Constructor
StompProtocol::StompProtocol(ConnectionHandler& handler) :  
    terminate(false),
    isConnected(false),
    connectionHandler(handler),
    loggedUser(""),
    channelSubscriptions(),
    waitingReceipt(),
    userEvents(),
    mutex() {}

void StompProtocol::processFromServer(const std::string& input) {
    StompFrame frame = StompFrame::parseFromServer(input);
    std::string act = frame.toRawFrame();
    // std::cout << "\nReceived frame from server:" << std::endl;
    // std::cout << act << std::endl;


    //STOMP frames from server
    if (frame.getCommand() == "CONNECTED") {
        std::lock_guard<std::mutex> lock(mutex);
        isConnected=true;
        std::cout << "Login successful!" << std::endl;
    } else if (frame.getCommand() == "MESSAGE") {
        handleMessageFrame(frame);
    } else if (frame.getCommand() == "ERROR") {
        connectionHandler.close();
        isConnected = false; 
        loggedUser="";
        channelSubscriptions.clear();
        waitingReceipt.clear();
        userEvents.clear();
        std::cout << "\nError received:" <<"\n" << act << "\nDisconnected." << std::endl;

    } else if(frame.getCommand() == "RECEIPT") {
        std::lock_guard<std::mutex> lock(mutex);
        std::string receiptId = frame.getHeader("receipt-id"); 
        if(waitingReceipt[receiptId]=="DISCONNECT"){
            connectionHandler.close();
            isConnected = false; 
            loggedUser="";
            channelSubscriptions.clear();
            waitingReceipt.clear();
            userEvents.clear();
            std::cout << "\nDisconnected successfully" << std::endl;
        }
        waitingReceipt.erase(receiptId);
    }
}


     
//parsing from user
void StompProtocol::processFromUser(const std::string& input) 
{
    //Command word
    std::istringstream stream(input);
    std::string command;
    stream >> command;

    if (!isLoggedin() && command != "login") {
        std::cerr << "You must log in first with the `login` command." << std::endl;
        return;
    }

    //login {accept-version} {host} {login} {passcode}- CONNECT
    if (command == "login") {
        std::string hostPort, username, password;
        stream >> hostPort >> username >> password;

        //parse host:port
        size_t colonPos = hostPort.find(':');
        if (colonPos == std::string::npos) {
            throw std::invalid_argument("Invalid host:port format. Expected format is host:port (e.g., 127.0.0.1:7777).");
        }
        std::string host = hostPort.substr(0, colonPos);
        std::string portStr = hostPort.substr(colonPos + 1);
        short port = static_cast<short>(std::stoi(portStr));
        connectionHandler.setHostPort(host,port);

        // Reinitialize the connection handler if necessary
        if (!isLoggedin()) {
            if (!connectionHandler.connect()) {
                std::cerr << "Failed to connect to the server" << std::endl;
                return;
            }
        }

        StompFrame frame = StompFrame(
            "CONNECT",
            {
                {"accept-version", "1.2"},
                {"host", "stomp.cs.bgu.ac.il"},
                {"login", username},
                {"passcode", password}
            },
            "\n\0");
        sendConnect(frame);
    }

    //join {channel_name} - SUBSCRIBE 
    else if (command == "join") {
        std::string channel_name;
        stream >> channel_name ;
        StompFrame frame = StompFrame(
            "SUBSCRIBE",
            {
                {"destination", channel_name},
                {"id", std::to_string(SingletonCounter::getInstance().getNextId())},
                {"receipt", std::to_string(SingletonCounter::getInstance().getNextReceipt())}
            },
            "\n\0");
        sendSubscribe(frame);
        std::cout << "Joined channel " << channel_name << std::endl;
    }

    //exit {channel_name}- UNSUBSCRIBE
    else if (command == "exit") {
        std::string channel_name;
        stream >> channel_name ;

        // Find the subscription ID for the channel
        std::string subscriptionId = "";
        for (const auto& subscription : channelSubscriptions) {
            const std::string& id = subscription.first;         // key
            const std::string& destination = subscription.second; // value
            if (destination == channel_name) {
                subscriptionId = id;
                userEvents.erase(channel_name);
                channelSubscriptions.erase(id);
                break;
            }
        }

        if (subscriptionId.empty()) {
        std::cerr << "Error: No subscription found for channel " << channel_name << std::endl;
        return;
        }


        StompFrame frame = StompFrame(
            "UNSUBSCRIBE",
            {
                {"destination", channel_name},
                {"id", subscriptionId},
                {"receipt", std::to_string(SingletonCounter::getInstance().getNextReceipt())}
            },
            "\n\0");
        sendUnsubscribe(frame);
        std::cout << "Exited channel " << channel_name << std::endl;
    }

    //report {eventsPath} - SEND multiple times
    else if (command == "report") {

        //parse events 
        std::string eventsPath;
        stream >> eventsPath ;
        names_and_events eventsData=parseEventsFile(eventsPath);
        
        //events vector 
        std::vector<Event>& events = eventsData.events;
            
        //send all the events
        for (Event& event : events){
            event.setEventOwnerUser(loggedUser);
            sendEvent(event);
        }
    }

            
    //summary {channel_name} {user} {file} 
    else if (command == "summary") {
        std::string channel_name, user, file;
        stream >> channel_name >> user >> file ;
        

        sendSummary(channel_name,user,file);
    }

    //logout - DISCONNECT
    else if (command == "logout") {
        StompFrame frame = StompFrame(
            "DISCONNECT",
            {
                {"receipt", std::to_string(SingletonCounter::getInstance().getNextReceipt())}
            },
            "");
        sendDisconnect(frame);

    }
    else if (command == "abort") {
        terminate = true;
        std::abort();
    }

    else {
    std::cout << "Unknown command from user"<< std::endl;
    }
}
    

bool StompProtocol::shouldTerminate() const {
    return terminate;
}

bool StompProtocol::isLoggedin() const
{
    return isConnected;
}

void StompProtocol::sendConnect(const StompFrame &frame)
{
    std::cout << "Login request sent. Waiting for CONNECTED frame." << std::endl;
    loggedUser = frame.getHeader("login");
    isConnected=true;
    sendFrame(frame);    
}

void StompProtocol::sendEvent(Event& event){
    StompFrame frame = StompFrame::parseEvent(event);
    sendFrame(frame);
}

void StompProtocol::sendSubscribe(const StompFrame& frame){
    std::lock_guard<std::mutex> lock(mutex);
    channelSubscriptions[frame.getHeader("id")] = frame.getHeader("destination");
    waitingReceipt[frame.getHeader("receipt")] = frame.getCommand();
    sendFrame(frame);
}

void StompProtocol::sendUnsubscribe(const StompFrame& frame){
    std::lock_guard<std::mutex> lock(mutex);
    waitingReceipt[frame.getHeader("receipt")] = frame.getCommand();
    channelSubscriptions.erase(frame.getHeader("id"));
    sendFrame(frame);
}
    
void StompProtocol::sendSummary(const std::string& channel_name, const std::string& user, const std::string& file) {
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
    int reportsCount=0;
    int activeCount=0;
    int forcesArrivalCount=0;
    std::map<std::string,std::string> generalInformation;
    std::ostringstream output;
    output << "Channel " << channel_name << "\n";
    output << "Stats:\n";

    //Stats counts
    for (const auto& event : events) {
        generalInformation=event.get_general_information();
        reportsCount++;
        if(generalInformation["active"]=="true")
            {activeCount++;}
        if(generalInformation["forces_arrival_at_scene"]=="true")
            {forcesArrivalCount++;}
    }

    // Add stats to output
    output << "Total: " << reportsCount << "\n";
    output << "active: " << activeCount << "\n";
    output << "forces arrival at scene: " << forcesArrivalCount << "\n";

    // Event Reports
    output << "\nEvent Reports:\n\n";

    size_t reportIndex = 1;
    for (const auto& event : events) {
        std::string description = event.get_description();
        std::string summary = description.substr(0, 27);
        if (description.size() > 27 && reportIndex<events.size() ) {
            summary += "...\n\n    ...\n\n";
        }

        // Convert date time
        std::string dateTimeString = epochToDate(event.get_date_time());

        // Add event details to the output
        output << "Report_" << reportIndex++ << ":\n";
        output << "    city: " << event.get_city() << "\n";
        output << "    date time: " << dateTimeString << "\n";
        output << "    event name: " << event.get_name() << "\n";
        output << "    summary: " << summary << "\n";
    }

    // Write the output to the file
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
std::string StompProtocol::epochToDate(time_t epoch) {
    std::tm* tm = std::localtime(&epoch);
    std::ostringstream dateStream;
    dateStream << std::put_time(tm, "%d/%m/%y %H:%M");
    return dateStream.str();
}

void StompProtocol::sendDisconnect(const StompFrame& frame){
    
    std::cout << "Logout request sent. Waiting for DISCONNECT RECEIPT num:" << frame.getHeader("receipt") << std::endl;
    {
    std::lock_guard<std::mutex> lock(mutex);
    sendFrame(frame);
    waitingReceipt[frame.getHeader("receipt")] = frame.getCommand();
    loggedUser="";
    }
}

void StompProtocol::sendFrame(StompFrame frame){
    std::string rawFrame = frame.toRawFrame();
    bool result = connectionHandler.sendLine(rawFrame);
    if (!result) {
        std::cerr << "\nFailed to send " << frame.getCommand() << " frame:\n" << std::endl;
        std::cerr << rawFrame << std::endl;
        return; // Exit without modifying state
    }
    else{
        std::cerr <<  "\nFrame " << frame.getCommand() << " sent to server:\n"<< rawFrame << std::endl;
    }
}

void StompProtocol::handleMessageFrame(StompFrame frame){

    std::string destination = frame.getHeader("destination"); // Channel name
    std::string user = frame.getHeader("user");
    std::string body = frame.getBody();                       

    //Check if valid input
    if (user.empty()) {
        std::cerr << "Error: 'user' header is missing in MESSAGE frame" << std::endl;
        return;
    }

    if (destination.empty()) {
        std::cerr << "Error: 'destination' header is missing in MESSAGE frame" << std::endl;
        return;
    }

    if (body.empty()) {
        std::cerr << "Error: 'body' header is missing in MESSAGE frame" << std::endl;
        return;
    }

    // Parse the body to extract event details
    std::istringstream bodyStream(body);
    std::string bodyUser, city, eventName, description;
    int dateTime = 0; // Default value for the date-time
    std::map<std::string, std::string> generalInformation;

    std::string line;
    while (std::getline(bodyStream, line)) {
        // Parse individual fields in the body
        if (line.find("user: ") == 0) {
            bodyUser = line.substr(6);
        } else if (line.find("city: ") == 0) {
            city = line.substr(6);
        } else if (line.find("event name: ") == 0) {
            eventName = line.substr(12);
        } else if (line.find("date time: ") == 0) {
            dateTime = std::stoi(line.substr(11));
        } else if (line.find("    active: ") == 0) {
            generalInformation["active"] = line.substr(12);
        } else if (line.find("    forces_arrival_at_scene: ") == 0) {
            generalInformation["forces_arrival_at_scene"] = line.substr(29);
        } else if (line.find("description: ") == 0) {
            description = line.substr(12);
        }
    }

    
    // Create an Event object
    Event event(destination, city, eventName , dateTime, description, generalInformation);

    // Add the event to userEvents
    std::lock_guard<std::mutex> lock(mutex); // Ensure thread safety
    if (userEvents.find(user) == userEvents.end()) {
        userEvents[user] = std::map<std::string, std::vector<Event>>();
    }
    if (userEvents[user].find(destination) == userEvents[user].end()) {
        userEvents[user][destination] = std::vector<Event>();
    }
    userEvents[user][destination].push_back(event);

    // Sort events by date, then by name
    userEvents[user][destination] = sortEvents(userEvents[user][destination]);
    
    std::cout << "\nEvent added for user '" << user << "' in channel '" << destination << "'." << std::endl;
}

std::vector<Event> StompProtocol::sortEvents(std::vector<Event> events)
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
