/*======================================================================
COIS-4310H Assignment 1 - MessageLayerTests
Name: MessageLayerTests.cpp
Written By:  Adam Melaney & Trevor Gilbert 
Purpose: Test the creation, verification, and unpacking of message headers
	as a single class and interface, following the builder pattern; to handle
	everything we do in the client and server with message headers.

Usage: ./MessageLayerTests
	(No output means the tests passed)
	if there are assertion errors, the tests failed.

Description of Parameters
	None

Creation: Please use the provided Make file that will make both the
	client and the server.
----------------------------------------------------------------------*/

#include <cassert>
#include <iostream>
#include "MessageLayer.hpp"

int main(void)
{
	MessageLayer header_1;
	MessageHeader &header = header_1.set_packet_number(1)
					.set_version_number(1)
					.set_source_username("BananaSoup")
					.set_dest_username("Blargato_Man")
					.set_message_type(4)
					.set_data_packet_length(26)
					.build();
	// Make sure the header was set correctly
	assert(header_1.get_packet_number() == 1);
	assert(header_1.get_version_number() == 1);
	assert(header_1.get_source_username() == "BananaSoup");
	assert(header_1.get_dest_username() == "Blargato_Man");
	assert(header_1.get_message_type() == 4);
	assert(header_1.get_data_packet_length() == 26);
	// Make sure the header verifies on copy constructor
	MessageLayer header_2(header);
	assert(header_2.valid);
	assert(header_1.valid);
	// Move and verify
	MessageLayer header_3(std::move(header));
	// 'header' variable is now invalid.
	assert(header_3.valid);
	// Try move constructor of the entire MessageLayer, and
	// do a recalculation to make sure its valid.
	MessageLayer header_4(std::move(header_3));
	auto &internal_header = header_4.build();
	assert(header_4.valid);
	// Make sure that a bad checksum is caught
	header_4.set_message_type(0);
	MessageLayer header_5(internal_header);
	assert(!(header_5.valid));
	// Test the data_packet checksum system
	std::string message = "banana bread\0";
	header_5.calculate_data_packet_checksum(message);
	assert(header_5.verify_data_packet_checksum(message));
	assert(!(header_5.verify_data_packet_checksum<std::string>(
		"banana soup\0")));
}
