/**
 * wasm-mini - A mini C++ CLI tool mirroring WasmEdge CLI sub-commands
 * 
 * Phase 1: CLI skeleton with argument parsing and sub-command routing
 * Phase 2: parse sub-command implementation using WasmEdge C API
 * Phase 3: validate sub-command implementation using WasmEdge C API
 * Phase 4: instantiate sub-command implementation using WasmEdge C API
 * Phase 5: Production-quality error handling, exit codes, and resource discipline
 */

#include <iostream>
#include <string>
#include <string_view>
#include <cstdlib>

#include <wasmedge/wasmedge.h>

// ============================================================================
// Exit Codes (consistent across all commands)
// ============================================================================
constexpr int EXIT_OK = 0;           // Success
constexpr int EXIT_CLI_ERROR = 1;    // CLI / user input error (wrong arguments, unknown command)
constexpr int EXIT_RUNTIME_ERROR = 2; // WasmEdge runtime error (parse, validate, instantiate failure)

// ============================================================================
// Program Metadata
// ============================================================================
constexpr std::string_view PROGRAM_NAME = "wasm-mini";
constexpr std::string_view VERSION = "0.1.0";

// ============================================================================
// Output Helpers - Centralized for consistent formatting
// ============================================================================

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
 * Print CLI error message (for user input errors)
 * 
 * @param message Error description
 */
void printCliError(std::string_view message) {
    std::cerr << "Error: " << message << "\n\n";
}

/**
 * Print structured WasmEdge runtime error output
 * Centralized helper for consistent error formatting across all commands.
 * 
 * @param command   Command name (PARSE, VALIDATE, INSTANTIATE)
 * @param filename  Path to the .wasm file
 * @param status    Status string (FAILED, INVALID, etc.)
 * @param result    WasmEdge result containing error details
 */
void printWasmEdgeError(std::string_view command, std::string_view filename,
                        std::string_view status, WasmEdge_Result result) {
    uint32_t errorCode = WasmEdge_ResultGetCode(result);
    const char* errorMessage = WasmEdge_ResultGetMessage(result);
    
    std::cerr << "[" << command << "]\n"
              << "File   : " << filename << "\n"
              << "Status : " << status << "\n"
              << "Error  : [" << errorCode << "] " 
              << (errorMessage ? errorMessage : "Unknown error") << "\n";
}

/**
 * Print structured context creation error
 * Used when WasmEdge context creation fails (returns nullptr).
 * 
 * @param command      Command name
 * @param filename     Path to the .wasm file
 * @param contextName  Name of the context that failed to create
 */
void printContextError(std::string_view command, std::string_view filename,
                       std::string_view contextName) {
    std::cerr << "[" << command << "]\n"
              << "File   : " << filename << "\n"
              << "Status : FAILED\n"
              << "Error  : Failed to create " << contextName << "\n";
}

/**
 * Print structured success output
 * 
 * @param command   Command name
 * @param filename  Path to the .wasm file
 * @param status    Status string (SUCCESS, VALID, READY)
 */
void printSuccess(std::string_view command, std::string_view filename,
                  std::string_view status) {
    std::cout << "[" << command << "]\n"
              << "File   : " << filename << "\n"
              << "Status : " << status << "\n";
}

// ============================================================================
// Sub-command Implementations
// ============================================================================

/**
 * Parse sub-command implementation using WasmEdge C API
 * 
 * Demonstrates: Parser context lifecycle, AST module creation
 * 
 * @param filename Path to the .wasm file to parse
 * @return Exit code (EXIT_OK or EXIT_RUNTIME_ERROR)
 */
int cmdParse(const std::string& filename) {
    // Resource pointers - initialized to nullptr for safe cleanup
    WasmEdge_ParserContext* parserCtx = nullptr;
    WasmEdge_ASTModuleContext* astModuleCtx = nullptr;
    int exitCode = EXIT_OK;

    // Step 1: Create the parser context
    parserCtx = WasmEdge_ParserCreate(nullptr);
    if (parserCtx == nullptr) {
        printContextError("PARSE", filename, "parser context");
        return EXIT_RUNTIME_ERROR;
    }

    // Step 2: Parse the WebAssembly file
    WasmEdge_Result result = WasmEdge_ParserParseFromFile(parserCtx, &astModuleCtx, filename.c_str());

    if (!WasmEdge_ResultOK(result)) {
        printWasmEdgeError("PARSE", filename, "FAILED", result);
        exitCode = EXIT_RUNTIME_ERROR;
    } else {
        // Success
        printSuccess("PARSE", filename, "SUCCESS");
    }

    // Cleanup resources (always executed)
    if (astModuleCtx != nullptr) {
        WasmEdge_ASTModuleDelete(astModuleCtx);
    }
    if (parserCtx != nullptr) {
        WasmEdge_ParserDelete(parserCtx);
    }

    return exitCode;
}

/**
 * Validate sub-command implementation using WasmEdge C API
 * 
 * Pipeline: Parse -> Validate
 * Demonstrates: Multi-context lifecycle, semantic validation
 * 
 * @param filename Path to the .wasm file to validate
 * @return Exit code (EXIT_OK or EXIT_RUNTIME_ERROR)
 */
int cmdValidate(const std::string& filename) {
    // Resource pointers - initialized to nullptr for safe cleanup
    WasmEdge_ParserContext* parserCtx = nullptr;
    WasmEdge_ASTModuleContext* astModuleCtx = nullptr;
    WasmEdge_ValidatorContext* validatorCtx = nullptr;
    int exitCode = EXIT_OK;

    // Step 1: Create the parser context
    parserCtx = WasmEdge_ParserCreate(nullptr);
    if (parserCtx == nullptr) {
        printContextError("VALIDATE", filename, "parser context");
        return EXIT_RUNTIME_ERROR;
    }

    // Step 2: Parse the WebAssembly file
    WasmEdge_Result result = WasmEdge_ParserParseFromFile(parserCtx, &astModuleCtx, filename.c_str());

    if (!WasmEdge_ResultOK(result)) {
        printWasmEdgeError("VALIDATE", filename, "FAILED (Parse Error)", result);
        exitCode = EXIT_RUNTIME_ERROR;
        goto cleanup;
    }

    // Step 3: Create the validator context
    validatorCtx = WasmEdge_ValidatorCreate(nullptr);
    if (validatorCtx == nullptr) {
        printContextError("VALIDATE", filename, "validator context");
        exitCode = EXIT_RUNTIME_ERROR;
        goto cleanup;
    }

    // Step 4: Validate the AST module
    result = WasmEdge_ValidatorValidate(validatorCtx, astModuleCtx);

    if (!WasmEdge_ResultOK(result)) {
        printWasmEdgeError("VALIDATE", filename, "INVALID", result);
        exitCode = EXIT_RUNTIME_ERROR;
    } else {
        // Success
        printSuccess("VALIDATE", filename, "VALID");
    }

cleanup:
    // Cleanup resources (always executed, order matters)
    if (validatorCtx != nullptr) {
        WasmEdge_ValidatorDelete(validatorCtx);
    }
    if (astModuleCtx != nullptr) {
        WasmEdge_ASTModuleDelete(astModuleCtx);
    }
    if (parserCtx != nullptr) {
        WasmEdge_ParserDelete(parserCtx);
    }

    return exitCode;
}

/**
 * Instantiate sub-command implementation using WasmEdge C API
 * 
 * Pipeline: VM Create -> Load -> Validate -> Instantiate
 * Demonstrates: VM lifecycle, streamlined module loading
 * Does not execute any functions - only creates a ready VM instance.
 * 
 * @param filename Path to the .wasm file to instantiate
 * @return Exit code (EXIT_OK or EXIT_RUNTIME_ERROR)
 */
int cmdInstantiate(const std::string& filename) {
    // Resource pointer - initialized to nullptr for safe cleanup
    WasmEdge_VMContext* vmCtx = nullptr;
    int exitCode = EXIT_OK;

    // Step 1: Create the VM context
    vmCtx = WasmEdge_VMCreate(nullptr, nullptr);
    if (vmCtx == nullptr) {
        printContextError("INSTANTIATE", filename, "VM context");
        return EXIT_RUNTIME_ERROR;
    }

    // Step 2: Load the WebAssembly module from file
    WasmEdge_Result result = WasmEdge_VMLoadWasmFromFile(vmCtx, filename.c_str());

    if (!WasmEdge_ResultOK(result)) {
        printWasmEdgeError("INSTANTIATE", filename, "FAILED (Load Error)", result);
        exitCode = EXIT_RUNTIME_ERROR;
        goto cleanup;
    }

    // Step 3: Validate the loaded module
    result = WasmEdge_VMValidate(vmCtx);

    if (!WasmEdge_ResultOK(result)) {
        printWasmEdgeError("INSTANTIATE", filename, "FAILED (Validation Error)", result);
        exitCode = EXIT_RUNTIME_ERROR;
        goto cleanup;
    }

    // Step 4: Instantiate the module
    result = WasmEdge_VMInstantiate(vmCtx);

    if (!WasmEdge_ResultOK(result)) {
        printWasmEdgeError("INSTANTIATE", filename, "FAILED (Instantiation Error)", result);
        exitCode = EXIT_RUNTIME_ERROR;
    } else {
        // Success
        printSuccess("INSTANTIATE", filename, "READY");
    }

cleanup:
    // Cleanup resources (always executed)
    if (vmCtx != nullptr) {
        WasmEdge_VMDelete(vmCtx);
    }

    return exitCode;
}

// ============================================================================
// Main Entry Point
// ============================================================================

/**
 * Main entry point - argument parsing and command routing
 * 
 * Exit codes:
 *   EXIT_OK (0)           - Success
 *   EXIT_CLI_ERROR (1)    - Invalid arguments, unknown command
 *   EXIT_RUNTIME_ERROR (2) - WasmEdge runtime error
 */
int main(int argc, char* argv[]) {
    // Check minimum argument count
    if (argc < 2) {
        printCliError("No command specified.");
        printUsage();
        return EXIT_CLI_ERROR;
    }

    std::string_view arg1 = argv[1];

    // Handle help option
    if (arg1 == "-h" || arg1 == "--help") {
        printUsage();
        return EXIT_OK;
    }

    // Handle version option
    if (arg1 == "-v" || arg1 == "--version") {
        printVersion();
        return EXIT_OK;
    }

    // From here, we expect a command
    std::string_view command = arg1;

    // Validate known commands
    if (command != "parse" && command != "validate" && command != "instantiate") {
        printCliError(std::string("Unknown command '") + std::string(command) + "'.");
        printUsage();
        return EXIT_CLI_ERROR;
    }

    // Check for file argument
    if (argc < 3) {
        printCliError(std::string("Missing file argument for '") + std::string(command) + "' command.");
        printUsage();
        return EXIT_CLI_ERROR;
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

    // Should not reach here (defensive)
    return EXIT_CLI_ERROR;
}
