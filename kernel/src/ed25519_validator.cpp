#include "brain/ed25519_validator.hpp"
#include <fstream>
#include <cstring>
#include <iostream>

namespace hab {

Ed25519Validator::~Ed25519Validator() {
    if (pkey_) {
        EVP_PKEY_free(pkey_);
        pkey_ = nullptr;
    }
}

bool Ed25519Validator::load_public_key(const std::string& public_key_path) {
    FILE* fp = fopen(public_key_path.c_str(), "r");
    if (!fp) {
        last_error_ = "Failed to open public key file: " + public_key_path;
        return false;
    }
    
    pkey_ = PEM_read_PUBKEY(fp, nullptr, nullptr, nullptr);
    fclose(fp);
    
    if (!pkey_) {
        last_error_ = "Failed to parse Ed25519 public key from PEM";
        return false;
    }
    
    // Verify it's Ed25519
    if (EVP_PKEY_id(pkey_) != EVP_PKEY_ED25519) {
        EVP_PKEY_free(pkey_);
        pkey_ = nullptr;
        last_error_ = "Key is not Ed25519";
        return false;
    }
    
    return true;
}

bool Ed25519Validator::load_public_key_raw(const std::vector<uint8_t>& public_key_bytes) {
    if (public_key_bytes.size() != 32) {
        last_error_ = "Ed25519 public key must be 32 bytes";
        return false;
    }
    
    pkey_ = EVP_PKEY_new_raw_public_key(EVP_PKEY_ED25519, nullptr,
                                         public_key_bytes.data(),
                                         public_key_bytes.size());
    if (!pkey_) {
        last_error_ = "Failed to create Ed25519 key from raw bytes";
        return false;
    }
    
    return true;
}

std::vector<uint8_t> Ed25519Validator::read_file(const std::string& path) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file) {
        last_error_ = "Failed to open file: " + path;
        return {};
    }
    
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);
    
    std::vector<uint8_t> buffer(size);
    if (!file.read(reinterpret_cast<char*>(buffer.data()), size)) {
        last_error_ = "Failed to read file: " + path;
        return {};
    }
    
    return buffer;
}

bool Ed25519Validator::verify_file(const std::string& file_path,
                                    const std::string& signature_path) {
    if (!pkey_) {
        last_error_ = "No public key loaded";
        return false;
    }
    
    // Read file data
    auto data = read_file(file_path);
    if (data.empty()) {
        return false;
    }
    
    // Read signature (should be 64 bytes for Ed25519)
    auto signature = read_file(signature_path);
    if (signature.size() != 64) {
        last_error_ = "Invalid signature size (expected 64 bytes, got " + 
                      std::to_string(signature.size()) + ")";
        return false;
    }
    
    return verify_data(data, signature);
}

bool Ed25519Validator::verify_data(const std::vector<uint8_t>& data,
                                    const std::vector<uint8_t>& signature) {
    if (!pkey_) {
        last_error_ = "No public key loaded";
        return false;
    }
    
    if (signature.size() != 64) {
        last_error_ = "Ed25519 signature must be 64 bytes";
        return false;
    }
    
    // Create verification context
    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    if (!ctx) {
        last_error_ = "Failed to create EVP_MD_CTX";
        return false;
    }
    
    // Initialize verification
    if (EVP_DigestVerifyInit(ctx, nullptr, nullptr, nullptr, pkey_) != 1) {
        EVP_MD_CTX_free(ctx);
        last_error_ = "Failed to initialize signature verification";
        return false;
    }
    
    // Verify signature (Ed25519 is one-shot operation)
    int result = EVP_DigestVerify(ctx, signature.data(), signature.size(),
                                   data.data(), data.size());
    
    EVP_MD_CTX_free(ctx);
    
    if (result == 1) {
        return true;  // Signature valid
    } else if (result == 0) {
        last_error_ = "Signature verification failed - signature is invalid";
        return false;
    } else {
        last_error_ = "Signature verification error";
        return false;
    }
}

bool validate_config_signature(const std::string& config_path,
                               const std::string& public_key_path) {
    // Check if signature validation is required
    const char* require_sig = std::getenv("CONFIG_SIGNATURE_REQUIRED");
    if (!require_sig || std::string(require_sig) != "1") {
        // Signature validation not required
        return true;
    }
    
    std::cout << "Ed25519 signature validation enabled for config" << std::endl;
    
    Ed25519Validator validator;
    
    // Load public key
    if (!validator.load_public_key(public_key_path)) {
        std::cerr << "Failed to load public key: " << validator.last_error() << std::endl;
        return false;
    }
    
    // Verify signature
    std::string sig_path = config_path + ".sig";
    if (!validator.verify_file(config_path, sig_path)) {
        std::cerr << "Config signature verification failed: " 
                  << validator.last_error() << std::endl;
        return false;
    }
    
    std::cout << "✓ Config signature verified successfully" << std::endl;
    return true;
}

} // namespace hab
