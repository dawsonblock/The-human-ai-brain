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
  c.qdrant_api_key = j.value("qdrant_api_key","");
  c.qdrant_collection = j.value("qdrant_collection","docs");
  c.embed_url = j.value("embed_url","http://localhost:8081/embed");
  return c;
}

static size_t write_cb(void* contents, size_t sz, size_t nm, void* userp){
  ((std::string*)userp)->append((char*)contents, sz*nm);
  return sz*nm;
}

static json http_request_json(const std::string& url, const json& payload, const std::string& api_key = "", const std::string& method = "POST"){
  CURL* curl = curl_easy_init();
  if(!curl) throw std::runtime_error("curl init failed");
  std::string resp; struct curl_slist* hdr=nullptr;
  hdr = curl_slist_append(hdr, "Content-Type: application/json");
  if(!api_key.empty()) {
    std::string auth = "api-key: " + api_key;
    hdr = curl_slist_append(hdr, auth.c_str());
  }
  std::string body = payload.dump();
  curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, hdr);
  if(method == "PUT") {
    curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PUT");
  }
  curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body.c_str());
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_cb);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &resp);
  auto rc = curl_easy_perform(curl);
  long http_code = 0;
  curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
  curl_slist_free_all(hdr); curl_easy_cleanup(curl);
  if(rc!=CURLE_OK) throw std::runtime_error("curl failed: " + std::string(curl_easy_strerror(rc)));
  if(resp.empty()) {
    // Empty response is OK for some operations
    return json::object();
  }
  return json::parse(resp);
}

static json http_post_json(const std::string& url, const json& payload, const std::string& api_key = ""){
  return http_request_json(url, payload, api_key, "POST");
}

int main(int argc, char** argv){
  if(argc<3){ 
    std::cerr<<"usage: rag_index <config.json> <jsonl>\n";
    std::cerr<<"\nExample:\n";
    std::cerr<<"  ./rag_index config.json output/train/ocr_sft.jsonl\n";
    return 1; 
  }
  
  // Validate config file exists
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
  
  fs::path jl_path = argv[2];
  
  // Validate JSONL file exists
  if(!fs::exists(jl_path)) {
    std::cerr << "Error: JSONL file not found: " << jl_path << std::endl;
    return 1;
  }

  std::cout << "Creating Qdrant collection: " << cfg.qdrant_collection << std::endl;
  
  // create collection
  {
    json recreate = {
      {"vectors", {{"size", 1024}, {"distance","Cosine"}}}
    };
    try {
      http_request_json(cfg.qdrant_url + "/collections/" + cfg.qdrant_collection, recreate, cfg.qdrant_api_key, "PUT");
    } catch (const std::exception& e) {
      std::cerr << "Warning: Collection creation failed (may already exist): " << e.what() << std::endl;
    }
  }

  std::ifstream jl(jl_path);
  std::string line;
  
  // Batch data for Qdrant API format
  std::vector<uint64_t> batch_ids;
  std::vector<json> batch_vectors;
  std::vector<json> batch_payloads;
  size_t total_indexed = 0;
  
  std::mt19937_64 rng(123);

  size_t line_num = 0;
  while(std::getline(jl, line)){
    line_num++;
    if(line.empty()) continue;
    
    json rec;
    try {
      rec = json::parse(line);
    } catch(const std::exception& e) {
      std::cerr << "Error parsing JSON at line " << line_num << ": " << e.what() << std::endl;
      continue;
    }
    
    // Validate required fields
    if(!rec.contains("input")) {
      std::cerr << "Warning: Missing 'input' field at line " << line_num << std::endl;
      continue;
    }
    
    // embed with error handling
    json emb_req = {{"text", rec["input"]}};
    json emb;
    try {
      auto emb_response = http_post_json(cfg.embed_url, emb_req);
      if(!emb_response.contains("embedding")) {
        std::cerr << "Error: Embedding response missing 'embedding' field at line " << line_num << std::endl;
        continue;
      }
      emb = emb_response["embedding"];
    } catch(const std::exception& e) {
      std::cerr << "Error getting embedding at line " << line_num << ": " << e.what() << std::endl;
      continue;
    }
    
    // random-ish id
    uint64_t id = rng();

    batch_ids.push_back(id);
    batch_vectors.push_back(emb);
    batch_payloads.push_back(rec);

    if(batch_ids.size() >= 256){
      json points_array = json::array();
      
      for(size_t i = 0; i < batch_ids.size(); ++i) {
        points_array.push_back({
          {"id", batch_ids[i]},
          {"vector", batch_vectors[i]},
          {"payload", batch_payloads[i]}
        });
      }
      
      json upsert_payload = {
        {"points", points_array}
      };
      
      http_request_json(cfg.qdrant_url + "/collections/" + cfg.qdrant_collection + "/points?wait=true",
                       upsert_payload, cfg.qdrant_api_key, "PUT");
      total_indexed += batch_ids.size();
      std::cout << "Indexed " << total_indexed << " points..." << std::endl;
      batch_ids.clear();
      batch_vectors.clear();
      batch_payloads.clear();
    }
  }
  
  // Upload remaining points in upsert format (points array with objects)
  if(!batch_ids.empty()){
    json points_array = json::array();
    
    for(size_t i = 0; i < batch_ids.size(); ++i) {
      points_array.push_back({
        {"id", batch_ids[i]},
        {"vector", batch_vectors[i]},
        {"payload", batch_payloads[i]}
      });
    }
    
    json upsert_payload = {
      {"points", points_array}
    };
    
    std::string url = cfg.qdrant_url + "/collections/" + cfg.qdrant_collection + "/points?wait=true";
    std::cout << "Uploading " << batch_ids.size() << " points to Qdrant..." << std::endl;
    
    try {
      auto resp = http_request_json(url, upsert_payload, cfg.qdrant_api_key, "PUT");
      std::cout << "✅ Response: " << resp.dump() << std::endl;
      
      if(resp.contains("status") && resp["status"] == "ok") {
        total_indexed += batch_ids.size();
        std::cout << "✅ Successfully indexed " << batch_ids.size() << " points" << std::endl;
      } else {
        std::cerr << "⚠️  Unexpected response from Qdrant" << std::endl;
      }
    } catch(const std::exception& e) {
      std::cerr << "❌ Failed to upload points: " << e.what() << std::endl;
    }
  }
  
  std::cout << "Indexing complete! Total points: " << total_indexed << std::endl;
  return 0;
}
