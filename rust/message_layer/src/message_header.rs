extern crate sha2;
use sha2::{Digest, Sha256};
use std::convert::TryInto;
// Header indices constants
const PACKET_NUMBER_BEGIN: usize = 0;
const HEADER_VERSION_BEGIN: usize = 2;
const SOURCE_USERNAME_BEGIN: usize = 3;
const DEST_USERNAME_BEGIN: usize = 35;
const MESSAGE_TYPE_BEGIN: usize = 67;
const DATA_PACKET_LENGTH_BEGIN: usize = 68;
const DATA_PACKET_CHECKSUM_BEGIN: usize = 70;
const FUTURE_USE_BEGIN: usize = 102;
const HEADER_CHECKSUM_BEGIN: usize = 134;
// Maximum username length
const USERNAME_LEN: usize = 32;

// A message header is composed of many fields read from the client
pub struct MessageHeader(pub [u8; 166]);

impl MessageHeader {
	// Insert a username into the header at the specified beginning index. If
	// the username is longer than 32 bytes, then this will fail.
	fn set_username(
		&mut self,
		username: &str,
		begin_idx: usize,
	) -> Result<&mut Self, &'static str> {
		// Look at the passed string as a sequence of utf-8 bytes
		let bytes = username.as_bytes();
		let mut idx: usize = 0;
		for byte in bytes {
			// We are at the end of the username. Place a null terminator
			if idx == bytes.len() {
				self.0[begin_idx + idx] = 0;
			// Still placing characters
			} else if idx < USERNAME_LEN {
				self.0[begin_idx + idx] = *byte;
				idx += 1;
			// The username is too long.
			} else {
				return Err("The username is too long");
			}
		}
		Ok(self)
	}
	// Pull a username from the header at a beginning with a maximum length of
	// 32 bytes. (A shorter username will be null terminated)
	fn get_username(&self, begin_idx: usize) -> Result<String, &'static str> {
		// Extract a slice of the username from the header
		let byte_slice: &[u8] = &(self.0[begin_idx..begin_idx + 32]);
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
	// Put a unsigned short into the header at a specified index. Before
	// insertion the short is converted to network byte order.
	fn set_u16_at_index(&mut self, num: u16, header_idx: usize) -> &mut Self {
		// Convert the packet number to big endian (Network Byte order)
		let net_packet_num: [u8; 2] = num.to_be_bytes();
		// Place it at its spot in the message header.
		for (idx, byte) in net_packet_num.iter().enumerate() {
			self.0[idx + header_idx] = *byte;
		}
		self
	}
	// Return an unsigned short from the specified index in the header. It will
	// automatically be converted from network byte order to the host byte
	// order.
	fn get_u16_at_index(&self, idx: usize) -> u16 {
		// Convert 2 bytes to a u16
		// If this somehow failed... we'd have bigger fish to fry.
		let packet_num: &[u8; 2] = &self.0[idx..idx + 2].try_into().unwrap();
		u16::from_be_bytes(*packet_num)
	}
	// Create a new empty message header for use.
	pub fn new() -> MessageHeader {
		MessageHeader { 0: [0u8; 166] }
	}
	// Calculate the checksum of the entire header except for the checksum
	// field at the end.
	pub fn calculate_header_checksum(&mut self) {
		let mut hasher = Sha256::new();
		// Pass a reference of the portion of the header to be checksummed
		// to the hasher.
		hasher.update(&self.0[0..HEADER_CHECKSUM_BEGIN]);
		// Emplace the checksum into the header field.
		let mut begin_idx = HEADER_CHECKSUM_BEGIN;
		for byte in hasher.finalize().iter() {
			self.0[begin_idx] = *byte;
			begin_idx += 1;
		}
	}
	// Verify that the message header hasn't been tampered, by calculating
	// its SHA256 checksum and comparing it to the one stored within it.
	pub fn verify_header_checksum(&self) -> bool {
		let mut hasher = Sha256::new();
		hasher.update(&self.0[0..HEADER_CHECKSUM_BEGIN]);
		let hash = hasher.finalize();
		hash[..] == self.0[HEADER_CHECKSUM_BEGIN..]
	}
	// Calculate a SHA256 checksum for the message itself, and store it
	// within the header.
	pub fn calculate_data_checksum(&mut self, data_portion: &Vec<u8>) {
		let mut hasher = Sha256::new();
		// Pass a reference of the portion of data to be checksummed
		// to the hasher.
		hasher.update(data_portion);
		// Emplace the checksum into the header field.
		let mut begin_idx = DATA_PACKET_CHECKSUM_BEGIN;
		for byte in hasher.finalize().iter() {
			self.0[begin_idx] = *byte;
			begin_idx += 1;
		}
	}
	// Verify that the message itself hasn't been tampered with
	pub fn verify_data_checksum(&self, data_portion: &Vec<u8>) -> bool {
		let mut hasher = Sha256::new();
		hasher.update(data_portion);
		let hash = hasher.finalize();
		hash[..] == self.0[DATA_PACKET_CHECKSUM_BEGIN..FUTURE_USE_BEGIN]
	}
	// Emplace the packet number into the message header
	pub fn set_packet_num(&mut self, packet_num: u16) -> &mut Self {
		self.set_u16_at_index(packet_num, PACKET_NUMBER_BEGIN)
	}
	// return the packet number by extracting it from the message header.
	pub fn get_packet_num(&self) -> u16 {
		self.get_u16_at_index(PACKET_NUMBER_BEGIN)
	}
	pub fn set_version_num(&mut self, version_num: u8) -> &mut Self {
		self.0[HEADER_VERSION_BEGIN] = version_num;
		self
	}
	pub fn get_version_num(&self) -> u8 {
		self.0[HEADER_VERSION_BEGIN]
	}
	pub fn set_source_username(&mut self, username: &str) -> Result<&mut Self, &'static str> {
		self.set_username(username, SOURCE_USERNAME_BEGIN)
	}
	pub fn get_source_username(&self) -> Result<String, &'static str> {
		self.get_username(SOURCE_USERNAME_BEGIN)
	}
	pub fn set_dest_username(&mut self, username: &str) -> Result<&mut Self, &'static str> {
		self.set_username(username, DEST_USERNAME_BEGIN)
	}
	pub fn get_dest_username(&self) -> Result<String, &'static str> {
		self.get_username(DEST_USERNAME_BEGIN)
	}
	pub fn set_message_type(&mut self, m_type: u8) -> &mut Self {
		self.0[MESSAGE_TYPE_BEGIN] = m_type;
		self
	}
	pub fn get_message_type(&self) -> u8 {
		self.0[MESSAGE_TYPE_BEGIN]
	}
	pub fn set_data_packet_length(&mut self, packet_len: u16) -> &mut Self {
		self.set_u16_at_index(packet_len, DATA_PACKET_LENGTH_BEGIN)
	}
	pub fn get_data_packet_length(&self) -> u16 {
		self.get_u16_at_index(DATA_PACKET_LENGTH_BEGIN)
	}
}

impl Clone for MessageHeader {
	fn clone(&self) -> Self {
		MessageHeader { 0: self.0 }
	}
}

#[cfg(test)]
mod tests {
	use super::*;
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
		header.set_dest_username("brown").unwrap();
		let result = header.get_source_username().unwrap();
		assert!(result == "meow");
		let result = header.get_dest_username().unwrap();
		assert!(result == "brown");
		// Make sure usernames that are too long are not accepted.
		match header.set_source_username("123456789012345678901234567890123") {
			Ok(_) => Err("This username is too long, and should fail."),
			Err(_) => Ok(()),
		}
	}
	#[test]
	fn test_header_checksumming() {
		let mut header = MessageHeader::new();
		header.set_packet_num(5).set_version_num(3);
		header.calculate_header_checksum();
		// Make sure verification works
		assert!(header.verify_header_checksum());
		// Mess up the header
		header.set_packet_num(16);
		// make sure it gets caught.
		assert!(!(header.verify_header_checksum()));
	}
	#[test]
	fn test_data_checksumming() {
		// Make sure that the checksum catches when the data portion has been
		// tampered with
		let mut header = MessageHeader::new();
		let garbage: Vec<u8> = vec![0, 1, 2, 3];
		let garbage2: Vec<u8> = vec![3, 2, 1, 0];
		header.calculate_data_checksum(&garbage);
		assert!(header.verify_data_checksum(&garbage));
		assert!(!(header.verify_data_checksum(&garbage2)));
	}
	#[test]
	fn test_message_types() {
		let mut header = MessageHeader::new();
		assert!(header.set_message_type(5).get_message_type() == 5);
	}
	#[test]
	fn test_packet_lengths() {
		let mut header = MessageHeader::new();
		assert!(header.set_data_packet_length(56).get_data_packet_length() == 56);
	}
	#[test]
	fn clone_test() {
		let mut header = MessageHeader::new();
		header
			.set_data_packet_length(6)
			.set_dest_username("brown")
			.unwrap()
			.set_message_type(4);
		assert_eq!(header.0, header.clone().0)
	}
}
