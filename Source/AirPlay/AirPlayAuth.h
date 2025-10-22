#pragma once
#include <JuceHeader.h>
#include <memory>

/**
 * AirPlay Authentication Handler
 * 
 * Implements the AirPlay authentication protocol including:
 * - RSA key pair generation and management
 * - Challenge-response authentication
 * - AES encryption for audio streams
 * - Password-protected device support
 * 
 * Based on the RAOP (Remote Audio Output Protocol) specification
 * and reverse-engineered from shairport-sync implementation.
 */
class AirPlayAuth
{
public:
    AirPlayAuth();
    ~AirPlayAuth();

    /**
     * Initialize authentication with RSA key generation
     * @return true if initialization successful
     */
    bool initialize();

    /**
     * Generate Apple-Challenge for RTSP handshake
     * @return base64-encoded challenge string
     */
    juce::String generateChallenge();

    /**
     * Verify Apple-Response from server
     * @param response base64-encoded response from server
     * @param clientIP client IP address
     * @param serverIP server IP address
     * @return true if response is valid
     */
    bool verifyResponse(const juce::String& response, 
                       const juce::String& clientIP,
                       const juce::String& serverIP);

    /**
     * Get RSA public key in PEM format for ANNOUNCE request
     * @return base64-encoded RSA public key
     */
    juce::String getPublicKeyBase64() const;

    /**
     * Setup AES encryption for audio stream
     * @param aesKey base64-encoded AES key from server (if provided)
     * @param aesIV base64-encoded AES initialization vector
     * @return true if encryption setup successful
     */
    bool setupEncryption(const juce::String& aesKey, const juce::String& aesIV);

    /**
     * Encrypt audio data for transmission
     * @param data audio data to encrypt
     * @param size size of data
     * @param encryptedData output buffer for encrypted data
     * @return size of encrypted data, or -1 on error
     */
    int encryptAudioData(const void* data, size_t size, juce::MemoryBlock& encryptedData);

    /**
     * Set password for password-protected devices
     * @param password device password
     */
    void setPassword(const juce::String& password);

    /**
     * Check if authentication is initialized
     */
    bool isInitialized() const { return initialized; }

    /**
     * Check if encryption is enabled
     */
    bool isEncryptionEnabled() const { return encryptionEnabled; }

    /**
     * Get last error message
     */
    juce::String getLastError() const { return lastError; }

private:
    // Forward declarations for implementation-specific types
    struct Impl;
    std::unique_ptr<Impl> pimpl;

    bool initialized = false;
    bool encryptionEnabled = false;
    juce::String lastError;
    juce::String devicePassword;
    juce::MemoryBlock challengeData;

    // Helper methods
    juce::String base64Encode(const void* data, size_t size) const;
    juce::MemoryBlock base64Decode(const juce::String& encoded) const;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AirPlayAuth)
};
