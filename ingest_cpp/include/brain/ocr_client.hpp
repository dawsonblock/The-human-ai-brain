#pragma once
#include <string>
#include <vector>

std::string ocr_page_markdown(
  const std::string& ocr_url,
  const std::string& model,
  const std::string& image_png_path,
  const std::string& page_prompt);

std::string read_file(const std::string& path); // small util
