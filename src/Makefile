CC=g++
CPPFLAGS=-std=c++11 -O2 -I./shared
ServerCPPFLAGS = $(CPPFLAGS) -lpthread
# Headers
DEPS = ./shared/MessageLayer.hpp ./shared/picosha2.hpp \
	   ./server/MessagingClient.hpp ./server/Server.hpp
# Object files
MessageLayerTests = ./shared/MessageLayerTests.o \
					./shared/MessageLayer.o

MessageServer = ./shared/MessageLayer.o \
				./server/Server.o \
				./server/MessagingClient.o

.PHONY : all
all : MessageLayerTests MessageServer

%.o: %.cpp $(DEPS)
	$(CC) -c -o $@ $< $(CPPFLAGS)

MessageLayerTests: $(MessageLayerTests)
	$(CC) -o $@ $^ $(CPPFLAGS)

MessageServer: $(MessageServer)
	$(CC) -o $@ $^ $(ServerCPPFLAGS)

clean:
	$(RM) $(MessageLayerTests) $(MessageServer) ./MessageServer ./MessageLayerTests
