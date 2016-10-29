all: CXXFLAGS += -std=c++11
all: NameKei.exe upload.exe

%.exe: %.cpp
	$(CXX) $^ -o $@ $(CXXFLAGS)
