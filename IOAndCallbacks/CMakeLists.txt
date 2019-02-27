cmake_minimum_required(VERSION 3.10)

file(GLOB SRC_FILES "*.h" "*.cpp")

add_executable(ioandcallbacks ${SRC_FILES})

add_executable(tests ${SRC_FILES})
target_compile_definitions(tests PRIVATE TEST_BUILD=1)

if (UNIX)
target_link_libraries(ioandcallbacks rt)
endif()
