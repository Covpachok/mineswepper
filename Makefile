CXX = g++
CXXFLAGS = -Wall -g -lncurses
OBJMODULES = 

%.o: %.cpp %.hpp
		$(CXX) $(CXXFLAGS) -c $< -o $@

sapper: main.cpp $(OBJMODULES)
		$(CXX) $(CXXFLAGS) $^ -o $@
