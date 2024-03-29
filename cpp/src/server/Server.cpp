/*======================================================================
COIS-4310H Assignment 1 - Server
Name: Server.cpp
Written By:  Adam Melaney & Trevor Gilbert 
Purpose: This is a server for a messenger application, that will use one
	thread for each client connecting.

Usage: ./MessageServer

Description of Parameters
	None

Creation: Please use the provided Make file that will make both the
client and the server.

Requires the shared MessageLayer Class that is used in to create
a shared header for transit.
----------------------------------------------------------------------*/

#include <iostream>
#include <thread>
#include <csignal>
extern "C" {
#include <unistd.h>
#include <arpa/inet.h>
}
#include "SharedClients.hpp"

// The server socket file descriptor. Global to this translation unit
// so that the cleanup signal handler can close it.
static int server_socket_fd;

// On exit, this function is called to close the server_socket_fd
// and destroy the rwlock.
void cleanup_on_exit(int signum)
{
	// Close the server socket
	close(server_socket_fd);
	exit(signum);
}

// Increment and overflow packet numbers in a defined way.
// this will be useful for the coming assignments to deal
// with 'packet' loss.
uint16_t &increment_packet_number(uint16_t &num)
{
	num = (num + 1) % UINT16_MAX;
	return num;
}

// Setup the login procedure, which requires a write to the
// client_objects map. This function is called in a new thread
// whenever a new client connects to the server. It logs the client
// in, and lets them know if they were successful or not.

// If they successfully log in, a MessagingClient is made, and the client()
// method is executed, which waits for more messages from the client
// (the main loop).

// If they failed to login, the failure message will be sent to the
// client, and their socket will be closed, then this thread will exit.
static void login_procedure(int client_socket)
{
	// Packet numbers for the login sequence
	uint16_t login_packet_number = 1;
	// See if the client is trying to login:
	MessageHeader header;
	// zero out the header
	header.fill(0);
	// Read in what is supposed to be a login request...
	if (read(client_socket, header.data(), header.size()) <
	    (ssize_t)(header.size())) {
		std::cerr << "Initial Client header is too short; or error."
			  << std::endl;
		close(client_socket);
		return;
	}
	// Pass the header to the message layer
	MessageLayer ml(std::move(header));
	// variable 'header' no longer valid after move.
	// Is the header with a valid sum?
	if (!ml.valid) {
		std::cerr << "Initial Client header sum is bad." << std::endl;
		close(client_socket);
		return;
	}
	// Is the header a login request message?
	if (ml.get_message_type() != 0) {
		std::cerr << "Message is not a login request." << std::endl;
		close(client_socket);
		return;
	}
	// Were good. Pull the username.
	std::string username = ml.get_source_username();
	// Build the login response message early, so we can move
	// the MessageLayer to MessagingClient on creation.
	ml.get_internal_header().fill(0);
	MessageHeader login_response =
		ml.set_version_number(MessagingClient::version)
			.set_packet_number(login_packet_number)
			.set_message_type(MessageTypes::LOGIN)
			.set_dest_username(username)
			.build_cpy();
	// Get the instance of SharedClients (Singleton)
	SharedClients &sc = SharedClients::get_instance();
	// Add the user to the system
	MessagingClient *messaging_client = sc.add_new_user(
		username, client_socket, login_packet_number, std::move(ml));
	// Make sure we didn't find a duplicate username while
	// we were within the write lock:
	// if this statement is entered, it means that ml wasn't moved
	// into messaging_client, and we can use it in here to send
	// the error message to the client.
	if (messaging_client == nullptr) {
		std::cerr << "Client: " << username << " already exists."
			  << std::endl;
		// The user was unable to get logged in due to their
		// bad username.
		// Clear the header
		ml.get_internal_header().fill(0);
		// Set the required header information
		std::string error_message = "Invalid username to login with.\0";
		MessageHeader &header =
			ml.set_message_type(MessageTypes::ERROR)
				.set_version_number(MessagingClient::version)
				.set_packet_number(login_packet_number)
				.set_dest_username(username)
				.set_data_packet_length(error_message.length())
				.build();
		// build the data portion of the message, and combine it
		// with the header.
		auto message_to_send = build_message(header, error_message);
		// Send off the error message to the client
		if (send(client_socket, message_to_send.data(),
			 message_to_send.size(),
			 0) < (ssize_t)message_to_send.size()) {
			std::cerr << "Error sending login error to client."
				  << std::endl;
		}
		// Goodbye duplicate client.
		close(client_socket);
		return;
	}
	// The client was able to successfully login.
	// Send back the login verification we built before the write lock began
	if (send(client_socket, login_response.data(), login_response.size(),
		 0) < (ssize_t)login_response.size()) {
		std::cerr
			<< "Unable to send back the login verification message."
			<< std::endl;
		close(client_socket);
		return;
	}
	// This thread becomes the client thread in MessagingClient.
	// Begin receiving messages from the client.
	messaging_client->client();
	// When we return to here, it means we are done with the
	// connection to this client.
	// remove the client from the system
	sc.log_out_user(username);
	// Close the client connection.
	close(client_socket);
}

// Set up the server socket to listen to client connections,
// and spawn new threads for each new accepted client connection.
int main(void)
{
	// Attach our cleanup handler to SIGINT
	signal(SIGINT, cleanup_on_exit);
	// Server socket setup loosely followed from:
	//     https://www.geeksforgeeks.org/socket-programming-cc/
	// Build the socket to listen on
	server_socket_fd = socket(AF_INET, SOCK_STREAM, 0);
	// Make sure socket descriptor initialization was successful.
	if (server_socket_fd == 0) {
		std::cerr << "Failed to Initialize server socket descriptor."
			  << std::endl;
		exit(EXIT_FAILURE);
	}
	// Set the socket options to allow bind to succeed even if there
	// are still connections open on this address and port.
	// (Important on restarts of the server daemon)
	// Explained in depth here:
	//     https://stackoverflow.com/questions/3229860/what-is-the-meaning-of-so-reuseaddr-setsockopt-option-linux
	int opt = 1; // (true)
	if (setsockopt(server_socket_fd, SOL_SOCKET,
		       SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(int))) {
		std::cerr
			<< "Failed to modify the socket options of the server socket."
			<< std::endl;
		exit(EXIT_FAILURE);
	}
	// Build our address
	sockaddr_in address = { .sin_family = AF_INET,
				.sin_port = htons(34551) };
	// Convert our ip address string to the required binary format.
	if (inet_pton(AF_INET, "0.0.0.0", &(address.sin_addr)) <= 0) {
		std::cerr << "Error building IPV4 Address." << std::endl;
		exit(EXIT_FAILURE);
	}
	// Attach our socket to our address
	if (bind(server_socket_fd, (sockaddr *)&address, sizeof(sockaddr_in)) <
	    0) {
		std::cerr << "Error binding to address." << std::endl;
		exit(EXIT_FAILURE);
	}
	// Set up listener for new connections
	if (listen(server_socket_fd, 5) < 0) {
		std::cerr << "Error trying to listen for connections on socket."
			  << std::endl;
		exit(EXIT_FAILURE);
	}
	// Accept and accommodate the incoming connections
	socklen_t addr_len = sizeof(sockaddr_in);
	while (true) {
		// Accept a client connection
		int new_client_socket = accept(server_socket_fd,
					       (sockaddr *)&address, &addr_len);
		// Make sure the client connection is valid
		if (new_client_socket < 0) {
			std::cerr
				<< "Error trying to accept connections on server socket."
				<< std::endl;
			exit(EXIT_FAILURE);
		}
		// Set up the client thread for this connection.
		std::thread(login_procedure, new_client_socket).detach();
	}
	return 0;
}
