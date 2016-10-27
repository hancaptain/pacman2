all: CXXFLAGS += -std=c++11
all: randomplay.exe server.exe testPlayer.exe copyexe.bat

copyexe.bat: randomplay.exe server.exe
	./copyexe.bat

%.exe: %.cpp
	$(CXX) $^ -o $@ $(CXXFLAGS)
