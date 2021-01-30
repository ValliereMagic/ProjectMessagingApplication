#include "MessageLayer.hpp"
#include "MessagingClient.hpp"
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

// Live threads of execution. Joined on exit
static std::vector<std::thread> client_threads;
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
	// Join back all the client threads
	for (auto &th : client_threads) {
		th.join();
	}
	// destroy the rwlock
	pthread_rwlock_destroy(&client_objects_lock);
	// Close the server socket
	close(server_socket_fd);
	exit(signum);
}

// Lookup client's fd by username using the rwlock
// within client_objects
// -1 if the client doesn't exist!
int lookup_client(std::string &username)
{
	int client_fd = -1;
	// Open the lock for reading
	if (pthread_rwlock_rdlock(&client_objects_lock) != 0) {
		std::cerr << "Unable to lock the rwlock for reading."
			  << std::endl;
		exit(EXIT_FAILURE);
	}

	// Check if the user exists. If it does,
	// return its socket fd. Otherwise, don't bother.
	auto client_fd_it = client_objects.find(username);
	if (client_fd_it != client_objects.end()) {
		client_fd = (client_fd_it->second).get_client_socket();
	}

	// Close the lock for reading
	if (pthread_rwlock_unlock(&client_objects_lock) != 0) {
		std::cerr << "Unable to unlock the rwlock after reading."
			  << std::endl;
		exit(EXIT_FAILURE);
	}

	return client_fd;
}

// Setup the login procedure, which requires a write to the
// client_objects map.
static void login_procedure(int client_socket)
{
	// See if the client is trying to login:
	MessageHeader header;
	header.fill(0);
	// Read in what is supposed to be a login request...
	read(client_socket, header.data(), header.size());
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
			username, MessagingClient(client_socket, username)));
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
		client_threads.push_back(std::move(
			std::thread(login_procedure, new_client_socket)));
	}
	return 0;
}
