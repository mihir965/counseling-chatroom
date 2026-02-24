cmake_minimum_required(VERSION 3.20)
project(CounselChatServer VERSION 1.0 LANGUAGES CXX)

# Use C++20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Compiler warnings (helps catch bugs early)
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/cpp_include)

# Socket library (will grow as we add more classes)
add_library(sockets_cpp
    cpp_src/socket.cpp
    cpp_src/server_utils.cpp
    cpp_src/buffer.cpp
)

# Main server executable (for now, just a test)
add_executable(counsel_server_cpp
    cpp_src/main.cpp
)

# Link the socket library to the executable
target_link_libraries(counsel_server_cpp PRIVATE sockets_cpp)

# Optional: Add test executable later
# add_executable(socket_test tests/socket_test.cpp)
# target_link_libraries(socket_test PRIVATE sockets_cpp)
