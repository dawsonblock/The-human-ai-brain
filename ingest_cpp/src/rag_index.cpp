#include "brain/config.hpp"
#include <nlohmann/json.hpp>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <curl/curl.h>
#include <random>

using json = nlohmann::json;
namespace fs = std::filesystem;

static Config load_config(const std::string& path) {
  auto j = json::parse(std::ifstream(path));
  Config c;
  c.qdrant_url = j.value("qdrant_url","http://localhost:6333");
  c.qdrant_collection = j.value("qdrant_collection","docs");
  c.embed_url = j.value("embed_url","http://localhost:8081/embed");
  return c;
}

static size_t write_cb(void* contents, size_t sz, size_t nm, void* userp){
  ((std::string*)userp)->append((char*)contents, sz*nm);
  return sz*nm;
}

static json http_post_json(const std::string& url, const json& payload){
  CURL* curl = curl_easy_init();
  if(!curl) throw std::runtime_error("curl init failed");
  std::string resp; struct curl_slist* hdr=nullptr;
  hdr = curl_slist_append(hdr, "Content-Type: application/json");
  std::string body = payload.dump();
  curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, hdr);
  curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body.c_str());
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_cb);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &resp);
  auto rc = curl_easy_perform(curl);
  curl_slist_free_all(hdr); curl_easy_cleanup(curl);
  if(rc!=CURLE_OK) throw std::runtime_error("curl failed");
  return json::parse(resp);
}

int main(int argc, char** argv){
  if(argc<3){ std::cerr<<"usage: rag_index <config.json> <jsonl>\n"; return 1; }
  Config cfg = load_config(argv[1]);
  fs::path jl_path = argv[2];

  std::cout << "Creating Qdrant collection: " << cfg.qdrant_collection << std::endl;
  
  // create collection
  {
    json recreate = {
      {"vectors", {{"size", 1024}, {"distance","Cosine"}}}
    };
    try {
      http_post_json(cfg.qdrant_url + "/collections/" + cfg.qdrant_collection, recreate);
    } catch (const std::exception& e) {
      std::cerr << "Warning: Collection creation failed (may already exist): " << e.what() << std::endl;
    }
  }

  std::ifstream jl(jl_path);
  std::string line;
  std::vector<json> points; points.reserve(1000);
  size_t total_indexed = 0;

  while(std::getline(jl, line)){
    if(line.empty()) continue;
    auto rec = json::parse(line);
    
    // embed
    json emb_req = {{"text", rec["input"]}};
    auto emb = http_post_json(cfg.embed_url, emb_req)["embedding"];
    
    // random-ish id
    static std::mt19937_64 rng(123);
    uint64_t id = rng();

    points.push_back({
      {"id", id},
      {"vector", emb},
      {"payload", rec}
    });

    if(points.size() >= 256){
      http_post_json(cfg.qdrant_url + "/collections/" + cfg.qdrant_collection + "/points?wait=true",
                     {{"points", points}});
      total_indexed += points.size();
      std::cout << "Indexed " << total_indexed << " points..." << std::endl;
      points.clear();
    }
  }
  
  if(!points.empty()){
    http_post_json(cfg.qdrant_url + "/collections/" + cfg.qdrant_collection + "/points?wait=true",
                   {{"points", points}});
    total_indexed += points.size();
  }
  
  std::cout << "Indexing complete! Total points: " << total_indexed << std::endl;
  return 0;
}
