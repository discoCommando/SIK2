TARGETS	= klient serwer 
CXX	= g++
CXXFLAGS=  -std=c++0x -Wall

all: $(TARGETS) 

klient: klient.cpp
	$(CXX) $(CXXFLAGS) $^ -o $@  -lboost_system -lboost_program_options -lpthread

serwer: serwer.cpp
	$(CXX) $(CXXFLAGS) $^ -o $@ -lboost_system -lboost_program_options -lpthread


.PHONY: clean TARGET
clean:
	rm -f serwer klient *.o *~ *.bak
