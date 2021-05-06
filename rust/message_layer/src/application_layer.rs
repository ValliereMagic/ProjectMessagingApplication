use crate::message_header::MessageHeader;
use std::io::{Read, Write};
use std::net::TcpStream;
type Message = (MessageHeader, Option<Vec<u8>>);

pub struct MessageLayer<'a> {
	client: &'a TcpStream,
}

impl<'a> MessageLayer<'a> {
	// Create a new message layer by taking ownership of the client handle
	pub fn new(client_fd: &'a TcpStream) -> MessageLayer<'a> {
		MessageLayer { client: client_fd }
	}
	// Read a message from the client FD
	pub fn read_basic_message(&mut self) -> Result<Message, String> {
		// Read in the message header from the FD
		let mut header = MessageHeader::new();
		// Try to read a header from the FD
		match self.client.read_exact(&mut header.0[..]) {
			Ok(_) => (),
			Err(err) => {
				return Err(format!("{}", err));
			}
		}
		// Make sure the header checksum is valid.
		if !header.verify_header_checksum() {
			return Err(format!("Error. Invalid header checksum."));
		}
		// If the message has a body, read it in
		let mut message_body: Option<Vec<u8>> = None;
		let data_portion_len = header.get_data_packet_length();
		if data_portion_len > 0 {
			// Allocate the vector
			let mut body_vec: Vec<u8> = Vec::with_capacity(data_portion_len as usize);
			// Set the length to the capacity. This is unsafe, because the
			// memory is uninitialized.
			unsafe {
				body_vec.set_len(data_portion_len as usize);
			}
			// Read in the body initializing the vector.
			// Make sure we were able to read it in correctly.
			match self.client.read_exact(&mut body_vec[..]) {
				Ok(_) => (),
				Err(err) => {
					return Err(format!("{}", err));
				}
			}
			message_body = Some(body_vec);
		}
		return Ok((header, message_body));
	}
	// Write a message to the FD
	pub fn write_basic_message(&mut self, message: &Message) -> std::io::Result<usize> {
		// Write out the header
		self.client.write(&message.0 .0[..])?;
		// Write out the body if it exists.
		match &message.1 {
			Some(message) => self.client.write(&message[..]),
			None => Ok(0),
		}
	}
}

// Create an iterator allowing for looping over the messages from app_layer
// as they come in.
impl<'a> Iterator for MessageLayer<'a> {
	type Item = Result<Message, String>;

	fn next(&mut self) -> Option<Self::Item> {
		Some(self.read_basic_message())
	}
}
