/**
 * wasm-mini - A mini C++ CLI tool mirroring WasmEdge CLI sub-commands
 * 
 * Phase 1: CLI skeleton with argument parsing and sub-command routing
 * Phase 2: parse sub-command implementation using WasmEdge C API
 * Phase 3: validate sub-command implementation using WasmEdge C API
 * Phase 4: instantiate sub-command implementation using WasmEdge C API
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
 * Validate sub-command implementation using WasmEdge C API
 * 
 * Pipeline: Parse -> Validate
 * 
 * @param filename Path to the .wasm file to validate
 * @return Exit code (0 for success, non-zero for failure)
 */
int cmdValidate(const std::string& filename) {
    // Step 1: Create the parser context
    WasmEdge_ParserContext* parserCtx = WasmEdge_ParserCreate(nullptr);
    if (parserCtx == nullptr) {
        std::cerr << "[VALIDATE]\n"
                  << "File   : " << filename << "\n"
                  << "Result : FAILED\n"
                  << "Error  : Failed to create parser context\n";
        return EXIT_VALIDATE_ERROR;
    }

    // Step 2: Parse the WebAssembly file
    WasmEdge_ASTModuleContext* astModuleCtx = nullptr;
    WasmEdge_Result result = WasmEdge_ParserParseFromFile(parserCtx, &astModuleCtx, filename.c_str());

    if (!WasmEdge_ResultOK(result)) {
        uint32_t errorCode = WasmEdge_ResultGetCode(result);
        const char* errorMessage = WasmEdge_ResultGetMessage(result);
        
        std::cerr << "[VALIDATE]\n"
                  << "File   : " << filename << "\n"
                  << "Result : FAILED (Parse Error)\n"
                  << "Error  : [" << errorCode << "] " 
                  << (errorMessage ? errorMessage : "Unknown error") << "\n";
        
        WasmEdge_ParserDelete(parserCtx);
        return EXIT_VALIDATE_ERROR;
    }

    // Step 3: Create the validator context
    WasmEdge_ValidatorContext* validatorCtx = WasmEdge_ValidatorCreate(nullptr);
    if (validatorCtx == nullptr) {
        std::cerr << "[VALIDATE]\n"
                  << "File   : " << filename << "\n"
                  << "Result : FAILED\n"
                  << "Error  : Failed to create validator context\n";
        
        WasmEdge_ASTModuleDelete(astModuleCtx);
        WasmEdge_ParserDelete(parserCtx);
        return EXIT_VALIDATE_ERROR;
    }

    // Step 4: Validate the AST module
    result = WasmEdge_ValidatorValidate(validatorCtx, astModuleCtx);

    if (!WasmEdge_ResultOK(result)) {
        uint32_t errorCode = WasmEdge_ResultGetCode(result);
        const char* errorMessage = WasmEdge_ResultGetMessage(result);
        
        std::cerr << "[VALIDATE]\n"
                  << "File   : " << filename << "\n"
                  << "Result : INVALID\n"
                  << "Error  : [" << errorCode << "] " 
                  << (errorMessage ? errorMessage : "Unknown error") << "\n";
        
        WasmEdge_ValidatorDelete(validatorCtx);
        WasmEdge_ASTModuleDelete(astModuleCtx);
        WasmEdge_ParserDelete(parserCtx);
        return EXIT_VALIDATE_ERROR;
    }

    // Success - print structured output
    std::cout << "[VALIDATE]\n"
              << "File   : " << filename << "\n"
              << "Result : VALID\n";

    // Cleanup resources
    WasmEdge_ValidatorDelete(validatorCtx);
    WasmEdge_ASTModuleDelete(astModuleCtx);
    WasmEdge_ParserDelete(parserCtx);

    return EXIT_SUCCESS_CODE;
}

/**
 * Instantiate sub-command implementation using WasmEdge C API
 * 
 * Uses WasmEdge VM for streamlined load + instantiate workflow.
 * Does not execute any functions - only creates a ready VM instance.
 * 
 * @param filename Path to the .wasm file to instantiate
 * @return Exit code (0 for success, non-zero for failure)
 */
int cmdInstantiate(const std::string& filename) {
    // Step 1: Create the VM context with default configuration
    WasmEdge_VMContext* vmCtx = WasmEdge_VMCreate(nullptr, nullptr);
    if (vmCtx == nullptr) {
        std::cerr << "[INSTANTIATE]\n"
                  << "File      : " << filename << "\n"
                  << "VM Status : FAILED\n"
                  << "Error     : Failed to create VM context\n";
        return EXIT_INSTANTIATE_ERROR;
    }

    // Step 2: Load the WebAssembly module from file
    WasmEdge_Result result = WasmEdge_VMLoadWasmFromFile(vmCtx, filename.c_str());

    if (!WasmEdge_ResultOK(result)) {
        uint32_t errorCode = WasmEdge_ResultGetCode(result);
        const char* errorMessage = WasmEdge_ResultGetMessage(result);
        
        std::cerr << "[INSTANTIATE]\n"
                  << "File      : " << filename << "\n"
                  << "VM Status : FAILED (Load Error)\n"
                  << "Error     : [" << errorCode << "] " 
                  << (errorMessage ? errorMessage : "Unknown error") << "\n";
        
        WasmEdge_VMDelete(vmCtx);
        return EXIT_INSTANTIATE_ERROR;
    }

    // Step 3: Validate the loaded module
    result = WasmEdge_VMValidate(vmCtx);

    if (!WasmEdge_ResultOK(result)) {
        uint32_t errorCode = WasmEdge_ResultGetCode(result);
        const char* errorMessage = WasmEdge_ResultGetMessage(result);
        
        std::cerr << "[INSTANTIATE]\n"
                  << "File      : " << filename << "\n"
                  << "VM Status : FAILED (Validation Error)\n"
                  << "Error     : [" << errorCode << "] " 
                  << (errorMessage ? errorMessage : "Unknown error") << "\n";
        
        WasmEdge_VMDelete(vmCtx);
        return EXIT_INSTANTIATE_ERROR;
    }

    // Step 4: Instantiate the module
    result = WasmEdge_VMInstantiate(vmCtx);

    if (!WasmEdge_ResultOK(result)) {
        uint32_t errorCode = WasmEdge_ResultGetCode(result);
        const char* errorMessage = WasmEdge_ResultGetMessage(result);
        
        std::cerr << "[INSTANTIATE]\n"
                  << "File      : " << filename << "\n"
                  << "VM Status : FAILED (Instantiation Error)\n"
                  << "Error     : [" << errorCode << "] " 
                  << (errorMessage ? errorMessage : "Unknown error") << "\n";
        
        WasmEdge_VMDelete(vmCtx);
        return EXIT_INSTANTIATE_ERROR;
    }

    // Success - print structured output
    std::cout << "[INSTANTIATE]\n"
              << "File      : " << filename << "\n"
              << "VM Status : READY\n";

    // Cleanup resources
    WasmEdge_VMDelete(vmCtx);

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
