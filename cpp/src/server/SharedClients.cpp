/*======================================================================
COIS-4310H Assignment 1 - SharedClients
Name: SharedClients.cpp
Written By:  Adam Melaney & Trevor Gilbert 
Purpose: This Class facilitates communication between MessagingClient objects.

Compilation: Please use the provided Make file that will make both the
	client and the server.

Requires the shared MessageLayer Class that is used in to create
a shared header for transit.
----------------------------------------------------------------------*/

#include <iostream>
extern "C" {
#include <sys/socket.h>
}

#include "SharedClients.hpp"

SharedClients::SharedClients(void)
{
	// Initialize the client_objects rwlock
	pthread_rwlock_init(&client_objects_lock, nullptr);
}

SharedClients::~SharedClients(void)
{
	// destroy the rwlock
	pthread_rwlock_destroy(&client_objects_lock);
}

SharedClients &SharedClients::get_instance(void)
{
	// Return the instance of SharedClients
	static SharedClients s;
	return s;
}

// Send a message to another client by username
// return false if we weren't able to send it to the recipient
// (if they don't exist.)
bool SharedClients::send_to_client(const std::string &dest_username,
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
	auto client_fd_it = client_objects.find(dest_username);
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

// Send a message to all connected clients except for ourselves.
// Using the passed username field to omit ourselves.
// (return false if we weren't able to send the message to one
// of the clients.)
bool SharedClients::send_to_all(const std::string &sender_username,
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
		if (user.first != sender_username) {
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

// Get CSV list of logged in users from the client_objects
// hash map.
std::string SharedClients::get_logged_in_users(void)
{
	// String stream to build the CSV message within
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
	// Turn the stream into a string and return it.
	return usernames.str();
}

// Add a logged in user to the client_objects map,
// and return a pointer to the newly created MessagingClient
// object.
MessagingClient *SharedClients::add_new_user(const std::string &username,
					     int client_socket,
					     int login_packet_number,
					     MessageLayer &&ml)
{
	// Create a MessagingClient, and try to insert it into the shared
	// HashMap.
	MessagingClient *messaging_client = nullptr;
	// Lock the wrlock for writing, allowing us to add this new user
	// to the system.
	if (pthread_rwlock_wrlock(&client_objects_lock) != 0) {
		std::cerr << "Unable to lock the rwlock for writing."
			  << std::endl;
		exit(EXIT_FAILURE);
	}
	// Only add it, if it doesn't already exist.
	if (client_objects.find(username) == client_objects.end()) {
		client_objects.insert(std::make_pair(
			username,
			MessagingClient(client_socket, login_packet_number,
					username, std::move(ml))));
		// variable 'ml' no longer valid after move, if this branch
		// was executed.
		// Cannot copy a client object. Only reference it and move it.
		// It is owned by client_objects, and we are now borrowing it.
		messaging_client = &(client_objects.at(username));
	}
	// Unlock the rwlock, we are done writing now.
	if (pthread_rwlock_unlock(&client_objects_lock) != 0) {
		std::cerr << "Unable to unlock the rwlock after writing."
			  << std::endl;
		exit(EXIT_FAILURE);
	}
	return messaging_client;
}

bool SharedClients::log_out_user(const std::string &username)
{
	bool success = false;
	// Lock the client_objects hashmap, to remove this exiting
	// client from the system.
	if (pthread_rwlock_wrlock(&client_objects_lock) != 0) {
		std::cerr << "Unable to lock the rwlock for writing."
			  << std::endl;
		exit(EXIT_FAILURE);
	}
	// Remove and destruct the object for this client.
	success = client_objects.erase(username);
	// Were done writing now, unlock the write lock.
	if (pthread_rwlock_unlock(&client_objects_lock) != 0) {
		std::cerr << "Unable to unlock the rwlock after writing."
			  << std::endl;
		exit(EXIT_FAILURE);
	}
	return success;
}
