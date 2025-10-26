// chunking.cpp
//
// FDQC v4.0: Chunking Implementation
//
// Implements pattern-based chunking to expand working memory capacity
// from base n=4 up to effective capacity of 7±2 items.

#include "chunking.h"
#include <algorithm>
#include <numeric>
#include <sstream>
#include <functional>

namespace chunking {

//===========================================================================
// ENCODING
//===========================================================================

std::vector<std::string> Chunker::encode(const std::vector<std::string>& tokens) {
    if (tokens.empty()) {
        return tokens;
    }
    
    std::vector<std::string> compressed;
    size_t i = 0;
    
    // Scan through tokens looking for chunkable patterns
    while (i < tokens.size()) {
        // Try to find longest matching chunk starting at position i
        bool found_chunk = false;
        
        // Search for patterns of length 2-4 tokens
        for (size_t pattern_len = std::min(size_t(4), tokens.size() - i); 
             pattern_len >= 2 && !found_chunk; 
             --pattern_len) {
            
            if (i + pattern_len > tokens.size()) {
                continue;
            }
            
            // Extract subsequence
            std::vector<std::string> subseq(
                tokens.begin() + i, 
                tokens.begin() + i + pattern_len
            );
            
            // Compute embedding
            std::vector<double> embedding = compute_embedding(subseq);
            
            // Check if it matches existing chunk
            int chunk_id = find_matching_chunk(embedding);
            
            if (chunk_id >= 0) {
                // Found match - replace with chunk ID
                std::ostringstream oss;
                oss << "CHUNK_" << chunk_id;
                compressed.push_back(oss.str());
                
                // Update chunk frequency
                if (chunk_library_.find(chunk_id) != chunk_library_.end()) {
                    chunk_library_[chunk_id].frequency++;
                }
                
                i += pattern_len;
                found_chunk = true;
            }
        }
        
        // No chunk found - copy token as-is
        if (!found_chunk) {
            compressed.push_back(tokens[i]);
            i++;
        }
    }
    
    // Update statistics
    update_stats(tokens.size(), compressed.size());
    
    // Prune library if it gets too large
    prune_library_if_needed();
    
    return compressed;
}

//===========================================================================
// DECODING
//===========================================================================

std::vector<std::string> Chunker::decode(const std::vector<std::string>& codes) const {
    std::vector<std::string> decoded;
    
    for (const std::string& code : codes) {
        // Check if this is a chunk ID
        if (code.substr(0, 6) == "CHUNK_") {
            try {
                int chunk_id = std::stoi(code.substr(6));
                
                // Look up chunk in library
                auto it = chunk_library_.find(chunk_id);
                if (it != chunk_library_.end()) {
                    // Expand chunk to original tokens
                    const std::vector<std::string>& tokens = it->second.tokens;
                    decoded.insert(decoded.end(), tokens.begin(), tokens.end());
                } else {
                    // Chunk not found - keep code as-is
                    decoded.push_back(code);
                }
            } catch (...) {
                // Invalid chunk ID format
                decoded.push_back(code);
            }
        } else {
            // Regular token
            decoded.push_back(code);
        }
    }
    
    return decoded;
}

//===========================================================================
// CHUNK LEARNING
//===========================================================================

void Chunker::learn_chunk(const std::vector<double>& pattern_embedding,
                          const std::vector<std::string>& tokens) {
    // Check if similar chunk already exists
    int existing_id = find_matching_chunk(pattern_embedding);
    
    if (existing_id >= 0) {
        // Update existing chunk frequency
        if (chunk_library_.find(existing_id) != chunk_library_.end()) {
            chunk_library_[existing_id].frequency++;
        }
        return;
    }
    
    // Create new chunk
    Chunk new_chunk(pattern_embedding, tokens);
    new_chunk.salience = 1.0;  // Initial salience
    
    int chunk_id = next_chunk_id_++;
    chunk_library_[chunk_id] = new_chunk;
    
    stats_.total_chunks = chunk_library_.size();
    
    // Update capacity multiplier
    double boost = 1.0 + fdqc_params::CHUNK_GAMMA * 
                   chunk_library_.size() / 100.0;
    stats_.capacity_multiplier = std::min(boost, fdqc_params::CHUNK_BOOST_MAX);
}

//===========================================================================
// PATTERN MATCHING
//===========================================================================

int Chunker::find_matching_chunk(const std::vector<double>& pattern_embedding) const {
    double best_similarity = -1.0;
    int best_chunk_id = -1;
    
    for (const auto& pair : chunk_library_) {
        int chunk_id = pair.first;
        const Chunk& chunk = pair.second;
        
        double similarity = cosine_similarity(pattern_embedding, chunk.pattern);
        
        if (similarity > best_similarity && 
            similarity >= fdqc_params::CHUNK_SIMILARITY_THRESHOLD) {
            best_similarity = similarity;
            best_chunk_id = chunk_id;
        }
    }
    
    return best_chunk_id;
}

//===========================================================================
// EMBEDDING COMPUTATION (SIMPLIFIED)
//===========================================================================

std::vector<double> Chunker::compute_embedding(
    const std::vector<std::string>& tokens) const {
    
    // Simplified embedding: hash-based feature vector
    // In a real system, this would use learned embeddings (e.g., Word2Vec)
    
    constexpr size_t EMBEDDING_DIM = 64;
    std::vector<double> embedding(EMBEDDING_DIM, 0.0);
    
    // Create deterministic features from tokens
    std::hash<std::string> hasher;
    
    for (size_t i = 0; i < tokens.size(); ++i) {
        size_t hash = hasher(tokens[i]);
        
        // Spread hash across embedding dimensions
        for (size_t d = 0; d < EMBEDDING_DIM; ++d) {
            double value = std::sin((hash + i * 1000 + d) * 0.01);
            embedding[d] += value;
        }
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

void Chunker::update_stats(size_t original_length, size_t compressed_length) {
    stats_.sequences_processed++;
    
    // Update average compression ratio
    double compression = (original_length > 0) 
        ? static_cast<double>(compressed_length) / original_length
        : 1.0;
    
    double alpha = 0.9;  // Exponential moving average
    stats_.average_compression = alpha * stats_.average_compression + 
                                (1.0 - alpha) * compression;
}

//===========================================================================
// LIBRARY PRUNING
//===========================================================================

void Chunker::prune_library_if_needed() {
    if (chunk_library_.size() <= fdqc_params::MAX_CHUNK_LIBRARY_SIZE) {
        return;
    }
    
    // Find least frequently used chunks
    std::vector<std::pair<int, size_t>> chunk_frequencies;
    for (const auto& pair : chunk_library_) {
        chunk_frequencies.push_back({pair.first, pair.second.frequency});
    }
    
    // Sort by frequency (ascending)
    std::sort(chunk_frequencies.begin(), chunk_frequencies.end(),
              [](const auto& a, const auto& b) { return a.second < b.second; });
    
    // Remove bottom 10%
    size_t num_to_remove = chunk_library_.size() / 10;
    for (size_t i = 0; i < num_to_remove && i < chunk_frequencies.size(); ++i) {
        chunk_library_.erase(chunk_frequencies[i].first);
    }
    
    stats_.total_chunks = chunk_library_.size();
    
    // Recalculate capacity multiplier
    double boost = 1.0 + fdqc_params::CHUNK_GAMMA * 
                   chunk_library_.size() / 100.0;
    stats_.capacity_multiplier = std::min(boost, fdqc_params::CHUNK_BOOST_MAX);
}

} // namespace chunking
