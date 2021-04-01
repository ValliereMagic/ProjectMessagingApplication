/*======================================================================
COIS-4310H Assignment 1 - Client
Name: Client.cpp
Written By: Trevor Gilbert & Adam Melaney
Purpose: This is a client for a messenger application, that will use 2
	threads to to listen and send to a server. 

Usage: ./MessageClient

Description of Parameters
	None

Compilation: Please use the provided Make file that will make both the
	client and the server.

Requires the shared MessageLayer Class that is used in to create
a shared header for transit. Also requires the CryptoLayer Class.
----------------------------------------------------------------------*/

#include <iostream>
#include <thread>
#include <csignal>
#include <unordered_map>
#include <string>
#include <cstring>
#include <atomic>
#include <mutex>
#include <random>
extern "C" {
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
}
#include "MessageLayer.hpp"
#include "CryptoLayer.hpp"

#define VERSION 3
#define SERVER_ADDRESS "0.0.0.0"
#define SERVER_PORT 34551


// Mutex to protect our Unordered Map from multithreaded use 
std::mutex messages_mutex;
// List of sent messages
static std::unordered_map<uint16_t, std::vector<uint8_t> > client_messages;

// Live thread of execution. Joined on exit
static std::thread client_thread;
// Atomic bool for showing when both threads are running.
static std::atomic<bool> is_running;

// The client socket file descriptor. Global
static int client_socket_fd;

// Encryption key for the room
static Crypto::StreamKey encryption_key;

// This function is multipurposed. It is used by the sig handler to cleanup on ^c
// It is also called when the program is closing normally.
void cleanup_on_exit(int signum)
{
	// Set the atomic bool to false
	is_running = false;
	// Call a signal to the other thread to kill themself
	pthread_kill(client_thread.native_handle(), SIGUSR1);
	// Join back the client thread
	client_thread.join();
	// Close the client socket
	close(client_socket_fd);
	exit(signum);
}

// Handler SIGUSR1
// Kill off this thread
void close_thread(int signum)
{
	// Slightly dirty method to get rid of thread without being able to join
	// Returning will just put us back in socket read block.
	client_thread.detach();
	exit(0);
}

// This function is run by the thread that will receive messages from the server.
// It will wait for a message to be received and then act upon it.
void message_receiver()
{
	// Largely taken from https://devarea.com/linux-handling-signals-in-a-multithreaded-application/
	// This will make this thread ignore SIGINT calls and have the other thread take care of them.
	sigset_t mask;
	sigemptyset(&mask);
	sigaddset(&mask, SIGINT);
	pthread_sigmask(SIG_BLOCK, &mask, NULL);
	// Attach our closing handler to SIGUSR1
	signal(SIGUSR1, close_thread);
	size_t read_size;
	MessageHeader header;
	while (true) {
		// Clear out the message header
		header.fill(0);

		// Check if other thread is still running
		if (!is_running) {
			return;
		}

		// Wait for a new message from the server
		read_size =
			read(client_socket_fd, header.data(), header.size());
		// Check if other thread is still running
		if (!is_running) {
			std::cout << "Running" << std::endl;
			return;
		}

		// Check if socket is dead
		if (read_size == 0) {
			std::cerr << "Socket is closed." << std::endl;
			is_running = false;
			return;
		}
		// Check if size is correct
		else if (read_size < (ssize_t)header.size()) {
			std::cerr << "Unable to read the right amount of data."
				  << std::endl;
			continue;
		}

		// React to message!

		// Pass the header to the message layer
		MessageLayer ml(header);

		// Is the header with a valid sum?
		if (!ml.valid) {
			std::cerr << "Server header sum is bad." << std::endl;
			continue;
		}

		// Create a vector to hold the second data package if needed
		std::vector<uint8_t> data_package(ml.get_data_packet_length());

		// What type of message is it? And how to handle it.
		switch (ml.get_message_type()) {
		// Message Type - Login Request
		case (MessageTypes::LOGIN):
			std::cout << "You have logged in." << std::endl;
			break;
		// Message Type - Error
		case (MessageTypes::ERROR):
			// Wait for a new message from the server
			read_size = read(client_socket_fd, data_package.data(),
					 data_package.size());

			// Check if other thread is still running
			if (!is_running) {
				return;
			}

			// Check if socket is dead
			if (read_size == 0) {
				std::cerr << "Socket is closed." << std::endl;
				is_running = false;
				return;
			}

			// Check if size is correct
			else if (read_size <
				 (ssize_t)ml.get_data_packet_length()) {
				std::cerr
					<< "Unable to read the right amount of data."
					<< std::endl;
				continue;
			}

			// Put the error message to console.
			std::cout << "Error - "
				  << build_string_safe(
					     (char *)data_package.data(),
					     ml.get_data_packet_length())
				  << std::endl;

			break;

		// Message Type - Who
		case (MessageTypes::WHO):
			// Wait for a new message from the server
			read_size = read(client_socket_fd, data_package.data(),
					 data_package.size());

			// Check if other thread is still running
			if (!is_running) {
				return;
			}

			// Check if socket is dead
			if (read_size == 0) {
				std::cerr << "Socket is closed." << std::endl;
				is_running = false;
				return;
			}

			// Check if size is correct
			else if (read_size <
				 (ssize_t)ml.get_data_packet_length()) {
				std::cerr
					<< "Unable to read the right amount of data."
					<< std::endl;
				continue;
			}

			// Put the message to console.
			std::cout << "Users - "
				  << build_string_safe(
					     (char *)data_package.data(),
					     ml.get_data_packet_length())
				  << std::endl;

			break;

		// Message Type - Message Acknowledge
		case (MessageTypes::ACK): {
				// Critical section that must be run under lock
				// Grab Ownership of the Mutex and lock
				const std::lock_guard<std::mutex> lock(messages_mutex);
				// Clear the acknowledged packet from our list
				if (client_messages.erase(ml.get_packet_number()) ==
					0) {
					std::cerr
						<< "Server acknowledged a packet already acknowledged."
						<< std::endl;
				}

				// Mutex Guard will deconstruct when leaving scope, thus freeing lock on mutex
			}
			continue;
			break;
		// Message Type - Message
		case (MessageTypes::MESSAGE): {
			// Wait for a new message from the server
			read_size = read(client_socket_fd, data_package.data(),
					 data_package.size());

			// Check if other thread is still running
			if (!is_running) {
				return;
			}

			// Check if socket is dead
			if (read_size == 0) {
				std::cerr << "Socket is closed." << std::endl;
				is_running = false;
				return;
			}

			// Check if size is correct
			else if (read_size <
				 (ssize_t)ml.get_data_packet_length()) {
				std::cerr
					<< "Unable to read the right amount of data."
					<< std::endl;
				continue;
			}
			
			// Check if its an unencrypted server message
			if (ml.get_source_username() == "server") {
				// Output message from server
				if (ml.get_dest_username() == "all")
					std::cout
						<< "(Room) " << ml.get_source_username()
						<< " says > "
						<< build_string_safe(
					     (char *)data_package.data(),
					     ml.get_data_packet_length())
						<< std::endl;
				else
					std::cout
						<< ml.get_source_username()
						<< " whispers to you > "
						<< build_string_safe(
					     (char *)data_package.data(),
					     ml.get_data_packet_length())
						<< std::endl;
			}
			// Else its an encrypted message
			else {
				// Decrypt the message
				// Returns a tuple with a bool as value 0 and a string value 1
				auto decrypted_message = Crypto::decrypt(data_package, encryption_key);

				// Check if key was able decrypt message
				if (std::get<0>(decrypted_message) == false) {
					std::cout << "Message from " << ml.get_source_username() 
					<< " not able to decrypt." << std::endl;
					continue;
				}

				// Output message
				if (ml.get_dest_username() == "all")
					std::cout
						<< "(Room) " << ml.get_source_username()
						<< " says > "
						<< std::get<1>(decrypted_message)
						<< std::endl;
				else
					std::cout
						<< ml.get_source_username()
						<< " whispers to you > "
						<< std::get<1>(decrypted_message)
						<< std::endl;
			}

			break;
		}
		// Message Type - Disconnect
		case (MessageTypes::DISCONNECT):
			std::cout << "Server has disconnected you."
				  << std::endl;
			is_running = false;
			return;
			break;
		// Message Type No Acknowledge
		case (MessageTypes::NACK): {
			// Critical section that must be run under lock
			// Grab Ownership of the Mutex and lock
			const std::lock_guard<std::mutex> lock(messages_mutex);

			// Find the location to the packet
			auto full_message =
				client_messages.find(ml.get_packet_number());
			// Check if we found the packet.
			if (full_message == client_messages.end()) {
				std::cerr
					<< "Server Sent a NACK for a packet we don't have."
					<< std::endl;
				is_running = false;
				return;
			}

			// Send the vector to the server. With no flags. Check to make sure sent.
			if (send(client_socket_fd,
				 (full_message->second).data(),
				 (full_message->second).size(), 0) == -1) {
				is_running = false;
				return;
			}
			
			// Mutex Guard will deconstruct when leaving scope, thus freeing lock on mutex
		} break;
		// Unsupported Message Type
		default:
			std::cerr << "Unsupported Message Type." << std::endl;
		}
	}
}

// This function
void console_help()
{
	std::cout << "Help" << std::endl;
	std::cout << "====" << std::endl;
	std::cout << "help                      - this message" << std::endl;
	std::cout
		<< "message <username> <message>  - send a message to username"
		<< std::endl;
	std::cout
		<< "message all <message>         - send a message to the room"
		<< std::endl;
	std::cout
		<< "who                             - find out who is in the room"
		<< std::endl;
	std::cout
		<< "exit                            - exit the room (and the program)"
		<< std::endl;
}

// This function is used for the thread running the send portion of the client.
// It will get input from the user and create a message from it to send to the
// server.
void message_sender()
{
	// Set our random distributions.
	std::random_device random_device("default");
	std::uniform_int_distribution<int> message_messing_chance(1, 6);
	signal(SIGUSR2, cleanup_on_exit);
	std::string username;
	std::string input;
	std::string message;
	std::string recipient;
	std::string password;
	std::size_t position;
	std::size_t position2;
	std::uint16_t packet_number = 0;
	
	// Get username from the user
	std::cout << "What will your username be(31 Max):";
	std::cin >> username;

	// Get the password from the user
	std::cout << "Enter the password for the server:";
	std::cin >> password;
	auto derived_key = Crypto::derive_key_from_password(password);

	// Check if key was successfully derived
	if (std::get<0>(derived_key) == false) {
		std::cerr << "Failure to derive key" << std::endl;
		// Cleanup and exit
		cleanup_on_exit(EXIT_FAILURE);
	}
	
	// Set the global encryption_key with our derived key
	// Returns a tuple with a bool as value 0 and a Streamkey as value 1
	encryption_key = std::get<1>(derived_key);

	// Create message header for login
	MessageLayer header_1;
	MessageHeader &header = header_1.set_packet_number(packet_number)
					.set_version_number(VERSION)
					.set_source_username(username)
					.set_dest_username("server")
					.set_message_type(MessageTypes::LOGIN)
					.set_data_packet_length(0)
					.build();

	// Update the packet number
	packet_number++;
	// Send the message to the server. With no flags. Check to make sure sent.
	if (send(client_socket_fd, (void *)(header.data()), header.size(), 0) ==
	    -1) {
		std::cerr << "Failure to send login through socket"
			  << std::endl;
		// Cleanup and exit
		cleanup_on_exit(EXIT_FAILURE);
	}

	// Display the instructions for input.
	console_help();

	// Flush the \n from cin
	std::cin.ignore();

	// Loop for user input
	while (true) {
		// Get user input
		std::cout << ">";
		getline(std::cin, input);

		// Check if read thread still exists
		if (!is_running) {
			std::cout << "Disconnected from server" << std::endl;
			cleanup_on_exit(EXIT_SUCCESS);
		}

		// Find the first space
		position = input.find(" ");

		// If size is less than 3, its not a valid command.
		if (input.size() < 3) {
			std::cout
				<< "That is not a proper command. Type 'help' for options."
				<< std::endl;
			continue;
		}

		// Command contains no " ", so must be 'who' 'exit' 'help' or invalid.
		if (position == std::string::npos) {
			// Check if its 'who'
			if (input.compare(0, 3, "who") == 0) {
				// Create a who packet
				MessageHeader &header =
					header_1.set_packet_number(
							packet_number)
						.set_version_number(VERSION)
						.set_source_username(username)
						.set_dest_username("server")
						.set_message_type(
							MessageTypes::WHO)
						.set_data_packet_length(0)
						.build();

				// Update the packet number
				packet_number++;

				// Send the message to the server. With no flags. Check to make sure sent.
				if (send(client_socket_fd,
					 (void *)(header.data()), header.size(),
					 0) == -1) {
					// Cleanup and exit
					cleanup_on_exit(EXIT_FAILURE);
				}

				// Iterate and get next input
				continue;
			}
			// Check if its 'exit'
			else if (input.size() > 3 &&
				 (input.compare(0, 4, "exit") == 0)) {
				// Create a bye packet

				MessageHeader &header =
					header_1.set_packet_number(
							packet_number)
						.set_version_number(VERSION)
						.set_source_username(username)
						.set_dest_username("server")
						.set_message_type(
							MessageTypes::DISCONNECT)
						.set_data_packet_length(0)
						.build();

				// Update the packet number
				packet_number++;

				// Send the message to the server. With no flags. Check to make sure sent.
				if (send(client_socket_fd,
					 (void *)(header.data()), header.size(),
					 0) == -1) {
					// Cleanup and exit
					cleanup_on_exit(EXIT_FAILURE);
				}
				is_running = false;
				// Clean and kill both threads
				cleanup_on_exit(0);
			}
			// Check if its 'help'
			else if (input.size() > 3 &&
				 (input.compare(0, 4, "help") == 0)) {
				console_help();
				continue;
			}
			// Must be invalid then
			else {
				std::cout
					<< "That is not a proper command. Type 'help' for options."
					<< std::endl;
				continue;
			}
		}
		// Contains a space, so is it a message command?
		else if (input.compare(0, position, "message") == 0) {
			// If the string isn't big enough, must not have included a recipient
			if (input.size() < 8) {
				std::cout
					<< "You specify the recipient. Type 'help' for options."
					<< std::endl;
				continue;
			}
			// Find the second space (the one after username)
			position2 = input.find(" ", 8);
			// If string isn't big enough to have a message or no second space
			if (position == std::string::npos ||
			    (input.size() < (position2 + 1))) {
				std::cout
					<< "You did not specify a message. Type 'help' for options."
					<< std::endl;
				continue;
			}
			// Check if the recipient was 'all'
			else if (input.compare(8, position2 - position - 1,
					       "all") == 0) {
				message = input.substr(position2 + 1,
						       std::string::npos);
				recipient = "all";
			}
			// Otherwise must be a personal message
			else {
				recipient = input.substr(8, (position2 - 8));
				message = input.substr(position2 + 1,
						       std::string::npos);
			}
			// Execute the same whether personal or all

			// Make the string  resemble a cstring
			message.append("\0");

			// Encrypt the message
			// Returns a tuple with a bool as value 0 and a vector value 1
			auto encrypted_message = Crypto::encrypt(message, encryption_key);

			// Check if encryption was able to be completed.		
			if (std::get<0>(encrypted_message) == false) {
				std::cerr << "Unable to encrypt" << std::endl;
				cleanup_on_exit(EXIT_FAILURE);
			}

			// Create an personal message
			MessageHeader &header =
				header_1.set_packet_number(packet_number)
					.set_version_number(VERSION)
					.set_source_username(username)
					.set_dest_username(recipient)
					.set_message_type(MessageTypes::MESSAGE)
					.calculate_data_packet_checksum(
						std::get<1>(encrypted_message)) // Add the checksum for the data
					.set_data_packet_length(
						std::get<1>(encrypted_message).size())
					.build();

			// Concatenate the vectors to a super vector
			auto full_message = build_message(header, std::get<1>(encrypted_message));
			
			// Critical Section that must be run under lock
			{
				// Grab Ownership of the Mutex and lock
				const std::lock_guard<std::mutex> lock(messages_mutex);

				// See if the Packet Num is already active
				if (client_messages.find(packet_number) !=
					client_messages.end()) {
					std::cerr
						<< "Unable to add Message to list, packet # already in use."
						<< std::endl;
					cleanup_on_exit(EXIT_FAILURE);
				}
				// Otherwise add the full packet to the list
				else {
					client_messages.insert(std::make_pair(
						packet_number, full_message));
				}
			// Leave scope to remove the lock
			}

			// 1/6 chance of message mockery
			if (message_messing_chance(random_device) == 1) {
				// Changing first letter of the message to 'a'
				if (full_message.size() > 167) {
					// Obviously won't produce an NAK if started with a anyways
					full_message[166] = 'a';
					std::cout << "First letter of Message changed to 'a' to test NACK" << std::endl;
				}
			}

			// Send the vector to the server. With no flags. Check to make sure sent.
			if (send(client_socket_fd, full_message.data(),
				 full_message.size(), 0) == -1) {
				// Cleanup and exit
				cleanup_on_exit(EXIT_FAILURE);
			}

			// Update the packet number
			packet_number++;

			// Sent, so iterate to the next user command.
			continue;
		}
		// Contains a space but the command is not message. Must not be proper
		else {
			std::cout
				<< "That is not a proper command. Type 'help' for options."
				<< std::endl;
			continue;
		}
	}
}

int main(void)
{
	// Attach our cleanup handler to SIGINT
	signal(SIGINT, cleanup_on_exit);
	is_running = true;
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

	if (inet_pton(AF_INET, SERVER_ADDRESS, &(address.sin_addr)) <= 0) {
		std::cerr << "Error building IPV4 Address." << std::endl;
		exit(EXIT_FAILURE);
	}

	// Create a connection to the server.
	if (connect(client_socket_fd, (sockaddr *)&address,
		    sizeof(sockaddr_in)) < 0) {
		std::cerr << "Error could not connect to server." << std::endl;
		exit(EXIT_FAILURE);
	}

	// Create a thread to receive
	client_thread = std::thread(message_receiver);
	// Check if thread was created
	if (!client_thread.joinable()) {
		std::cerr << "Error could not create thread." << std::endl;
		exit(EXIT_FAILURE);
	}
	// This one will be the sender
	message_sender();
}
