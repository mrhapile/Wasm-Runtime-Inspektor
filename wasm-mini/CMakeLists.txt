cmake_minimum_required(VERSION 3.16)
project(wasm-mini
    VERSION 0.1.0
    DESCRIPTION "Mini C++ CLI tool mirroring WasmEdge CLI sub-commands"
    LANGUAGES CXX
)

# C++17 standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Compiler warnings
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Find WasmEdge package
find_package(wasmedge REQUIRED)

# Create the executable
add_executable(wasm-mini
    src/main.cpp
)

# Link against WasmEdge
target_link_libraries(wasm-mini PRIVATE wasmedge)

# Include directories
target_include_directories(wasm-mini PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Installation
install(TARGETS wasm-mini
    RUNTIME DESTINATION bin
)
