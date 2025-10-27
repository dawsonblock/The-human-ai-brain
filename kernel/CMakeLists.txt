# Core library source files
file(GLOB CORE_SRCS src/*.cpp)
# Exclude brain_service.cpp, metrics.cpp, and trainer.cpp (has compilation errors)
list(FILTER CORE_SRCS EXCLUDE REGEX ".*brain_service\\.cpp$")
list(FILTER CORE_SRCS EXCLUDE REGEX ".*metrics\\.cpp$")
list(FILTER CORE_SRCS EXCLUDE REGEX ".*trainer\\.cpp$")

# Ensure config_validate.cpp is included (it should be in brain_core)
if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/config_validate.cpp")
    message(WARNING "config_validate.cpp not found - configuration validation will not be available")
endif()

# Brain core library
add_library(brain_core STATIC ${CORE_SRCS})

# Metrics library (for observability)
add_library(brain_metrics STATIC src/metrics.cpp)
target_include_directories(brain_metrics PUBLIC include)
target_link_libraries(brain_metrics PUBLIC brain_core)

target_include_directories(brain_core
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
        $<INSTALL_INTERFACE:include>
)

target_compile_options(brain_core PRIVATE
    -Wall -Wextra -Wpedantic
    $<$<CONFIG:Release>:-O3>
    $<$<CONFIG:Debug>:-g -O0>
)

target_link_libraries(brain_core
    PUBLIC
        Eigen3::Eigen
        OpenSSL::Crypto
)

if(ENABLE_OPENMP AND OpenMP_CXX_FOUND)
    target_link_libraries(brain_core PUBLIC OpenMP::OpenMP_CXX)
endif()

# Installation
install(TARGETS brain_core
    EXPORT brain-targets
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
)

install(DIRECTORY include/ DESTINATION include)

# gRPC Server
if(BUILD_GRPC_SERVER)
    # Generate protobuf and gRPC code
    set(PROTO_FILES "${CMAKE_CURRENT_SOURCE_DIR}/proto/brain.proto")
    set(PROTO_SRC_DIR "${CMAKE_CURRENT_BINARY_DIR}/proto")
    file(MAKE_DIRECTORY ${PROTO_SRC_DIR})
    
    set(PROTO_SRCS "${PROTO_SRC_DIR}/brain.pb.cc")
    set(PROTO_HDRS "${PROTO_SRC_DIR}/brain.pb.h")
    set(GRPC_SRCS "${PROTO_SRC_DIR}/brain.grpc.pb.cc")
    set(GRPC_HDRS "${PROTO_SRC_DIR}/brain.grpc.pb.h")
    
    # Find protoc and grpc_cpp_plugin
    find_program(PROTOC protoc REQUIRED)
    find_program(GRPC_CPP_PLUGIN grpc_cpp_plugin REQUIRED)
    
    # Custom command to generate protobuf and gRPC sources
    add_custom_command(
        OUTPUT ${PROTO_SRCS} ${PROTO_HDRS} ${GRPC_SRCS} ${GRPC_HDRS}
        COMMAND ${PROTOC}
        ARGS --cpp_out=${PROTO_SRC_DIR}
             --grpc_out=${PROTO_SRC_DIR}
             --plugin=protoc-gen-grpc=${GRPC_CPP_PLUGIN}
             -I${CMAKE_CURRENT_SOURCE_DIR}/proto
             ${PROTO_FILES}
        DEPENDS ${PROTO_FILES}
        COMMENT "Generating protobuf and gRPC C++ sources"
    )
    
    # Proto library
    add_library(brain_proto STATIC ${PROTO_SRCS} ${GRPC_SRCS})
    target_include_directories(brain_proto PUBLIC ${PROTO_SRC_DIR} ${Protobuf_INCLUDE_DIRS})
    target_link_libraries(brain_proto PUBLIC ${Protobuf_LIBRARIES})
    
    # Link gRPC libraries
    if(gRPC_FOUND)
        target_link_libraries(brain_proto PUBLIC gRPC::grpc++ gRPC::grpc++_reflection)
    else()
        target_include_directories(brain_proto PUBLIC ${GRPC_INCLUDE_DIRS} ${GRPCPP_INCLUDE_DIRS})
        target_link_libraries(brain_proto PUBLIC ${GRPC_LIBRARIES} ${GRPCPP_LIBRARIES})
    endif()
    
    # Brain gRPC server executable
    add_executable(brain_server 
        server/main.cpp
        src/brain_service.cpp
    )
    
    target_include_directories(brain_server PRIVATE ${PROTO_SRC_DIR})
    
    target_link_libraries(brain_server PRIVATE
        brain_core
        brain_proto
        brain_metrics
    )
    
    install(TARGETS brain_server DESTINATION bin)
endif()

# Interactive Demo
add_executable(interactive_demo demos/interactive_demo.cpp)
target_link_libraries(interactive_demo PRIVATE brain_core)
install(TARGETS interactive_demo DESTINATION bin)

# Testing
if(BUILD_TESTS)
    enable_testing()
    include(GoogleTest)
    
    add_executable(minimal_test tests/minimal_test.cpp)
    target_link_libraries(minimal_test PRIVATE brain_core GTest::gtest GTest::gtest_main)
    add_test(NAME minimal_tests COMMAND minimal_test)
    gtest_discover_tests(minimal_test)
    
    add_executable(qw_tests tests/qw_tests.cpp)
    target_link_libraries(qw_tests PRIVATE brain_core GTest::gtest GTest::gtest_main)
    add_test(NAME qw_physics_tests COMMAND qw_tests)
    gtest_discover_tests(qw_tests)
    
    add_executable(cognitive_tests tests/cognitive_tests.cpp)
    target_link_libraries(cognitive_tests PRIVATE brain_core GTest::gtest GTest::gtest_main)
    add_test(NAME cognitive_modules_tests COMMAND cognitive_tests)
    gtest_discover_tests(cognitive_tests)
    
    add_executable(brain_system_tests tests/brain_system_tests.cpp)
    target_link_libraries(brain_system_tests PRIVATE brain_core GTest::gtest GTest::gtest_main)
    add_test(NAME unified_brain_system_tests COMMAND brain_system_tests)
    gtest_discover_tests(brain_system_tests)
    
    add_executable(tiered_memory_tests tests/tiered_memory_tests.cpp)
    target_link_libraries(tiered_memory_tests PRIVATE brain_core GTest::gtest GTest::gtest_main)
    add_test(NAME tiered_memory_tests COMMAND tiered_memory_tests)
    gtest_discover_tests(tiered_memory_tests)
endif()
