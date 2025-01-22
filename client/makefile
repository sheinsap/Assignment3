CFLAGS:=-c -Wall -Weffc++ -g -std=c++11 -Iinclude
LDFLAGS:=-lboost_system -lpthread

all: StompWCIClient

# Build StompWCIClient executable
StompWCIClient: bin/ConnectionHandler.o bin/StompClient.o bin/event.o bin/StompFrame.o bin/StompProtocol.o bin/SingletonCounter.o
	g++ -o bin/StompWCIClient bin/ConnectionHandler.o bin/StompClient.o bin/event.o bin/StompFrame.o bin/StompProtocol.o bin/SingletonCounter.o $(LDFLAGS)

# Object files for each source file
bin/ConnectionHandler.o: src/ConnectionHandler.cpp include/ConnectionHandler.h
	g++ $(CFLAGS) -o bin/ConnectionHandler.o src/ConnectionHandler.cpp

bin/event.o: src/event.cpp include/event.h
	g++ $(CFLAGS) -o bin/event.o src/event.cpp

bin/StompClient.o: src/StompClient.cpp include/StompClient.h include/StompProtocol.h
	g++ $(CFLAGS) -o bin/StompClient.o src/StompClient.cpp

bin/StompFrame.o: src/StompFrame.cpp include/StompFrame.h
	g++ $(CFLAGS) -o bin/StompFrame.o src/StompFrame.cpp

bin/StompProtocol.o: src/StompProtocol.cpp include/StompProtocol.h include/StompFrame.h
	g++ $(CFLAGS) -o bin/StompProtocol.o src/StompProtocol.cpp

bin/SingletonCounter.o: src/SingletonCounter.cpp include/SingletonCounter.h
	g++ $(CFLAGS) -o bin/SingletonCounter.o src/SingletonCounter.cpp

.PHONY: clean
clean:
	rm -f bin/*