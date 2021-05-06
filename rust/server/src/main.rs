extern crate message_layer;
use message_layer::application_layer::MessageLayer;
use std::net::{TcpListener, TcpStream};
use std::thread;

fn login(client: TcpStream) {
	let app_layer = MessageLayer::new(client);
	// Iterate through the messages being sent to us from the client.
	for message in app_layer {
		// message is a Result, if Ok then it is a tuple of (header, data
		// portion). If it is an Err, then its a String containing an error.
		match message {
			// For now, lets print out all the information about the messages we
			// are receiving.
			Ok(m) => {
				println!("Message Information:");
				println!("Packet Number: {}", m.0.get_packet_num());
				println!("Version Number: {}", m.0.get_version_num());
				let username = match m.0.get_source_username() {
					Ok(m) => m,
					Err(_) => String::from("None"),
				};
				println!("Source Username: {}", username);
				let username = match m.0.get_dest_username() {
					Ok(m) => m,
					Err(_) => String::from("None"),
				};
				println!("Dest username: {}", username);
				println!("Message Type: {}", m.0.get_message_type());
				println!(
					"Message data packet length: {}",
					m.0.get_data_packet_length()
				);
				println!("Sent message if it exists:");
				match m.1 {
					Some(message) => {
						for byte in message {
							print!("{:x}:", byte);
						}
						println!();
					}
					None => (),
				}
			}
			Err(e) => {
				println!("An error occurred reading the message!: {}", e);
				break;
			}
		}
	}
	println!("Done with this connection.");
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
