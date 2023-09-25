cmake_minimum_required(VERSION 3.22)
set(CMAKE_C_COMPILER /usr/bin/gcc-12)
set(CMAKE_CXX_COMPILER /usr/bin/g++-12)

project(cppcon2023)

cmake_path(GET PROJECT_BINARY_DIR STEM buildDir)
string(TOLOWER ${buildDir} buildDir)
if(buildDir STREQUAL "release")
    set(buildType RelWithDebInfo)
elseif(buildDir STREQUAL "debug")
    set(buildType Debug)
else()
    set(buildType Debug)
endif()
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE ${buildType})
endif()

set(BOOST_ROOT /mnt/c/Users/Charles/AppData/Local/lxss/rootfs/usr/)
find_package(Boost REQUIRED)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED true)
set(CMAKE_CXX_EXTENSIONS OFF)
add_compile_options(-Wall -Werror -Wextra -Wconversion)
add_compile_options(-Wno-unused-parameter)

function(add_fifo fifo)
    add_executable(${fifo} ${CMAKE_SOURCE_DIR}/${fifo}.cpp)
    target_link_libraries(${fifo} pthread)

    add_executable(${fifo}.tsan ${CMAKE_SOURCE_DIR}/${fifo}.cpp)
    target_compile_options(${fifo}.tsan PRIVATE -fsanitize=thread)
    target_link_libraries(${fifo}.tsan PRIVATE pthread tsan)
endfunction()


add_fifo(fifo1)
add_fifo(fifo2)
add_fifo(fifo3)
add_fifo(fifo4)
add_fifo(fifo4a)
add_fifo(fifo4b)
add_fifo(fifo5)
add_fifo(fifo5a)
add_fifo(boost_lockfree)
add_fifo(rigtorp)
target_compile_options(rigtorp PRIVATE -Wno-interference-size)
target_compile_options(rigtorp.tsan PRIVATE -Wno-interference-size)
add_fifo(mutex)
add_fifo(tryLock)


include(GoogleTest)
enable_testing()

add_executable(unitTests unitTests.cpp)
target_compile_options(unitTests PRIVATE -fsanitize=undefined -fsanitize=address)
target_link_libraries(unitTests PRIVATE asan ubsan gtest gtest_main)
target_link_options(unitTests PRIVATE -fsanitize=undefined -fsanitize=address)
gtest_add_tests(TARGET unitTests)


find_package(benchmark REQUIRED)
add_executable(bench bench.cpp)
target_compile_options(bench PRIVATE -Wno-interference-size)
target_link_libraries(bench PRIVATE benchmark::benchmark)

add_executable(bench_all bench_all.cpp)
target_compile_options(bench_all PRIVATE -Wno-interference-size)
