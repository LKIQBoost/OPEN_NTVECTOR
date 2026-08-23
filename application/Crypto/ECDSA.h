#include <openssl/ec.h>
#include <openssl/ecdsa.h>
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/err.h>
#include <string>
#include <vector>
#include <sstream>

#pragma warning(disable: 4996)
#pragma once
class ECDSA
{
public:
	static std::string sign_es384(const std::string& data, EC_KEY* ec_key);
	static void EVP_PKEY_AS_EC_KEY(EVP_PKEY* EVPKey, EC_KEY* ECKey);
	static void EC_KEY_AS_EVP_PKEY(EVP_PKEY* EVPKey, EC_KEY* ECKey);
};

