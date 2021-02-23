/*======================================================================
COIS-4310H Assignment 1 - MessageLayer
Name: MessageLayer.cpp
Written By:  Adam Melaney & Trevor Gilbert 
Purpose: Allow the creation, verification, and unpacking of message headers
	as a single class and interface, following the builder pattern; to handle
	everything we do in the client and server with message headers. As well
	as functionality to build strings that can be sent as message data portions.

Creation: Please use the provided Make file that will make both the
	client and the server.
----------------------------------------------------------------------*/

#include <cstdint>
#include <memory>
#include <cstring>
extern "C" {
#include <netinet/in.h>
}
#include "MessageLayer.hpp"

// From GitHub, MIT licenced
// SHA256 hashing function
#include "picosha2.hpp"

// Calculate the checksum of the contents of the header,
// and write it to the 32 bytes (256 bits) that make up
// the checksum section of the header.
void MessageLayer::calculate_sha256_sum(void)
{
	picosha2::hash256(header.begin(), header.begin() + sha256sum_begin,
			  header.begin() + sha256sum_begin, header.end());
}

// Verify the checksum of the header's contents against
// the checksum stored in the header. If they aren't the same
// there is something amiss within the header.
bool MessageLayer::verify_sha256_sum(void)
{
	// Hash the content of the header, and place it in the checksum buffer
	// to check whether it is valid.
	picosha2::hash256(header.begin(), header.begin() + sha256sum_begin,
			  checksum.begin(), checksum.end());
	// Make sure the checksums match
	return std::equal(header.begin() + sha256sum_begin, header.end(),
			  checksum.begin());
}

// Build a brand-new squeaky clean MessageLayer.
MessageLayer::MessageLayer(void)
{
	header.fill(0);
}

// Consume the moved MessageLayer passed, and take ownership
// of all its members. After this is done, the passed message layer should
// not be used again and should be considered invalid.
MessageLayer::MessageLayer(MessageLayer &&ml)
	: header(std::move(ml.header)), checksum(std::move(ml.checksum)),
	  valid(ml.valid)

{
}

// Construct a new MessageLayer using a passed message header.
// Check the validity of the passed header upon initialization.
MessageLayer::MessageLayer(MessageHeader &header_ref)
{
	// Copy over what's been passed.
	header = header_ref;
	// Make sure that the checksum is valid.
	valid = verify_sha256_sum();
}

// Constructor that takes ownership of the passed MessageHeader.
// The MessageHeader passed in should be considered invalid on its
// own after this is executed, as it is now apart of this new
// MessageLayer.
MessageLayer::MessageLayer(MessageHeader &&header_rvalue_ref)
	: header(std::move(header_rvalue_ref))
{
	valid = verify_sha256_sum();
}

// Return a reference to the internal header stored within this message
// layer. Very useful for reusing the same header over and over again.
MessageHeader &MessageLayer::get_internal_header(void)
{
	return header;
}

// Being able to verify the checksum at any time is very useful for the
// reuse of the MessageLayer.
void MessageLayer::verify_checksum(void)
{
	valid = verify_sha256_sum();
}

// Retrieve the first 2 bytes from the header
// and convert them to host format and return
uint16_t MessageLayer::get_packet_number(void)
{
	return ntohs(*((uint16_t *)&(header[packet_number_begin])));
}

// Convert the passed short to network byte order,
// and assign it to its place in the header.
MessageLayer &MessageLayer::set_packet_number(uint16_t p_num)
{
	(*((uint16_t *)&(header[packet_number_begin]))) = htons(p_num);
	return (*this);
}

uint8_t MessageLayer::get_version_number(void)
{
	return header[header_version_begin];
}

MessageLayer &MessageLayer::set_version_number(uint8_t v_num)
{
	header[header_version_begin] = v_num;
	return (*this);
}

// Internal function for setting usernames within
// the header
// Specific function, making sure the length is never longer than
// 31 bytes plus the null terminator.
void MessageLayer::set_username(const std::string &source_username,
				char *header_ptr)
{
	size_t min;
	// minus 1 is to make room for the null terminator
	if (source_username.length() < (username_len - 1))
		min = source_username.length();
	else
		min = (username_len - 1);
	// Clear the entire username field of the header
	std::memset(header_ptr, 0, username_len);
	// Put the source_username at the beginning of the
	// field in the header.
	std::memcpy(header_ptr, (source_username.c_str()), min);
	// Put a null terminator at the end of the username.
	header_ptr[min] = '\0';
}

// Index into header to the start of the source username,
// and pull the correct number of bytes (up to 32)
std::string MessageLayer::get_source_username(void)
{
	char *username = (char *)&(header[source_username_begin]);
	return build_string_safe(username, username_len);
}

// Copy passed string into the message header, and
// ensure that a null terminator is set, by setting one ourselves.
MessageLayer &
MessageLayer::set_source_username(const std::string &source_username)
{
	char *username = (char *)&(header[source_username_begin]);
	this->set_username(source_username, username);
	return (*this);
}

// Index into header to the start of the destination username,
// and pull the correct number of bytes (up to 32)
std::string MessageLayer::get_dest_username(void)
{
	char *username = (char *)&(header[dest_username_begin]);
	return build_string_safe(username, username_len);
}

// Copy passed string into the message header, and
// ensure that a null terminator is set, by setting one ourselves.
MessageLayer &
MessageLayer::set_dest_username(const std::string &source_username)
{
	char *username = (char *)&(header[dest_username_begin]);
	this->set_username(source_username, username);
	return (*this);
}

uint8_t MessageLayer::get_message_type(void)
{
	return header[67];
}

MessageLayer &MessageLayer::set_message_type(uint8_t m_type)
{
	header[message_type_begin] = m_type;
	return (*this);
}

// Retrieve the data packet length from the header
// Which is 2 bytes, 68 bytes into the header,
// and convert them to host format and return
uint16_t MessageLayer::get_data_packet_length(void)
{
	return ntohs(*((uint16_t *)&(header[data_packet_length_begin])));
}

// convert the passed short to network byte order
// and put it in it's place within the header.
MessageLayer &MessageLayer::set_data_packet_length(uint16_t data_packet_len)
{
	// Convert the passed short to network byte order,
	// and assign it to its place in the header.
	(*((uint16_t *)&(header[data_packet_length_begin]))) =
		htons(data_packet_len);
	return (*this);
}

// calculate the checksum for the header, and return a reference to
// the internal header of the MessageLayer.
// (last function called in builder pattern when setting the attributes)
MessageHeader &MessageLayer::build(void)
{
	calculate_sha256_sum();
	return header;
}

// calculate the checksum for the header, and return a copy
// of the internal header of the MessageLayer.
// (last function called in builder pattern when setting the attributes)
MessageHeader MessageLayer::build_cpy(void)
{
	calculate_sha256_sum();
	MessageHeader cpy = header;
	return cpy;
}

// Function for extracting strings using a length and a pointer.
// (Message data packets)
// using a length instead of a null terminator, hence the hopefully
// added safety.
std::string build_string_safe(const char *str, size_t len)
{
	std::string username_str = "";
	for (size_t i = 0; i < len; ++i) {
		// While there are still characters to add, continue.
		// Otherwise stop.
		if (str[i] != '\0') {
			username_str.push_back(str[i]);
		} else {
			break;
		}
	}
	return username_str;
}
