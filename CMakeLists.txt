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


set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED true)
set(CMAKE_CXX_EXTENSIONS OFF)
add_compile_options(-Wall -Werror -Wextra -Wconversion)
add_compile_options(-Wno-unused-parameter)


add_executable(fifo1 fifo1.cpp)
target_link_libraries(fifo1 pthread)

add_executable(fifo1.tsan fifo1.cpp)
target_compile_options(fifo1.tsan PRIVATE -fsanitize=thread)
target_link_libraries(fifo1.tsan PRIVATE pthread tsan)


add_executable(fifo2 fifo2.cpp)
target_link_libraries(fifo2 pthread)

add_executable(fifo2.tsan fifo2.cpp)
target_compile_options(fifo2.tsan PRIVATE -fsanitize=thread)
target_link_libraries(fifo2.tsan PRIVATE pthread tsan)


add_executable(fifo3 fifo3.cpp)
target_link_libraries(fifo3 pthread)

add_executable(fifo3.tsan fifo3.cpp)
target_compile_options(fifo3.tsan PRIVATE -fsanitize=thread)
target_link_libraries(fifo3.tsan PRIVATE pthread tsan)


add_executable(fifo4 fifo4.cpp)
target_link_libraries(fifo4 pthread)

add_executable(fifo4.tsan fifo4.cpp)
target_compile_options(fifo4.tsan PRIVATE -fsanitize=thread)
target_link_libraries(fifo4.tsan PRIVATE pthread tsan)


# add_executable(fifo7 fifo7.cpp)
# target_link_libraries(fifo7 pthread)
# 
# add_executable(fifo7.tsan fifo7.cpp)
# target_compile_options(fifo7.tsan PRIVATE -fsanitize=thread)
# target_link_libraries(fifo7.tsan PRIVATE pthread tsan)


include(GoogleTest)
enable_testing()

add_executable(unitTests unitTests.cpp)
target_compile_options(unitTests PRIVATE -fsanitize=undefined -fsanitize=address)
target_link_libraries(unitTests PRIVATE asan ubsan gtest gtest_main)
target_link_options(unitTests PRIVATE -fsanitize=undefined -fsanitize=address)
gtest_add_tests(TARGET unitTests)
