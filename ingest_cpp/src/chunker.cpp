#include "brain/chunker.hpp"
#include <regex>
#include <algorithm>

std::string normalize_md(const std::string& s) {
  std::string t = s;
  t = std::regex_replace(t, std::regex("\r"), "");
  t = std::regex_replace(t, std::regex("\n{3,}"), "\n\n");
  return t;
}

std::vector<std::string> chunk_text(const std::string& s, size_t max_chars, size_t overlap) {
  std::string t = normalize_md(s);
  std::vector<std::string> parts;
  if (t.empty()) return parts;

  size_t i = 0;
  while (i < t.size()) {
    size_t j = std::min(t.size(), i + max_chars);
    parts.emplace_back(t.substr(i, j - i));
    if (j == t.size()) break;
    i = j - std::min(overlap, j);
  }
  return parts;
}
