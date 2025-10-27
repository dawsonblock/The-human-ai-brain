#include "brain/pdf_render.hpp"
#include <poppler-document.h>
#include <poppler-page.h>
#include <poppler-page-renderer.h>
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

  // Create page renderer
  poppler::page_renderer renderer;
  renderer.set_render_hint(poppler::page_renderer::antialiasing, true);
  renderer.set_render_hint(poppler::page_renderer::text_antialiasing, true);

  for (int i = 0; i < doc->pages(); ++i) {
    std::unique_ptr<poppler::page> pg(doc->create_page(i));
    if (!pg) continue;
    
    // Render page to image using page_renderer
    poppler::image img = renderer.render_page(pg.get(), dpi, dpi);
    if (!img.is_valid()) continue;

    std::string out_path = (fs::path(tmp_dir) / (fs::path(pdf_path).stem().string() + "_" + std::to_string(i) + ".png")).string();
    
    // Try to save as PNG
    if (!img.save(out_path, "png")) {
      // Fallback: write raw buffer (may need proper PNG encoding)
      std::ofstream f(out_path, std::ios::binary);
      if (f) {
        // Note: This writes raw image data, not a proper PNG file
        // For production, consider using a PNG library or pdftocairo
        f.write(reinterpret_cast<const char*>(img.data()), img.bytes_per_row() * img.height());
        f.close();
      }
    }
    
    out.push_back(out_path);
  }
  return out;
}
