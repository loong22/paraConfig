/********************************************************************
MIT License

Copyright (c) 2025 loong22

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*********************************************************************/

#include "paraConfig.h" // Provides declarations for helper functions and core classes
#include <iostream>
#include <string>
#include <vector>
#include <filesystem> // For path manipulation, C++17

/**
 * @brief Main entry point for the application.
 * Parses command-line arguments to either write default configuration files
 * or run the simulation based on a provided configuration file.
 *
 * Command-line options:
 * --write-config <directory_path> : Generates default config.json and register.json
 *                                    in the specified directory.
 * --config <config_file_path>     : Runs the simulation using the specified
 *                                    JSON configuration file.
 *
 * @param argc Number of command-line arguments.
 * @param argv Array of command-line arguments.
 * @return 0 on successful execution, 1 on error.
 */
int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << std::endl;
        std::cerr << "  " << argv[0] << " --write-config <directory_path>" << std::endl;
        std::cerr << "  " << argv[0] << " --config <config_file_path>" << std::endl; 
        return 1;
    }

    std::string option = argv[1];

    try {
        if (option == "--write-config") {
            if (argc < 3) {
                std::cerr << "Error: Missing directory path for --write-config." << std::endl;
                return 1;
            }
            std::string dir_path = argv[2];
            WriteDefaultConfigs(dir_path); 
            std::cout << "Default configuration files generated in: " << dir_path << std::endl;

        } else if (option == "--config") {
            if (argc < 3) {
                std::cerr << "Error: Missing config file path for --config." << std::endl; 
                return 1;
            }
            std::filesystem::path config_file_path = argv[2];
            
            nlohmann::json global_config_json;
            LoadGlobalConfig(config_file_path, global_config_json);

            std::cout << "Global configuration loaded from: " << config_file_path.string() << std::endl;
            // std::cout << "Loaded config content:\n" << global_config_json.dump(4) << std::endl; // For debugging

            if (!global_config_json.contains("EngineMainProcess")) {
                throw std::runtime_error("Entry point 'EngineMainProcess' not found in the global configuration file.");
            }

            // The parameters for EngineMainProcess itself are under its key in the global config.
            EngineMainProcess main_process(global_config_json.at("EngineMainProcess"), global_config_json);
            
            main_process.Initialize();
            main_process.Execute();
            main_process.Release();

            std::cout << "Processing finished." << std::endl;

        } else {
            std::cerr << "Error: Unknown option: " << option << std::endl;
            std::cerr << "Run with no arguments for usage information." << std::endl;
            return 1;
        }
    } catch (const std::exception& e) {
        std::cerr << "An error occurred: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
