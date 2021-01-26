#include "MessageLayer.hpp"
#include <cassert>
#include <iostream>

int main(void)
{
	MessageLayer header_1;
	std::array<uint8_t, 166> header =
		header_1.set_packet_number(1)
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
}
