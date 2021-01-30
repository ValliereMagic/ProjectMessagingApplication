#include "MessageLayer.hpp"
#include <iostream>
#include <thread>
#include <csignal>
#include <unordered_map>
extern "C" {
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
}

#define VERSION 1
#define SERVER_ADDRESS "0.0.0.0"
#define SERVER_PORT 34551

// Live threads of execution. Joined on exit
static std::vector<std::thread> client_threads;


// The client socket file descriptor. Global 
static int client_socket_fd;


void cleanup_on_exit(int signum)
{
	// Join back all the client threads
	for (auto &th : client_threads) {
		th.join();
	}
	// Close the client socket
	close(client_socket_fd);
	exit(signum);
}

// This function is run by the thread that will recieve messages from the server.
// It will wait for a message to be recieved and then act upon it.
void message_reciever()
{
	MessageHeader header;
	while (true) {
		// Clear out the message header
		header.fill(0);
		// Wait for a new message from the server
		read(client_socket_fd, header.data(), header.size());

		// React to message!

		// Pass the header to the message layer
		MessageLayer ml(header);

		// Is the header with a valid sum?
		if (!ml.valid) {
			std::cerr << "Server header sum is bad." << std::endl;
			continue;
		}

		// What type of message is it? And how to handle it.
		switch(ml.get_message_type())
		{
			// Message Type - Login Request
			case 0:
				break;
			// Message Type - Error
			case 1:
				break;
			// Message Type - Who
			case 2:
				break;
			// Message Type - Message Acknowledge
			case 3:
				break;
			// Message Type - Message
			case 4:
				break;
			// Message Type - Disconnect
			case 5:
				break;
			// Unsupported Message Type
			default:
				break;
		}

	}


}


// This function 
void console_help()
{
	std::cout << "Help" << std::endl;
    std::cout << "====" << std::endl;
	std::cout << "help                            - this message" << std::endl;
	std::cout << "message <username> : <message>  - send a message to username" << std::endl;
	std::cout << "message all : <message>         - send a message to the room" << std::endl;
	std::cout << "who                             - find out who is in the room" << std::endl;
	std::cout << "exit                            - exit the room (and the program)" << std::endl;
}

// This function is used for the thread running the send portion of the client.
// It will get input from the user and create a message from it to send to the
// server. 
void message_sender()
{
	std::string username;
	// Get username from the user
	std::cout << "What will your username be(31 Max):";
	std::cin >> username;

	// Create
	MessageLayer header_1;
	MessageHeader &header = header_1.set_packet_number(1)
					.set_version_number(VERSION)
					.set_source_username(username)
					.set_message_type(0)
					.set_data_packet_length(0)
					.build();
	// Send the message to the server. With no flags
	send(client_socket_fd , (void *)(header.data()), header.size(), 0 );
	

	// Loop for user input
	while (true) {

	}
}


int main(void)
{
	// Attach our cleanup handler to SIGINT
	signal(SIGINT, cleanup_on_exit);
    
    // Client socket setup loosely followed from:
	//     https://www.geeksforgeeks.org/socket-programming-cc/
	// Build the socket to listen on
	client_socket_fd = socket(AF_INET, SOCK_STREAM, 0);
	// Make sure socket descriptor initialization was successful.
	if (client_socket_fd == 0) {
		std::cerr << "Failed to Initialize client socket descriptor."
			  << std::endl;
		exit(EXIT_FAILURE);
	}
    

	// Build our address
	sockaddr_in address = { .sin_family = AF_INET,
				.sin_port = htons(SERVER_PORT) };

	if (inet_pton(AF_INET, SERVER_ADDRESS , &(address.sin_addr)) <= 0) {
		std::cerr << "Error building IPV4 Address." << std::endl;
		exit(EXIT_FAILURE);
	}

    // Create a connection to the server. 
    if (connect(client_socket_fd, (sockaddr *)&address,  sizeof(sockaddr_in)) < 
        0) {
        std::cerr << "Error could not connect to server." << std::endl;
        exit(EXIT_FAILURE);
    }

	// Create a thread to recieve
	client_threads.push_back(std::move(std::thread(message_reciever)));
	// This one will be the sender
	message_sender();
}