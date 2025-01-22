#include <iostream>
#include <thread>
#include <mutex>
#include "../include/ConnectionHandler.h"
#include "../include/StompProtocol.h"

class StompClient{
    private:
    ConnectionHandler connectionHandler;
    StompProtocol protocol;
    bool isConnected;
    bool terminate;
    std::mutex mutex;

    void handleUserInput();
    void handleServerResponses();
    
    public:
    StompClient();
    void run();

};