CXX=g++
CPPFLAGS=-std=c++17 -O2

all:
	$(CXX) $(CPPFLAGS) -o screen-worms-server src/server.cpp
	$(CXX) $(CPPFLAGS) -o screen-worms-client src/client.cpp

clean:
	rm screen-worms-server screen-worms-client
