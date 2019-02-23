cmake_minimum_required(VERSION 3.10)

file(GLOB SRC_FILES "*.h" "*.cpp")

set(CMAKE_BUILD_TYPE Debug)
add_executable(ioandcallbacks ${SRC_FILES})
target_link_libraries(ioandcallbacks rt)


