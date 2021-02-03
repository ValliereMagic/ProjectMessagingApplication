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
