#include "brain/ocr_client.hpp"
#include "brain/base64.hpp"
#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <fstream>
#include <stdexcept>
#include <vector>

using json = nlohmann::json;

std::string read_file(const std::string& path) {
  std::ifstream f(path, std::ios::binary);
  if (!f) throw std::runtime_error("Cannot open file: " + path);
  std::string s((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
  return s;
}

static size_t write_cb(void* contents, size_t sz, size_t nm, void* userp) {
  ((std::string*)userp)->append((char*)contents, sz*nm);
  return sz*nm;
}

// vLLM OpenAI-style /v1/chat/completions with image
std::string ocr_page_markdown(
  const std::string& ocr_url,
  const std::string& model,
  const std::string& image_png_path,
  const std::string& page_prompt)
{
  std::string img = read_file(image_png_path);
  std::string b64 = base64_encode(reinterpret_cast<const unsigned char*>(img.data()), img.size());

  json req;
  req["model"] = model;
  req["messages"] = json::array({
    {
      {"role","user"},
      {"content", json::array({
          {{"type","text"},{"text", page_prompt}},
          {{"type","image_url"},{"image_url", {{"url", "data:image/png;base64," + b64}}}}
      })}
    }
  });
  req["temperature"] = 0.0;
  req["max_tokens"] = 4096;

  std::string resp;
  CURL* curl = curl_easy_init();
  if (!curl) throw std::runtime_error("curl init failed");
  struct curl_slist* hdr = nullptr;
  hdr = curl_slist_append(hdr, "Content-Type: application/json");

  std::string body = req.dump();
  curl_easy_setopt(curl, CURLOPT_URL, ocr_url.c_str());
  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, hdr);
  curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body.c_str());
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_cb);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &resp);
  auto rc = curl_easy_perform(curl);
  if (rc != CURLE_OK) {
    curl_easy_cleanup(curl);
    curl_slist_free_all(hdr);
    throw std::runtime_error("curl request failed");
  }
  curl_easy_cleanup(curl);
  curl_slist_free_all(hdr);

  auto j = json::parse(resp);
  // OpenAI style: choices[0].message.content
  return j["choices"][0]["message"]["content"].get<std::string>();
}
