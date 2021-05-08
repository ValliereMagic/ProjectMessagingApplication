use crate::message_header::MessageHeader;
use std::io::{Error, ErrorKind, Read, Result, Write};
use std::net::TcpStream;
pub type Message<'h> = (&'h MessageHeader, Option<Vec<u8>>);

pub struct MessageLayer {
	client: TcpStream,
}

impl MessageLayer {
	// Very interesting things going on here, borrowing an *immutable reference*
	// as mutable...
	// https://stackoverflow.com/questions/36233193/why-can-i-just-pass-an-immutable-reference-to-bufreader-instead-of-a-mutable-re
	pub fn new(client_fd: TcpStream) -> MessageLayer {
		MessageLayer { client: client_fd }
	}
	// Read a message from the client FD
	pub fn read_basic_message<'h>(&self, header: &'h mut MessageHeader) -> Result<Message<'h>> {
		// Try to read a header from the FD
		(&mut (&self.client)).read_exact(&mut header.0[..])?;
		// Make sure the header checksum is valid.
		if !header.verify_header_checksum() {
			return Err(Error::new(ErrorKind::Other, "Invalid header checksum."));
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
			(&mut (&self.client)).read_exact(&mut body_vec[..])?;
			message_body = Some(body_vec);
		}
		return Ok((header, message_body));
	}
	// Write a message to the FD
	pub fn write_basic_message(&self, message: &Message) -> std::io::Result<usize> {
		// Write out the header
		(&mut (&self.client)).write(&message.0 .0[..])?;
		// Write out the body if it exists.
		match &message.1 {
			Some(message) => (&mut (&self.client)).write(&message[..]),
			None => Ok(0),
		}
	}
}
