# wasm-mini

A minimal C++ CLI tool that mirrors core [WasmEdge](https://wasmedge.org/) CLI sub-commands. Built using the WasmEdge C API with production-quality error handling and resource management.

## Overview

`wasm-mini` provides three essential WebAssembly operations:

| Command | Description |
|---------|-------------|
| `parse` | Parse a `.wasm` file and produce an AST module |
| `validate` | Parse and semantically validate a `.wasm` module |
| `instantiate` | Load, validate, and instantiate a module in the VM |

This tool demonstrates proper WasmEdge C API usage patterns including:
- Context lifecycle management with RAII wrappers
- Structured error reporting with actionable messages
- Consistent exit codes for scripting integration

## Prerequisites

- **CMake** 3.16 or higher
- **C++17** compatible compiler (GCC 8+, Clang 7+, MSVC 2019+)
- **WasmEdge** runtime installed ([installation guide](https://wasmedge.org/docs/start/install))

### Verify WasmEdge Installation

```bash
# Check WasmEdge is installed
wasmedge --version

# Ensure pkg-config can find it (optional)
pkg-config --modversion wasmedge
```

## Build Instructions

```bash
# Clone the repository
git clone https://github.com/user/Wasm-Runtime-Inspektor.git
cd Wasm-Runtime-Inspektor/wasm-mini

# Create build directory
mkdir build && cd build

# Configure with CMake
cmake ..

# Build
make

# (Optional) Install to system
sudo make install
```

### Build Options

```bash
# Debug build
cmake -DCMAKE_BUILD_TYPE=Debug ..

# Release build with optimizations
cmake -DCMAKE_BUILD_TYPE=Release ..

# Custom WasmEdge installation path
cmake -Dwasmedge_DIR=/path/to/wasmedge/lib/cmake/wasmedge ..
```

## Usage

```
wasm-mini [options] <command> <file.wasm>
```

### Options

| Option | Description |
|--------|-------------|
| `-h, --help` | Show help message |
| `-v, --version` | Show version information |
| `--verbose` | Enable verbose output |

### Commands

#### parse

Parse a WebAssembly binary file and validate its structure.

```bash
./wasm-mini parse example.wasm
```

**Output (success):**
```
[PARSE]
File   : example.wasm
Status : SUCCESS
```

**Output (failure):**
```
[PARSE]
File   : invalid.wasm
Status : FAILED
Error  : [102] Invalid magic number
```

#### validate

Parse and semantically validate a WebAssembly module.

```bash
./wasm-mini validate example.wasm
```

**Output (success):**
```
[VALIDATE]
File   : example.wasm
Status : VALID
```

**Output (failure):**
```
[VALIDATE]
File   : invalid.wasm
Status : INVALID
Error  : [201] Type mismatch in function call
```

#### instantiate

Load, validate, and instantiate a WebAssembly module in the VM.

```bash
./wasm-mini instantiate example.wasm
```

**Output (success):**
```
[INSTANTIATE]
File   : example.wasm
Status : READY
```

**Output (failure):**
```
[INSTANTIATE]
File   : missing_import.wasm
Status : FAILED (Instantiation Error)
Error  : [301] Unknown import: env.print
```

### Verbose Mode

Enable detailed progress output for debugging:

```bash
./wasm-mini --verbose validate example.wasm
```

**Output:**
```
[VERBOSE] WasmEdge version: 0.14.0
[VERBOSE] Processing file: example.wasm
[VERBOSE] Creating parser context...
[VERBOSE] Parsing WebAssembly module...
[VERBOSE] Creating validator context...
[VERBOSE] Validating WebAssembly module...
[VERBOSE] Validation completed successfully.
[VALIDATE]
File   : example.wasm
Status : VALID
```

## Exit Codes

| Code | Name | Description |
|------|------|-------------|
| `0` | `EXIT_OK` | Operation completed successfully |
| `1` | `EXIT_CLI_ERROR` | Invalid arguments, unknown command, or file not found |
| `2` | `EXIT_RUNTIME_ERROR` | WasmEdge runtime error (parse, validate, or instantiate failure) |

### Scripting Integration

```bash
# Check if a module is valid
if ./wasm-mini validate module.wasm; then
    echo "Module is valid"
else
    echo "Validation failed with exit code $?"
fi

# Batch validation
for wasm in *.wasm; do
    if ./wasm-mini validate "$wasm" > /dev/null 2>&1; then
        echo "✓ $wasm"
    else
        echo "✗ $wasm"
    fi
done
```

## Error Output Format

All errors follow a consistent structured format:

```
[COMMAND]
File   : <filepath>
Status : <status>
Error  : [<code>] <message>
```

| Field | Description |
|-------|-------------|
| `COMMAND` | The operation attempted (`PARSE`, `VALIDATE`, `INSTANTIATE`) |
| `File` | Path to the `.wasm` file |
| `Status` | Result status (`SUCCESS`, `VALID`, `READY`, `FAILED`, `INVALID`) |
| `Error` | WasmEdge error code and human-readable message |

## Architecture

### Project Structure

```
wasm-mini/
├── CMakeLists.txt      # Build configuration
├── README.md           # This file
└── src/
    └── main.cpp        # All CLI logic (single-file design)
```

### Design Decisions

1. **Single-file design**: All logic in `main.cpp` for simplicity and portability
2. **RAII wrappers**: Custom deleters with `std::unique_ptr` ensure leak-free resource management
3. **Centralized error helpers**: Consistent output formatting via `printWasmEdgeError()`, `printContextError()`, `printSuccess()`
4. **Staged VM pipeline**: `instantiate` command explicitly runs Load → Validate → Instantiate stages

### WasmEdge API Usage

| Context | Create | Delete | Purpose |
|---------|--------|--------|---------|
| `WasmEdge_ParserContext` | `WasmEdge_ParserCreate()` | `WasmEdge_ParserDelete()` | Parse `.wasm` binary |
| `WasmEdge_ValidatorContext` | `WasmEdge_ValidatorCreate()` | `WasmEdge_ValidatorDelete()` | Semantic validation |
| `WasmEdge_ASTModuleContext` | (from parser) | `WasmEdge_ASTModuleDelete()` | AST representation |
| `WasmEdge_VMContext` | `WasmEdge_VMCreate()` | `WasmEdge_VMDelete()` | Virtual machine instance |

## Testing

### Create a Test Module

You can create a minimal WebAssembly module using [wat2wasm](https://github.com/WebAssembly/wabt):

```wat
;; test.wat - Minimal valid WebAssembly module
(module
  (func (export "add") (param i32 i32) (result i32)
    local.get 0
    local.get 1
    i32.add
  )
)
```

```bash
# Convert to .wasm
wat2wasm test.wat -o test.wasm

# Test with wasm-mini
./wasm-mini parse test.wasm
./wasm-mini validate test.wasm
./wasm-mini instantiate test.wasm
```

### Test Invalid Input

```bash
# Non-existent file
./wasm-mini parse nonexistent.wasm
# Error: File not found: nonexistent.wasm

# Invalid binary
echo "not a wasm file" > invalid.wasm
./wasm-mini parse invalid.wasm
# [PARSE]
# File   : invalid.wasm
# Status : FAILED
# Error  : [102] Invalid magic number
```

## Version Information

```bash
./wasm-mini --version
```

**Output:**
```
wasm-mini version 0.1.0
WasmEdge version: 0.14.0
```

## License

This project is part of the Wasm-Runtime-Inspektor toolkit.

## See Also

- [WasmEdge Documentation](https://wasmedge.org/docs/)
- [WasmEdge C API Reference](https://wasmedge.org/docs/embed/c/reference)
- [WebAssembly Specification](https://webassembly.github.io/spec/)
