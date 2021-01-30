#pragma once
#include "MessageLayer.hpp"

class MessagingClient {
	// MessageHeader Version
	static constexpr int const &version = 1;
	const int client_socket;
	const std::string our_username;
	MessageLayer ml;

    public:
	MessagingClient(int client_socket, std::string &our_username,
			MessageLayer &&ml);
	MessagingClient(MessagingClient &&client);
	// Handled by the thread that creates and runs this object on
	// accept. Handles messages sent to the server from the client.
	void client(void);
	// Ability to retrieve the socket fd of another thread.
	// Accessed through rwlock from other threads
	int get_client_socket(void);
};
