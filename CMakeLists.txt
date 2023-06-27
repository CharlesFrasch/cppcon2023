cmake_minimum_required(VERSION 3.22)
set(CMAKE_C_COMPILER /usr/bin/gcc-12)
set(CMAKE_CXX_COMPILER /usr/bin/g++-12)

project(cppcon2023)


# TODO fix this
set(CMAKE_BUILD_TYPE Debug)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED true)
set(CMAKE_CXX_EXTENSIONS OFF)
add_compile_options(-Wall -Werror -Wextra -Wconversion)
add_compile_options(-Wno-unused-parameter)

add_executable(benchmark benchmark.cpp)
target_link_libraries(benchmark pthread)

# add_executable(benchmark.tsan benchmark.cpp)
# target_compile_options(benchmark.tsan -fsanitize=thread)
# target_link_libraries(benchmark.tsan pthread)

include(GoogleTest)
enable_testing()

add_executable(tests sanitizers.cpp Fifo1_ut.cpp)
target_compile_options(tests PRIVATE -fsanitize=undefined -fsanitize=address)
target_link_libraries(tests PRIVATE asan ubsan gtest gtest_main)
target_link_options(tests PRIVATE -fsanitize=undefined -fsanitize=address)
gtest_add_tests(TARGET tests)
