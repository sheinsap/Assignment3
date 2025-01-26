#include <iostream>
#include <thread>
#include <mutex>
#include "../include/ConnectionHandler.h"
#include "../include/StompProtocol.h"
#include "../include/StompClient.h"

void StompClient::handleUserInput() {
    while (!terminate && !protocol.shouldTerminate() ) {
        std::string command;
        std::getline(std::cin, command);

        std::lock_guard<std::mutex> lock(mutex);
        if (!protocol.isLoggedin() && command.substr(0, 5) != "login") {

            std::cout << "You must log in first with the `login` command." << std::endl;
            continue;
        }
        protocol.processFromUser(command); 
    }
}

void StompClient::handleServerResponses() {
    while (!terminate && !protocol.shouldTerminate()) {
        std::string response;
        if (protocol.isLoggedin()) {            
            if (connectionHandler.getFrameAscii(response,'\0')){
                protocol.processFromServer(response);

                if(protocol.isLoggedin()==false)
                {
                    std::cout << "Waiting for login" << std::endl;
                }
            }
        }
    }
}

StompClient::StompClient()
    : connectionHandler("127.0.0.1", 7777), 
        protocol(connectionHandler), gotCONNECTED(false), terminate(false), mutex() {}

void StompClient::run() {
    // Start thread for server response handling
    std::thread serverResponseThread(&StompClient::handleServerResponses, this);

    // Main thread handles user input
    handleUserInput();
    
    serverResponseThread.join();

    // Close the connection gracefully
    connectionHandler.close();
    std::cout << "Disconnected from the server." << std::endl;
}


int main(int argc, char *argv[]) {
    StompClient client;
    client.run();
    return 0;
}