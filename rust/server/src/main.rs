extern crate message_layer;
mod messaging_client;
mod shared_clients;
use message_layer::application_layer::{Message, MessageLayer};
use message_layer::message_header::{MessageHeader, MessageTypes, VERSION};
use messaging_client::MessagingClient;
use shared_clients::SharedClients;
use std::net::{TcpListener, TcpStream};
use std::rc::Rc;
use std::thread;

fn login(client: TcpStream) {
	let app_layer = MessageLayer::new(client);
	// Pull the first message from the client, which is hopefully a login
	// message.
	let message: Message;
	match app_layer.read_basic_message() {
		Ok(m) => {
			// Make sure this is a login request message
			if m.0.get_message_type() != MessageTypes::LOGIN as u8 {
				println!(
					"{}{}",
					"Error. First message is not a login request.", " Closing connection."
				);
				return;
			}
			message = m;
		}
		Err(err) => {
			println!("Error: {}", err);
			return;
		}
	}
	// Pull the source username string, and make sure we were
	// successful.
	let username: String;
	match message.0.get_source_username() {
		Ok(user) => {
			username = user;
		}
		Err(err) => {
			println!("Error: {}", err);
			return;
		}
	}
	// Create the login response message
	let mut login_response = MessageHeader::new();
	login_response
		.set_version_num(VERSION)
		.set_message_type(MessageTypes::LOGIN as u8)
		.set_source_username("server")
		.unwrap()
		.set_dest_username(&username)
		.unwrap()
		.calculate_header_checksum();
	// Create our MessagingClient
	let client = MessagingClient::new(app_layer, &username);
	// Get an instance of SharedClients
	let sc = SharedClients::get_instance();
	// Add our new user to the SharedClients
	let client_ptr: Rc<MessagingClient>;
	match sc.add_user(&username, client) {
		// We logged in successfully
		Ok(c) => client_ptr = c,
		// We failed to login (duplicate username)
		Err(client_ret) => {
			// Send the login failure message and return.
			let response_message = "Error. Username already in the system.";
			login_response
				.clear()
				.set_message_type(MessageTypes::ERROR as u8)
				.set_version_num(VERSION)
				.set_source_username("server")
				.unwrap()
				.set_dest_username(&username)
				.unwrap()
				.set_data_packet_length(response_message.len() as u16)
				.calculate_header_checksum();
			client_ret
				.get_client_fd()
				.write_basic_message(&(&login_response, Some(Vec::from(response_message))))
				.unwrap();
			return;
		}
	}
	// Send out the login success message
	client_ptr
		.get_client_fd()
		.write_basic_message(&(&login_response, None))
		.unwrap();
	// Start the receiving method
	client_ptr.client(login_response);
	// Logout
	sc.remove_user(&username);
}

fn main() -> std::io::Result<()> {
	// Bind to our address and port, as well as start listening.
	let server_socket = TcpListener::bind("0.0.0.0:34551")?;
	// Accept connections as they come in
	for stream in server_socket.incoming() {
		// Auto match statement
		let stream = stream?;
		// Spawn a detached thread, and process the client.
		thread::spawn(move || login(stream));
	}
	Ok(())
}
