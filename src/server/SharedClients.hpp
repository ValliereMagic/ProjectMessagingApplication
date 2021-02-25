/*======================================================================
COIS-4310H Assignment 1 - SharedClients
Name: SharedClients.hpp
Written By:  Adam Melaney & Trevor Gilbert 
Purpose: This Class facilitates communication between MessagingClient objects.

Compilation: Please use the provided Make file that will make both the
	client and the server.

Requires the shared MessageLayer Class that is used in to create
a shared header for transit.
----------------------------------------------------------------------*/

#pragma once
#include <pthread.h>
#include <unordered_map>

#include "MessagingClient.hpp"

class SharedClients {
	// client_objects map accessable from all client threads
	// Thread safe access from the functions implemented in this file
	// using posix rw_locks.
	pthread_rwlock_t client_objects_lock;
	std::unordered_map<std::string, MessagingClient> client_objects;
	SharedClients(void);
	~SharedClients(void);

    public:
	// Do not allow assignment operations, and copy construction
	SharedClients(SharedClients const &) = delete;
	void operator=(SharedClients const &) = delete;
	// Retrieve the single instance of SharedClients (Its a Singleton)
	static SharedClients &get_instance(void);
	// Send a message to another client by username
	// return false if we weren't able to send it to the recipient
	// (if they don't exist.)
	bool send_to_client(const std::string &dest_username,
			    const std::vector<uint8_t> &message);
	// Send a message to all connected clients except for ourselves.
	// Using the passed username field to omit ourselves.
	// (return false if we weren't able to send the message to one
	// of the clients.)
	bool send_to_all(const std::string &sender_username,
			 const std::vector<uint8_t> &message);
	// Get CSV list of logged in users from the client_objects
	// hash map.
	std::string get_logged_in_users(void);
	// Add a logged in user to the client_objects map,
	// and return a pointer to the newly created MessagingClient
	// object.
	MessagingClient *add_new_user(const std::string &username,
				      int client_socket,
				      int login_packet_number,
				      MessageLayer &&ml);
	// Log out a user from the server
	bool log_out_user(const std::string &username);
};
