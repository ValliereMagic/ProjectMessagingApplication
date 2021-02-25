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

// Increment and overflow packet numbers in a defined way.
// this will be useful for the coming assignments to deal
// with 'packet' loss.
uint16_t &increment_packet_number(uint16_t &num);
