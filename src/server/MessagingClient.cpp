#include "Server.hpp"
#include "MessageLayer.hpp"
#include "MessagingClient.hpp"
#include <iostream>

MessagingClient::MessagingClient(int client_socket, std::string &our_username,
				 MessageLayer &&ml)
	: client_socket(client_socket), our_username(our_username),
	  ml(std::move(ml))
{
}

MessagingClient::MessagingClient(MessagingClient &&client)
	: client_socket(client.client_socket),
	  our_username(std::move(client.our_username)), ml(std::move(client.ml))
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
