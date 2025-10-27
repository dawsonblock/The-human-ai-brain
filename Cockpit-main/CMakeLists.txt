cmake_minimum_required(VERSION 3.20)
project(cockpit LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find dependencies
find_package(OpenSSL REQUIRED)
find_package(nlohmann_json 3.2.0 QUIET)
if(NOT nlohmann_json_FOUND)
    # nlohmann_json is header-only, can be included from parent or fetched
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/include/nlohmann/json.hpp")
        # Header already present
        add_library(nlohmann_json INTERFACE)
        target_include_directories(nlohmann_json INTERFACE include)
    else()
        # Fetch if needed
        include(FetchContent)
        FetchContent_Declare(json
            URL https://github.com/nlohmann/json/releases/download/v3.11.2/json.tar.xz
        )
        FetchContent_MakeAvailable(json)
    endif()
endif()

# Cockpit safety library
add_library(cockpit STATIC 
    src/kill.cpp 
    src/change_gate.cpp 
    src/moral_core.cpp 
    src/explainer.cpp 
    src/self_writer.cpp
    src/change_audit.cpp
    src/fdqc_system.cpp
    src/affective_core.cpp
    src/epistemic_drive.cpp
    src/meta_monitor.cpp
    src/preconscious_buffer.cpp
    src/theory_of_mind.cpp
    src/imagination_engine.cpp
    src/episodic_memory.cpp
    src/vcca_controller.cpp
    src/chunking.cpp
    src/collapse.cpp
)

target_include_directories(cockpit 
    PUBLIC 
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Link to nlohmann_json - handle both namespaced and non-namespaced targets
if(TARGET nlohmann_json::nlohmann_json)
    set(JSON_TARGET nlohmann_json::nlohmann_json)
else()
    set(JSON_TARGET nlohmann_json)
endif()

target_link_libraries(cockpit 
    PUBLIC 
        OpenSSL::SSL 
        OpenSSL::Crypto
        ${JSON_TARGET}
)

target_compile_options(cockpit PRIVATE
    -Wall -Wextra -Wpedantic
    $<$<CONFIG:Release>:-O3>
    $<$<CONFIG:Debug>:-g -O0>
)

# Cockpit tests
if(BUILD_TESTS AND TARGET GTest::gtest)
    add_executable(cockpit_tests tests/cockpit_tests.cpp)
    target_link_libraries(cockpit_tests 
        PRIVATE 
            cockpit 
            GTest::gtest 
            GTest::gtest_main
    )
    
    include(GoogleTest)
    gtest_discover_tests(cockpit_tests)
    
    add_test(NAME cockpit_safety_tests COMMAND cockpit_tests)
endif()

# Installation
install(TARGETS cockpit 
    EXPORT cockpit-targets
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
)

install(DIRECTORY include/ DESTINATION include)
