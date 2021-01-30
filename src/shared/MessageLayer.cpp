#include "MessageLayer.hpp"
#include <cstring>
extern "C" {
#include <netinet/in.h>
}

// From GitHub, MIT licenced
// SHA256 hashing function
#include "picosha2.hpp"

std::vector<uint8_t> MessageLayer::calculate_sha256_sum(void)
{
	// Copy the header into a vector, to calculate the checksum from
	// (Everything up to the sha256 checksum field)
	// This constructor is not inclusive on the end byte. hence 134.
	std::vector<uint8_t> data_to_sum(&(header[0]), &(header[134]));
	// Calculate the checksum...
	std::vector<uint8_t> resultant_checksum(picosha2::k_digest_size);
	picosha2::hash256(data_to_sum.begin(), data_to_sum.end(),
			  resultant_checksum.begin(), resultant_checksum.end());
	// Write it to the header
	std::memcpy(&(header[134]), &(resultant_checksum[0]),
		    picosha2::k_digest_size);
	// Return the checksum
	return std::move(resultant_checksum);
}

// Verify the header's SHA256 checksum
bool MessageLayer::verify_sha256_sum(void)
{
	// Retrieve the sha256sum from the header 134 bytes in
	// This constructor is not inclusive on the end byte. hence 166.
	std::vector<uint8_t> checksum_vec(&(header[134]), &(header[166]));
	// Make sure the checksums match
	return (checksum_vec == calculate_sha256_sum());
}

// Empty Constructor
MessageLayer::MessageLayer(void)
{
	header.fill(0);
}

// Move constructor
MessageLayer::MessageLayer(MessageLayer &&ml)
	: header(std::move(ml.header)), valid(ml.valid)
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

// Move constructor (with header)
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

std::string MessageLayer::get_source_username(void)
{
	// Index into header to the start of the source username,
	// and pull the correct number of bytes (up to 32)
	char *username = (char *)&(header[3]);
	std::string username_str = "";
	for (uint8_t i = 0; i < 32; ++i) {
		// While there are still characters to add, continue.
		// Otherwise stop.
		if (username[i] != '\0') {
			username_str.push_back(username[i]);
		} else {
			break;
		}
	}
	return std::move(username_str);
}

MessageLayer &MessageLayer::set_source_username(std::string source_username)
{
	// Copy passed string into the message header, and
	// ensure that a null terminator is set, by setting one ourselves.
	char *username = (char *)&(header[3]);
	size_t min;
	if (source_username.length() < 31)
		min = source_username.length();
	else
		min = 31;
	// Clear the entire username field of the header
	std::memset(username, 0, 32);
	// Put the source_username at the beginning of the
	// field in the header.
	std::memcpy(username, (source_username.c_str()), min);
	// Put a null terminator at the end of the username.
	username[min] = '\0';
	return (*this);
}

std::string MessageLayer::get_dest_username(void)
{
	// Index into header to the start of the destination username,
	// and pull the correct number of bytes (up to 32)
	char *username = (char *)&(header[35]);
	std::string username_str = "";
	for (uint8_t i = 0; i < 32; ++i) {
		// While there are still characters to add, continue.
		// Otherwise stop.
		if (username[i] != '\0') {
			username_str.push_back(username[i]);
		} else {
			break;
		}
	}
	return std::move(username_str);
}

MessageLayer &MessageLayer::set_dest_username(std::string source_username)
{
	// Copy passed string into the message header, and
	// ensure that a null terminator is set, by setting one ourselves.
	char *username = (char *)&(header[35]);
	size_t min;
	if (source_username.length() < 31)
		min = source_username.length();
	else
		min = 31;
	// Clear the entire username field of the header
	std::memset(username, 0, 32);
	// Put the source_username at the beginning of the
	// field in the header.
	std::memcpy(username, (source_username.c_str()), min);
	// Put a null terminator at the end of the username.
	username[min] = '\0';
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
	return std::move(cpy);
}
