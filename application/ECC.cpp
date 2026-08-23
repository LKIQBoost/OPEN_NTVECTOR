#include "ECC.h"
#include "Logger.h"


void ECC::openssl_error() {
    LOG(LOG_ERROR, "[ECC] OpenSSL error occurred!");
    ERR_print_errors_fp(stderr);
    exit(EXIT_FAILURE);
}

// ������ԿΪ�ֽڴ�
unsigned char* ECC::export_public_key(EVP_PKEY* pkey, size_t* pubkey_len) {
    BIO* bio = BIO_new(BIO_s_mem());
    if (!bio) openssl_error();

    if (PEM_write_bio_PUBKEY(bio, pkey) <= 0) {
        BIO_free(bio);
        openssl_error();
    }

    BUF_MEM* bptr;
    BIO_get_mem_ptr(bio, &bptr);
    unsigned char* pubkey = (unsigned char*)malloc(bptr->length);
    if (!pubkey) {
        BIO_free(bio);
        openssl_error();
    }

    memcpy(pubkey, bptr->data, bptr->length);
    *pubkey_len = bptr->length;
    BIO_free(bio);

    return pubkey;
}

// ���ֽڴ����빫Կ
EVP_PKEY* ECC::import_public_key(const unsigned char* pubkey, size_t pubkey_len) {
    BIO* bio = BIO_new_mem_buf(pubkey, pubkey_len);
    if (!bio) openssl_error();

    EVP_PKEY* pkey = PEM_read_bio_PUBKEY(bio, NULL, NULL, NULL);
    if (!pkey) {
        BIO_free(bio);
        openssl_error();
    }

    BIO_free(bio);
    return pkey;
}

EVP_PKEY* ECC::GetECPair(std::string* EC_PUBLIC, std::string* EC_PRIVATE) {
    LOG(LOG_INFO, "[ECC] Generating EC key pair (secp384r1)");

    // ���� secp384r1 ������Կ��
    EC_KEY* ec_key = EC_KEY_new_by_curve_name(NID_secp384r1);
    if (!ec_key) {
        LOG(LOG_ERROR, "[ECC] Failed to create EC_KEY structure");
        openssl_error();
        return NULL;
    }

    // ������Կ��
    if (EC_KEY_generate_key(ec_key) != 1) {
        LOG(LOG_ERROR, "[ECC] Failed to generate EC key pair");
        openssl_error();
        return NULL;
    }
    LOG(LOG_INFO, "[ECC] EC key pair generated successfully");

    // ���� EVP_PKEY �ṹ���� EC_KEY �������
    EVP_PKEY* pkey = EVP_PKEY_new();
    if (!pkey) {
        openssl_error();
        return NULL;
    }

    if (EVP_PKEY_assign_EC_KEY(pkey, ec_key) != 1) {
        openssl_error();
        return NULL;
    }

    // ���� BIO �ڴ滺�����������
    BIO* bio_private = BIO_new(BIO_s_mem());
    BIO* bio_public = BIO_new(BIO_s_mem());

    if (!bio_private || !bio_public) {
        openssl_error();
        return NULL;
    }

    // �� PKCS#8 ��ʽд��˽Կ
    if (PEM_write_bio_PrivateKey(bio_private, pkey, NULL, NULL, 0, NULL, NULL) != 1) {
        openssl_error();
        return NULL;
    }

    // �� PKCS#8 ��ʽд�빫Կ
    if (PEM_write_bio_PUBKEY(bio_public, pkey) != 1) {
        openssl_error();
        return NULL;
    }

    // ��ȡ����������
    char* private_key_data;
    long private_key_len = BIO_get_mem_data(bio_private, &private_key_data);

    char* public_key_data;
    long public_key_len = BIO_get_mem_data(bio_public, &public_key_data);

    EC_PRIVATE->assign(private_key_data, private_key_len);
    EC_PUBLIC->assign(public_key_data, public_key_len);
    // ������Դ
    BIO_free(bio_private);
    BIO_free(bio_public);
    //EVP_PKEY_free(pkey); // ����Զ��ͷ� ec_key
    return pkey;

}


// ִ��ECDH��Կ���������ɹ�����Կ
unsigned char* ECC::derive_shared_secret(EVP_PKEY* own_privkey, EVP_PKEY* peer_pubkey, size_t* secret_len) {
    LOG(LOG_NETWORK, "[ECC] Deriving shared secret via ECDH");
    EVP_PKEY_CTX* ctx = EVP_PKEY_CTX_new(own_privkey, NULL);
    if (!ctx) {
        LOG(LOG_ERROR, "[ECC] Failed to create PKEY context for ECDH");
        openssl_error();
    }

    if (EVP_PKEY_derive_init(ctx) <= 0) {
        EVP_PKEY_CTX_free(ctx);
        openssl_error();
    }

    if (EVP_PKEY_derive_set_peer(ctx, peer_pubkey) <= 0) {
        EVP_PKEY_CTX_free(ctx);
        openssl_error();
    }

    // �Ȼ�ȡ������Կ����
    size_t len;
    if (EVP_PKEY_derive(ctx, NULL, &len) <= 0) {
        EVP_PKEY_CTX_free(ctx);
        openssl_error();
    }

    unsigned char* secret = (unsigned char*)malloc(len);
    if (!secret) {
        EVP_PKEY_CTX_free(ctx);
        openssl_error();
    }

    if (EVP_PKEY_derive(ctx, secret, &len) <= 0) {
        free(secret);
        EVP_PKEY_CTX_free(ctx);
        openssl_error();
    }

    *secret_len = len;
    LOG(LOG_NETWORK, "[ECC] Shared secret derived successfully, length: ", len);
    EVP_PKEY_CTX_free(ctx);
    return secret;
}

// �� PEM �ַ������� EC ˽Կ
EC_KEY* ECC::load_ec_private_key_from_pem(const std::string& pem_string) {
    BIO* bio = BIO_new_mem_buf(pem_string.data(), pem_string.size());
    if (!bio) {
        std::cerr << "Error creating BIO" << std::endl;
        return nullptr;
    }

    EC_KEY* ec_key = PEM_read_bio_ECPrivateKey(bio, nullptr, nullptr, nullptr);
    if (!ec_key) {
        std::cerr << "Error reading EC private key: " << ERR_error_string(ERR_get_error(), nullptr) << std::endl;
    }

    BIO_free(bio);
    return ec_key;
}

// �� PEM �ַ������� EC ��Կ
EC_KEY* ECC::load_ec_public_key_from_pem(const std::string& pem_string) {
    BIO* bio = BIO_new_mem_buf(pem_string.data(), pem_string.size());
    if (!bio) {
        std::cerr << "Error creating BIO" << std::endl;
        return nullptr;
    }

    EC_KEY* ec_key = PEM_read_bio_EC_PUBKEY(bio, nullptr, nullptr, nullptr);
    if (!ec_key) {
        std::cerr << "Error reading EC public key: " << ERR_error_string(ERR_get_error(), nullptr) << std::endl;
    }

    BIO_free(bio);
    return ec_key;
}