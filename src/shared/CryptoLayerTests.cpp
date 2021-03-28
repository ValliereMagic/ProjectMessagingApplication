#include "CryptoLayer.hpp"
#include <cassert>
#include <iostream>

int main(void)
{
	std::string banana = "banana";
	std::string password = "password";
	// Derive the enc key from the password
	auto key_deriv = Crypto::derive_key_from_password(password);
	assert(std::get<0>(key_deriv));
	// Test encrypting a string
	auto enc_banana = Crypto::encrypt(banana, std::get<1>(key_deriv));
	assert(std::get<0>(enc_banana));
	// Test decrypting a string
	auto dec_banana = Crypto::decrypt(std::get<1>(enc_banana),
					  std::get<1>(key_deriv));
	assert(std::get<0>(dec_banana));
	// Print out the result for fun
	std::cout << std::get<1>(dec_banana) << "\n";
	return 0;
}
