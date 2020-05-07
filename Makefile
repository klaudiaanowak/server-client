CXX=gcc
CFLAGS= -c
CXXFLAGS= -std=gnu99 -Wall -Wextra
OUTPUT= transport
all: transport

transport: transport.o sliding_window.o
	$(CXX) $(CXXFLAGS) transport.o sliding_window.o -o $(OUTPUT)
transport.o: transport.c
	$(CXX) $(CFLAGS) transport.c sliding_window.h
sliding_window.o: sliding_window.c
	$(CXX) $(CFLAGS) sliding_window.c sliding_window.h 

clean:
	rm -rf *.o  *.gch

distclean:
	rm -rf *.o *.gch $(OUTPUT)
	