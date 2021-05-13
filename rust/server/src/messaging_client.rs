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

	fn send_verification_message(
		&self,
		message_type: MessageTypes,
		packet_num: u16,
		header: &mut MessageHeader,
	) {
		header
			.clear()
			.set_message_type(message_type as u8)
			.set_version_num(VERSION)
			.set_packet_num(packet_num)
			.set_source_username("server")
			.unwrap()
			.set_dest_username(&self.our_username)
			.unwrap()
			.calculate_header_checksum();
		self.other_clients_handle
			.send_to_client(&self.our_username, &(&header, None));
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
			&(&client_header, Some(login_message.as_bytes())),
		);
		// Handle the incoming messages
		for message_res in &self.client_fd {
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
			// Borrow the message header
			let header = message.0.borrow();
			// Handle the message based on what type it is.
			match MessageTypes::from_u8(header.get_message_type()) {
				MessageTypes::LOGIN => {
					self.send_error_message("You're already logged in dingus.", &mut client_header);
				}
				MessageTypes::ERROR => (),
				MessageTypes::WHO => {
					// Retrieve the string of logged in users
					let usernames = self.other_clients_handle.get_logged_in_users();
					// Setup the header
					client_header
						.clear()
						.set_message_type(MessageTypes::WHO as u8)
						.set_version_num(3)
						.set_source_username("server")
						.unwrap()
						.set_dest_username(&self.our_username)
						.unwrap()
						.set_data_packet_length(usernames.len() as u16)
						.calculate_header_checksum();
					// Send off the message
					self.other_clients_handle.send_to_client(
						&self.our_username,
						&(&client_header, Some(usernames.as_bytes())),
					);
				}
				MessageTypes::ACK => (),
				MessageTypes::MESSAGE => {
					// If there is no message from this header, no point in
					// forwarding it
					if !message.1.is_some() {
						// Send back an error message to the client.
						self.send_error_message(
							&format!(
								"{}{}",
								"The message you sent contains no content. ",
								"It will not be forwarded."
							),
							&mut client_header,
						);
						continue;
					}
					// Borrow the message body
					let body = message.1.as_ref().unwrap().borrow();
					// Verify the integrity of the message contents. We already
					// checked whether it is there, so unwrap it.
					// Cannot use client_header in here as it has been already
					// mutable borrowed. Therefore, we are accessing it through
					// message instead
					{
						let mut verification_header = MessageHeader::new();
						if !(header.verify_data_checksum(&body)) {
							self.send_verification_message(
								MessageTypes::NACK,
								client_header.get_packet_num(),
								&mut verification_header,
							);
							continue;
						}
						self.send_verification_message(
							MessageTypes::ACK,
							header.get_packet_num(),
							&mut verification_header,
						);
					}
					// Either broadcast the message, or forward it to a specific
					// user
					let dest_username = header.get_dest_username().unwrap();
					if dest_username == "all" {
						self.other_clients_handle
							.send_to_all(&self.our_username, &(&header, Some(&body[..])));
					} else {
						self.other_clients_handle
							.send_to_client(&dest_username, &(&header, Some(&body[..])));
					}
				}
				MessageTypes::DISCONNECT => {
					let leave_message =
						format!("User: {} disconnected from the room.", self.our_username);
					// Setup the header information
					client_header
						.clear()
						.set_message_type(MessageTypes::MESSAGE as u8)
						.set_version_num(VERSION)
						.set_source_username("server")
						.unwrap()
						.set_dest_username("all")
						.unwrap()
						.set_data_packet_length(leave_message.len() as u16)
						.calculate_header_checksum();
					// Send it out
					self.other_clients_handle.send_to_all(
						&self.our_username,
						&(&client_header, Some(leave_message.as_bytes())),
					);
					// return, as we are done with this connection now.
					return;
				}
				MessageTypes::NACK => (),
			}
		}
	}
	// Return an immutable reference to this MessageClient so others can send us
	// messages.
	pub fn get_client_fd(&self) -> &MessageLayer {
		&self.client_fd
	}
}
