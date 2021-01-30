#include "Server.hpp"
#include <iostream>

MessagingClient::MessagingClient(int client_socket, std::string &our_username)
	: client_socket(client_socket), our_username(our_username)
{
}

MessagingClient::MessagingClient(MessagingClient &&client)
	: client_socket(client.client_socket),
	  our_username(std::move(client.our_username))
{
}

void MessagingClient::client(void)
{
	std::cout << "Started Receiving thread for client: " << our_username
		  << std::endl;
	// TODO
}

int MessagingClient::get_client_socket(void)
{
	return client_socket;
}
