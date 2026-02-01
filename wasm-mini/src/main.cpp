/**
 * wasm-mini - A mini C++ CLI tool mirroring WasmEdge CLI sub-commands
 * 
 * Phase 1: CLI skeleton with argument parsing and sub-command routing
 * Phase 2: parse sub-command implementation using WasmEdge C API
 * Phase 3: validate sub-command implementation using WasmEdge C API
 * Phase 4: instantiate sub-command implementation using WasmEdge C API
 * Phase 5: Production-quality error handling, exit codes, and resource discipline
 * Phase 6: File validation, RAII wrappers, and verbose mode
 */

#include <iostream>
#include <string>
#include <string_view>
#include <memory>
#include <filesystem>

#include <wasmedge/wasmedge.h>

namespace fs = std::filesystem;

// ============================================================================
// Exit Codes (consistent across all commands)
// ============================================================================
constexpr int EXIT_OK = 0;           // Success
constexpr int EXIT_CLI_ERROR = 1;    // CLI / user input error (wrong arguments, unknown command)
constexpr int EXIT_RUNTIME_ERROR = 2; // WasmEdge runtime error (parse, validate, instantiate failure)

// ============================================================================
// Global State
// ============================================================================
bool g_verbose = false;  // Verbose mode flag

// ============================================================================
// RAII Wrappers for WasmEdge C Contexts
// ============================================================================

/**
 * RAII wrapper for WasmEdge_ParserContext
 * Automatically calls WasmEdge_ParserDelete on destruction
 */
struct ParserDeleter {
    void operator()(WasmEdge_ParserContext* ctx) const {
        if (ctx) WasmEdge_ParserDelete(ctx);
    }
};
using ParserPtr = std::unique_ptr<WasmEdge_ParserContext, ParserDeleter>;

/**
 * RAII wrapper for WasmEdge_ValidatorContext
 * Automatically calls WasmEdge_ValidatorDelete on destruction
 */
struct ValidatorDeleter {
    void operator()(WasmEdge_ValidatorContext* ctx) const {
        if (ctx) WasmEdge_ValidatorDelete(ctx);
    }
};
using ValidatorPtr = std::unique_ptr<WasmEdge_ValidatorContext, ValidatorDeleter>;

/**
 * RAII wrapper for WasmEdge_ASTModuleContext
 * Automatically calls WasmEdge_ASTModuleDelete on destruction
 */
struct ASTModuleDeleter {
    void operator()(WasmEdge_ASTModuleContext* ctx) const {
        if (ctx) WasmEdge_ASTModuleDelete(ctx);
    }
};
using ASTModulePtr = std::unique_ptr<WasmEdge_ASTModuleContext, ASTModuleDeleter>;

/**
 * RAII wrapper for WasmEdge_VMContext
 * Automatically calls WasmEdge_VMDelete on destruction
 */
struct VMDeleter {
    void operator()(WasmEdge_VMContext* ctx) const {
        if (ctx) WasmEdge_VMDelete(ctx);
    }
};
using VMPtr = std::unique_ptr<WasmEdge_VMContext, VMDeleter>;

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
    std::cout << "Usage: " << PROGRAM_NAME << " [options] <command> <file.wasm>\n"
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
              << "  --verbose      Enable verbose output\n"
              << "\n"
              << "Examples:\n"
              << "  " << PROGRAM_NAME << " parse example.wasm\n"
              << "  " << PROGRAM_NAME << " validate example.wasm\n"
              << "  " << PROGRAM_NAME << " --verbose instantiate example.wasm\n";
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

/**
 * Print verbose information (only when --verbose is enabled)
 * 
 * @param message Information to display
 */
void printVerbose(std::string_view message) {
    if (g_verbose) {
        std::cout << "[VERBOSE] " << message << "\n";
    }
}

/**
 * Check if file exists
 * 
 * @param filepath Path to check
 * @return true if file exists, false otherwise
 */
bool fileExists(const std::string& filepath) {
    std::error_code ec;
    return fs::exists(filepath, ec) && fs::is_regular_file(filepath, ec);
}

/**
 * Check if file has .wasm extension
 * 
 * @param filepath Path to check
 * @return true if file ends with .wasm, false otherwise
 */
bool hasWasmExtension(const std::string& filepath) {
    return filepath.size() >= 5 && 
           filepath.substr(filepath.size() - 5) == ".wasm";
}

/**
 * Validate file before processing
 * Checks existence and warns about extension.
 * 
 * @param filepath Path to validate
 * @return true if file is valid for processing, false otherwise
 */
bool validateFile(const std::string& filepath) {
    // Check file existence
    if (!fileExists(filepath)) {
        std::cerr << "Error: File not found: " << filepath << "\n";
        return false;
    }
    
    // Warn about extension (informational only)
    if (!hasWasmExtension(filepath)) {
        std::cerr << "Warning: File does not have .wasm extension: " << filepath << "\n";
    }
    
    return true;
}

// ============================================================================
// Sub-command Implementations
// ============================================================================

/**
 * Parse sub-command implementation using WasmEdge C API
 * 
 * Demonstrates: Parser context lifecycle, AST module creation
 * Uses RAII wrappers for automatic resource cleanup.
 * 
 * @param filename Path to the .wasm file to parse
 * @return Exit code (EXIT_OK or EXIT_RUNTIME_ERROR)
 */
int cmdParse(const std::string& filename) {
    printVerbose(std::string("WasmEdge version: ") + WasmEdge_VersionGet());
    printVerbose(std::string("Processing file: ") + filename);
    
    // Step 1: Create the parser context (RAII managed)
    printVerbose("Creating parser context...");
    ParserPtr parserCtx(WasmEdge_ParserCreate(nullptr));
    if (!parserCtx) {
        printContextError("PARSE", filename, "parser context");
        return EXIT_RUNTIME_ERROR;
    }

    // Step 2: Parse the WebAssembly file
    printVerbose("Parsing WebAssembly module...");
    WasmEdge_ASTModuleContext* rawAstModule = nullptr;
    WasmEdge_Result result = WasmEdge_ParserParseFromFile(parserCtx.get(), &rawAstModule, filename.c_str());
    ASTModulePtr astModuleCtx(rawAstModule);  // Take ownership immediately

    if (!WasmEdge_ResultOK(result)) {
        printWasmEdgeError("PARSE", filename, "FAILED", result);
        return EXIT_RUNTIME_ERROR;
    }

    // Success
    printVerbose("Parse completed successfully.");
    printSuccess("PARSE", filename, "SUCCESS");
    return EXIT_OK;
    // RAII: parserCtx and astModuleCtx automatically cleaned up
}

/**
 * Validate sub-command implementation using WasmEdge C API
 * 
 * Pipeline: Parse -> Validate
 * Demonstrates: Multi-context lifecycle, semantic validation
 * Uses RAII wrappers for automatic resource cleanup.
 * 
 * @param filename Path to the .wasm file to validate
 * @return Exit code (EXIT_OK or EXIT_RUNTIME_ERROR)
 */
int cmdValidate(const std::string& filename) {
    printVerbose(std::string("WasmEdge version: ") + WasmEdge_VersionGet());
    printVerbose(std::string("Processing file: ") + filename);
    
    // Step 1: Create the parser context (RAII managed)
    printVerbose("Creating parser context...");
    ParserPtr parserCtx(WasmEdge_ParserCreate(nullptr));
    if (!parserCtx) {
        printContextError("VALIDATE", filename, "parser context");
        return EXIT_RUNTIME_ERROR;
    }

    // Step 2: Parse the WebAssembly file
    printVerbose("Parsing WebAssembly module...");
    WasmEdge_ASTModuleContext* rawAstModule = nullptr;
    WasmEdge_Result result = WasmEdge_ParserParseFromFile(parserCtx.get(), &rawAstModule, filename.c_str());
    ASTModulePtr astModuleCtx(rawAstModule);  // Take ownership immediately

    if (!WasmEdge_ResultOK(result)) {
        printWasmEdgeError("VALIDATE", filename, "FAILED (Parse Error)", result);
        return EXIT_RUNTIME_ERROR;
    }

    // Step 3: Create the validator context (RAII managed)
    printVerbose("Creating validator context...");
    ValidatorPtr validatorCtx(WasmEdge_ValidatorCreate(nullptr));
    if (!validatorCtx) {
        printContextError("VALIDATE", filename, "validator context");
        return EXIT_RUNTIME_ERROR;
    }

    // Step 4: Validate the AST module
    printVerbose("Validating WebAssembly module...");
    result = WasmEdge_ValidatorValidate(validatorCtx.get(), astModuleCtx.get());

    if (!WasmEdge_ResultOK(result)) {
        printWasmEdgeError("VALIDATE", filename, "INVALID", result);
        return EXIT_RUNTIME_ERROR;
    }

    // Success
    printVerbose("Validation completed successfully.");
    printSuccess("VALIDATE", filename, "VALID");
    return EXIT_OK;
    // RAII: All contexts automatically cleaned up
}

/**
 * Instantiate sub-command implementation using WasmEdge C API
 * 
 * Pipeline: VM Create -> Load -> Validate -> Instantiate
 * Demonstrates: VM lifecycle, streamlined module loading
 * Uses RAII wrappers for automatic resource cleanup.
 * Does not execute any functions - only creates a ready VM instance.
 * 
 * @param filename Path to the .wasm file to instantiate
 * @return Exit code (EXIT_OK or EXIT_RUNTIME_ERROR)
 */
int cmdInstantiate(const std::string& filename) {
    printVerbose(std::string("WasmEdge version: ") + WasmEdge_VersionGet());
    printVerbose(std::string("Processing file: ") + filename);
    
    // Step 1: Create the VM context (RAII managed)
    printVerbose("Creating VM context...");
    VMPtr vmCtx(WasmEdge_VMCreate(nullptr, nullptr));
    if (!vmCtx) {
        printContextError("INSTANTIATE", filename, "VM context");
        return EXIT_RUNTIME_ERROR;
    }

    // Step 2: Load the WebAssembly module from file
    printVerbose("Loading WebAssembly module...");
    WasmEdge_Result result = WasmEdge_VMLoadWasmFromFile(vmCtx.get(), filename.c_str());

    if (!WasmEdge_ResultOK(result)) {
        printWasmEdgeError("INSTANTIATE", filename, "FAILED (Load Error)", result);
        return EXIT_RUNTIME_ERROR;
    }

    // Step 3: Validate the loaded module
    printVerbose("Validating loaded module...");
    result = WasmEdge_VMValidate(vmCtx.get());

    if (!WasmEdge_ResultOK(result)) {
        printWasmEdgeError("INSTANTIATE", filename, "FAILED (Validation Error)", result);
        return EXIT_RUNTIME_ERROR;
    }

    // Step 4: Instantiate the module
    printVerbose("Instantiating module...");
    result = WasmEdge_VMInstantiate(vmCtx.get());

    if (!WasmEdge_ResultOK(result)) {
        printWasmEdgeError("INSTANTIATE", filename, "FAILED (Instantiation Error)", result);
        return EXIT_RUNTIME_ERROR;
    }

    // Success
    printVerbose("Instantiation completed successfully.");
    printSuccess("INSTANTIATE", filename, "READY");
    return EXIT_OK;
    // RAII: vmCtx automatically cleaned up
}

// ============================================================================
// Main Entry Point
// ============================================================================

/**
 * Main entry point - argument parsing and command routing
 * 
 * Exit codes:
 *   EXIT_OK (0)           - Success
 *   EXIT_CLI_ERROR (1)    - Invalid arguments, unknown command, file not found
 *   EXIT_RUNTIME_ERROR (2) - WasmEdge runtime error
 */
int main(int argc, char* argv[]) {
    // Check minimum argument count
    if (argc < 2) {
        printCliError("No command specified.");
        printUsage();
        return EXIT_CLI_ERROR;
    }

    // Parse arguments - support options before command
    int argIndex = 1;
    
    // Process options first
    while (argIndex < argc) {
        std::string_view arg = argv[argIndex];
        
        if (arg == "-h" || arg == "--help") {
            printUsage();
            return EXIT_OK;
        }
        
        if (arg == "-v" || arg == "--version") {
            printVersion();
            return EXIT_OK;
        }
        
        if (arg == "--verbose") {
            g_verbose = true;
            argIndex++;
            continue;
        }
        
        // Not an option, must be command
        break;
    }
    
    // Check if we have a command
    if (argIndex >= argc) {
        printCliError("No command specified.");
        printUsage();
        return EXIT_CLI_ERROR;
    }

    std::string_view command = argv[argIndex];
    argIndex++;

    // Validate known commands
    if (command != "parse" && command != "validate" && command != "instantiate") {
        printCliError(std::string("Unknown command '") + std::string(command) + "'.");
        printUsage();
        return EXIT_CLI_ERROR;
    }

    // Check for file argument
    if (argIndex >= argc) {
        printCliError(std::string("Missing file argument for '") + std::string(command) + "' command.");
        printUsage();
        return EXIT_CLI_ERROR;
    }

    std::string filename = argv[argIndex];
    
    // Validate file before processing
    if (!validateFile(filename)) {
        return EXIT_CLI_ERROR;
    }
    
    printVerbose("File validation passed.");

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
