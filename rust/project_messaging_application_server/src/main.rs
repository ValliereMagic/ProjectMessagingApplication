use std::io::Read;
use std::net::{TcpListener, TcpStream};
use std::thread;

fn login(mut client: TcpStream) {
	let mut message = [0u8; 166];
	match client.read_exact(&mut message) {
		// Were good, don't need to do anything
		Ok(_) => {}
		// Error out and return.
		Err(val) => {
			eprintln!(
				"Unable to read message from client, disconnecting.: {}",
				val
			);
			return;
		}
	}
	// For now, print out the message...
	for byte in message.iter() {
		print!("{:x}:", byte);
	}
	println!("\nDone with this connection.");
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
