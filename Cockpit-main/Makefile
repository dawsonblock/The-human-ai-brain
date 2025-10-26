#===========================================================================
# Cockpit Self-Write System - Production Makefile
#===========================================================================

CXX = g++
CXXFLAGS = -std=c++17 -O2 -Wall -Wextra -Iinclude -pthread
LDFLAGS = -pthread -lssl -lcrypto -lsqlite3

# Directories
SRC_DIR = src
BUILD_DIR = build
BIN_DIR = bin

# Source files (exclude tools subdirectory and api_change.cpp)
MAIN_SOURCES = $(filter-out $(SRC_DIR)/api_change.cpp, $(wildcard $(SRC_DIR)/*.cpp))
MAIN_OBJECTS = $(MAIN_SOURCES:$(SRC_DIR)/%.cpp=$(BUILD_DIR)/%.o)

# Main executable
MAIN_BIN = $(BIN_DIR)/cockpit

# Test binaries
TEST_BINS = $(BIN_DIR)/test_energy $(BIN_DIR)/test_collapse_correct \
            $(BIN_DIR)/test_sally_anne $(BIN_DIR)/test_affective \
            $(BIN_DIR)/test_memory $(BIN_DIR)/test_integration

#===========================================================================
# Default target
#===========================================================================

.PHONY: all
all: dirs $(MAIN_BIN) tests

#===========================================================================
# Create directories
#===========================================================================

.PHONY: dirs
dirs:
	@mkdir -p $(BUILD_DIR) $(BIN_DIR) config audit logs

#===========================================================================
# Main executable
#===========================================================================

$(MAIN_BIN): $(MAIN_OBJECTS)
	@echo "Linking main executable..."
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)
	@echo "✓ Built: $@"

#===========================================================================
# Object files
#===========================================================================

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	@echo "Compiling $<..."
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

#===========================================================================
# Tests
#===========================================================================

.PHONY: tests
tests: $(TEST_BINS)

$(BIN_DIR)/test_energy: tests/test_energy.cpp $(BUILD_DIR)/vcca_controller.o
	@echo "Building test_energy..."
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

$(BIN_DIR)/test_collapse_correct: tests/test_collapse_correct.cpp $(BUILD_DIR)/collapse.o
	@echo "Building test_collapse_correct..."
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

$(BIN_DIR)/test_sally_anne: tests/test_sally_anne.cpp $(BUILD_DIR)/theory_of_mind.o
	@echo "Building test_sally_anne..."
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

$(BIN_DIR)/test_affective: tests/test_affective.cpp $(BUILD_DIR)/affective_core.o
	@echo "Building test_affective..."
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

$(BIN_DIR)/test_memory: tests/test_memory.cpp $(BUILD_DIR)/preconscious_buffer.o $(BUILD_DIR)/episodic_memory.o
	@echo "Building test_memory..."
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

$(BIN_DIR)/test_integration: tests/test_integration.cpp $(filter-out $(BUILD_DIR)/main.o, $(MAIN_OBJECTS))
	@echo "Building test_integration..."
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

#===========================================================================
# Run tests
#===========================================================================

.PHONY: test
test: tests
	@echo "\n=== Running All Tests ==="
	@$(BIN_DIR)/test_energy && echo "✓ test_energy passed"
	@$(BIN_DIR)/test_collapse_correct && echo "✓ test_collapse_correct passed"
	@$(BIN_DIR)/test_sally_anne && echo "✓ test_sally_anne passed"
	@$(BIN_DIR)/test_affective && echo "✓ test_affective passed"
	@$(BIN_DIR)/test_memory && echo "✓ test_memory passed"
	@if [ -f $(BIN_DIR)/test_integration ]; then \
		$(BIN_DIR)/test_integration && echo "✓ test_integration passed"; \
	fi
	@echo "\n✓ All tests passed!"

#===========================================================================
# Clean
#===========================================================================

.PHONY: clean
clean:
	rm -rf $(BUILD_DIR) $(BIN_DIR)
	@echo "✓ Cleaned build artifacts"

.PHONY: distclean
distclean: clean
	rm -rf logs/*.log audit/*.json
	@echo "✓ Cleaned logs and audit files"

#===========================================================================
# Run
#===========================================================================

.PHONY: run
run: $(MAIN_BIN)
	@echo "Starting Cockpit server..."
	$(MAIN_BIN)

#===========================================================================
# Help
#===========================================================================

.PHONY: help
help:
	@echo "Cockpit Build Targets:"
	@echo "  make all        - Build everything (default)"
	@echo "  make tests      - Build all tests"
	@echo "  make test       - Build and run all tests"
	@echo "  make clean      - Remove build artifacts"
	@echo "  make run        - Build and run server"
	@echo "  make help       - Show this help"
