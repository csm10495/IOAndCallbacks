cmake_minimum_required(VERSION 3.10)

file(GLOB SRC_FILES "*.h" "*.cpp")

add_executable(ioandcallbacks ${SRC_FILES})

if (UNIX)
target_link_libraries(ioandcallbacks rt)
endif()
