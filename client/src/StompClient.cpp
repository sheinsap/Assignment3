#include <iostream>
#include <thread>
#include <mutex>
#include "../include/ConnectionHandler.h"
#include "../include/StompProtocol.h"
#include "../include/StompClient.h"

    void StompClient::handleUserInput() {
        //lock terminate??
        while (!terminate && !protocol.isTerminate() ) {
            std::string command;
            std::getline(std::cin, command);

            std::lock_guard<std::mutex> lock(mutex);
            if (!isConnected && command.substr(0, 5) != "login") {
                std::cout << "You must log in first with the `login` command." << std::endl;
                continue;
            }

            protocol.processFromUser(command);

            if (command == "logout") {
                std::lock_guard<std::mutex> lock(mutex);
                isConnected = false;
            }
        }
    }

    void StompClient::handleServerResponses() {
        while (!terminate && !protocol.isTerminate()) {
            std::cout << "Checking termination condition: terminate=" << terminate
              << ", protocol.isTerminate()=" << protocol.isTerminate() << std::endl;
            std::string response;

            if (connectionHandler.getFrameAscii(response,'\0')){
                // std::cout << "Received frame from server: " << response << std::endl;
                protocol.processFromServer(response);

            if (response.find("CONNECTED") != std::string::npos) {
                std::lock_guard<std::mutex> lock(mutex);
                isConnected = true;
                std::cout << "Successfully connected to the server!" << std::endl;
            } else if (response.find("ERROR") != std::string::npos) {
                std::cerr << "Error received: " << response << std::endl;
                std::lock_guard<std::mutex> lock(mutex);
                // terminate = true;
            }
        } else if (!connectionHandler.getLine(response)) {
                std::cerr << "Connection lost with the server." << std::endl;
                terminate = true;
                break;
            }
        }
    }

StompClient::StompClient()
        // : connectionHandler("stomp.cs.bgu.ac.il", 7777),
        : connectionHandler("127.0.0.1", 7777), 
          protocol(connectionHandler), isConnected(false), terminate(false), mutex() {}

 void StompClient::run() {
        // Attempt initial connection to the server
        if (!connectionHandler.connect()) {
            std::cerr << "Failed to connect to the server at stomp.cs.bgu.ac.il:7777" << std::endl;
            return;
        }
        
        // Start thread for server response handling
        std::thread serverResponseThread(&StompClient::handleServerResponses, this);
        // Wait for the connection to be established
        while (!isConnected) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    
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



// void readInput(ConnectionHandler &connectionHandler) {
//     // std::string line;
//     while (true) {
// 		const short bufsize = 1024;
//         char buf[bufsize];
//         std::cin.getline(buf, bufsize);
// 		std::string line(buf);
// 		int len=line.length();
//         // std::getline(std::cin, line);
//         if (line == "logout") {
//             // Graceful shutdown logic
// 			// std::string frame = StompProtocol::constructDisconnectFrame();

//             connectionHandler.sendLine(line);  // Send logout frame
//             break;
//         }
//         if (!connectionHandler.sendLine(line)) {
//             std::cerr << "Error sending message to server" << std::endl;
//             break;
//         }
// 		connectionHandler.sendLine(line);
// 		// connectionHandler.sendLine(line) appends '\n' to the message. Therefor we send len+1 bytes.
//         std::cout << "Sent " << len+1 << " bytes to server" << std::endl;
//     }
// }
