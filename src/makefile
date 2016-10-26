all: CXXFLAGS += -std=c++11
all: randomplay.exe server.exe copyexe.bat

randomplay.exe: randomplay.cpp gamefield.o
	$(CXX) $^ -o $@ $(CXXFLAGS)

server.exe: server.cpp
	$(CXX) $^ -o $@ $(CXXFLAGS)

copyexe.bat:
	.\copyexe.bat

%.o: %.cpp
	$(CXX) -c $^ -o $@ $(CXXFLAGS)
