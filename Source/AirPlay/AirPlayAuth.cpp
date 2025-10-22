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
