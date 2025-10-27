#pragma once
#include <string>
#include <vector>

std::vector<std::string> chunk_text(const std::string& s, size_t max_chars, size_t overlap);
std::string normalize_md(const std::string& s);
