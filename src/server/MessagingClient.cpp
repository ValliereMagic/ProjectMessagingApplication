#include <iostream>
#include "Server.hpp"
#include "MessagingClient.hpp"
extern "C" {
#include <unistd.h>
}

MessagingClient::MessagingClient(int client_socket, uint16_t packet_number,
				 std::string &our_username, MessageLayer &&ml)
	: client_socket(client_socket), our_username(our_username),
	  packet_number(packet_number), ml(std::move(ml))
{
}

MessagingClient::MessagingClient(MessagingClient &&client)
	: client_socket(client.client_socket),
	  our_username(std::move(client.our_username)),
	  packet_number(client.packet_number), ml(std::move(client.ml))
{
}

bool MessagingClient::send_error_message(const std::string &message)
{
	// Clear the header
	ml.get_internal_header().fill(0);
	// Set the required header information
	MessageHeader &header =
		ml.set_message_type(1)
			.set_version_number(MessagingClient::version)
			.set_packet_number(
				increment_packet_number(packet_number))
			.set_dest_username(our_username)
			.set_data_packet_length(message.length())
			.build();
	auto message_to_send = build_message(header, message);
	return send_to_client(our_username, message_to_send);
}

void MessagingClient::client(void)
{
	std::cout << "Started Receiving thread for client: " << our_username
		  << std::endl;
	// Retrieve our message header for writing
	MessageHeader &header = ml.get_internal_header();
	// Send out a message that I have logged in
	std::string login_message =
		"User: " + our_username + " entered the room.\0";
	// Header clear
	header.fill(0);
	// Fill out header and build.
	ml.set_message_type(4)
		.set_version_number(MessagingClient::version)
		.set_packet_number(increment_packet_number(packet_number))
		.set_source_username("server")
		.set_dest_username("all")
		.set_data_packet_length(login_message.length())
		.build();
	send_to_all(our_username, build_message(header, login_message));
	// The main receive loop
	while (true) {
		// Re-clean the header after sending our login message
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
			// Header clear
			header.fill(0);
			// Set the required header information
			ml.set_message_type(2)
				.set_version_number(MessagingClient::version)
				.set_packet_number(
					increment_packet_number(packet_number))
				.set_source_username("server")
				.set_dest_username(our_username)
				// Not +1, null terminator is included in
				// the string.
				.set_data_packet_length(usernames.size())
				.build();
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
			// Read in the data portion of the header
			ssize_t read_size =
				read(client_socket, data_package.data(),
				     data_package.size());
			// Check whether the socket had an error on read
			if (read_size <= 0) {
				std::cerr
					<< "Client socket is closed, or error."
					<< std::endl;
				return;
				// Make sure we read in enough data to make up the data packet
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
			std::string leave_message =
				"User: " + our_username +
				" disconnected from the room.\0";
			// Clear the header
			header.fill(0);
			// Set the header information
			ml.set_message_type(4)
				.set_version_number(MessagingClient::version)
				.set_packet_number(
					increment_packet_number(packet_number))
				.set_source_username("server")
				.set_dest_username("all")
				.set_data_packet_length(leave_message.size())
				.build();
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
