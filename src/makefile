all: CXXFLAGS += -std=c++11
all: randomplay.exe server.exe testPlayer.exe copyexe.bat

copyexe.bat: randomplay.exe server.exe
	./copyexe.bat

%.exe: %.o gamefield.o
	$(CXX) $^ -o $@ $(CXXFLAGS)

%.o: %.cpp
	$(CXX) -c $^ -o $@ $(CXXFLAGS)
