# Changelog

All notable changes to the Human-AI Brain Document Ingestion Pipeline will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/).

## [1.0.0] - 2025-10-27

### Added
- **Complete C++ document ingestion pipeline** with PDF processing, OCR, chunking, and RAG indexing
- **Poppler-cpp 22.12.0 support** using `page_renderer` API for modern Poppler versions
- **Qdrant Cloud integration** with API key authentication
- **Batch upsert** using proper Qdrant REST API format (`PUT` with points array)
- **Mock OCR service** for testing without GPU dependencies (Flask-based)
- **Embedding service stub** for local development and testing
- **Enhanced error handling** with detailed error messages and recovery
- **Progress tracking** with per-page OCR status and processing summaries
- **Comprehensive validation** of input files, configuration, and output format
- **Integration test suite** with automated end-to-end testing
- **Validation script** (`scripts/validate_setup.sh`) for environment checking
- **Full test runner** (`scripts/run_full_test.sh`) for automated testing

### Changed
- **Updated PDF rendering** from deprecated `page->render_to_image()` to `page_renderer.render_page()`
- **Improved logging** with color-coded output and detailed progress information
- **Enhanced build_jsonl** with per-PDF processing statistics and failure tracking
- **Better error messages** with usage examples in executables

### Fixed
- **Poppler API compatibility** for versions 22.12.0 and newer
- **Qdrant batch upload** format (was using POST with separate arrays, now using PUT with points array)
- **API key authentication** header format for Qdrant Cloud (`api-key` header)
- **Empty response handling** in HTTP client for operations that return no body
- **JSON parsing errors** with try-catch blocks and line number reporting
- **Missing field validation** for JSONL records before processing

### Security
- **API key support** for Qdrant Cloud connections
- **Input validation** for all file paths and configuration parameters
- **Error handling** prevents crashes from malformed input

### Documentation
- **README.md** - Comprehensive usage guide with examples
- **QUICKSTART.md** - Quick start guide for new users
- **INTEGRATION.md** - Integration guide for brain system
- **TEST_RESULTS.md** - Test results and performance metrics
- **CHANGELOG.md** - This file

### Testing
- **2 test PDFs** with multi-page content (5 pages total)
- **3 document chunks** successfully indexed in Qdrant Cloud
- **5 semantic search queries** validated with keyword matching
- **Zero failed tests** in comprehensive integration suite
- **All performance targets met** (< 10s per document end-to-end)

## [0.1.0] - 2025-10-20 (Initial Implementation)

### Added
- Basic PDF rendering with Poppler
- OCR client for DeepSeek-OCR
- Text chunking with overlap
- JSONL generation for training
- Local Qdrant indexing support
- Base64 encoding for images
- CMake build system
- Basic documentation

### Known Issues (Resolved in 1.0.0)
- Poppler API compatibility issues with modern versions
- Qdrant batch upload format incorrect
- No API key support for Qdrant Cloud
- Limited error handling and validation
- No comprehensive testing

---

## Versioning

This project follows [Semantic Versioning](https://semver.org/):
- **MAJOR** version for incompatible API changes
- **MINOR** version for new functionality in a backward compatible manner
- **PATCH** version for backward compatible bug fixes

## Release Process

1. Update version in `CMakeLists.txt`
2. Update this CHANGELOG
3. Create git tag: `git tag -a v1.0.0 -m "Release v1.0.0"`
4. Push tag: `git push origin v1.0.0`

