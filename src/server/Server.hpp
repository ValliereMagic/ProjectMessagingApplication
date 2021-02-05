/*======================================================================
COIS-4310H Assignment 1 - Server
Name: Server.hpp
Written By:  Adam Melaney & Trevor Gilbert 
Purpose: This is the header for Server.hpp
	It is mostly used in MessagingClient.cpp for talking to and acquiring
	information about other clients.

Creation: Please use the provided Make file that will make both the
	client and the server.

Requires the shared MessageLayer Class that is used in to create
a shared header for transit.
----------------------------------------------------------------------*/

#pragma once
#include <vector>
#include <string>
#include "MessageLayer.hpp"

// Send a message to another client by username
// return false if we weren't able to send it to the recipient
// (if they don't exist.)
bool send_to_client(const std::string &username,
		    const std::vector<uint8_t> &message);
// Send a message to all connected clients except for ourselves.
// Using the passed username field to omit ourselves.
// (return false if we weren't able to send the message to one
// of the clients.)
bool send_to_all(const std::string &our_username,
		 const std::vector<uint8_t> &message);
// Get CSV list of logged in users from the client_objects
// hash map.
std::string get_logged_in_users(void);
// Increment and overflow packet numbers in a defined way.
// this will be useful for the coming assignments to deal
// with 'packet' loss.
uint16_t &increment_packet_number(uint16_t &num);
