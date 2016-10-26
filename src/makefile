all: CXXFLAGS += -std=c++11
all: randomplay.exe server.exe copyexe.bat

randomplay.exe: randomplay.cpp gamefield.o
	$(CXX) $^ -o $@ $(CXXFLAGS)

server.exe: server.cpp gamefield.o
	$(CXX) $^ -o $@ $(CXXFLAGS)

copyexe.bat: randomplay.exe server.exe
	./copyexe.bat

%.o: %.cpp
	$(CXX) -c $^ -o $@ $(CXXFLAGS)
