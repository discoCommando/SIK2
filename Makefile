TARGETS	= client server 
CXX	= g++
CXXFLAGS=  -std=c++0x -I ../../boost_1_55_0/

all: $(TARGETS) 

client: chat_client.cpp
	$(CXX) $(CXXFLAGS) $^ -o $@ -lboost_system -lboost_program_options -lpthread

server: chat_server.cpp
	$(CXX) $(CXXFLAGS) $^ -o $@ -lboost_system -lboost_program_options -lpthread


.PHONY: clean TARGET
clean:
	rm -f server client *.o *~ *.bak
