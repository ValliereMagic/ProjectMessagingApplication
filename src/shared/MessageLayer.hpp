/*======================================================================
COIS-4310H Assignment 1 - MessageLayer
Name: MessageLayer.hpp
Written By:  Adam Melaney & Trevor Gilbert 
Purpose: Allow the creation, verification, and unpacking of message headers
	as a single class and interface, following the builder pattern; to handle
	everything we do in the client and server with message headers. As well
	as functionality to build strings that can be sent as message data portions.

Creation: Please use the provided Make file that will make both the
	client and the server.
----------------------------------------------------------------------*/

#pragma once
#include <vector>
#include <string>
#include <array>

using MessageHeader = std::array<uint8_t, 166>;

class MessageLayer {
	// Message header for communications between the client and server.
	// 166 bytes
	MessageHeader header;
	// Checksum verifying the integrity of the header.
	std::array<uint8_t, 32> checksum;
	// Calculate the checksum of the contents of the header,
	// and write it to the 32 bytes (256 bits) that make up
	// the checksum section of the header.
	void calculate_sha256_sum(void);
	// Verify the checksum of the header's contents against
	// the checksum stored in the header. If they aren't the same
	// there is something amiss within the header.
	bool verify_sha256_sum(void);
	// Internal function for setting usernames within
	// the header
	// Specific function, making sure the length is never longer than
	// 31 bytes plus the null terminator.
	static inline void set_username(const std::string &source_username,
					char *header_ptr);

    public:
	// If this is false you probably shouldn't try to send this...
	// It means the checksum is bad.
	bool valid = true;
	// Build a brand-new squeaky clean MessageLayer.
	MessageLayer(void);
	// Consume the moved MessageLayer passed, and take ownership
	// of all its members. After this is done, the passed message layer should
	// not be used again and should be considered invalid.
	MessageLayer(MessageLayer &&ml);
	// Construct a new MessageLayer using a passed message header.
	// Check the validity of the passed header upon initialization.
	MessageLayer(MessageHeader &header_ref);
	// Constructor that takes ownership of the passed MessageHeader.
	// The MessageHeader passed in should be considered invalid on its
	// own after this is executed, as it is now apart of this new
	// MessageLayer.
	MessageLayer(MessageHeader &&header_rvalue_ref);

	// Return a reference to the internal header stored within this message
	// layer. Very useful for reusing the same header over and over again.
	MessageHeader &get_internal_header(void);
	// Being able to verify the checksum at any time is very useful for the
	// reuse of the MessageLayer.
	void verify_checksum(void);
	// Retrieve the first 2 bytes from the header
	// and convert them to host format and return
	uint16_t get_packet_number(void);
	// Convert the passed short to network byte order,
	// and assign it to its place in the header.
	MessageLayer &set_packet_number(uint16_t p_num);
	uint8_t get_version_number(void);
	MessageLayer &set_version_number(uint8_t v_num);
	// Index into header to the start of the source username,
	// and pull the correct number of bytes (up to 32)
	std::string get_source_username(void);
	// Copy passed string into the message header, and
	// ensure that a null terminator is set, by setting one ourselves.
	MessageLayer &set_source_username(const std::string &source_username);
	// Index into header to the start of the destination username,
	// and pull the correct number of bytes (up to 32)
	std::string get_dest_username(void);
	// Copy passed string into the message header, and
	// ensure that a null terminator is set, by setting one ourselves.
	MessageLayer &set_dest_username(const std::string &source_username);
	uint8_t get_message_type(void);
	MessageLayer &set_message_type(uint8_t m_type);
	// Retrieve the data packet length from the header
	// Which is 2 bytes, 68 bytes into the header,
	// and convert them to host format and return
	uint16_t get_data_packet_length(void);
	// convert the passed short to network byte order
	// and put it in it's place within the header.
	MessageLayer &set_data_packet_length(uint16_t data_packet_len);

	// Additional stuff to be added for future use here

	// calculate the checksum for the header, and return a reference to
	// the internal header of the MessageLayer.
	// (last function called in builder pattern when setting the attributes)
	MessageHeader &build(void);
	// calculate the checksum for the header, and return a copy
	// of the internal header of the MessageLayer.
	// (last function called in builder pattern when setting the attributes)
	MessageHeader build_cpy(void);
};

// Function for extracting strings using a length and a pointer.
// (Message data packets)
// using a length instead of a null terminator, hence the hopefully
// added safety.
std::string build_string_safe(const char *str, size_t len);
// Template function to build a message from a container and a header
template <typename T>
std::vector<uint8_t> build_message(const MessageHeader &message_header,
				   const T &message)
{
	// construct the entire message to send
	std::vector<uint8_t> message_to_send;
	// Reserve the message to be the correct size
	message_to_send.reserve(message_header.size() + message.size());
	// Put the header in first
	message_to_send.insert(message_to_send.end(), message_header.begin(),
			       message_header.end());
	// Put in the string
	message_to_send.insert(message_to_send.end(), message.begin(),
			       message.end());
	return message_to_send;
}
