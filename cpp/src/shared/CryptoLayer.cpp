#include <iostream>
#include "CryptoLayer.hpp"

namespace Crypto
{
// Simple salt to use in password derivation
static const constexpr uint8_t salt[] = "(Q*&^#$lkjdashfg";

// Take the passed password, and turn it into a proper symmetric key for use
// with xchacha20. Returns false on failure. (As element 0 of the tuple)
std::tuple<bool, StreamKey>
derive_key_from_password(const std::string &password)
{
	// Initialize libsodium
	if (sodium_init() < 0) {
		std::cerr << "Unable to initialize libsodium.\n";
		return std::make_pair(false, StreamKey());
	}
	StreamKey key;
	key.fill(0);
	// Derive the key using crypto_pwhash
	uint8_t success = crypto_pwhash(
		key.data(), crypto_secretstream_xchacha20poly1305_KEYBYTES,
		password.c_str(), password.size(), salt,
		crypto_pwhash_OPSLIMIT_MODERATE,
		crypto_pwhash_MEMLIMIT_MODERATE, crypto_pwhash_ALG_DEFAULT);
	// Make sure we were able to successfully derive a key from the password
	// passed.
	if (success != 0) {
		std::cerr << "Error. Unable to derive symmetric "
			     "key from password in key_derive.\n";
		return std::make_pair(false, StreamKey());
	}
	return std::make_pair(true, std::move(key));
}

// Take the passed cipher text and password, and return whether we were
// successful or failed, as the first element of a tuple. If we were successful
// the second element in the tuple will be the successfully decrypted cleartext.
std::tuple<bool, std::string> decrypt(const std::vector<uint8_t> &cipher_txt,
				      const StreamKey &dec_key)
{
	// Initialize libsodium
	if (sodium_init() < 0) {
		std::cerr << "Unable to initialize libsodium.\n";
		return std::make_pair(false, std::string());
	}
	// Pull the header from the ciphertext
	std::array<uint8_t, crypto_secretstream_xchacha20poly1305_HEADERBYTES>
		header;
	// Pull the header from the ciphertext
	for (size_t it = 0;
	     it < crypto_secretstream_xchacha20poly1305_HEADERBYTES; ++it) {
		header[it] = cipher_txt[it];
	}
	// set up the state
	crypto_secretstream_xchacha20poly1305_state dec_state;
	if (crypto_secretstream_xchacha20poly1305_init_pull(
		    &dec_state, header.data(), dec_key.data()) != 0) {
		std::cerr << "Error setting up the header in decryption.\n";
		return std::make_pair(false, std::string());
	}
	// Begin decryption process
	std::vector<uint8_t> clear_txt(
		cipher_txt.size() -
		crypto_secretstream_xchacha20poly1305_ABYTES -
		crypto_secretstream_xchacha20poly1305_HEADERBYTES);
	unsigned long long out_buffer_length;
	unsigned char tag;
	// Decrypt the message
	if (crypto_secretstream_xchacha20poly1305_pull(
		    &dec_state, clear_txt.data(), &out_buffer_length, &tag,
		    cipher_txt.data() +
			    crypto_secretstream_xchacha20poly1305_HEADERBYTES,
		    cipher_txt.size() -
			    crypto_secretstream_xchacha20poly1305_HEADERBYTES,
		    nullptr, 0) != 0) {
		std::cerr << "Error while trying to decrypt message.\n";
		return std::make_pair(false, std::string());
	}
	// We only ever encrypted 1 chunk (This is a stream cipher)
	// So tag better be final.
	if (tag != crypto_secretstream_xchacha20poly1305_TAG_FINAL) {
		std::cerr << "Error. Tag is messed up in decryption.\n";
		return std::make_pair(false, std::string());
	}
	// Package up the cleartext into a string, and return.
	return std::make_pair(true,
			      std::string(clear_txt.begin(), clear_txt.end()));
}

// Take the passed clear text and password, and return whether we were
// successful or failed, as the first element of a tuple. If we were successful
// the second element in the tuple will be the successfully encrypted cleartext.
std::tuple<bool, std::vector<uint8_t> > encrypt(const std::string &clear_txt,
						const StreamKey &enc_key)
{
	// Initialize libsodium
	if (sodium_init() < 0) {
		std::cerr << "Unable to initialize libsodium.\n";
		return std::make_pair(false, std::vector<uint8_t>());
	}
	// Begin the encryption process
	std::vector<uint8_t> cipher_txt;
	// Create the encryption header and state
	crypto_secretstream_xchacha20poly1305_state enc_state;
	std::array<uint8_t, crypto_secretstream_xchacha20poly1305_HEADERBYTES>
		header;
	if (crypto_secretstream_xchacha20poly1305_init_push(
		    &enc_state, header.data(), enc_key.data()) != 0) {
		std::cerr
			<< "Error. Unable to initialize header in encrypt function.\n";
		return std::make_pair(false, std::vector<uint8_t>());
	}
	// Put the header at the start of the cipher_txt vector
	cipher_txt.insert(cipher_txt.end(), header.begin(), header.end());
	// Encrypt the string, and put it in the message
	std::vector<uint8_t> enc_buff(
		clear_txt.size() +
		crypto_secretstream_xchacha20poly1305_ABYTES);
	unsigned long long enc_buff_len;
	if (crypto_secretstream_xchacha20poly1305_push(
		    &enc_state, enc_buff.data(), &enc_buff_len,
		    (const unsigned char *)clear_txt.data(), clear_txt.size(),
		    nullptr, 0,
		    crypto_secretstream_xchacha20poly1305_TAG_FINAL) != 0) {
		std::cerr << "Error. Unable to encrypt message.\n";
		return std::make_pair(false, std::vector<uint8_t>());
	}
	cipher_txt.insert(cipher_txt.end(), enc_buff.begin(), enc_buff.end());
	// Were done here.
	return std::make_pair(true, std::move(cipher_txt));
}
} // namespace Crypto
