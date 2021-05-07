use crate::messaging_client::MessagingClient;
use message_layer::application_layer::Message;
use std::collections::HashMap;
use std::rc::Rc;
use std::sync::RwLock;

static mut CLIENTS_SINGLETON: Option<SharedClients> = None;

pub struct SharedClients {
	// The clients currently logged into the server
	clients: RwLock<HashMap<String, Rc<MessagingClient>>>,
}

impl SharedClients {
	// Return an immutable reference to the static SharedClients instance.
	// The contents of which are protected by a RwLock.
	pub fn get_instance() -> &'static SharedClients {
		// unsafe because of mutable static
		unsafe {
			if CLIENTS_SINGLETON.is_none() {
				CLIENTS_SINGLETON = Some(SharedClients {
					clients: RwLock::new(HashMap::new()),
				})
			}
			&CLIENTS_SINGLETON.as_ref().unwrap()
		}
	}
	// Send a message to a specific client, represented by their username (dest_username)
	pub fn send_to_client(&'static self, dest_username: &str, message: &Message) -> bool {
		// unlock RwLock for reading
		let unlocked_clients = self.clients.read().unwrap();
		// Find the user in the clients Map
		let user: &MessagingClient;
		match unlocked_clients.get(dest_username) {
			Some(u) => user = u,
			None => return false,
		};
		// Send them the message.
		let client_handle = user.get_client_fd();
		match client_handle.write_basic_message(message) {
			// Was able to send
			Ok(_) => true,
			// Failed to send
			Err(e) => {
				println!("Error: {}", e);
				false
			}
		}
	}
	// Send the passed message to all logged clients except ourselves
	pub fn send_to_all(&'static self, source_username: &str, message: &Message) -> bool {
		let unlocked_clients = self.clients.read().unwrap();
		// Send the message to all other clients
		for (username, client) in unlocked_clients.iter() {
			// Don't send the message to ourselves
			if username != source_username {
				match client.get_client_fd().write_basic_message(message) {
					Ok(_) => (),
					Err(e) => {
						println!("Error: {}", e);
						return false;
					}
				}
			}
		}
		true
	}
	// Get a list of the logged in users (For the WHO message request)
	pub fn get_logged_in_users(&'static self) -> String {
		let unlocked_clients = self.clients.read().unwrap();
		let mut csv_usernames = String::new();
		// Pull each of the usernames, and format them as part of a string.
		for (username, _) in unlocked_clients.iter() {
			csv_usernames.push_str(&(format!("{}, ", username)));
		}
		csv_usernames
	}
	// Add a new user that has just logged in, to the system This returns an Arc
	// pointing to the freshly added client on success, and moves the client
	// back to the caller on failure.
	pub fn add_user<'a>(
		&'static self,
		username: &str,
		client: MessagingClient,
	) -> Result<Rc<MessagingClient>, MessagingClient> {
		let mut unlocked_clients = self.clients.write().unwrap();
		if !unlocked_clients.contains_key(username) {
			unlocked_clients.insert(username.to_owned(), Rc::new(client));
			// Get a pointer to the now-inserted MessagingClient.
			return Ok(unlocked_clients.get(username).unwrap().clone());
		}
		Err(client)
	}
	// Remove a user from the system, and return it
	pub fn remove_user(&'static self, username: &str) -> Option<Rc<MessagingClient>> {
		let mut unlocked_clients = self.clients.write().unwrap();
		unlocked_clients.remove(username)
	}
}
