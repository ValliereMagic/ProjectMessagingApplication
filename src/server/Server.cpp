#include "MessageLayer.hpp"
#include <iostream>
extern "C" {
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
}

int main(void)
{
	// Server socket setup loosely followed from:
	//     https://www.geeksforgeeks.org/socket-programming-cc/
	// Build the socket to listen on
	int server_socket = socket(AF_INET, SOCK_STREAM, 0);
	// Make sure socket descriptor initialization was successful.
	if (server_socket == 0) {
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
	if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT,
		       &opt, sizeof(int))) {
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
	if (bind(server_socket, (sockaddr *)&address, sizeof(sockaddr_in)) <
	    0) {
		std::cerr << "Error binding to address." << std::endl;
		exit(EXIT_FAILURE);
	}
	// Set up listener for new connections
	if (listen(server_socket, 5) < 0) {
		std::cerr << "Error trying to listen for connections on socket."
			  << std::endl;
		exit(EXIT_FAILURE);
	}
	// Accept and accommodate the incoming connections
	socklen_t addr_len = sizeof(sockaddr_in);
	while (true) {
		// Accept a client connection
		int new_client_socket =
			accept(server_socket, (sockaddr *)&address, &addr_len);
		// Make sure the client connection is valid
		if (new_client_socket < 0) {
			std::cerr
				<< "Error trying to accept connections on server socket."
				<< std::endl;
			exit(EXIT_FAILURE);
		}
		// Close immediately for test.
		close(new_client_socket);
	}
	close(server_socket);
	return 0;
}
