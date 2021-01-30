#include "MessageLayer.hpp"
#include <cassert>
#include <iostream>

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
	header_4.build();
	assert(header_4.valid);
}
