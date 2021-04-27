pub mod message_layer {
	use std::convert::TryInto;
	// Header indices constants
	const PACKET_NUMBER_BEGIN: usize = 0;
	const HEADER_VERSION_BEGIN: usize = 2;
	const SOURCE_USERNAME_BEGIN: usize = 3;
	const DEST_USERNAME_BEGIN: usize = 35;
	const MESSAGE_TYPE_BEGIN: usize = 67;
	const DATA_PACKET_LENGTH_BEGIN: usize = 68;
	const DATA_PACKET_CHECKSUM_BEGIN: usize = 70;
	const FUTURE_USE_BEGIN: usize = 134;
	const HEADER_CHECKSUM_BEGIN: usize = 165;
	// Maximum username length
	const USERNAME_LEN: usize = 32;

	// A message header is composed of many fields read from the client
	pub struct MessageHeader([u8; 166]);

	impl MessageHeader {
		// Create a new empty message header for use.
		pub fn new() -> MessageHeader {
			MessageHeader { 0: [0u8; 166] }
		}
		pub fn verify_header_checksum(&self) -> Result<Self, &'static str> {
			Err("Unimplemented")
		}
		pub fn verify_data_checksum(&self, data_portion: &Vec<u8>) -> Result<Self, &'static str> {
			Err("Unimplemented")
		}
		pub fn set_packet_num(&mut self, packet_num: u16) -> &Self {
			// Convert the packet number to big endian (Network Byte order)
			let net_packet_num: [u8; 2] = packet_num.to_be_bytes();
			// Place it at its spot in the message header.
			for (idx, byte) in net_packet_num.iter().enumerate() {
				self.0[PACKET_NUMBER_BEGIN + idx] = *byte;
			}
			self
		}
		pub fn get_packet_num(&self) -> u16 {
			// Convert bytes 0..2(exclusive) to a u16
			// If this somehow failed... we'd have bigger fish to fry.
			u16::from_be_bytes(
				(self.0[PACKET_NUMBER_BEGIN..HEADER_VERSION_BEGIN])
					.try_into()
					.unwrap(),
			)
		}
		pub fn set_version_num(&mut self, version_num: u8) -> &Self {
			self.0[HEADER_VERSION_BEGIN] = version_num;
			self
		}
		pub fn get_version_num(&self) -> u8 {
			self.0[HEADER_VERSION_BEGIN]
		}
		pub fn set_source_username(&mut self, username: &str) -> Result<&Self, &'static str> {
			// Look at the passed string as a sequence of utf-8 bytes
			let bytes = username.as_bytes();
			let mut idx: usize = 0;
			for byte in bytes {
				// We are at the end of the username. Place a null terminator
				if idx == bytes.len() {
					self.0[SOURCE_USERNAME_BEGIN + idx] = 0;
				// Still placing characters
				} else if idx < 32 {
					self.0[SOURCE_USERNAME_BEGIN + idx] = *byte;
					idx += 1;
				// The username is too long.
				} else {
					return Err("The username is too long");
				}
			}
			Ok(self)
		}
		pub fn get_source_username(&self) -> Result<String, &'static str> {
			// Extract a slice of the username from the header
			let byte_slice: &[u8] = &(self.0[SOURCE_USERNAME_BEGIN..DEST_USERNAME_BEGIN]);
			// Create a vector to hold the username
			let mut string: Vec<u8> = Vec::new();
			for byte in byte_slice {
				if *byte == 0 {
					break;
				} else {
					string.push(*byte);
				}
			}
			// Convert the vector to a string, and return accordingly
			match String::from_utf8(string) {
				Ok(converted) => Ok(converted),
				Err(_) => Err("An error occurred pulling in the username string."),
			}
		}
	}
}

#[cfg(test)]
mod tests {
	use super::*;
	use message_layer::*;
	// Make sure that packet numbers that go in, come out the same.
	#[test]
	fn test_packet_numbers() {
		let mut header = MessageHeader::new();
		assert!(header.set_packet_num(5).get_packet_num() == 5);
	}
	// Make sure that header numbers are settable and retrievable
	#[test]
	fn test_header_versions() {
		let mut header = MessageHeader::new();
		assert!(header.set_version_num(5).get_version_num() == 5);
	}
	#[test]
	fn test_usernames() -> Result<(), &'static str> {
		// Make sure set usernames are retrievable
		let mut header = MessageHeader::new();
		header.set_source_username("meow").unwrap();
		let result = header.get_source_username().unwrap();
		assert!(result == "meow");
		// Make sure usernames that are too long are not accepted.
		match header.set_source_username("123456789012345678901234567890123") {
			Ok(_) => Err("This username is too long, and should fail."),
			Err(_) => Ok(()),
		}
	}
}
