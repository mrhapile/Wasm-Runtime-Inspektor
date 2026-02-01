# Wasm-Runtime-Inspektor

A toolkit for inspecting and analyzing WebAssembly runtimes.

## Projects

### wasm-mini

A minimal C++ CLI tool that mirrors core WasmEdge CLI sub-commands.

**Features:**
- `parse` — Parse a WebAssembly binary file
- `validate` — Semantically validate a WebAssembly module
- `instantiate` — Load and instantiate a module in the WasmEdge VM

**Quick Start:**
```bash
cd wasm-mini
mkdir build && cd build
cmake .. && make
./wasm-mini --help
```

See [wasm-mini/README.md](wasm-mini/README.md) for full documentation.

## License

MIT