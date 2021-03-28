CC=g++
CPPFLAGS= -Wall -std=c++11 -O2 -I./shared 
LINKFLAGS = -lpthread -z muldefs
# Headers
DEPS = ./shared/MessageLayer.hpp ./shared/picosha2.hpp \
	   ./server/MessagingClient.hpp ./server/Server.hpp \
	   ./server/SharedClients.hpp
# Object files
MessageLayerTests = ./shared/MessageLayer.o \
					./shared/MessageLayerTests.o

MessageServer = ./shared/MessageLayer.o \
				./server/Server.o \
				./server/MessagingClient.o \
				./server/SharedClients.o

MessageClient = ./shared/MessageLayer.o \
				./client/Client.o

CryptoTests = ./shared/CryptoLayer.o \
			  ./shared/CryptoLayerTests.o

.PHONY : all
all : MessageLayerTests MessageServer MessageClient CryptoTests

%.o: %.cpp $(DEPS)
	$(CC) -c -o $@ $< $(CPPFLAGS)

MessageLayerTests: $(MessageLayerTests)
	$(CC) -o $@ $^

MessageServer: $(MessageServer)
	$(CC) -o $@ $^ $(LINKFLAGS)

MessageClient: $(MessageClient)
	$(CC) -o $@ $^ $(LINKFLAGS)

CryptoTests: $(CryptoTests)
	$(CC) -o $@ $^ $(LINKFLAGS) -lsodium

clean:
	$(RM) $(MessageLayerTests) $(MessageServer) $(MessageClient) $(CryptoTests) \
	./MessageServer ./MessageLayerTests ./MessageClient ./CryptoTests
