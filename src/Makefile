CC=g++
CPPFLAGS=-O2 -std=c++11 -I./shared
# Headers
DEPS = ./shared/MessageLayer.hpp ./shared/picosha2.hpp
# Object files
MessageLayerTests = ./shared/MessageLayerTests.o \
					./shared/MessageLayer.o


%.o: %.cpp $(DEPS)
	$(CC) -c -o $@ $< $(CPPFLAGS)

MessageLayerTests: $(MessageLayerTests)
	$(CC) -o $@ $^ $(CPPFLAGS)
