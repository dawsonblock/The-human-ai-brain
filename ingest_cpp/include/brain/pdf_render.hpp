#pragma once
#include <string>
#include <vector>

std::vector<std::string> pdf_to_pngs(const std::string& pdf_path, int dpi, const std::string& tmp_dir);
