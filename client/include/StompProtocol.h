#pragma once

#include "../include/ConnectionHandler.h"

// TODO: implement the STOMP protocol
class StompProtocol
{
private: bool terminate;
public:
    // Constructor
    StompProtocol();
    std::string process(const std::string& msg);
};
