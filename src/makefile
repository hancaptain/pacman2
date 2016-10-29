all: CXXFLAGS += -std=c++11
all: Foreseeable_Amadeus_edited.exe greedy.exe huaji.exe qr_pacman_edited.exe randomplay.exe server.exe test.exe testPlayer.exe Waltz_wy_greedy_0_edited.exe copyexe.bat

copyexe.bat: randomplay.exe server.exe
	./copyexe.bat

%.exe: %.cpp
	$(CXX) $^ -o $@ $(CXXFLAGS)
