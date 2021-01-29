#pragma once
#include "MessagingClient.hpp"

// Lookup client by username using the rwlock
// within client_objects
MessagingClient &lookup_client(std::string &username);
