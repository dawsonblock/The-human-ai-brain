#pragma once

#include <string>
#include <vector>
#include <chrono>
#include <array>

namespace hab {

// Audit log entry
struct AuditEntry {
    std::chrono::system_clock::time_point timestamp;
    std::string operation;
    std::string details;
    std::array<unsigned char, 32> hash;  // SHA-256
};

// Merkle Audit Log - Tamper-evident event logging
class MerkleAuditLog {
public:
    MerkleAuditLog();
    
    // Log an event
    void log_event(const std::string& operation, const std::string& details);
    
    // Get Merkle root
    std::array<unsigned char, 32> get_merkle_root() const;
    
    // Verify log integrity
    bool verify_integrity() const;
    
    // Get all entries
    const std::vector<AuditEntry>& entries() const { return entries_; }
    
    // Export to JSON
    std::string export_json() const;
    
private:
    std::array<unsigned char, 32> compute_hash(const std::string& data) const;
    std::array<unsigned char, 32> combine_hashes(
        const std::array<unsigned char, 32>& left,
        const std::array<unsigned char, 32>& right) const;
    void rebuild_merkle_tree();
    
    std::vector<AuditEntry> entries_;
    std::array<unsigned char, 32> merkle_root_;
    std::vector<std::array<unsigned char, 32>> merkle_tree_;
};

} // namespace hab
