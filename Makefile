CC=gcc
CXX=g++
RM=rm -f
CPPFLAGS = -Wall -Wextra -pedantic
CFDIR = clientfiles/
SERVER_DIR = serverfiles/

all:server.o client.o 

server.o : $(SERVER_DIR)server.cpp $(SERVER_DIR)filetracker.o $(SERVER_DIR)filetracker.h
	$(CXX) -o server.o $(SERVER_DIR)server.cpp $(SERVER_DIR)filetracker.cpp -lssl -lcrypto -pthread $(CPPFLAGS)


client.o : $(CFDIR)client.cpp
	$(CXX) -o client.o $(CFDIR)client.cpp -lssl -lcrypto -pthread $(CPPFLAGS)


clean:
	$(RM) $(SERVER_DIR)*.o
	$(RM) $(CLIENT_DIR)*.o
