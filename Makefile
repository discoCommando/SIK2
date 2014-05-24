TARGETS	= client server 
CXX	= g++
CXXFLAGS=  -std=c++11 -Wall 

all: $(TARGETS) 

client: client.cpp
	$(CXX) $(CXXFLAGS) $^ -o $@  -lboost_system -lboost_program_options -lpthread

server: server.cpp
	$(CXX) $(CXXFLAGS) $^ -o $@ -lboost_system -lboost_program_options -lpthread


.PHONY: clean TARGET
clean:
	rm -f server client *.o *~ *.bak
