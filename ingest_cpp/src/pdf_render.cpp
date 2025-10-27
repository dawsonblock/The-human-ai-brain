#include "brain/pdf_render.hpp"
#include <poppler-document.h>
#include <poppler-page.h>
#include <poppler-image.h>
#include <filesystem>
#include <fstream>
#include <stdexcept>

namespace fs = std::filesystem;

std::vector<std::string> pdf_to_pngs(const std::string& pdf_path, int dpi, const std::string& tmp_dir) {
  std::vector<std::string> out;
  auto doc = std::unique_ptr<poppler::document>(poppler::document::load_from_file(pdf_path));
  if (!doc) throw std::runtime_error("Failed to open PDF: " + pdf_path);
  fs::create_directories(tmp_dir);

  for (int i = 0; i < doc->pages(); ++i) {
    std::unique_ptr<poppler::page> pg(doc->create_page(i));
    if (!pg) continue;
    poppler::image img = pg->render_to_image(dpi, dpi);
    if (!img.is_valid()) continue;

    std::string out_path = (fs::path(tmp_dir) / (fs::path(pdf_path).stem().string() + "_" + std::to_string(i) + ".png")).string();
    
    // Prefer save() if available in your poppler build
    if (!img.save(out_path, "png")) {
      // Fallback: write raw buffer (may need Cairo conversion for proper PNG)
      std::ofstream f(out_path, std::ios::binary);
      f.write(reinterpret_cast<const char*>(img.data()), img.bytes_per_row() * img.height());
      f.close();
    }
    
    out.push_back(out_path);
  }
  return out;
}
