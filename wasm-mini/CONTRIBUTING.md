# Contributing to wasm-mini

Thank you for your interest in contributing to wasm-mini! This document provides guidelines for contributing to the project.

## Development Setup

### Prerequisites

1. **C++17 compatible compiler**
   - GCC 8+ or Clang 7+ on Linux/macOS
   - MSVC 2019+ on Windows

2. **CMake 3.16+**
   ```bash
   cmake --version
   ```

3. **WasmEdge runtime**
   ```bash
   # macOS
   brew install wasmedge

   # Linux (Ubuntu/Debian)
   curl -sSf https://raw.githubusercontent.com/WasmEdge/WasmEdge/master/utils/install.sh | bash

   # Verify installation
   wasmedge --version
   ```

4. **WABT toolkit** (optional, for creating test modules)
   ```bash
   # macOS
   brew install wabt

   # Linux
   apt install wabt
   ```

### Building from Source

```bash
git clone https://github.com/user/Wasm-Runtime-Inspektor.git
cd Wasm-Runtime-Inspektor/wasm-mini

mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Debug ..
make
```

### Running Tests

```bash
# Test with provided sample
./wasm-mini parse ../examples/test.wasm
./wasm-mini validate ../examples/test.wasm
./wasm-mini instantiate ../examples/test.wasm

# Test verbose mode
./wasm-mini --verbose validate ../examples/test.wasm

# Test error handling
./wasm-mini parse nonexistent.wasm  # Should exit with code 1
```

## Code Style

### C++ Guidelines

- **Standard**: C++17
- **Naming**:
  - Functions: `camelCase` (e.g., `printUsage()`, `cmdValidate()`)
  - Constants: `UPPER_SNAKE_CASE` (e.g., `EXIT_OK`, `PROGRAM_NAME`)
  - Types/Structs: `PascalCase` (e.g., `ParserDeleter`, `VMPtr`)
  - Variables: `camelCase` (e.g., `filename`, `parserCtx`)

- **Formatting**:
  - 4-space indentation
  - Opening braces on same line
  - Max line length: 100 characters

- **Documentation**:
  - All public functions must have Doxygen-style comments
  - Include `@param` and `@return` annotations

### Example

```cpp
/**
 * Parse sub-command implementation using WasmEdge C API
 * 
 * @param filename Path to the .wasm file to parse
 * @return Exit code (EXIT_OK or EXIT_RUNTIME_ERROR)
 */
int cmdParse(const std::string& filename) {
    // Implementation...
}
```

## Project Structure

```
wasm-mini/
├── CMakeLists.txt          # Build configuration
├── README.md               # User documentation
├── CONTRIBUTING.md         # This file
├── src/
│   └── main.cpp            # All CLI logic
└── examples/
    ├── test.wat            # Sample module (text format)
    └── test.wasm           # Sample module (binary)
```

## Making Changes

### Workflow

1. **Fork** the repository
2. **Create a branch** for your feature/fix
   ```bash
   git checkout -b feature/my-feature
   ```
3. **Make changes** following the code style guidelines
4. **Test** your changes thoroughly
5. **Commit** with clear messages
   ```bash
   git commit -m "feat: add support for XYZ"
   ```
6. **Push** and create a Pull Request

### Commit Message Format

Follow conventional commits:

- `feat:` New feature
- `fix:` Bug fix
- `docs:` Documentation only
- `refactor:` Code change that neither fixes a bug nor adds a feature
- `test:` Adding or updating tests
- `chore:` Maintenance tasks

### Pull Request Checklist

- [ ] Code compiles without warnings (`-Wall -Wextra -Wpedantic`)
- [ ] All three commands work correctly
- [ ] No memory leaks (RAII patterns followed)
- [ ] Exit codes are correct
- [ ] Error messages are clear and actionable
- [ ] Documentation updated if needed

## Adding a New Command

To add a new sub-command (e.g., `run`):

1. **Create the command handler**
   ```cpp
   int cmdRun(const std::string& filename) {
       // Implementation using WasmEdge C API
   }
   ```

2. **Add RAII wrappers** if new contexts are needed

3. **Update `printUsage()`** with the new command

4. **Add routing in `main()`**
   ```cpp
   if (command == "run") {
       return cmdRun(filename);
   }
   ```

5. **Update README.md** with usage examples

## Reporting Issues

When reporting bugs, please include:

- OS and version
- Compiler and version
- WasmEdge version (`wasmedge --version`)
- Steps to reproduce
- Expected vs actual behavior
- Relevant error output

## Questions?

Open an issue with the `question` label or reach out to the maintainers.
