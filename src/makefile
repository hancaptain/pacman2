all: CXXFLAGS += -std=c++11
all: Foreseeable_Amadeus_edited.exe greedy.exe huaji.exe randomplay.exe server.exe test.exe testPlayer.exe copyexe.bat

copyexe.bat: randomplay.exe server.exe
	./copyexe.bat

%.exe: %.cpp
	$(CXX) $^ -o $@ $(CXXFLAGS)
