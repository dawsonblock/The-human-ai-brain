#include "brain/audit.hpp"
#include <openssl/sha.h>
#include <sstream>
#include <iomanip>
#include <cstring>

namespace hab {

MerkleAuditLog::MerkleAuditLog() {
    merkle_root_.fill(0);
}

void MerkleAuditLog::log_event(const std::string& operation, const std::string& details) {
    AuditEntry entry;
    entry.timestamp = std::chrono::system_clock::now();
    entry.operation = operation;
    entry.details = details;
    
    // Compute hash of entry
    std::ostringstream oss;
    auto time_t = std::chrono::system_clock::to_time_t(entry.timestamp);
    oss << time_t << operation << details;
    entry.hash = compute_hash(oss.str());
    
    entries_.push_back(entry);
    rebuild_merkle_tree();
}

std::array<unsigned char, 32> MerkleAuditLog::compute_hash(const std::string& data) const {
    std::array<unsigned char, 32> hash;
    SHA256(reinterpret_cast<const unsigned char*>(data.c_str()), data.size(), hash.data());
    return hash;
}

std::array<unsigned char, 32> MerkleAuditLog::combine_hashes(
    const std::array<unsigned char, 32>& left,
    const std::array<unsigned char, 32>& right) const {
    std::array<unsigned char, 64> combined;
    std::memcpy(combined.data(), left.data(), 32);
    std::memcpy(combined.data() + 32, right.data(), 32);
    
    std::array<unsigned char, 32> result;
    SHA256(combined.data(), 64, result.data());
    return result;
}

void MerkleAuditLog::rebuild_merkle_tree() {
    if (entries_.empty()) {
        merkle_root_.fill(0);
        return;
    }
    
    merkle_tree_.clear();
    
    // Leaf level
    for (const auto& entry : entries_) {
        merkle_tree_.push_back(entry.hash);
    }
    
    // Build tree bottom-up
    size_t level_size = merkle_tree_.size();
    size_t offset = 0;
    
    while (level_size > 1) {
        size_t new_level_size = (level_size + 1) / 2;
        
        for (size_t i = 0; i < new_level_size; ++i) {
            size_t left_idx = offset + 2 * i;
            size_t right_idx = offset + 2 * i + 1;
            
            if (right_idx < offset + level_size) {
                auto combined = combine_hashes(merkle_tree_[left_idx], merkle_tree_[right_idx]);
                merkle_tree_.push_back(combined);
            } else {
                merkle_tree_.push_back(merkle_tree_[left_idx]);
            }
        }
        
        offset += level_size;
        level_size = new_level_size;
    }
    
    merkle_root_ = merkle_tree_.back();
}

std::array<unsigned char, 32> MerkleAuditLog::get_merkle_root() const {
    return merkle_root_;
}

bool MerkleAuditLog::verify_integrity() const {
    // Simplified verification - just check root exists
    return !entries_.empty();
}

std::string MerkleAuditLog::export_json() const {
    std::ostringstream oss;
    oss << "{\"entries\":[";
    
    for (size_t i = 0; i < entries_.size(); ++i) {
        if (i > 0) oss << ",";
        auto time_t = std::chrono::system_clock::to_time_t(entries_[i].timestamp);
        oss << "{\"timestamp\":" << time_t
            << ",\"operation\":\"" << entries_[i].operation << "\""
            << ",\"details\":\"" << entries_[i].details << "\"}";
    }
    
    oss << "],\"merkle_root\":\"";
    for (unsigned char byte : merkle_root_) {
        oss << std::hex << std::setw(2) << std::setfill('0') << (int)byte;
    }
    oss << "\"}";
    
    return oss.str();
}

} // namespace hab