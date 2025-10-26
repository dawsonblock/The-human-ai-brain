// preconscious_buffer.cpp
//
// FDQC v4.0: Preconscious Buffer Implementation
//
// Implements 2-second sensory memory buffer with similarity-based
// deduplication and importance-gated consolidation.

#include "preconscious_buffer.h"
#include <algorithm>
#include <cmath>
#include <functional>

namespace preconscious_buffer {

//===========================================================================
// ENTRY ADDITION
//===========================================================================

void PreConsciousBuffer::push(const std::vector<double>& embedding,
                              const std::string& content,
                              double salience) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Check for similar recent entry (deduplication)
    bool is_duplicate = false;
    
    if (!buffer_.empty()) {
        const auto& last_entry = buffer_.back();
        double similarity = cosine_similarity(embedding, last_entry.embedding);
        
        if (similarity >= fdqc_params::BUFFER_SIMILARITY_THRESHOLD) {
            is_duplicate = true;
            stats_.duplicates_filtered++;
        }
    }
    
    // Add entry even if duplicate (but mark it)
    BufferEntry entry(embedding, content, salience);
    
    if (!buffer_.empty()) {
        const auto& last_entry = buffer_.back();
        entry.similarity_to_previous = cosine_similarity(embedding, last_entry.embedding);
    }
    
    buffer_.push_back(entry);
    
    // Update statistics
    update_stats(salience, is_duplicate);
    
    // Maintain buffer size limit
    if (buffer_.size() > fdqc_params::BUFFER_SIZE) {
        buffer_.pop_front();
    }
    
    // Periodically clean up expired entries
    if (stats_.total_entries_processed % 10 == 0) {
        expire_old_entries();
    }
}

void PreConsciousBuffer::push(const std::string& entry) {
    // Convert string to embedding
    std::vector<double> embedding = string_to_embedding(entry);
    
    // Use default salience
    push(embedding, entry, 0.5);
}

//===========================================================================
// SIMILARITY SEARCH
//===========================================================================

std::vector<size_t> PreConsciousBuffer::find_similar(
    const std::vector<double>& query_embedding,
    double threshold) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::vector<size_t> results;
    
    for (size_t i = 0; i < buffer_.size(); ++i) {
        const auto& entry = buffer_[i];
        
        // Skip expired entries
        if (is_expired(entry)) {
            continue;
        }
        
        double similarity = cosine_similarity(query_embedding, entry.embedding);
        
        if (similarity >= threshold) {
            results.push_back(i);
        }
    }
    
    return results;
}

bool PreConsciousBuffer::contains_similar(
    const std::vector<double>& embedding,
    double threshold) const {
    
    for (const auto& entry : buffer_) {
        if (is_expired(entry)) {
            continue;
        }
        
        double similarity = cosine_similarity(embedding, entry.embedding);
        
        if (similarity >= threshold) {
            return true;
        }
    }
    
    return false;
}

//===========================================================================
// RETRIEVAL
//===========================================================================

std::vector<BufferEntry> PreConsciousBuffer::get_salient_entries(
    double threshold) const {
    
    std::vector<BufferEntry> result;
    
    for (const auto& entry : buffer_) {
        if (!is_expired(entry) && entry.salience >= threshold) {
            result.push_back(entry);
        }
    }
    
    return result;
}

std::vector<BufferEntry> PreConsciousBuffer::get_recent(size_t n) const {
    std::vector<BufferEntry> result;
    
    // Get last n non-expired entries
    for (auto it = buffer_.rbegin(); it != buffer_.rend() && result.size() < n; ++it) {
        if (!is_expired(*it)) {
            result.push_back(*it);
        }
    }
    
    // Reverse to get chronological order
    std::reverse(result.begin(), result.end());
    
    return result;
}

//===========================================================================
// EXPIRATION
//===========================================================================

void PreConsciousBuffer::expire_old_entries() {
    // Remove entries older than 2 seconds
    auto now = std::chrono::steady_clock::now();
    
    auto new_end = std::remove_if(
        buffer_.begin(),
        buffer_.end(),
        [this](const BufferEntry& entry) {
            return this->is_expired(entry);
        }
    );
    
    buffer_.erase(new_end, buffer_.end());
}

//===========================================================================
// EMBEDDING COMPUTATION (SIMPLIFIED)
//===========================================================================

std::vector<double> PreConsciousBuffer::string_to_embedding(
    const std::string& str) const {
    
    // Simplified embedding: hash-based feature vector
    // In a real system, use learned embeddings or pre-trained models
    
    constexpr size_t EMBEDDING_DIM = 128;
    std::vector<double> embedding(EMBEDDING_DIM, 0.0);
    
    // Create deterministic features from string
    std::hash<std::string> hasher;
    size_t base_hash = hasher(str);
    
    // Generate features
    for (size_t d = 0; d < EMBEDDING_DIM; ++d) {
        // Mix hash with dimension index
        size_t dim_hash = base_hash ^ (d * 0x9e3779b9);  // Golden ratio constant
        
        // Map to [-1, 1] using sine
        embedding[d] = std::sin(dim_hash * 0.001);
    }
    
    // Normalize
    double norm = 0.0;
    for (double val : embedding) {
        norm += val * val;
    }
    
    if (norm > fdqc_params::EPSILON) {
        double inv_norm = 1.0 / std::sqrt(norm);
        for (double& val : embedding) {
            val *= inv_norm;
        }
    }
    
    return embedding;
}

//===========================================================================
// STATISTICS UPDATE
//===========================================================================

void PreConsciousBuffer::update_stats(double salience, bool is_duplicate) {
    stats_.total_entries_processed++;
    
    // Update average salience (exponential moving average)
    double alpha = 0.9;
    if (stats_.total_entries_processed == 1) {
        stats_.average_salience = salience;
    } else {
        stats_.average_salience = alpha * stats_.average_salience + 
                                 (1.0 - alpha) * salience;
    }
}

} // namespace preconscious_buffer
