#pragma once
#include <vector>
#include <string>
#include "MessageLayer.hpp"

// Send a message to another client by username
bool send_to_client(const std::string &username,
		    const std::vector<uint8_t> &message);
// send a message to all participating clients
bool send_to_all(const std::string &our_username,
		 const std::vector<uint8_t> &message);
// Get list of logged in users
std::string get_logged_in_users(void);
// Rolling packet incrementer
uint16_t &increment_packet_number(uint16_t &num);
