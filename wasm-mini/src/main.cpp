/**
 * wasm-mini - A mini C++ CLI tool mirroring WasmEdge CLI sub-commands
 * 
 * Phase 1: CLI skeleton with argument parsing and sub-command routing
 * Phase 2: parse sub-command implementation using WasmEdge C API
 */

#include <iostream>
#include <string>
#include <string_view>
#include <cstdlib>

#include <wasmedge/wasmedge.h>

// Exit codes
constexpr int EXIT_SUCCESS_CODE = 0;
constexpr int EXIT_USAGE_ERROR = 1;
constexpr int EXIT_PARSE_ERROR = 2;
constexpr int EXIT_VALIDATE_ERROR = 3;
constexpr int EXIT_INSTANTIATE_ERROR = 4;

// Program name for usage messages
constexpr std::string_view PROGRAM_NAME = "wasm-mini";
constexpr std::string_view VERSION = "0.1.0";

/**
 * Print program usage information
 */
void printUsage() {
    std::cout << "Usage: " << PROGRAM_NAME << " <command> <file.wasm>\n"
              << "\n"
              << "A mini CLI tool mirroring WasmEdge CLI sub-commands.\n"
              << "\n"
              << "Commands:\n"
              << "  parse        Parse a WebAssembly module\n"
              << "  validate     Validate a WebAssembly module\n"
              << "  instantiate  Instantiate a WebAssembly module\n"
              << "\n"
              << "Options:\n"
              << "  -h, --help     Show this help message\n"
              << "  -v, --version  Show version information\n"
              << "\n"
              << "Examples:\n"
              << "  " << PROGRAM_NAME << " parse example.wasm\n"
              << "  " << PROGRAM_NAME << " validate example.wasm\n"
              << "  " << PROGRAM_NAME << " instantiate example.wasm\n";
}

/**
 * Print version information
 */
void printVersion() {
    std::cout << PROGRAM_NAME << " version " << VERSION << "\n"
              << "WasmEdge version: " << WasmEdge_VersionGet() << "\n";
}

/**
 * Print structured error output
 */
void printError(std::string_view command, std::string_view filename, 
                uint32_t errorCode, std::string_view errorMessage) {
    std::cerr << "[" << command << "]\n"
              << "File   : " << filename << "\n"
              << "Status : FAILED\n"
              << "Error  : [" << errorCode << "] " << errorMessage << "\n";
}

/**
 * Print structured success output
 */
void printSuccess(std::string_view command, std::string_view filename) {
    std::cout << "[" << command << "]\n"
              << "File   : " << filename << "\n"
              << "Status : SUCCESS\n";
}

/**
 * Parse sub-command implementation using WasmEdge C API
 * 
 * @param filename Path to the .wasm file to parse
 * @return Exit code (0 for success, non-zero for failure)
 */
int cmdParse(const std::string& filename) {
    // Create the parser context with default configuration
    WasmEdge_ParserContext* parserCtx = WasmEdge_ParserCreate(nullptr);
    if (parserCtx == nullptr) {
        std::cerr << "[PARSE]\n"
                  << "File   : " << filename << "\n"
                  << "Status : FAILED\n"
                  << "Error  : Failed to create parser context\n";
        return EXIT_PARSE_ERROR;
    }

    // Parse the WebAssembly file
    WasmEdge_ASTModuleContext* astModuleCtx = nullptr;
    WasmEdge_Result result = WasmEdge_ParserParseFromFile(parserCtx, &astModuleCtx, filename.c_str());

    // Check for parsing errors
    if (!WasmEdge_ResultOK(result)) {
        uint32_t errorCode = WasmEdge_ResultGetCode(result);
        const char* errorMessage = WasmEdge_ResultGetMessage(result);
        
        printError("PARSE", filename, errorCode, errorMessage ? errorMessage : "Unknown error");
        
        // Cleanup parser context
        WasmEdge_ParserDelete(parserCtx);
        return EXIT_PARSE_ERROR;
    }

    // Success - print structured output
    printSuccess("PARSE", filename);

    // Cleanup resources
    if (astModuleCtx != nullptr) {
        WasmEdge_ASTModuleDelete(astModuleCtx);
    }
    WasmEdge_ParserDelete(parserCtx);

    return EXIT_SUCCESS_CODE;
}

/**
 * Validate sub-command (stub - Phase 3)
 * 
 * @param filename Path to the .wasm file to validate
 * @return Exit code (0 for success, non-zero for failure)
 */
int cmdValidate(const std::string& filename) {
    std::cout << "[VALIDATE]\n"
              << "File   : " << filename << "\n"
              << "Status : NOT IMPLEMENTED\n"
              << "Note   : Validation will be implemented in Phase 3\n";
    return EXIT_SUCCESS_CODE;
}

/**
 * Instantiate sub-command (stub - Phase 4)
 * 
 * @param filename Path to the .wasm file to instantiate
 * @return Exit code (0 for success, non-zero for failure)
 */
int cmdInstantiate(const std::string& filename) {
    std::cout << "[INSTANTIATE]\n"
              << "File   : " << filename << "\n"
              << "Status : NOT IMPLEMENTED\n"
              << "Note   : Instantiation will be implemented in Phase 4\n";
    return EXIT_SUCCESS_CODE;
}

/**
 * Main entry point - argument parsing and command routing
 */
int main(int argc, char* argv[]) {
    // Check minimum argument count
    if (argc < 2) {
        std::cerr << "Error: No command specified.\n\n";
        printUsage();
        return EXIT_USAGE_ERROR;
    }

    std::string_view arg1 = argv[1];

    // Handle help option
    if (arg1 == "-h" || arg1 == "--help") {
        printUsage();
        return EXIT_SUCCESS_CODE;
    }

    // Handle version option
    if (arg1 == "-v" || arg1 == "--version") {
        printVersion();
        return EXIT_SUCCESS_CODE;
    }

    // From here, we expect a command
    std::string_view command = arg1;

    // Validate known commands
    if (command != "parse" && command != "validate" && command != "instantiate") {
        std::cerr << "Error: Unknown command '" << command << "'.\n\n";
        printUsage();
        return EXIT_USAGE_ERROR;
    }

    // Check for file argument
    if (argc < 3) {
        std::cerr << "Error: Missing file argument for '" << command << "' command.\n\n";
        printUsage();
        return EXIT_USAGE_ERROR;
    }

    std::string filename = argv[2];

    // Route to the appropriate sub-command handler
    if (command == "parse") {
        return cmdParse(filename);
    } else if (command == "validate") {
        return cmdValidate(filename);
    } else if (command == "instantiate") {
        return cmdInstantiate(filename);
    }

    // Should not reach here
    return EXIT_USAGE_ERROR;
}
