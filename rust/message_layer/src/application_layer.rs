use crate::message_header::MessageHeader;
use std::cell::RefCell;
use std::io::{Error, ErrorKind, Read, Result, Write};
use std::net::TcpStream;
use std::rc::Rc;
// Message returned from reading
pub type Message = (Rc<RefCell<MessageHeader>>, Option<Rc<RefCell<Vec<u8>>>>);
// Message reference sent in for writing
pub type MessageRef<'h> = (&'h MessageHeader, Option<&'h [u8]>);

pub struct MessageLayer {
	client: TcpStream,
	header: Rc<RefCell<MessageHeader>>,
	body: Rc<RefCell<Vec<u8>>>,
}

impl MessageLayer {
	// Very interesting things going on here, borrowing an *immutable reference*
	// as mutable...
	// https://stackoverflow.com/questions/36233193/why-can-i-just-pass-an-immutable-reference-to-bufreader-instead-of-a-mutable-re
	pub fn new(client_fd: TcpStream) -> MessageLayer {
		MessageLayer {
			client: client_fd,
			header: Rc::new(RefCell::new(MessageHeader::new())),
			body: Rc::new(RefCell::new(Vec::new())),
		}
	}
	// Read a message from the client FD
	pub fn read_basic_message(&self) -> Result<Message> {
		// Create header to read into
		let mut header = self.header.borrow_mut();
		// Try to read a header from the FD
		(&mut (&self.client)).read_exact(&mut header.0[..])?;
		// Make sure the header checksum is valid.
		if !header.verify_header_checksum() {
			return Err(Error::new(ErrorKind::Other, "Invalid header checksum."));
		}
		// If the message has a body, read it in
		let mut message_body: Option<Rc<RefCell<Vec<u8>>>> = None;
		let data_portion_len = header.get_data_packet_length();
		if data_portion_len > 0 {
			// Clear our body vector
			let mut body_vec = self.body.borrow_mut();
			body_vec.clear();
			body_vec.reserve(data_portion_len as usize);
			// Set the length to the capacity. This is unsafe, because the
			// memory is uninitialized.
			unsafe {
				body_vec.set_len(data_portion_len as usize);
			}
			// Read in the body initializing the vector.
			// Make sure we were able to read it in correctly.
			(&mut (&self.client)).read_exact(&mut body_vec[..])?;
			message_body = Some(self.body.clone());
		}
		return Ok((self.header.clone(), message_body));
	}
	// Write a message to the FD
	pub fn write_basic_message(&self, message: &MessageRef) -> std::io::Result<usize> {
		// Write out the header
		(&mut (&self.client)).write(&message.0 .0)?;
		// Write out the body if it exists.
		match &message.1 {
			Some(message) => (&mut (&self.client)).write(message),
			None => Ok(0),
		}
	}
}

impl Iterator for &MessageLayer {
	type Item = Result<Message>;
	fn next(&mut self) -> Option<Self::Item> {
		Some(self.read_basic_message())
	}
}
