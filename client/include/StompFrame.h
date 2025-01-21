#pragma once

#include <string>
#include <map>
#include <sstream>
#include <iostream>

class StompFrame {
private:
    std::string command;
    std::map<std::string, std::string> headers;
    std::string body;

public:
    // Constructor
    StompFrame(const std::string& command, const std::map<std::string, std::string>& headers, const std::string& body);

    // Getters
    std::string getCommand() const;
    std::string getHeader(const std::string& key) const;
    std::string getBody() const;

    // Static method to parse a raw frame string into a StompFrame object
    //static StompFrame parse(const std::string& frame);
    StompFrame parseFromServer(const std::string& frame);

    // Convert the StompFrame object to a raw frame string
    std::string toRawFrame() const;

    static StompFrame parseEvent(Event& event);
    
};
