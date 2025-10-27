#include "brain/config.hpp"
#include "brain/pdf_render.hpp"
#include "brain/ocr_client.hpp"
#include "brain/chunker.hpp"
#include <nlohmann/json.hpp>
#include <filesystem>
#include <fstream>
#include <iostream>

using json = nlohmann::json;
namespace fs = std::filesystem;

static Config load_config(const std::string& path) {
  auto j = json::parse(std::ifstream(path));
  Config c;
  c.ocr_url  = j.at("ocr_url").get<std::string>();
  c.ocr_model= j.at("ocr_model").get<std::string>();
  c.prompt   = j.value("prompt", "Return clean Markdown for this page.");
  c.qdrant_url = j.value("qdrant_url", "http://localhost:6333");
  c.qdrant_collection = j.value("qdrant_collection", "docs");
  c.embed_url = j.value("embed_url", "http://localhost:8081/embed");
  c.dpi = j.value("dpi", 300);
  c.chunk_size = j.value("chunk_size", 2000);
  c.chunk_overlap = j.value("chunk_overlap", 200);
  return c;
}

int main(int argc, char** argv) {
  if (argc < 4) {
    std::cerr << "usage: build_jsonl <config.json> <pdf_dir> <out_dir>\n";
    return 1;
  }
  Config cfg = load_config(argv[1]);
  fs::path pdf_dir = argv[2];
  fs::path out_dir = argv[3];
  fs::create_directories(out_dir / "ocr_md");
  fs::create_directories(out_dir / "train");

  std::ofstream jl(out_dir / "train" / "ocr_sft.jsonl", std::ios::binary);

  for (auto& p : fs::recursive_directory_iterator(pdf_dir)) {
    if (!p.is_regular_file() || p.path().extension() != ".pdf") continue;
    
    std::cout << "Processing: " << p.path().filename() << std::endl;
    auto pngs = pdf_to_pngs(p.path().string(), cfg.dpi, (out_dir/"tmp").string());

    std::string md_join;
    for (auto& png : pngs) {
      std::string md = ocr_page_markdown(cfg.ocr_url, cfg.ocr_model, png, cfg.prompt);
      md_join += "\n\n" + md;
    }

    // write page-joined md
    auto md_path = out_dir/"ocr_md"/(p.path().stem().string() + ".md");
    std::ofstream md_f(md_path);
    md_f << md_join;
    md_f.close();

    // chunk -> jsonl (instruction, input, output)
    auto chunks = chunk_text(md_join, cfg.chunk_size, cfg.chunk_overlap);
    for (auto& c : chunks) {
      json rec{
        {"instruction","Read and summarize the content in 5 bullets."},
        {"input", c},
        {"output",""}
      };
      jl << rec.dump() << "\n";
    }
    
    std::cout << "  Generated " << chunks.size() << " chunks" << std::endl;
  }
  
  std::cout << "Complete! Output: " << out_dir.string() << std::endl;
  return 0;
}
