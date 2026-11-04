CXX      ?= g++
CXXFLAGS ?= -std=c++17 -O2 -Wall -Wextra -pthread

all: backend/reverser

backend/reverser: backend/main.cpp
	$(CXX) $(CXXFLAGS) -o $@ $<

run: all
	cd backend && ./reverser 8080 ../frontend

clean:
	rm -f backend/reverser backend/reverser.exe
