/*======================================================================
COIS 4310 Assignment 1 - server
Name: server.cpp
Written By:  Adam Melaney & Trevor Gilbert 
Purpose: This is a server for a messenger application, that will use one
thread for each client connecting.

usage: ./MessageServer

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
#include <unordered_map>
#include <sstream>
extern "C" {
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
}
#include "MessagingClient.hpp"

// client_objects map accessable from all client threads
// Thread safe access from the functions implemented in this file
// using posix rw_locks.
static pthread_rwlock_t client_objects_lock;
static std::unordered_map<std::string, MessagingClient> client_objects;
// The server socket file descriptor. Global so that the cleanup signal
// handler can close it.
static int server_socket_fd;

void cleanup_on_exit(int signum)
{
	// destroy the rwlock
	pthread_rwlock_destroy(&client_objects_lock);
	// Close the server socket
	close(server_socket_fd);
	exit(signum);
}

// Send a message to all connected clients except for ourselves.
bool send_to_all(const std::string &our_username,
		 const std::vector<uint8_t> &message)
{
	// Open the lock for reading
	if (pthread_rwlock_rdlock(&client_objects_lock) != 0) {
		std::cerr << "Unable to lock the rwlock for reading."
			  << std::endl;
		exit(EXIT_FAILURE);
	}
	bool send_success = true;
	// Send the message to each client
	for (auto &user : client_objects) {
		// Don't send it to ourselves
		if (user.first != our_username) {
			int client_fd = (user.second).get_client_socket();
			// Send the message
			if (send(client_fd, message.data(), message.size(), 0) <
			    (ssize_t)message.size()) {
				std::cerr
					<< "Unable to send a message to a client socket."
					<< std::endl;
				send_success = false;
			}
		}
	}
	// Close the lock for reading
	if (pthread_rwlock_unlock(&client_objects_lock) != 0) {
		std::cerr << "Unable to unlock the rwlock after reading."
			  << std::endl;
		exit(EXIT_FAILURE);
	}
	return send_success;
}

// Send a message to another client by username
bool send_to_client(const std::string &username,
		    const std::vector<uint8_t> &message)
{
	// Open the lock for reading
	if (pthread_rwlock_rdlock(&client_objects_lock) != 0) {
		std::cerr << "Unable to lock the rwlock for reading."
			  << std::endl;
		exit(EXIT_FAILURE);
	}

	// Check if the user exists. If it does,
	// send them the message.
	auto client_fd_it = client_objects.find(username);
	bool send_success = true;
	if (client_fd_it != client_objects.end()) {
		// get the file discriptor for the client we are sending
		// a message to.
		int client_fd = (client_fd_it->second).get_client_socket();
		// Send the message
		if (send(client_fd, message.data(), message.size(), 0) <
		    (ssize_t)message.size()) {
			std::cerr
				<< "Unable to send a message to a client socket."
				<< std::endl;
			send_success = false;
		}
	} else {
		send_success = false;
	}

	// Close the lock for reading
	if (pthread_rwlock_unlock(&client_objects_lock) != 0) {
		std::cerr << "Unable to unlock the rwlock after reading."
			  << std::endl;
		exit(EXIT_FAILURE);
	}
	return send_success;
}

// Get CSV list of logged in users
std::string get_logged_in_users(void)
{
	std::stringstream usernames;
	// Open the lock for reading
	if (pthread_rwlock_rdlock(&client_objects_lock) != 0) {
		std::cerr << "Unable to lock the rwlock for reading."
			  << std::endl;
		exit(EXIT_FAILURE);
	}
	// Add all the usernames to CSV string
	for (auto &user : client_objects) {
		usernames << user.first << ", ";
	}
	// add null terminator
	usernames << '\0';
	// Close the lock for reading
	if (pthread_rwlock_unlock(&client_objects_lock) != 0) {
		std::cerr << "Unable to unlock the rwlock after reading."
			  << std::endl;
		exit(EXIT_FAILURE);
	}
	return usernames.str();
}

uint16_t &increment_packet_number(uint16_t &num)
{
	num = (num + 1) % UINT16_MAX;
	return num;
}

// Setup the login procedure, which requires a write to the
// client_objects map.
static void login_procedure(int client_socket)
{
	// Packet numbers for the login sequence
	uint16_t login_packet_number = 1;
	// See if the client is trying to login:
	MessageHeader header;
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
	ml.set_version_number(MessagingClient::version)
		.set_packet_number(login_packet_number)
		.set_message_type(0)
		.set_dest_username(username);
	MessageHeader login_response = ml.build_cpy();

	// Create a MessagingClient, and insert it into the shared
	// HashMap.
	bool user_duplicate = false;
	MessagingClient *messaging_client = nullptr;

	if (pthread_rwlock_wrlock(&client_objects_lock) != 0) {
		std::cerr << "Unable to lock the rwlock for writing."
			  << std::endl;
		exit(EXIT_FAILURE);
	}

	// See if the username is already active
	if (client_objects.find(username) != client_objects.end()) {
		user_duplicate = true;
	} else {
		client_objects.insert(std::make_pair(
			username,
			MessagingClient(client_socket, login_packet_number,
					username, std::move(ml))));
		// variable 'ml' no longer valid after move.
		// Cannot copy a client object. Only reference it and move it.
		// It is owned by client_objects, and we are now borrowing it.
		messaging_client = &(client_objects.at(username));
	}

	if (pthread_rwlock_unlock(&client_objects_lock) != 0) {
		std::cerr << "Unable to unlock the rwlock after writing."
			  << std::endl;
		exit(EXIT_FAILURE);
	}
	// Make sure we didn't find a duplicate username while
	// we were within the write lock:
	if (messaging_client == nullptr || user_duplicate == true) {
		std::cerr << "Client: " << username << " already exists."
			  << std::endl;
		// The user was unable to get logged in due to their
		// bad username.
		// Clear the header
		ml.get_internal_header().fill(0);
		// Set the required header information
		std::string error_message = "Invalid username to login with.\0";
		MessageHeader &header =
			ml.set_message_type(1)
				.set_version_number(MessagingClient::version)
				.set_packet_number(login_packet_number)
				.set_dest_username(username)
				.set_data_packet_length(error_message.length())
				.build();
		auto message_to_send = build_message(header, error_message);
		// Send off the error message to the client
		if (send(client_socket, message_to_send.data(),
			 message_to_send.size(),
			 0) < (ssize_t)message_to_send.size()) {
			std::cerr << "Error sending login error to client."
				  << std::endl;
		}
		close(client_socket);
		return;
	}

	// Send back the login verification we built before the lock began
	if (send(client_socket, login_response.data(), login_response.size(),
		 0) < (ssize_t)login_response.size()) {
		std::cerr
			<< "Unable to send back the login verification message."
			<< std::endl;
		close(client_socket);
		return;
	}

	// This thread becomes the client thread in MessagingClient.
	messaging_client->client();
	// When we return to here, it means we are done with the
	// connection to this client.
	if (pthread_rwlock_wrlock(&client_objects_lock) != 0) {
		std::cerr << "Unable to lock the rwlock for writing."
			  << std::endl;
		exit(EXIT_FAILURE);
	}
	// Remove and destruct the object for this client.
	client_objects.erase(username);

	if (pthread_rwlock_unlock(&client_objects_lock) != 0) {
		std::cerr << "Unable to unlock the rwlock after writing."
			  << std::endl;
		exit(EXIT_FAILURE);
	}
	// Close the client connection.
	close(client_socket);
}

int main(void)
{
	// Initialize the client_objects rwlock
	pthread_rwlock_init(&client_objects_lock, nullptr);
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
