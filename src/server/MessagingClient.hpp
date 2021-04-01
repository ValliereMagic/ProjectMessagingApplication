/*======================================================================
COIS-4310H Assignment 1 - MessagingClient Header
Name: MessagingClient.hpp
Written By: Trevor Gilbert & Adam Melaney
Purpose: This object represents the main receive thread of the server.
	Each client connected, has an instance of this class running on the server
	executing the 'client' method. When client is returned from, it means
	this client is disconnecting.

Compilation: Please use the provided Make file that will make both the
	client and the server.

Requires the shared MessageLayer Class that is used in to create
a shared header for transit.
----------------------------------------------------------------------*/

#pragma once
#include "MessageLayer.hpp"
// Forward declared to avoid circular dependency
class SharedClients;

class MessagingClient {
	const int client_socket;
	const std::string our_username;
	// This packet number will need to be atomic once we
	// actually need it. Right now, its going to have some
	// interesting values.
	uint16_t packet_number;
	MessageLayer ml;
	// Shared clients instance for talking to other connected clients.
	SharedClients &sc;
	// Send error messages to the client
	bool send_error_message(const std::string &message);
	// Send verification message back to the client (ACK or NACK)
	bool send_verification_message(const MessageTypes &type,
				       const uint16_t &packet_number_recv);

    public:
	// MessageHeader Version
	static const int version = 3;

	MessagingClient(int client_socket, uint16_t packet_number,
			const std::string &our_username, MessageLayer &&ml);
	MessagingClient(MessagingClient &&client);
	// Handled by the thread that creates and runs this object on
	// accept. Handles messages sent to the server from the client.
	void client(void);
	// Ability to retrieve the socket fd of another thread.
	// Accessed through rwlock from other threads
	int get_client_socket(void);
};
