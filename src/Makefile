CC=g++
CPPFLAGS=-std=c++11 -O2 -I./shared
# Headers
DEPS = ./shared/MessageLayer.hpp ./shared/picosha2.hpp
# Object files
MessageLayerTests = ./shared/MessageLayerTests.o \
					./shared/MessageLayer.o

MessageLayerServer = ./shared/MessageLayer.o \
					 ./server/Server.o

.PHONY : all
all : MessageLayerTests MessageLayerServer

%.o: %.cpp $(DEPS)
	$(CC) -c -o $@ $< $(CPPFLAGS)

MessageLayerTests: $(MessageLayerTests)
	$(CC) -o $@ $^ $(CPPFLAGS)

MessageLayerServer: $(MessageLayerServer)
	$(CC) -o $@ $^ $(CPPFLAGS)

clean:
	$(RM) $(MessageLayerTests) $(MessageLayerServer) ./MessageLayerServer ./MessageLayerTests