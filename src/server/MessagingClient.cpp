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
	ml.set_data_packet_length(our_username.length());
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
		ml.verify_checksum();
		// Is the header with a valid sum?
		if (!ml.valid) {
			std::cerr << "Client message header sum is bad."
				  << std::endl;
			continue;
		}
		// Create a vector to hold the second data package if needed
		std::vector<uint8_t> data_package(ml.get_data_packet_length());
		switch (ml.get_message_type()) {
		// Another login request? But you're logged in.
		case 0: {
			send_error_message("You already logged in, dingus.\0");
			break;
		}
		// Client is sending me an error?
		case 1: {
			continue;
			break;
		}
		// Who message
		case 2: {
			// Special string with a null terminator in it
			auto usernames = get_logged_in_users();
			header.fill(0);
			ml.set_message_type(2);
			ml.set_source_username("server");
			ml.set_dest_username(our_username);
			// Set the required header information
			// Not +1, null terminator is included in
			// the string.
			ml.set_data_packet_length(usernames.size());
			ml.build();
			send_to_client(our_username,
				       build_message(header, usernames));
			break;
		}
		// Message ACK
		case 3: {
			continue;
			break;
		}
		// Actual Message or Broadcast
		case 4: {
			ssize_t read_size =
				read(client_socket, data_package.data(),
				     data_package.size());
			// Check whether the socket had an error on read
			if (read_size <= 0) {
				std::cerr
					<< "Client socket is closed, or error."
					<< std::endl;
				return;
				// Make sure we read in enough data to make up a header
			} else if (read_size < (ssize_t)data_package.size()) {
				std::cerr
					<< "Unable to read enough bytes for the whole data package."
					<< std::endl;
				continue;
			}
			// Check whether this is a broadcast or a PM
			std::string dest_username = ml.get_dest_username();
			// This is a broadcast message
			if (dest_username == "all") {
				send_to_all(our_username,
					    build_message(header,
							  data_package));
				// This is a PM
			} else {
				// Send it off to the client, sending off an error
				// to the sender if they don't exist.
				if (!(send_to_client(
					    dest_username,
					    build_message(header,
							  data_package)))) {
					send_error_message(
						"User: " + dest_username +
						" does not exist.\0");
				}
			}
			break;
		}
		// Disconnect Message
		case 5:
			header.fill(0);
			ml.set_message_type(4);
			ml.set_source_username("server");
			ml.set_dest_username("all");
			std::string leave_message =
				"User: " + our_username +
				" disconnected from the room.\0";
			ml.set_data_packet_length(leave_message.size());
			ml.build();
			send_to_all(our_username,
				    build_message(header, leave_message));
			// Return and allow login_procedure to finish
			// and clean up this client.
			return;
			break;
		}
	}
}

int MessagingClient::get_client_socket(void)
{
	return client_socket;
}
