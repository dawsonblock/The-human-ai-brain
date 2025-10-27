cmake_minimum_required(VERSION 3.20)
project(human_ai_brain_ingest CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# Don't override CMAKE_BUILD_TYPE if it's already set
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Find dependencies
find_package(PkgConfig REQUIRED)
pkg_check_modules(POPPLER REQUIRED poppler-cpp)
find_package(OpenSSL REQUIRED)
find_package(CURL REQUIRED)

# nlohmann-json header-only interface library
# Check if target already exists (might be defined by Cockpit-main)
if(NOT TARGET nlohmann_json::nlohmann_json AND NOT TARGET nlohmann_json)
    find_package(nlohmann_json 3.2.0 QUIET)
    if(NOT nlohmann_json_FOUND)
        if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/third_party/nlohmann/json.hpp")
            add_library(nlohmann_json INTERFACE)
            target_include_directories(nlohmann_json INTERFACE third_party)
        else()
            include(FetchContent)
            FetchContent_Declare(json
                URL https://github.com/nlohmann/json/releases/download/v3.11.2/json.tar.xz
            )
            FetchContent_MakeAvailable(json)
        endif()
    endif()
endif()

# brain_ingest library
add_library(brain_ingest
  src/pdf_render.cpp
  src/ocr_client.cpp
  src/base64.cpp
  src/chunker.cpp
)

target_include_directories(brain_ingest 
  PUBLIC 
    include 
    ${POPPLER_INCLUDE_DIRS}
)

target_link_libraries(brain_ingest 
  PUBLIC 
    ${POPPLER_LIBRARIES} 
    OpenSSL::SSL 
    OpenSSL::Crypto 
    CURL::libcurl
    nlohmann_json
)

target_compile_options(brain_ingest PRIVATE -Wall -Wextra)

# build_jsonl executable
add_executable(build_jsonl src/build_jsonl.cpp)
target_link_libraries(build_jsonl PRIVATE brain_ingest)

# rag_index executable
add_executable(rag_index src/rag_index.cpp)
target_link_libraries(rag_index PRIVATE brain_ingest)

# Installation
install(TARGETS build_jsonl rag_index DESTINATION bin)
install(TARGETS brain_ingest DESTINATION lib)
install(DIRECTORY include/brain DESTINATION include)

# Print configuration summary
message(STATUS "================================")
message(STATUS "Human-AI Brain Ingestion Pipeline")
message(STATUS "================================")
message(STATUS "C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Poppler: ${POPPLER_VERSION}")
message(STATUS "OpenSSL: ${OPENSSL_VERSION}")
message(STATUS "CURL: ${CURL_VERSION_STRING}")
message(STATUS "================================")
