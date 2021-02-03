#include <iostream>
#include "Server.hpp"
#include "MessagingClient.hpp"
extern "C" {
#include <unistd.h>
}

MessagingClient::MessagingClient(int client_socket, std::string &our_username,
				 MessageLayer &&ml)
	: client_socket(client_socket), our_username(our_username),
	  ml(std::move(ml))
{
}

MessagingClient::MessagingClient(MessagingClient &&client)
	: client_socket(client.client_socket),
	  our_username(std::move(client.our_username)), ml(std::move(client.ml))
{
}

bool MessagingClient::send_error_message(const std::string &message)
{
	ml.get_internal_header().fill(0);
	ml.set_message_type(1);
	ml.set_dest_username(our_username);
	// Set the required header information
	ml.set_data_packet_length(our_username.length() + 1);
	MessageHeader &header = ml.build();
	auto message_to_send = build_message(header, message);
	return send_to_client(our_username, message_to_send);
}

void MessagingClient::client(void)
{
	std::cout << "Started Receiving thread for client: " << our_username
		  << std::endl;
	// Retrieve our message header for writing
	MessageHeader &header = ml.get_internal_header();
	// The main receive loop
	while (true) {
		header.fill(0);
		// Wait for a new message from the client
		ssize_t read_size =
			read(client_socket, header.data(), header.size());
		// Check whether the socket had an error on read
		if (read_size <= 0) {
			std::cerr << "Client socket is closed, or error."
				  << std::endl;
			return;
			// Make sure we read in enough data to make up a header
		} else if (read_size < (ssize_t)header.size()) {
			std::cerr
				<< "Unable to read enough bytes for a full header."
				<< std::endl;
			continue;
		}
		// Parse the header
		ml.recalculate_checksum();
		// Is the header with a valid sum?
		if (!ml.valid) {
			std::cerr << "Client message header sum is bad."
				  << std::endl;
			continue;
		}
		// Create a vector to hold the second data package if needed
		std::vector<uint8_t> data_package(ml.get_data_packet_length());
		switch (ml.get_message_type()) {
		case 0:
			break;
		}
	}
}

int MessagingClient::get_client_socket(void)
{
	return client_socket;
}
