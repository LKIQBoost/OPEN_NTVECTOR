#pragma once
#include <openssl/evp.h>
#include <openssl/ec.h>
#include <openssl/pem.h>
#include <openssl/err.h>
#include <stdio.h>
#include <openssl/ecdsa.h>
#include <openssl/bio.h>
#include <openssl/buffer.h>
#include <vector>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <ctime>
#pragma warning(disable: 4996)
class ECC
{
private:
	static void openssl_error();
public:
	static unsigned char* export_public_key(EVP_PKEY* pkey, size_t* pubkey_len);
	static EVP_PKEY* import_public_key(const unsigned char* pubkey, size_t pubkey_len);
	static EC_KEY* load_ec_public_key_from_pem(const std::string& pem_string);
	static EC_KEY* load_ec_private_key_from_pem(const std::string& pem_string);
	static EVP_PKEY* GetECPair(std::string* EC_PUBLIC, std::string* EC_PRIVATE);
	static unsigned char* derive_shared_secret(EVP_PKEY* own_privkey, EVP_PKEY* peer_pubkey, size_t* secret_len);
};

