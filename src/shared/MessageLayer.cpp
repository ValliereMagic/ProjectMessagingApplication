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

void MessageLayer::calculate_sha256_sum(void)
{
	// Calculate the checksum...
	// No longer copying to a vector. Reading directly from the
	// header instead.
	picosha2::hash256(header.begin(), header.begin() + 134,
			  checksum.begin(), checksum.end());
	// Write it to the header
	std::memcpy(&(header[134]), &(checksum[0]), picosha2::k_digest_size);
}

// Verify the header's SHA256 checksum
bool MessageLayer::verify_sha256_sum(void)
{
	// update the checksum
	calculate_sha256_sum();
	// Make sure the checksums match
	return std::equal(header.begin() + 134, header.end(), checksum.begin());
}

// Empty Constructor
MessageLayer::MessageLayer(void)
{
	header.fill(0);
}

// Move constructor
MessageLayer::MessageLayer(MessageLayer &&ml)
	: header(std::move(ml.header)), checksum(std::move(ml.checksum)),
	  valid(ml.valid)

{
}

// Copy Constructor (with checksum verification)
MessageLayer::MessageLayer(MessageHeader &header_ref)
{
	// Copy over what's been passed.
	header = header_ref;
	// Make sure that the checksum is valid.
	this->valid = verify_sha256_sum();
}

// Move header constructor
MessageLayer::MessageLayer(MessageHeader &&header_rvalue_ref)
	: header(std::move(header_rvalue_ref))
{
	this->valid = verify_sha256_sum();
}

uint16_t MessageLayer::get_packet_number(void)
{
	// Retrieve the first 2 bytes from the header
	// and convert them to host format and return
	return ntohs(*((uint16_t *)&(header[0])));
}

MessageLayer &MessageLayer::set_packet_number(uint16_t p_num)
{
	// Convert the passed short to network byte order,
	// and assign it to its place in the header.
	(*((uint16_t *)&(header[0]))) = htons(p_num);
	return (*this);
}

uint8_t MessageLayer::get_version_number(void)
{
	return header[2];
}

MessageLayer &MessageLayer::set_version_number(uint8_t v_num)
{
	// assign version number to its place in the header.
	header[2] = v_num;
	return (*this);
}

// Internal function for setting usernames within
// the header
void MessageLayer::set_username(const std::string &source_username,
				char *header_ptr)
{
	size_t min;
	if (source_username.length() < 31)
		min = source_username.length();
	else
		min = 31;
	// Clear the entire username field of the header
	std::memset(header_ptr, 0, 32);
	// Put the source_username at the beginning of the
	// field in the header.
	std::memcpy(header_ptr, (source_username.c_str()), min);
	// Put a null terminator at the end of the username.
	header_ptr[min] = '\0';
}

std::string MessageLayer::get_source_username(void)
{
	// Index into header to the start of the source username,
	// and pull the correct number of bytes (up to 32)
	char *username = (char *)&(header[3]);
	return build_string_safe(username, 32);
}

MessageLayer &
MessageLayer::set_source_username(const std::string &source_username)
{
	// Copy passed string into the message header, and
	// ensure that a null terminator is set, by setting one ourselves.
	char *username = (char *)&(header[3]);
	this->set_username(source_username, username);
	return (*this);
}

std::string MessageLayer::get_dest_username(void)
{
	// Index into header to the start of the destination username,
	// and pull the correct number of bytes (up to 32)
	char *username = (char *)&(header[35]);
	return build_string_safe(username, 32);
}

MessageLayer &
MessageLayer::set_dest_username(const std::string &source_username)
{
	// Copy passed string into the message header, and
	// ensure that a null terminator is set, by setting one ourselves.
	char *username = (char *)&(header[35]);
	this->set_username(source_username, username);
	return (*this);
}

uint8_t MessageLayer::get_message_type(void)
{
	// 67 bytes into the header is our message type
	return header[67];
}

MessageLayer &MessageLayer::set_message_type(uint8_t m_type)
{
	// assign message type to its place in the header.
	header[67] = m_type;
	return (*this);
}

uint16_t MessageLayer::get_data_packet_length(void)
{
	// Retrieve the data packet length from the header
	// Which is 2 bytes, 68 bytes into the header,
	// and convert them to host format and return
	return ntohs(*((uint16_t *)&(header[68])));
}

MessageLayer &MessageLayer::set_data_packet_length(uint16_t data_packet_len)
{
	// Convert the passed short to network byte order,
	// and assign it to its place in the header.
	(*((uint16_t *)&(header[68]))) = htons(data_packet_len);
	return (*this);
}

MessageHeader &MessageLayer::build(void)
{
	calculate_sha256_sum();
	return header;
}

MessageHeader MessageLayer::build_cpy(void)
{
	calculate_sha256_sum();
	MessageHeader cpy = header;
	return cpy;
}

// Function for extracting strings using a length and a pointer.
// (Message data packets)
std::string build_string_safe(const char *str, size_t len)
{
	std::string username_str = "";
	for (uint8_t i = 0; i < len; ++i) {
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
