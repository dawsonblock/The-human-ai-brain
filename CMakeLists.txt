cmake_minimum_required(VERSION 3.20)
project(human-ai-brain VERSION 3.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Export compile commands for IDE support
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

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

# Installation
install(DIRECTORY configs/ DESTINATION etc/brain)
install(FILES README.md DESTINATION share/doc/brain)

# CPack configuration for packaging
set(CPACK_PACKAGE_NAME "human-ai-brain")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Human-AI Brain FDQC v3 Architecture")
include(CPack)
