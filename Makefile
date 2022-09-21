CXX=g++
CPPFLAGS=-std=c++17 -O2

all:
	$(CXX) $(CPPFLAGS) -o screen-worms-server server.cpp
	$(CXX) $(CPPFLAGS) -o screen-worms-client client.cpp

clean:
	rm screen-worms-server screen-worms-client
