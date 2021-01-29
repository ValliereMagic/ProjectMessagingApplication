#pragma once
#include <queue>
#include <vector>
#include <cstdint>
#include <thread>
#include <atomic>

class MessagingClient {
	// This will be set to false
	// when the shutdown handler is called,
	// and the client threads are joined.
	std::atomic<bool> running;
	std::thread incoming_thread;
	std::queue<std::vector<uint8_t> > incoming_messages;
	int client_socket;
	std::string our_username;

    public:
	MessagingClient(int client_socket, std::string &our_username);
	MessagingClient(MessagingClient &&client);
	~MessagingClient();
	// Handled by the incoming_thread. Takes messages out of the
	// incoming messages queue, and sends them to the client
	// managed by this object.
	void sent_to_us();
	// Handled by the thread that creates and runs this object on
	// accept. Handles messages sent to the server from the client.
	void recv();
};