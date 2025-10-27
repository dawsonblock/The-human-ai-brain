#pragma once
#include <string>

struct Config {
  std::string ocr_url;          // e.g. http://localhost:8000/v1/chat/completions
  std::string ocr_model;        // e.g. deepseek-ai/DeepSeek-OCR
  std::string prompt;           // e.g. "Return clean Markdown for this page."
  std::string qdrant_url;       // e.g. http://localhost:6333
  std::string qdrant_api_key;   // Qdrant Cloud API key
  std::string qdrant_collection;// e.g. docs
  std::string embed_url;        // embedding HTTP endpoint (returns JSON {embedding:[...]})
  int dpi = 300;
  size_t chunk_size = 2000;
  size_t chunk_overlap = 200;
};
