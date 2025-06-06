#include "paraConfig.h" // Provides auxiliary functions and core class declarations
#include <iostream>
#include <string>
#include <vector>
#include <filesystem> // For path operations, C++17

#ifdef _WIN32
#include <windows.h>
#include <io.h>
#include <fcntl.h>
#endif

/**
 * @brief Main entry point of the application.
 * Parses command line arguments to write default configuration files
 * or run simulation based on provided configuration file.
 *
 * Command line options:
 * --write-config <directory_path> : Generate default config.json and register.json in specified directory
 * --config <config_file_path>     : Run simulation using specified JSON configuration file
 *
 * @param argc Number of command line arguments.
 * @param argv Array of command line arguments.
 * @return Returns 0 on success, 1 on error.
 */
int main(int argc, char* argv[]) {

#ifdef _WIN32
    // Set console code page to UTF-8
    SetConsoleOutputCP(CP_UTF8);
    _setmode(_fileno(stdout), _O_BINARY);
    std::cout.setf(std::ios::unitbuf);
    std::cerr.setf(std::ios::unitbuf);
    setvbuf(stdout, nullptr, _IONBF, 0);
    setvbuf(stderr, nullptr, _IONBF, 0);
#endif

    if (argc < 2) {
        std::cerr << "Usage:" << std::endl;
        std::cerr << "  " << argv[0] << " --write-config <directory_path>" << std::endl;
        std::cerr << "  " << argv[0] << " --config <config_file_path>" << std::endl; 
        return 1;
    }

    std::string option = argv[1];

    try {
        if (option == "--write-config") {
            if (argc < 3) {
                std::cerr << "Error: --write-config missing directory path." << std::endl;
                return 1;
            }
            std::string dir_path = argv[2];
            WriteDefaultConfigs(dir_path); 
            std::cout << "Default configuration files generated at: " << dir_path << std::endl;
        } 
        else if (option == "--config") {
            if (argc < 3) {
                std::cerr << "Error: --config missing configuration file path." << std::endl; 
                return 1;
            }
            std::string configFile = argv[2];
            
            nlohmann::json config;

            // The default read path is the ./config folder under the program execution directory.
            LoadConfig(configFile, config);

            // Parameters for EngineMainProcess itself are under its key in the global configuration.
            EngineMainProcess main_process(config);
            
            main_process.Initialize();

            main_process.Execute();

            main_process.Release();

            std::cout << "Processing completed." << std::endl;
        } 
        else {
            std::cerr << "Error: Unknown option: " << option << std::endl;
            std::cerr << "Run without arguments to get usage information." << std::endl;
            return 1;
        }
    } catch (const std::exception& e) {
        std::cerr << "An error occurred: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}