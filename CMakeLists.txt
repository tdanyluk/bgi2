# This is the minimal version for C++17
cmake_minimum_required (VERSION 3.8)
set(CMAKE_CXX_STANDARD 17)

project ("bgi2")
enable_testing()
find_package(SDL2 REQUIRED)
include_directories(${SDL2_INCLUDE_DIRS})

# Download Google Test
include(FetchContent)
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG release-1.12.1
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)
include(GoogleTest)

# Beginners' Graphics Interface 2
add_library(bgi2 include/bgi2.h src/bgi2.cc)
target_include_directories(bgi2 PUBLIC include/)
target_link_libraries(bgi2 ${SDL2_LIBRARIES})

add_executable(example_hello_world "example/hello_world.cc")
target_link_libraries(example_hello_world bgi2)

add_executable(example_simple "example/simple.cc")
target_link_libraries(example_simple bgi2)

add_executable(example_interactive "example/interactive.cc")
target_link_libraries(example_interactive bgi2)

add_executable(example_grill "example/grill.cc")
target_link_libraries(example_grill bgi2)

add_executable(bgi2_test "src/bgi2_test.cc")
target_link_libraries(bgi2_test bgi2 GTest::gtest_main)
gtest_discover_tests(bgi2_test)
