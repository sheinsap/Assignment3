#include <iostream>
#include <thread>
#include <mutex>
#include "../include/ConnectionHandler.h"
#include "../include/StompProtocol.h"

void readInput(ConnectionHandler &connectionHandler) {
    // std::string line;
    while (true) {
		const short bufsize = 1024;
        char buf[bufsize];
        std::cin.getline(buf, bufsize);
		std::string line(buf);
		int len=line.length();
        // std::getline(std::cin, line);
        if (line == "logout") {
            // Graceful shutdown logic
			// std::string frame = StompProtocol::constructDisconnectFrame();

            connectionHandler.sendLine(line);  // Send logout frame
            break;
        }
        if (!connectionHandler.sendLine(line)) {
            std::cerr << "Error sending message to server" << std::endl;
            break;
        }
		connectionHandler.sendLine(line);
		// connectionHandler.sendLine(line) appends '\n' to the message. Therefor we send len+1 bytes.
        std::cout << "Sent " << len+1 << " bytes to server" << std::endl;
    }
}

void readServerMessages(ConnectionHandler &connectionHandler) {
    std::string answer;
	int len;
    while (true) {
        if (!connectionHandler.getLine(answer)) {
            std::cerr << "Disconnected from server" << std::endl;
            break;
        }
        std::cout << "Server Response: " << answer << std::endl;
        // Additional parsing/handling of STOMP frames can be added here

		len=answer.length();
		// A C string must end with a 0 char delimiter.  When we filled the answer buffer from the socket
		// we filled up to the \n char - we must make sure now that a 0 char is also present. So we truncate last character.
        answer.resize(len-1);
        std::cout << "Reply: " << answer << " " << len << " bytes " << std::endl << std::endl;

		// StompProtocol::parseFrame(answer);
        if (answer == "bye") {
            std::cout << "Exiting...\n" << std::endl;
            break;
        }

    }
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << " host port" << std::endl << std::endl;
        return -1;
    }
    std::string host = argv[1];
    short port = atoi(argv[2]);
    
    ConnectionHandler connectionHandler(host, port);
    if (!connectionHandler.connect()) {
        std::cerr << "Cannot connect to " << host << ":" << port << std::endl;
        return 1;
    }
	

    std::thread inputThread(readInput, std::ref(connectionHandler));
    std::thread serverThread(readServerMessages, std::ref(connectionHandler));

    inputThread.join();
    serverThread.join();

    return 0;
}


// int main(int argc, char *argv[]) {
// 	// TODO: implement the STOMP client
// 	return 0;
// }