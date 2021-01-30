#pragma once
#include <string>
// Lookup client's fd by username using the rwlock
// within client_objects
// -1 if the client doesn't exist!
int lookup_client(std::string &username);
