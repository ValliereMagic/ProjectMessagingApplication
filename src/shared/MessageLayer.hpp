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

// From GitHub, MIT licenced
// SHA256 hashing function
#include "picosha2.hpp"

// Maximum username length
static const uint32_t constexpr username_len = 32;

// Header indicies
static const uint32_t constexpr packet_number_begin = 0;
static const uint32_t constexpr packet_number_end = 1;
static const uint32_t constexpr header_version_begin = 2;
static const uint32_t constexpr header_version_end = 2;
static const uint32_t constexpr source_username_begin = 3;
static const uint32_t constexpr source_username_end = 34;
static const uint32_t constexpr dest_username_begin = 35;
static const uint32_t constexpr dest_username_end = 66;
static const uint32_t constexpr message_type_begin = 67;
static const uint32_t constexpr message_type_end = 67;
static const uint32_t constexpr data_packet_length_begin = 68;
static const uint32_t constexpr data_packet_length_end = 69;
static const uint32_t constexpr data_packet_checksum_begin = 70;
static const uint32_t constexpr data_packet_checksum_end = 101;
static const uint32_t constexpr future_use_begin = 102;
static const uint32_t constexpr future_use_end = 133;
static const uint32_t constexpr header_checksum_begin = 134;
static const uint32_t constexpr header_checksum_end = 165;

using MessageHeader = std::array<uint8_t, 166>;

class MessageLayer {
	// Message header for communications between the client and server.
	// 166 bytes
	MessageHeader header;
	// Checksum container used for verifying the integrity of the
	// header as well as the data packet.
	std::array<uint8_t, 32> checksum;
	// Calculate the checksum of the contents of the header,
	// and write it to the 32 bytes (256 bits) that make up
	// the checksum section of the header.
	void calculate_header_sum(void);
	// Verify the checksum of the header's contents against
	// the checksum stored in the header. If they aren't the same
	// there is something amiss within the header.
	bool verify_header_sum(void);
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
	// Calculate the checksum of the data packet, and store it in
	// the appropriate place in the header
	// The +1's are because C++ iterators are exclusive
	template <typename T>
	void calculate_data_packet_checksum(const T &data_packet_container)
	{
		picosha2::hash256(data_packet_container.begin(),
				  data_packet_container.end(),
				  header.begin() + data_packet_checksum_begin,
				  header.begin() +
					  (data_packet_checksum_end + 1));
	}
	// Hash the passed data packet container and compare it to the checksum
	// stored within the header.
	template <typename T>
	bool verify_data_packet_checksum(const T &data_packet_container)
	{
		picosha2::hash256(data_packet_container.begin(),
				  data_packet_container.end(), checksum.begin(),
				  checksum.end());
		// Make sure the checksums match
		return std::equal(header.begin() + data_packet_checksum_begin,
				  header.begin() +
					  (data_packet_checksum_end + 1),
				  checksum.begin());
	}
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
