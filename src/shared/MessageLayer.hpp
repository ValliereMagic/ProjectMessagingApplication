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
	// Using the current contents of the header,
	// Calculate the the header's checksum.
	void calculate_sha256_sum(void);
	// Verify the header's SHA256 checksum
	bool verify_sha256_sum(void);
	// Internal function for setting usernames within
	// the header
	static inline void set_username(const std::string &source_username,
					char *header_ptr);

    public:
	// If this is false you probably shouldn't try to send this...
	// It means the checksum is bad.
	bool valid = true;

	MessageLayer(void);
	MessageLayer(MessageLayer &&ml);
	MessageLayer(MessageHeader &header_ref);
	MessageLayer(MessageHeader &&header_rvalue_ref);

	// New functions allowing complete reuse of the MessageLayer
	// object
	MessageHeader &get_internal_header(void);
	void verify_checksum(void);

	uint16_t get_packet_number(void);
	MessageLayer &set_packet_number(uint16_t p_num);

	uint8_t get_version_number(void);
	MessageLayer &set_version_number(uint8_t v_num);

	std::string get_source_username(void);
	MessageLayer &set_source_username(const std::string &source_username);

	std::string get_dest_username(void);
	MessageLayer &set_dest_username(const std::string &source_username);

	uint8_t get_message_type(void);
	MessageLayer &set_message_type(uint8_t m_type);

	uint16_t get_data_packet_length(void);
	MessageLayer &set_data_packet_length(uint16_t data_packet_len);

	// Additional stuff to be added for future use here

	MessageHeader &build(void);
	MessageHeader build_cpy(void);
};

// Function for extracting strings using a length and a pointer.
// (Message data packets)
// Strings must have a double null terminator to be passed to this
// function. Otherwise it will be built without!
std::string build_string_safe(const char *str, size_t len);
// Build a message from a container and a header
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
