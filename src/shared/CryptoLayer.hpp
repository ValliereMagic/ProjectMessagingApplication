#pragma once
extern "C" {
#include <sodium.h>
}
#include <string>
#include <vector>
#include <tuple>

namespace Crypto
{
using StreamKey =
	std::array<uint8_t, crypto_secretstream_xchacha20poly1305_KEYBYTES>;

// Take the passed password, and turn it into a proper symmetric key for use
// with xchacha20. Returns false on failure. (As element 0 of the tuple)
std::tuple<bool, StreamKey>
derive_key_from_password(const std::string &password);

// Take the passed cipher text and password, and return whether we were
// successful or failed, as the first element of a tuple. If we were successful
// the second element in the tuple will be the successfully decrypted cleartext.
std::tuple<bool, std::string> decrypt(const std::vector<uint8_t> &cipher_txt,
				      const StreamKey &dec_key);
// Take the passed clear text and password, and return whether we were
// successful or failed, as the first element of a tuple. If we were successful
// the second element in the tuple will be the successfully encrypted cleartext.
std::tuple<bool, std::vector<uint8_t> > encrypt(const std::string &clear_txt,
						const StreamKey &enc_key);
} // namespace Crypto
