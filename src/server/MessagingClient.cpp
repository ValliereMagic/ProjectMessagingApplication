#include "Server.hpp"
#include <iostream>

MessagingClient::MessagingClient(int client_socket, std::string &our_username)
	: client_socket(client_socket), our_username(our_username)
{
	incoming_thread = std::thread(&MessagingClient::sent_to_us, this);
}

MessagingClient::MessagingClient(MessagingClient &&client)
	: running(client.running.load()),
	  incoming_thread(std::move(client.incoming_thread)),
	  incoming_messages(std::move(client.incoming_messages)),
	  client_socket(client.client_socket),
	  our_username(std::move(client.our_username))
{
}

MessagingClient::~MessagingClient()
{
	incoming_thread.join();
}

void MessagingClient::sent_to_us()
{
	std::cout << "Started Sending thread for client: " << our_username
		  << std::endl;
	// TODO
}

void MessagingClient::recv()
{
	std::cout << "Started Receiving thread for client: " << our_username
		  << std::endl;
	// TODO
}
