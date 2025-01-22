#ifndef STOMP_PROTOCOL_H
#define STOMP_PROTOCOL_H

#include "../include/ConnectionHandler.h"
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

// TODO: implement the STOMP protocol
class StompProtocol
{
bool terminate;
bool isConnected;
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
    StompProtocol(ConnectionHandler& handler);
    // std::string process(const std::string& msg);
    void processFromServer(const std::string& input);
    void processFromUser(const std::string& input);
    bool isTerminate() const;

    void sendConnect(const StompFrame& frame);
    void sendEvent(Event& event);
    void sendSubscribe(const StompFrame& frame);
    void sendUnsubscribe(const StompFrame& frame);
    void sendSummary(const std::string& channel_name, const std::string& user, const std::string& file);
    std::string epochToDate(time_t epoch);
    void sendDisconnect(const StompFrame& frame);
    std::vector<Event> sortEvents(std::vector<Event> events);
};
#endif