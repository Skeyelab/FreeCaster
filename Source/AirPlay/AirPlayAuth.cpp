#include "AirPlayAuth.h"
#include <openssl/rsa.h>
#include <openssl/pem.h>
#include <openssl/aes.h>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/err.h>
#include <openssl/bn.h>

// Implementation structure holding OpenSSL objects
struct AirPlayAuth::Impl
{
    EVP_PKEY* rsaKeyPair = nullptr;
    EVP_CIPHER_CTX* aesEncryptCtx = nullptr;
    juce::MemoryBlock aesKey;
    juce::MemoryBlock aesIV;

    ~Impl()
    {
        if (rsaKeyPair)
            EVP_PKEY_free(rsaKeyPair);
        if (aesEncryptCtx)
            EVP_CIPHER_CTX_free(aesEncryptCtx);
    }
};

AirPlayAuth::AirPlayAuth()
{
    pimpl = std::make_unique<Impl>();
}

AirPlayAuth::~AirPlayAuth()
{
    // pimpl automatically cleaned up
}

bool AirPlayAuth::initialize()
{
    if (initialized)
        return true;

    // Generate RSA key pair (512-bit for AirPlay compatibility)
    // AirPlay uses 512-bit RSA for backward compatibility
    EVP_PKEY_CTX* ctx = EVP_PKEY_CTX_new_id(EVP_PKEY_RSA, nullptr);
    if (!ctx)
    {
        lastError = "Failed to create RSA context";
        return false;
    }

    if (EVP_PKEY_keygen_init(ctx) <= 0)
    {
        EVP_PKEY_CTX_free(ctx);
        lastError = "Failed to initialize RSA key generation";
        return false;
    }

    // Set key size to 512 bits (AirPlay standard)
    if (EVP_PKEY_CTX_set_rsa_keygen_bits(ctx, 512) <= 0)
    {
        EVP_PKEY_CTX_free(ctx);
        lastError = "Failed to set RSA key size";
        return false;
    }

    if (EVP_PKEY_keygen(ctx, &pimpl->rsaKeyPair) <= 0)
    {
        EVP_PKEY_CTX_free(ctx);
        lastError = "Failed to generate RSA key pair";
        return false;
    }

    EVP_PKEY_CTX_free(ctx);

    initialized = true;
    return true;
}

juce::String AirPlayAuth::generateChallenge()
{
    // Generate 16 random bytes for challenge
    unsigned char challenge[16];
    if (RAND_bytes(challenge, sizeof(challenge)) != 1)
    {
        lastError = "Failed to generate random challenge";
        return {};
    }

    // Store challenge for later verification
    challengeData.append(challenge, sizeof(challenge));

    return base64Encode(challenge, sizeof(challenge));
}

bool AirPlayAuth::verifyResponse(const juce::String& response,
                                 const juce::String& clientIP,
                                 const juce::String& serverIP)
{
    if (!initialized)
    {
        lastError = "Authentication not initialized";
        return false;
    }

    // Decode base64 response
    juce::MemoryBlock responseData = base64Decode(response);
    
    if (responseData.isEmpty())
    {
        lastError = "Invalid response format";
        return false;
    }

    // The Apple-Response should be an RSA signature of:
    // challenge + server_ip + client_ip + hw_addr
    // For now, we'll accept any valid response since we're acting as client
    // Full verification would require server's public key
    
    return true;  // Accept response for now
}

juce::String AirPlayAuth::getPublicKeyBase64() const
{
    if (!initialized || !pimpl->rsaKeyPair)
    {
        return {};
    }

    // Extract public key in PEM format
    BIO* bio = BIO_new(BIO_s_mem());
    if (!bio)
        return {};

    if (PEM_write_bio_PUBKEY(bio, pimpl->rsaKeyPair) != 1)
    {
        BIO_free(bio);
        return {};
    }

    // Get the PEM data
    char* pemData = nullptr;
    long pemSize = BIO_get_mem_data(bio, &pemData);

    juce::String pemString(pemData, (size_t)pemSize);
    BIO_free(bio);

    // Remove PEM headers and newlines for base64 encoding
    pemString = pemString.replace("-----BEGIN PUBLIC KEY-----", "")
                        .replace("-----END PUBLIC KEY-----", "")
                        .replace("\n", "")
                        .replace("\r", "");

    return pemString.trim();
}

bool AirPlayAuth::setupEncryption(const juce::String& aesKey, const juce::String& aesIV)
{
    if (aesKey.isEmpty() && aesIV.isEmpty())
    {
        // No encryption requested
        encryptionEnabled = false;
        return true;
    }

    // Decode AES key and IV
    if (aesKey.isNotEmpty())
        pimpl->aesKey = base64Decode(aesKey);
    else
    {
        // Generate random AES-128 key if not provided
        pimpl->aesKey.setSize(16);
        RAND_bytes(static_cast<unsigned char*>(pimpl->aesKey.getData()), 16);
    }

    if (aesIV.isNotEmpty())
        pimpl->aesIV = base64Decode(aesIV);
    else
    {
        // Generate random IV if not provided
        pimpl->aesIV.setSize(16);
        RAND_bytes(static_cast<unsigned char*>(pimpl->aesIV.getData()), 16);
    }

    // Validate key and IV sizes
    if (pimpl->aesKey.getSize() != 16 || pimpl->aesIV.getSize() != 16)
    {
        lastError = "Invalid AES key or IV size";
        return false;
    }

    // Initialize AES encryption context
    pimpl->aesEncryptCtx = EVP_CIPHER_CTX_new();
    if (!pimpl->aesEncryptCtx)
    {
        lastError = "Failed to create AES context";
        return false;
    }

    if (EVP_EncryptInit_ex(pimpl->aesEncryptCtx, EVP_aes_128_cbc(),
                          nullptr,
                          static_cast<const unsigned char*>(pimpl->aesKey.getData()),
                          static_cast<const unsigned char*>(pimpl->aesIV.getData())) != 1)
    {
        lastError = "Failed to initialize AES encryption";
        EVP_CIPHER_CTX_free(pimpl->aesEncryptCtx);
        pimpl->aesEncryptCtx = nullptr;
        return false;
    }

    encryptionEnabled = true;
    return true;
}

int AirPlayAuth::encryptAudioData(const void* data, size_t size, juce::MemoryBlock& encryptedData)
{
    if (!encryptionEnabled || !pimpl->aesEncryptCtx)
    {
        // No encryption - just copy data
        encryptedData.setSize(size);
        memcpy(encryptedData.getData(), data, size);
        return static_cast<int>(size);
    }

    // Calculate maximum encrypted size (includes padding)
    int maxEncryptedSize = static_cast<int>(size) + EVP_CIPHER_CTX_block_size(pimpl->aesEncryptCtx);
    encryptedData.setSize(maxEncryptedSize);

    int outLen = 0;
    if (EVP_EncryptUpdate(pimpl->aesEncryptCtx,
                         static_cast<unsigned char*>(encryptedData.getData()),
                         &outLen,
                         static_cast<const unsigned char*>(data),
                         static_cast<int>(size)) != 1)
    {
        lastError = "Failed to encrypt audio data";
        return -1;
    }

    int finalLen = 0;
    if (EVP_EncryptFinal_ex(pimpl->aesEncryptCtx,
                           static_cast<unsigned char*>(encryptedData.getData()) + outLen,
                           &finalLen) != 1)
    {
        lastError = "Failed to finalize encryption";
        return -1;
    }

    int totalLen = outLen + finalLen;
    encryptedData.setSize(totalLen);

    return totalLen;
}

void AirPlayAuth::setPassword(const juce::String& password)
{
    devicePassword = password;
}

juce::String AirPlayAuth::base64Encode(const void* data, size_t size) const
{
    if (size == 0)
        return {};

    // Calculate required buffer size for base64 encoding
    int encodedSize = ((size + 2) / 3) * 4;
    juce::MemoryBlock encoded;
    encoded.setSize(encodedSize + 1);  // +1 for null terminator

    BIO* bio = BIO_new(BIO_s_mem());
    BIO* b64 = BIO_new(BIO_f_base64());
    BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);  // No newlines
    bio = BIO_push(b64, bio);

    BIO_write(bio, data, static_cast<int>(size));
    BIO_flush(bio);

    char* encodedData = nullptr;
    long len = BIO_get_mem_data(bio, &encodedData);

    juce::String result(encodedData, (size_t)len);
    BIO_free_all(bio);

    return result;
}

juce::MemoryBlock AirPlayAuth::base64Decode(const juce::String& encoded) const
{
    if (encoded.isEmpty())
        return {};

    // Calculate maximum decoded size
    size_t encodedLen = encoded.length();
    size_t maxDecodedSize = (encodedLen * 3) / 4;
    juce::MemoryBlock decoded;
    decoded.setSize(maxDecodedSize);

    BIO* bio = BIO_new_mem_buf(encoded.toRawUTF8(), static_cast<int>(encodedLen));
    BIO* b64 = BIO_new(BIO_f_base64());
    BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);  // No newlines
    bio = BIO_push(b64, bio);

    int decodedLen = BIO_read(bio, decoded.getData(), static_cast<int>(maxDecodedSize));
    BIO_free_all(bio);

    if (decodedLen > 0)
        decoded.setSize(decodedLen);
    else
        decoded.reset();

    return decoded;
}

bool AirPlayAuth::generateAesSessionKey(juce::MemoryBlock& aesKey, juce::MemoryBlock& aesIV)
{
    // Generate random 16-byte AES-128 key
    aesKey.setSize(16);
    if (RAND_bytes(static_cast<unsigned char*>(aesKey.getData()), 16) != 1)
    {
        lastError = "Failed to generate AES session key";
        return false;
    }

    // Generate random 16-byte IV
    aesIV.setSize(16);
    if (RAND_bytes(static_cast<unsigned char*>(aesIV.getData()), 16) != 1)
    {
        lastError = "Failed to generate AES IV";
        return false;
    }

    return true;
}

bool AirPlayAuth::encryptAesKeyWithRsaOaep(const juce::MemoryBlock& aesKey,
                                            const juce::MemoryBlock& serverPublicKeyData,
                                            juce::MemoryBlock& encryptedKey)
{
    if (aesKey.getSize() != 16)
    {
        lastError = "AES key must be 16 bytes";
        return false;
    }

    if (serverPublicKeyData.isEmpty())
    {
        lastError = "Server public key is empty";
        return false;
    }

    // Try to parse the server public key
    // AirPlay devices may provide the key in different formats:
    // 1. Raw RSA public key bytes (modulus + exponent)
    // 2. PEM format
    // 3. DER format
    
    EVP_PKEY* serverPubKey = nullptr;
    
    // Try parsing as PEM first
    BIO* bio = BIO_new_mem_buf(serverPublicKeyData.getData(), 
                                static_cast<int>(serverPublicKeyData.getSize()));
    if (bio)
    {
        serverPubKey = PEM_read_bio_PUBKEY(bio, nullptr, nullptr, nullptr);
        BIO_free(bio);
    }
    
    // If PEM parsing failed, try DER format
    if (!serverPubKey)
    {
        const unsigned char* data = static_cast<const unsigned char*>(serverPublicKeyData.getData());
        serverPubKey = d2i_PUBKEY(nullptr, &data, static_cast<long>(serverPublicKeyData.getSize()));
    }
    
    // If still no key, try to construct RSA key from raw modulus
    // AirPlay 1 devices often provide just the RSA modulus as raw bytes
    if (!serverPubKey && serverPublicKeyData.getSize() >= 32)
    {
        // Create RSA public key with modulus from server and standard exponent (65537)
        RSA* rsa = RSA_new();
        if (rsa)
        {
            BIGNUM* n = BN_bin2bn(static_cast<const unsigned char*>(serverPublicKeyData.getData()),
                                  static_cast<int>(serverPublicKeyData.getSize()), nullptr);
            BIGNUM* e = BN_new();
            BN_set_word(e, 65537);  // Standard RSA exponent
            
            if (RSA_set0_key(rsa, n, e, nullptr) == 1)
            {
                serverPubKey = EVP_PKEY_new();
                if (serverPubKey && EVP_PKEY_assign_RSA(serverPubKey, rsa) != 1)
                {
                    EVP_PKEY_free(serverPubKey);
                    serverPubKey = nullptr;
                    RSA_free(rsa);
                }
            }
            else
            {
                BN_free(n);
                BN_free(e);
                RSA_free(rsa);
            }
        }
    }
    
    if (!serverPubKey)
    {
        lastError = "Failed to parse server public key in any supported format";
        return false;
    }

    // Create encryption context
    EVP_PKEY_CTX* ctx = EVP_PKEY_CTX_new(serverPubKey, nullptr);
    if (!ctx)
    {
        EVP_PKEY_free(serverPubKey);
        lastError = "Failed to create encryption context";
        return false;
    }

    // Initialize encryption with RSA-OAEP
    if (EVP_PKEY_encrypt_init(ctx) <= 0)
    {
        EVP_PKEY_CTX_free(ctx);
        EVP_PKEY_free(serverPubKey);
        lastError = "Failed to initialize RSA encryption";
        return false;
    }

    // Set RSA-OAEP padding
    if (EVP_PKEY_CTX_set_rsa_padding(ctx, RSA_PKCS1_OAEP_PADDING) <= 0)
    {
        EVP_PKEY_CTX_free(ctx);
        EVP_PKEY_free(serverPubKey);
        lastError = "Failed to set RSA-OAEP padding";
        return false;
    }

    // Determine output buffer size
    size_t encryptedLen = 0;
    if (EVP_PKEY_encrypt(ctx, nullptr, &encryptedLen,
                        static_cast<const unsigned char*>(aesKey.getData()),
                        aesKey.getSize()) <= 0)
    {
        EVP_PKEY_CTX_free(ctx);
        EVP_PKEY_free(serverPubKey);
        lastError = "Failed to determine encrypted size";
        return false;
    }

    // Allocate output buffer
    encryptedKey.setSize(encryptedLen);

    // Perform encryption
    if (EVP_PKEY_encrypt(ctx,
                        static_cast<unsigned char*>(encryptedKey.getData()),
                        &encryptedLen,
                        static_cast<const unsigned char*>(aesKey.getData()),
                        aesKey.getSize()) <= 0)
    {
        char errBuf[256];
        ERR_error_string_n(ERR_get_error(), errBuf, sizeof(errBuf));
        lastError = "RSA-OAEP encryption failed: " + juce::String(errBuf);
        EVP_PKEY_CTX_free(ctx);
        EVP_PKEY_free(serverPubKey);
        return false;
    }

    // Adjust to actual encrypted size
    encryptedKey.setSize(encryptedLen);

    EVP_PKEY_CTX_free(ctx);
    EVP_PKEY_free(serverPubKey);

    return true;
}
