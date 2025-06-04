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

#ifndef PARACONFIG_H
#define PARACONFIG_H

#include "nlohmann/json.hpp"
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <variant>
#include <stdexcept> // For std::runtime_error
#include <iostream>
#include <fstream>   // For file operations
#include <filesystem> // For std::filesystem::path

// Forward declaration for nlohmann::json, if not fully included by "nlohmann/json.hpp"
// namespace nlohmann { template<typename T, typename S, typename ...Args> class basic_json; using json = basic_json<std::map, std::vector, std::string, bool, std::int64_t, std::uint64_t, double, std::allocator, adl_serializer>; }

// Forward declarations
class ModulePreCGNS;
class ModulePrePlot3D;
class ModulePreTecplot;
class ModuleSA;
class ModuleSST;
class ModuleSSTWDF;
class ModulePostCGNS;
class ModulePostPlot3D;
class ModulePostTecplot;

class EnginePreGrid;
class EngineTurbulence;
class EngineFlowField;
class EnginePre;
class EngineSolve;
class EnginePost;
class EngineMainProcess;

// --- Helper Function Declarations ---

/**
 * @brief Writes a nlohmann::json object to a file.
 * @param dir_path The directory path where the file will be created.
 * @param filename The name of the file.
 * @param j_obj The nlohmann::json object to write.
 */
void WriteJsonFile(const std::string& dir_path, const std::string& filename, const nlohmann::json& j_obj);

/**
 * @brief Generates default configuration content from a JSON schema.
 * This function extracts direct default values from the schema, excluding nested
 * 'module_parameters_schemas' or 'sub_engine_parameters_schemas'.
 * @param schema The JSON schema from which to extract defaults.
 * @return nlohmann::json object containing the default values.
 */
nlohmann::json GenerateDefaultConfigContentFromSchema(const nlohmann::json& schema);

/**
 * @brief Generates a registration file (register.json) containing schemas for all modules and engines.
 * This file can be used by a GUI or validation tool to understand available components and their parameters.
 * @param dir_path The directory path where register.json will be created.
 */
void GenerateRegistrationFile(const std::string& dir_path);

/**
 * @brief Writes default configuration files, including register.json and a combined config.json.
 * The config.json will contain default parameters for all registered components.
 * @param dir_path The directory path where configuration files will be created.
 */
void WriteDefaultConfigs(const std::string& dir_path);

/**
 * @brief Loads a global configuration from a JSON file.
 * @param file_path The path to the configuration file.
 * @param global_config Output parameter; the loaded nlohmann::json object.
 * @throw std::runtime_error if the file cannot be found, opened, or parsed.
 */
void LoadGlobalConfig(const std::filesystem::path& file_path, nlohmann::json& global_config);

// --- Module Definitions ---

/**
 * @brief Module for CGNS file preprocessing.
 */
class ModulePreCGNS {
public:
    /**
     * @brief Constructs a ModulePreCGNS object.
     * @param params JSON object containing parameters for initialization.
     * @throw std::runtime_error If configuration parameters are missing or invalid.
     */
    ModulePreCGNS(const nlohmann::json& params);
    /** @brief Initializes the module. */
    void Initialize();
    /** @brief Executes the module's main logic. */
    void Execute();
    /** @brief Releases any resources held by the module. */
    void Release();
    /**
     * @brief Gets the JSON schema for this module's parameters.
     * @return nlohmann::json object representing the schema.
     */
    static nlohmann::json GetParamSchema();
protected:
    std::string cgns_type_;
    double cgns_value_;
};

/**
 * @brief Module for Plot3D file preprocessing.
 */
class ModulePrePlot3D {
public:
    /**
     * @brief Constructs a ModulePrePlot3D object.
     * @param params JSON object containing parameters for initialization.
     * @throw std::runtime_error If configuration parameters are missing or invalid.
     */
    ModulePrePlot3D(const nlohmann::json& params);
    /** @brief Initializes the module. */
    void Initialize();
    /** @brief Executes the module's main logic. */
    void Execute();
    /** @brief Releases any resources held by the module. */
    void Release();
    /**
     * @brief Gets the JSON schema for this module's parameters.
     * @return nlohmann::json object representing the schema.
     */
    static nlohmann::json GetParamSchema();
protected:
    std::string plot3d_option_;
};

/**
 * @brief Module for Tecplot file preprocessing.
 */
class ModulePreTecplot {
public:
    /**
     * @brief Constructs a ModulePreTecplot object.
     * @param params JSON object containing parameters for initialization.
     * @throw std::runtime_error If configuration parameters are missing or invalid.
     */
    ModulePreTecplot(const nlohmann::json& params);
    /** @brief Initializes the module. */
    void Initialize();
    /** @brief Executes the module's main logic. */
    void Execute();
    /** @brief Releases any resources held by the module. */
    void Release();
    /**
     * @brief Gets the JSON schema for this module's parameters.
     * @return nlohmann::json object representing the schema.
     */
    static nlohmann::json GetParamSchema();
protected:
    bool tecplot_binary_format_;
};

/**
 * @brief Module for the Spalart-Allmaras (SA) turbulence model.
 */
class ModuleSA {
public:
    /**
     * @brief Constructs a ModuleSA object.
     * @param params JSON object containing parameters for initialization.
     * @throw std::runtime_error If configuration parameters are invalid.
     */
    ModuleSA(const nlohmann::json& params);
    /** @brief Initializes the module. */
    void Initialize();
    /** @brief Executes the module's main logic. */
    void Execute();
    /** @brief Releases any resources held by the module. */
    void Release();
    /**
     * @brief Gets the JSON schema for this module's parameters.
     * @return nlohmann::json object representing the schema.
     */
    static nlohmann::json GetParamSchema(); 
protected:
    double sa_constant_;
    const double kSaMaxValue_ = 0.41; 
    const double kSaMinValue_ = 0.01; 
};

/**
 * @brief Module for the Shear Stress Transport (SST) k-omega turbulence model.
 */
class ModuleSST {
public:
    /**
     * @brief Constructs a ModuleSST object.
     * @param params JSON object containing parameters for initialization.
     * @throw std::runtime_error If configuration parameters are invalid.
     */
    ModuleSST(const nlohmann::json& params);
    /** @brief Initializes the module. */
    void Initialize();
    /** @brief Executes the module's main logic. */
    void Execute();
    /** @brief Releases any resources held by the module. */
    void Release();
    /**
     * @brief Gets the JSON schema for this module's parameters.
     * @return nlohmann::json object representing the schema.
     */
    static nlohmann::json GetParamSchema();
protected:
    int sst_iterations_;
};

/**
 * @brief Module for the SST model with Wall Damping Function (WDF).
 */
class ModuleSSTWDF {
public:
    /**
     * @brief Constructs a ModuleSSTWDF object.
     * @param params JSON object containing parameters for initialization.
     * @throw std::runtime_error If configuration parameters are invalid.
     */
    ModuleSSTWDF(const nlohmann::json& params);
    /** @brief Initializes the module. */
    void Initialize();
    /** @brief Executes the module's main logic. */
    void Execute();
    /** @brief Releases any resources held by the module. */
    void Release();
    /**
     * @brief Gets the JSON schema for this module's parameters.
     * @return nlohmann::json object representing the schema.
     */
    static nlohmann::json GetParamSchema();
protected:
    std::string wdf_model_name_;
};

/**
 * @brief Module for CGNS file postprocessing.
 */
class ModulePostCGNS {
public:
    /**
     * @brief Constructs a ModulePostCGNS object.
     * @param params JSON object containing parameters for initialization.
     * @throw std::runtime_error If configuration parameters are invalid.
     */
    ModulePostCGNS(const nlohmann::json& params);
    /** @brief Initializes the module. */
    void Initialize();
    /** @brief Executes the module's main logic. */
    void Execute();
    /** @brief Releases any resources held by the module. */
    void Release();
    /**
     * @brief Gets the JSON schema for this module's parameters.
     * @return nlohmann::json object representing the schema.
     */
    static nlohmann::json GetParamSchema();
protected:
    std::string output_cgns_name_;
};

/**
 * @brief Module for Plot3D file postprocessing.
 */
class ModulePostPlot3D {
public:
    /**
     * @brief Constructs a ModulePostPlot3D object.
     * @param params JSON object containing parameters for initialization.
     * @throw std::runtime_error If configuration parameters are invalid.
     */
    ModulePostPlot3D(const nlohmann::json& params);
    /** @brief Initializes the module. */
    void Initialize();
    /** @brief Executes the module's main logic. */
    void Execute();
    /** @brief Releases any resources held by the module. */
    void Release();
    /**
     * @brief Gets the JSON schema for this module's parameters.
     * @return nlohmann::json object representing the schema.
     */
    static nlohmann::json GetParamSchema();
protected:
    bool write_q_file_;
};

/**
 * @brief Module for Tecplot file postprocessing.
 */
class ModulePostTecplot {
public:
    /**
     * @brief Constructs a ModulePostTecplot object.
     * @param params JSON object containing parameters for initialization.
     * @throw std::runtime_error If configuration parameters are invalid.
     */
    ModulePostTecplot(const nlohmann::json& params);
    /** @brief Initializes the module. */
    void Initialize();
    /** @brief Executes the module's main logic. */
    void Execute();
    /** @brief Releases any resources held by the module. */
    void Release();
    /**
     * @brief Gets the JSON schema for this module's parameters.
     * @return nlohmann::json object representing the schema.
     */
    static nlohmann::json GetParamSchema();
protected:
    std::string tecplot_zone_title_;
};


// --- Engine Definitions ---

/**
 * @brief Engine for grid preprocessing tasks.
 * Manages and executes a sequence of preprocessing modules.
 */
class EnginePreGrid {
public:
    /**
     * @brief Constructs an EnginePreGrid object.
     * @param instance_params JSON object containing parameters specific to this engine instance.
     * @param global_config JSON object containing the global configuration, including module parameters.
     * @throw std::runtime_error If configuration is missing or invalid.
     */
    EnginePreGrid(const nlohmann::json& instance_params, const nlohmann::json& global_config);
    /** @brief Initializes the engine and its active modules. */
    void Initialize();
    /** @brief Executes the engine's logic, typically by running its modules. */
    void Execute();
    /** @brief Releases resources held by the engine and its modules. */
    void Release();
    /**
     * @brief Gets the JSON schema for this engine's parameters.
     * @return nlohmann::json object representing the schema.
     */
    static nlohmann::json GetParamSchema();

protected:
    std::vector<std::string> module_execution_order_;
    
    std::map<std::string, std::variant<
        std::unique_ptr<ModulePreCGNS>,
        std::unique_ptr<ModulePrePlot3D>,
        std::unique_ptr<ModulePreTecplot>
    >> active_modules_;

    const std::map<std::string, int> module_key_map_ = {
        {"ModulePreCGNS", 0},
        {"ModulePrePlot3D", 1},
        {"ModulePreTecplot", 2}
    };
};

/**
 * @brief Engine for turbulence modeling tasks.
 * Manages and executes a sequence of turbulence model modules.
 */
class EngineTurbulence {
public:
    /**
     * @brief Constructs an EngineTurbulence object.
     * @param instance_params JSON object containing parameters specific to this engine instance.
     * @param global_config JSON object containing the global configuration, including module parameters.
     * @throw std::runtime_error If configuration is missing or invalid.
     */
    EngineTurbulence(const nlohmann::json& instance_params, const nlohmann::json& global_config);
    /** @brief Initializes the engine and its active modules. */
    void Initialize();
    /** @brief Executes the engine's logic, typically by running its modules. */
    void Execute();
    /** @brief Releases resources held by the engine and its modules. */
    void Release();
    /**
     * @brief Gets the JSON schema for this engine's parameters.
     * @return nlohmann::json object representing the schema.
     */
    static nlohmann::json GetParamSchema();
protected:
    std::vector<std::string> module_execution_order_;
    std::map<std::string, std::variant<
        std::unique_ptr<ModuleSA>,
        std::unique_ptr<ModuleSST>,
        std::unique_ptr<ModuleSSTWDF>
    >> active_modules_;
    const std::map<std::string, int> module_key_map_ = {
        {"ModuleSA", 0},
        {"ModuleSST", 1},
        {"ModuleSSTWDF", 2}
    };
};

/**
 * @brief Engine for flow field postprocessing tasks.
 * Manages and executes a sequence of postprocessing modules.
 */
class EngineFlowField {
public:
    /**
     * @brief Constructs an EngineFlowField object.
     * @param instance_params JSON object containing parameters specific to this engine instance.
     * @param global_config JSON object containing the global configuration, including module parameters.
     * @throw std::runtime_error If configuration is missing or invalid.
     */
    EngineFlowField(const nlohmann::json& instance_params, const nlohmann::json& global_config);
    /** @brief Initializes the engine and its active modules. */
    void Initialize();
    /** @brief Executes the engine's logic, typically by running its modules. */
    void Execute();
    /** @brief Releases resources held by the engine and its modules. */
    void Release();
    /**
     * @brief Gets the JSON schema for this engine's parameters.
     * @return nlohmann::json object representing the schema.
     */
    static nlohmann::json GetParamSchema();
protected:
    std::vector<std::string> module_execution_order_;
    std::map<std::string, std::variant<
        std::unique_ptr<ModulePostCGNS>,
        std::unique_ptr<ModulePostPlot3D>,
        std::unique_ptr<ModulePostTecplot>
    >> active_modules_;
    const std::map<std::string, int> module_key_map_ = {
        {"ModulePostCGNS", 0},
        {"ModulePostPlot3D", 1},
        {"ModulePostTecplot", 2}
    };
};

/**
 * @brief Engine for pre-processing stage.
 * Manages and executes preprocessing sub-engines.
 */
class EnginePre {
public:
    /**
     * @brief Constructs an EnginePre object.
     * @param instance_params JSON object containing parameters specific to this engine instance.
     * @param global_config JSON object containing the global configuration, including sub-engine parameters.
     * @throw std::runtime_error If configuration is missing or invalid.
     */
    EnginePre(const nlohmann::json& instance_params, const nlohmann::json& global_config);
    /** @brief Initializes the engine and its active sub-engines. */
    void Initialize();
    /** @brief Executes the engine's logic by running its sub-engines. */
    void Execute();
    /** @brief Releases resources held by the engine and its sub-engines. */
    void Release();
    /**
     * @brief Gets the JSON schema for this engine's parameters.
     * @return nlohmann::json object representing the schema.
     */
    static nlohmann::json GetParamSchema();
protected:
    std::vector<std::string> execution_order_;
    std::map<std::string, std::variant<
        std::unique_ptr<EnginePreGrid>
    >> active_sub_engines_;

    const std::map<std::string, int> engine_key_map_ = {
        {"EnginePreGrid", 0}
    };
};

/**
 * @brief Engine for solving stage.
 * Manages and executes solver sub-engines.
 */
class EngineSolve {
public:
    /**
     * @brief Constructs an EngineSolve object.
     * @param instance_params JSON object containing parameters specific to this engine instance.
     * @param global_config JSON object containing the global configuration, including sub-engine parameters.
     * @throw std::runtime_error If configuration is missing or invalid.
     */
    EngineSolve(const nlohmann::json& instance_params, const nlohmann::json& global_config);
    /** @brief Initializes the engine and its active sub-engines. */
    void Initialize();
    /** @brief Executes the engine's logic by running its sub-engines. */
    void Execute();
    /** @brief Releases resources held by the engine and its sub-engines. */
    void Release();
    /**
     * @brief Gets the JSON schema for this engine's parameters.
     * @return nlohmann::json object representing the schema.
     */
    static nlohmann::json GetParamSchema();
protected:
    std::vector<std::string> execution_order_;
    std::map<std::string, std::variant<
        std::unique_ptr<EngineTurbulence>
    >> active_sub_engines_;

    const std::map<std::string, int> engine_key_map_ = {
        {"EngineTurbulence", 0}
    };
};

/**
 * @brief Engine for post-processing stage.
 * Manages and executes post-processing sub-engines.
 */
class EnginePost {
public:
    /**
     * @brief Constructs an EnginePost object.
     * @param instance_params JSON object containing parameters specific to this engine instance.
     * @param global_config JSON object containing the global configuration, including sub-engine parameters.
     * @throw std::runtime_error If configuration is missing or invalid.
     */
    EnginePost(const nlohmann::json& instance_params, const nlohmann::json& global_config);
    /** @brief Initializes the engine and its active sub-engines. */
    void Initialize();
    /** @brief Executes the engine's logic by running its sub-engines. */
    void Execute();
    /** @brief Releases resources held by the engine and its sub-engines. */
    void Release();
    /**
     * @brief Gets the JSON schema for this engine's parameters.
     * @return nlohmann::json object representing the schema.
     */
    static nlohmann::json GetParamSchema();
protected:
    std::vector<std::string> execution_order_;
    std::map<std::string, std::variant<
        std::unique_ptr<EngineFlowField>
    >> active_sub_engines_;

    const std::map<std::string, int> engine_key_map_ = {
        {"EngineFlowField", 0}
    };
};

/**
 * @brief Main process engine orchestrating the overall workflow.
 * Manages the sequence of major processing stages (Pre, Solve, Post).
 */
class EngineMainProcess {
public:
    /**
     * @brief Constructs an EngineMainProcess object.
     * @param instance_params JSON object containing parameters specific to this engine instance (e.g., execution order of stages).
     * @param global_config JSON object containing the global configuration for all components.
     * @throw std::runtime_error If configuration is missing or invalid.
     */
    EngineMainProcess(const nlohmann::json& instance_params, const nlohmann::json& global_config);
    /** @brief Initializes the main process and all configured top-level engines. */
    void Initialize();
    /** @brief Executes the main process by running the top-level engines in order. */
    void Execute();
    /** @brief Releases resources held by the main process and its engines. */
    void Release();
    /**
     * @brief Gets the JSON schema for the main process engine's parameters.
     * @return nlohmann::json object representing the schema.
     */
    static nlohmann::json GetParamSchema();
protected:
    std::vector<std::string> execution_order_;

    std::map<std::string, std::variant<
        std::unique_ptr<EnginePre>,
        std::unique_ptr<EngineSolve>,
        std::unique_ptr<EnginePost>
    >> active_engines_;

    const std::map<std::string, int> engine_key_map_ = {
        {"EnginePre", 0},
        {"EngineSolve", 1},
        {"EnginePost", 2}
    };
};


#endif // PARACONFIG_H
