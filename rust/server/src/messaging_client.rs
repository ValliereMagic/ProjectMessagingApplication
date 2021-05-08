extern crate message_layer;
use crate::shared_clients::SharedClients;
use message_layer::application_layer::{Message, MessageLayer};
use message_layer::message_header::{MessageHeader, MessageTypes, VERSION};
use std::io::ErrorKind;

pub struct MessagingClient {
	client_fd: MessageLayer,
	our_username: String,
	other_clients_handle: &'static SharedClients,
}

impl MessagingClient {
	pub fn new(client_fd: MessageLayer, our_username: &str) -> MessagingClient {
		MessagingClient {
			client_fd,
			our_username: our_username.to_owned(),
			other_clients_handle: SharedClients::get_instance(),
		}
	}

	fn send_error_message(&self, message: &str, header: &mut MessageHeader) {
		// Populate the header for error messages
		header
			.clear()
			.set_message_type(MessageTypes::ERROR as u8)
			.set_version_num(VERSION)
			.set_source_username("server")
			.unwrap()
			.set_dest_username(&self.our_username)
			.unwrap()
			.set_data_packet_length(message.len() as u16)
			.calculate_header_checksum();
		self.other_clients_handle
			.send_to_client(&self.our_username, &(header, None));
	}

	pub fn client(&self, mut client_header: MessageHeader) {
		println!(
			"Starting Receiving thread for client: {}",
			self.our_username
		);
		// Send out login join message
		let login_message = format!("User: {} entered the room.", self.our_username);
		// Set up the header
		client_header
			.clear()
			.set_message_type(MessageTypes::MESSAGE as u8)
			.set_version_num(VERSION)
			.set_source_username("server")
			.unwrap()
			.set_dest_username("all")
			.unwrap()
			.set_data_packet_length(login_message.len() as u16)
			.calculate_header_checksum();
		// Send it out
		self.other_clients_handle.send_to_all(
			&self.our_username,
			&(&client_header, Some(Vec::from(login_message))),
		);
		// Handle the incoming messages
		loop {
			let message_res = self.client_fd.read_basic_message(& mut client_header);
			let message: Message;
			match message_res {
				Ok(m) => message = m,
				Err(s) => {
					println!("Error: {}", s);
					match s.kind() {
						// Return on unexpected socket close
						ErrorKind::UnexpectedEof => return,
						_ => continue,
					}
				}
			}
			match MessageTypes::from_u8(message.0.get_message_type()) {
				MessageTypes::LOGIN => (),
				MessageTypes::ERROR => (),
				MessageTypes::WHO => (),
				MessageTypes::ACK => (),
				MessageTypes::MESSAGE => (),
				MessageTypes::DISCONNECT => (),
				MessageTypes::NACK => (),
			}
			println!("Message Received.");
		}
	}
	// Return an immutable reference to this MessageClient so others can send us
	// messages.
	pub fn get_client_fd(&self) -> &MessageLayer {
		&self.client_fd
	}
}
