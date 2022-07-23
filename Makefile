CXX       = g++
CXXFLAGS   = -W -Wall -O3 -std=c++14
DEVFLAGS = -W -Wall -ggdb -O0 -std=c++14
EXEC     = run

all:
	$(CXX) $(CXXFLAGS) -o $(EXEC) *.cpp
dev:
	$(CXX) $(DEVFLAGS) -o $(EXEC) *.cpp
