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
    std::cerr << "\nExample:\n";
    std::cerr << "  ./build_jsonl config.json /path/to/pdfs ./output\n";
    return 1;
  }
  
  // Validate config file
  if(!fs::exists(argv[1])) {
    std::cerr << "Error: Config file not found: " << argv[1] << std::endl;
    return 1;
  }
  
  Config cfg;
  try {
    cfg = load_config(argv[1]);
  } catch(const std::exception& e) {
    std::cerr << "Error loading config: " << e.what() << std::endl;
    return 1;
  }
  
  fs::path pdf_dir = argv[2];
  fs::path out_dir = argv[3];
  
  // Validate PDF directory exists
  if(!fs::exists(pdf_dir)) {
    std::cerr << "Error: PDF directory not found: " << pdf_dir << std::endl;
    return 1;
  }
  
  if(!fs::is_directory(pdf_dir)) {
    std::cerr << "Error: Path is not a directory: " << pdf_dir << std::endl;
    return 1;
  }
  // Create output directories
  try {
    fs::create_directories(out_dir / "ocr_md");
    fs::create_directories(out_dir / "train");
    fs::create_directories(out_dir / "tmp");
  } catch(const std::exception& e) {
    std::cerr << "Error creating output directories: " << e.what() << std::endl;
    return 1;
  }

  std::ofstream jl(out_dir / "train" / "ocr_sft.jsonl", std::ios::binary);
  if(!jl) {
    std::cerr << "Error: Failed to open output file: " << (out_dir / "train" / "ocr_sft.jsonl") << std::endl;
    return 1;
  }
  
  size_t total_pdfs = 0;
  size_t total_chunks = 0;
  size_t failed_pdfs = 0;

  for (auto& p : fs::recursive_directory_iterator(pdf_dir)) {
    if (!p.is_regular_file() || p.path().extension() != ".pdf") continue;
    
    total_pdfs++;
    std::cout << "\n[" << total_pdfs << "] Processing: " << p.path().filename() << std::endl;
    
    std::vector<std::string> pngs;
    try {
      pngs = pdf_to_pngs(p.path().string(), cfg.dpi, (out_dir/"tmp").string());
      std::cout << "  Rendered " << pngs.size() << " pages" << std::endl;
    } catch(const std::exception& e) {
      std::cerr << "  Error rendering PDF: " << e.what() << std::endl;
      failed_pdfs++;
      continue;
    }

    std::string md_join;
    for (size_t i = 0; i < pngs.size(); ++i) {
      try {
        std::string md = ocr_page_markdown(cfg.ocr_url, cfg.ocr_model, pngs[i], cfg.prompt);
        md_join += "\n\n" + md;
        std::cout << "  Page " << (i+1) << "/" << pngs.size() << " OCR complete" << std::endl;
      } catch(const std::exception& e) {
        std::cerr << "  Warning: OCR failed for page " << (i+1) << ": " << e.what() << std::endl;
      }
    }

    // write page-joined md
    auto md_path = out_dir/"ocr_md"/(p.path().stem().string() + ".md");
    std::ofstream md_f(md_path);
    md_f << md_join;
    md_f.close();

    // chunk -> jsonl (instruction, input, output)
    auto chunks = chunk_text(md_join, cfg.chunk_size, cfg.chunk_overlap);
    total_chunks += chunks.size();
    
    for (auto& c : chunks) {
      json rec{
        {"instruction","Read and summarize the content in 5 bullets."},
        {"input", c},
        {"output",""}
      };
      jl << rec.dump() << "\n";
    }
    
    std::cout << "  ✓ Generated " << chunks.size() << " chunks" << std::endl;
  }
  
  jl.close();
  
  std::cout << "\n" << std::string(60, '=') << std::endl;
  std::cout << "Processing Complete!" << std::endl;
  std::cout << std::string(60, '=') << std::endl;
  std::cout << "  Total PDFs processed: " << total_pdfs << std::endl;
  std::cout << "  Failed PDFs: " << failed_pdfs << std::endl;
  std::cout << "  Total chunks generated: " << total_chunks << std::endl;
  std::cout << "  Output directory: " << out_dir.string() << std::endl;
  std::cout << "  Training data: " << (out_dir / "train" / "ocr_sft.jsonl").string() << std::endl;
  std::cout << std::string(60, '=') << std::endl;
  
  return failed_pdfs > 0 ? 2 : 0;
}
