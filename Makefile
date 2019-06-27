CXX=clang++
CXXFLAGS=-Wall -g -std=c++11

all: qrkit

qrkit: qrkit.o qrencoder.o qrgrid.o bitstream.o
	$(CXX) $(CXXFLAGS) -o $@ $^

%.o: %.cc
	$(CXX) -c $(CXXFLAGS) -o $@ $<

clean:
	rm -f *.o qrkit
