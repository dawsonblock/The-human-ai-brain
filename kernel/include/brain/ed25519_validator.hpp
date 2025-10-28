#pragma once

#include <string>
#include <vector>
#include <optional>
#include <openssl/evp.h>
#include <openssl/pem.h>

namespace hab {

/**
 * @brief Ed25519 signature validator for configuration files
 * 
 * Verifies Ed25519 signatures on configuration files to prevent tampering.
 * Uses OpenSSL EVP interface for Ed25519 operations.
 * 
 * Expected format: config.json + config.json.sig (raw 64-byte signature)
 */
class Ed25519Validator {
public:
    /**
     * @brief Load public key from PEM file
     * @param public_key_path Path to Ed25519 public key in PEM format
     * @return true if key loaded successfully
     */
    bool load_public_key(const std::string& public_key_path);
    
    /**
     * @brief Load public key from raw bytes
     * @param public_key_bytes 32-byte Ed25519 public key
     * @return true if key loaded successfully
     */
    bool load_public_key_raw(const std::vector<uint8_t>& public_key_bytes);
    
    /**
     * @brief Verify signature on a file
     * @param file_path Path to file to verify
     * @param signature_path Path to signature file (raw 64 bytes)
     * @return true if signature is valid
     */
    bool verify_file(const std::string& file_path, 
                     const std::string& signature_path);
    
    /**
     * @brief Verify signature on data
     * @param data Data to verify
     * @param signature 64-byte Ed25519 signature
     * @return true if signature is valid
     */
    bool verify_data(const std::vector<uint8_t>& data,
                     const std::vector<uint8_t>& signature);
    
    /**
     * @brief Check if a valid public key is loaded
     */
    bool has_key() const { return pkey_ != nullptr; }
    
    /**
     * @brief Get last error message
     */
    std::string last_error() const { return last_error_; }
    
    // Cleanup
    ~Ed25519Validator();
    
private:
    EVP_PKEY* pkey_ = nullptr;
    std::string last_error_;
    
    std::vector<uint8_t> read_file(const std::string& path);
};

/**
 * @brief Validate configuration file with signature
 * 
 * Convenience function that checks for CONFIG_SIGNATURE_REQUIRED environment
 * variable and validates if required.
 * 
 * @param config_path Path to configuration file
 * @param public_key_path Path to Ed25519 public key (PEM format)
 * @return true if validation passes (or not required)
 */
bool validate_config_signature(const std::string& config_path,
                               const std::string& public_key_path);

} // namespace hab
