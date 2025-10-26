# Core library source files
file(GLOB CORE_SRCS src/*.cpp)

# Brain core library
add_library(brain_core STATIC ${CORE_SRCS})

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

# Testing
if(BUILD_TESTS)
    enable_testing()
    
    add_executable(minimal_test tests/minimal_test.cpp)
    target_link_libraries(minimal_test PRIVATE brain_core GTest::gtest)
    add_test(NAME minimal_tests COMMAND minimal_test)
    
    add_executable(qw_tests tests/qw_tests.cpp)
    target_link_libraries(qw_tests PRIVATE brain_core GTest::gtest GTest::gtest_main)
    add_test(NAME qw_physics_tests COMMAND qw_tests)
    
    gtest_discover_tests(minimal_test)
    gtest_discover_tests(qw_tests)
endif()
