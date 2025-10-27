cmake_minimum_required(VERSION 3.20)
project(human-ai-brain VERSION 3.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Export compile commands for IDE support
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Compiler warnings
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall -Wextra -Wpedantic)
    if(CMAKE_BUILD_TYPE MATCHES "Release")
        add_compile_options(-O3 -march=native -flto)
        set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)
    endif()
elseif(MSVC)
    add_compile_options(/W4 /WX-)
    if(CMAKE_BUILD_TYPE MATCHES "Release")
        add_compile_options(/O2 /GL)
    endif()
endif()

# Build options
option(BUILD_TESTS "Build unit tests" ON)
option(BUILD_SHARED_LIBS "Build shared libraries" OFF)
option(ENABLE_OPENMP "Enable OpenMP support" ON)

# Find Eigen3 (header-only, fetch if not found)
find_package(Eigen3 3.3 QUIET)
if(NOT Eigen3_FOUND)
    include(FetchContent)
    FetchContent_Declare(
        Eigen3
        GIT_REPOSITORY https://gitlab.com/libeigen/eigen.git
        GIT_TAG 3.4.0
        GIT_SHALLOW TRUE
    )
    set(EIGEN_BUILD_DOC OFF CACHE BOOL "" FORCE)
    set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
    set(EIGEN_BUILD_PKGCONFIG OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(Eigen3)
endif()

# Find OpenSSL for SHA256
find_package(OpenSSL REQUIRED)

# Build options for gRPC server
option(BUILD_GRPC_SERVER "Build gRPC server" ON)

# Find Protobuf and gRPC (optional, only if building gRPC server)
if(BUILD_GRPC_SERVER)
    find_package(Protobuf REQUIRED)
    find_package(gRPC CONFIG)
    if(NOT gRPC_FOUND)
        find_package(PkgConfig REQUIRED)
        pkg_check_modules(GRPC REQUIRED grpc++)
        pkg_check_modules(GRPCPP REQUIRED grpc++_reflection)
    endif()
endif()

if(ENABLE_OPENMP)
    find_package(OpenMP)
endif()

# GTest for testing
if(BUILD_TESTS)
    find_package(GTest QUIET)
    if(NOT GTest_FOUND)
        include(FetchContent)
        FetchContent_Declare(
            googletest
            GIT_REPOSITORY https://github.com/google/googletest.git
            GIT_TAG v1.14.0
            GIT_SHALLOW TRUE
        )
        set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
        FetchContent_MakeAvailable(googletest)
    endif()
endif()

# Add subdirectories
add_subdirectory(kernel)

# Add Cockpit subsystem (if available)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/Cockpit-main/CMakeLists.txt")
    add_subdirectory(Cockpit-main)
endif()

# Add document ingestion pipeline (if available)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/ingest_cpp/CMakeLists.txt")
    add_subdirectory(ingest_cpp)
endif()

# Installation
install(DIRECTORY configs/ DESTINATION etc/brain)
install(FILES README.md DESTINATION share/doc/brain)

# CPack configuration for packaging
set(CPACK_PACKAGE_NAME "human-ai-brain")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Human-AI Brain FDQC v3 Architecture")
set(CPACK_GENERATOR "TGZ;ZIP")
include(CPack)

# Configuration summary
message(STATUS "========================================")
message(STATUS " Human-AI Brain System - Build Configuration")
message(STATUS "========================================")
message(STATUS "Version: ${PROJECT_VERSION}")
message(STATUS "C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Compiler: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "----------------------------------------")
message(STATUS "Options:")
message(STATUS "  BUILD_TESTS: ${BUILD_TESTS}")
message(STATUS "  BUILD_GRPC_SERVER: ${BUILD_GRPC_SERVER}")
message(STATUS "  BUILD_SHARED_LIBS: ${BUILD_SHARED_LIBS}")
message(STATUS "  ENABLE_OPENMP: ${ENABLE_OPENMP}")
message(STATUS "----------------------------------------")
message(STATUS "Dependencies:")
message(STATUS "  Eigen3: ${Eigen3_VERSION}")
message(STATUS "  OpenSSL: ${OPENSSL_VERSION}")
if(BUILD_GRPC_SERVER)
    message(STATUS "  Protobuf: ${Protobuf_VERSION}")
    if(gRPC_FOUND)
        message(STATUS "  gRPC: Found (CONFIG)")
    else()
        message(STATUS "  gRPC: Found (PkgConfig)")
    endif()
endif()
if(BUILD_TESTS)
    if(GTest_FOUND)
        message(STATUS "  GTest: ${GTest_VERSION} (System)")
    else()
        message(STATUS "  GTest: Fetched")
    endif()
endif()
if(ENABLE_OPENMP AND OpenMP_CXX_FOUND)
    message(STATUS "  OpenMP: ${OpenMP_CXX_VERSION}")
endif()
message(STATUS "========================================")
message(STATUS "Install prefix: ${CMAKE_INSTALL_PREFIX}")
message(STATUS "========================================")

