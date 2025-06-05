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

// Forward declaration of nlohmann::json, if "nlohmann/json.hpp" is not fully included
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

// --- Auxiliary function declarations ---

class EnginePost;
class EngineMainProcess;

//If adding modules or engines, change here
using EngineMainProcessVariant = std::variant<
        std::unique_ptr<EnginePre>,
        std::unique_ptr<EngineSolve>,
        std::unique_ptr<EnginePost>>;

// First, need to define corresponding variant types for modules
using EnginePreGridVariant = std::variant<
    std::unique_ptr<ModulePreCGNS>,
    std::unique_ptr<ModulePrePlot3D>,
    std::unique_ptr<ModulePreTecplot>>;

using EngineTurbulenceVariant = std::variant<
    std::unique_ptr<ModuleSA>,
    std::unique_ptr<ModuleSST>,
    std::unique_ptr<ModuleSSTWDF>>;

using EngineFlowFieldVariant = std::variant<
    std::unique_ptr<ModulePostCGNS>,
    std::unique_ptr<ModulePostPlot3D>,
    std::unique_ptr<ModulePostTecplot>>;

using EnginePreVariant = std::variant<
    std::unique_ptr<EnginePreGrid>>;

using EngineSolveVariant = std::variant<
    std::unique_ptr<EngineTurbulence>>;

using EnginePostVariant = std::variant<
    std::unique_ptr<EngineFlowField>>;

// --- Auxiliary function declarations ---
/*
 * @param j_obj The nlohmann::json object to write.
 */
void WriteJsonFile(const std::string& dir_path, const std::string& filename, const nlohmann::json& j_obj);

/**
 * @brief Extracts the base name from an instance name.
 * For example, "ModuleClass_1" becomes "ModuleClass". This is used to map
 * an instance name (e.g., "ModulePreCGNS_custom") to its base type ("ModulePreCGNS")
 * for schema lookup or factory instantiation.
 * @param instance_name The full instance name, potentially with a suffix like "_1" or "_custom".
 * @return The base name of the component.
 */
static std::string GetBaseName(const std::string& instance_name);

/**
 * @brief Generate default configuration content from JSON schema.
 * This function extracts direct default values from the schema, excluding nested
 * 'module_parameters_schemas' or 'sub_engine_parameters_schemas'.
 * @param schema The JSON schema to extract default values from.
 * @return nlohmann::json object containing default values.
 */
nlohmann::json GenerateDefaultConfigContentFromSchema(const nlohmann::json& schema);

/**
 * @brief Generate registration file (register.json) containing all module and engine schemas.
 * This file can be used by GUI or validation tools to understand available components and their parameters.
 * @param dir_path Directory path where register.json will be created.
 */
void GenerateRegistrationFile(const std::string& dir_path);

/**
 * @brief Write default configuration files including register.json and combined config.json.
 * config.json will contain default parameters for all registered components.
 * @param dir_path Directory path where configuration files will be created.
 */
void WriteDefaultConfigs(const std::string& dir_path);

/**
 * @brief Load global configuration from JSON file.
 * @param configFile Configuration file name to load (with or without .json extension).
 * @param config Reference to nlohmann::json object that will store the loaded configuration.
 * @throw std::runtime_error If file cannot be found, opened, or parsed.
 */
void LoadConfig(const std::string& configFile,nlohmann::json& config);

// --- Module definitions ---
/**
 * @brief CGNS file preprocessing module.
 */
class ModulePreCGNS {
public:
    /**
     * @brief Construct ModulePreCGNS object.
     * @param params JSON object containing initialization parameters.
     * @throw std::runtime_error If configuration parameters are missing or invalid.
     */
    ModulePreCGNS(const nlohmann::json& params);
    /** @brief Initialize the module. */
    void Initialize();
    /** @brief Execute the module's main logic. */
    void Execute();
    /** @brief Release any resources held by the module. */
    void Release();
    /**
     * @brief Get JSON schema for this module's parameters.
     * @return nlohmann::json object representing the schema.
     */
    static nlohmann::json GetParamSchema();
protected:
    std::string cgns_type_;
    double cgns_value_;
};

/**
 * @brief Plot3D file preprocessing module.
 */
class ModulePrePlot3D {
public:
    /**
     * @brief Construct ModulePrePlot3D object.
     * @param params JSON object containing initialization parameters.
     * @throw std::runtime_error If configuration parameters are missing or invalid.
     */
    ModulePrePlot3D(const nlohmann::json& params);
    /** @brief Initialize the module. */
    void Initialize();
    /** @brief Execute the module's main logic. */
    void Execute();
    /** @brief Release any resources held by the module. */
    void Release();
    /**
     * @brief Get JSON schema for this module's parameters.
     * @return nlohmann::json object representing the schema.
     */
    static nlohmann::json GetParamSchema();
protected:
    std::string plot3d_option_;
};

/**
 * @brief Tecplot file preprocessing module.
 */
class ModulePreTecplot {
public:
    /**
     * @brief Construct ModulePreTecplot object.
     * @param params JSON object containing initialization parameters.
     * @throw std::runtime_error If configuration parameters are missing or invalid.
     */
    ModulePreTecplot(const nlohmann::json& params);
    /** @brief Initialize the module. */
    void Initialize();
    /** @brief Execute the module's main logic. */
    void Execute();
    /** @brief Release any resources held by the module. */
    void Release();
    /**
     * @brief Get JSON schema for this module's parameters.
     * @return nlohmann::json object representing the schema.
     */
    static nlohmann::json GetParamSchema();
protected:
    bool tecplot_binary_format_;
};

/**
 * @brief Spalart-Allmaras (SA) turbulence model module.
 */
class ModuleSA {
public:
    /**
     * @brief Construct ModuleSA object.
     * @param params JSON object containing initialization parameters.
     * @throw std::runtime_error If configuration parameters are invalid.
     */
    ModuleSA(const nlohmann::json& params);
    /** @brief Initialize the module. */
    void Initialize();
    /** @brief Execute the module's main logic. */
    void Execute();
    /** @brief Release any resources held by the module. */
    void Release();
    /**
     * @brief Get JSON schema for this module's parameters.
     * @return nlohmann::json object representing the schema.
     */
    static nlohmann::json GetParamSchema(); 
protected:
    double sa_constant_;
    const double kSaMaxValue_ = 0.41; 
    const double kSaMinValue_ = 0.01; 
};

/**
 * @brief Shear Stress Transport (SST) k-omega turbulence model module.
 */
class ModuleSST {
public:
    /**
     * @brief Construct ModuleSST object.
     * @param params JSON object containing initialization parameters.
     * @throw std::runtime_error If configuration parameters are invalid.
     */
    ModuleSST(const nlohmann::json& params);
    /** @brief Initialize the module. */
    void Initialize();
    /** @brief Execute the module's main logic. */
    void Execute();
    /** @brief Release any resources held by the module. */
    void Release();
    /**
     * @brief Get JSON schema for this module's parameters.
     * @return nlohmann::json object representing the schema.
     */
    static nlohmann::json GetParamSchema();
protected:
    int sst_iterations_;
};

/**
 * @brief SST model with Wall Damping Function (WDF) module.
 */
class ModuleSSTWDF {
public:
    /**
     * @brief Construct ModuleSSTWDF object.
     * @param params JSON object containing initialization parameters.
     * @throw std::runtime_error If configuration parameters are invalid.
     */
    ModuleSSTWDF(const nlohmann::json& params);
    /** @brief Initialize the module. */
    void Initialize();
    /** @brief Execute the module's main logic. */
    void Execute();
    /** @brief Release any resources held by the module. */
    void Release();
    /**
     * @brief Get JSON schema for this module's parameters.
     * @return nlohmann::json object representing the schema.
     */
    static nlohmann::json GetParamSchema();
protected:
    std::string wdf_model_name_;
};

/**
 * @brief CGNS file postprocessing module.
 */
class ModulePostCGNS {
public:
    /**
     * @brief Construct ModulePostCGNS object.
     * @param params JSON object containing initialization parameters.
     * @throw std::runtime_error If configuration parameters are invalid.
     */
    ModulePostCGNS(const nlohmann::json& params);
    /** @brief Initialize the module. */
    void Initialize();
    /** @brief Execute the module's main logic. */
    void Execute();
    /** @brief Release any resources held by the module. */
    void Release();
    /**
     * @brief Get JSON schema for this module's parameters.
     * @return nlohmann::json object representing the schema.
     */
    static nlohmann::json GetParamSchema();
protected:
    std::string output_cgns_name_;
};

/**
 * @brief Plot3D file postprocessing module.
 */
class ModulePostPlot3D {
public:
    /**
     * @brief Construct ModulePostPlot3D object.
     * @param params JSON object containing initialization parameters.
     * @throw std::runtime_error If configuration parameters are invalid.
     */
    ModulePostPlot3D(const nlohmann::json& params);
    /** @brief Initialize the module. */
    void Initialize();
    /** @brief Execute the module's main logic. */
    void Execute();
    /** @brief Release any resources held by the module. */
    void Release();
    /**
     * @brief Get JSON schema for this module's parameters.
     * @return nlohmann::json object representing the schema.
     */
    static nlohmann::json GetParamSchema();
protected:
    bool write_q_file_;
};

/**
 * @brief Tecplot file postprocessing module.
 */
class ModulePostTecplot {
public:
    /**
     * @brief Construct ModulePostTecplot object.
     * @param params JSON object containing initialization parameters.
     * @throw std::runtime_error If configuration parameters are invalid.
     */
    ModulePostTecplot(const nlohmann::json& params);
    /** @brief Initialize the module. */
    void Initialize();
    /** @brief Execute the module's main logic. */
    void Execute();
    /** @brief Release any resources held by the module. */
    void Release();
    /**
     * @brief Get JSON schema for this module's parameters.
     * @return nlohmann::json object representing the schema.
     */
    static nlohmann::json GetParamSchema();
protected:
    std::string tecplot_zone_title_;
};

// --- Engine definitions ---

/**
 * @brief Grid preprocessing task engine.
 * Manages and executes a series of preprocessing modules.
 */
class EnginePreGrid {
public:
    /**
     * @brief Construct EnginePreGrid object.
     * @param instance_params JSON object containing instance-specific parameters for this engine.
     * @throw std::runtime_error If configuration is missing or invalid.
     */
    EnginePreGrid(const nlohmann::json& instance_params);
    /** @brief Initialize the engine and its active modules. */
    void Initialize();
    /** @brief Execute the engine's logic, typically by running its modules. */
    void Execute();
    /** @brief Release resources held by the engine and its modules. */
    void Release();
    /**
     * @brief Get JSON schema for this engine's parameters.
     * @return nlohmann::json object representing the schema.
     */
    static nlohmann::json GetParamSchema();

protected:
    const std::string modulePreCGNS = "ModulePreCGNS";
    const std::string modulePrePlot3D = "ModulePrePlot3D";
    const std::string modulePreTecplot = "ModulePreTecplot";
    
    std::map<std::string, EnginePreGridVariant> subModulesPool;

    std::vector<std::string> execution_order_;
};

/**
 * @brief Turbulence modeling task engine.
 * Manages and executes a series of turbulence model modules.
 */
class EngineTurbulence {
public:
    /**
     * @brief Construct EngineTurbulence object.
     * @param instance_params JSON object containing instance-specific parameters for this engine.
     * @throw std::runtime_error If configuration is missing or invalid.
     */
    EngineTurbulence(const nlohmann::json& instance_params);
    /** @brief Initialize the engine and its active modules. */
    void Initialize();
    /** @brief Execute the engine's logic, typically by running its modules. */
    void Execute();
    /** @brief Release resources held by the engine and its modules. */
    void Release();
    /**
     * @brief Get JSON schema for this engine's parameters.
     * @return nlohmann::json object representing the schema.
     */
    static nlohmann::json GetParamSchema();
protected:
    const std::string moduleSA = "ModuleSA";
    const std::string moduleSST = "ModuleSST";
    const std::string moduleSSTWDF = "ModuleSSTWDF";
    
    std::map<std::string, EngineTurbulenceVariant> subModulesPool;

    std::vector<std::string> execution_order_;
};

/**
 * @brief Flow field postprocessing task engine.
 * Manages and executes a series of postprocessing modules.
 */
class EngineFlowField {
public:
    /**
     * @brief Construct EngineFlowField object.
     * @param instance_params JSON object containing instance-specific parameters for this engine.
     * @throw std::runtime_error If configuration is missing or invalid.
     */
    EngineFlowField(const nlohmann::json& instance_params);
    /** @brief Initialize the engine and its active modules. */
    void Initialize();
    /** @brief Execute the engine's logic, typically by running its modules. */
    void Execute();
    /** @brief Release resources held by the engine and its modules. */
    void Release();
    /**
     * @brief Get JSON schema for this engine's parameters.
     * @return nlohmann::json object representing the schema.
     */
    static nlohmann::json GetParamSchema();
protected:
    const std::string modulePostCGNS = "ModulePostCGNS";
    const std::string modulePostPlot3D = "ModulePostPlot3D";
    const std::string modulePostTecplot = "ModulePostTecplot";
    
    std::map<std::string, EngineFlowFieldVariant> subModulesPool;

    std::vector<std::string> execution_order_;
};

/**
 * @brief Preprocessing stage engine.
 * Manages and executes preprocessing sub-engines.
 */
class EnginePre {
public:
    /**
     * @brief Construct EnginePre object.
     * @param instance_params JSON object containing instance-specific parameters for this engine.
     * @throw std::runtime_error If configuration is missing or invalid.
     */
    EnginePre(const nlohmann::json& instance_params);
    /** @brief Initialize the engine and its active sub-engines. */
    void Initialize();
    /** @brief Execute the engine's logic by running its sub-engines. */
    void Execute();
    /** @brief Release resources held by the engine and its sub-engines. */
    void Release();
    /**
     * @brief Get JSON schema for this engine's parameters.
     * @return nlohmann::json object representing the schema.
     */
    static nlohmann::json GetParamSchema();
protected:
    const std::string enginePreGrid = "EnginePreGrid";
    
    std::map<std::string, EnginePreVariant> subEnginesPool;

    std::vector<std::string> execution_order_;
};

/**
 * @brief Solver stage engine.
 * Manages and executes solver sub-engines.
 */
class EngineSolve {
public:
    /**
     * @brief Construct EngineSolve object.
     * @param instance_params JSON object containing instance-specific parameters for this engine.
     * @throw std::runtime_error If configuration is missing or invalid.
     */
    EngineSolve(const nlohmann::json& instance_params);
    /** @brief Initialize the engine and its active sub-engines. */
    void Initialize();
    /** @brief Execute the engine's logic by running its sub-engines. */
    void Execute();
    /** @brief Release resources held by the engine and its sub-engines. */
    void Release();
    /**
     * @brief Get JSON schema for this engine's parameters.
     * @return nlohmann::json object representing the schema.
     */
    static nlohmann::json GetParamSchema();
protected:
    const std::string engineTurbulence = "EngineTurbulence";
    
    std::map<std::string, EngineSolveVariant> subEnginesPool;

    std::vector<std::string> execution_order_;
};

/**
 * @brief Postprocessing stage engine.
 * Manages and executes postprocessing sub-engines.
 */
class EnginePost {
public:
    /**
     * @brief Construct EnginePost object.
     * @param instance_params JSON object containing instance-specific parameters for this engine.
     * @throw std::runtime_error If configuration is missing or invalid.
     */
    EnginePost(const nlohmann::json& instance_params);
    /** @brief Initialize the engine and its active sub-engines. */
    void Initialize();
    /** @brief Execute the engine's logic by running its sub-engines. */
    void Execute();
    /** @brief Release resources held by the engine and its sub-engines. */
    void Release();
    /**
     * @brief Get JSON schema for this engine's parameters.
     * @return nlohmann::json object representing the schema.
     */
    static nlohmann::json GetParamSchema();
protected:
    const std::string engineFlowField = "EngineFlowField";
    
    std::map<std::string, EnginePostVariant> subEnginesPool;

    std::vector<std::string> execution_order_;
};

/**
 * @brief Main process engine that coordinates the overall workflow.
 * Manages the sequence of major processing stages (preprocessing, solving, postprocessing).
 */
class EngineMainProcess {
public:
    /**
     * @brief Construct EngineMainProcess object.
     * @param instance_params JSON object containing instance-specific parameters for this engine (such as stage execution order).
     * @throw std::runtime_error If configuration is missing or invalid.
     */
    EngineMainProcess(const nlohmann::json& instance_params);
    /** @brief Initialize the main process and all configured top-level engines. */
    void Initialize();
    /** @brief Execute the main process by running top-level engines in sequence. */
    void Execute();
    /** @brief Release resources held by the main process and its engines. */
    void Release();
    /**
     * @brief Get JSON schema for main process engine parameters.
     * @return nlohmann::json object representing the schema.
     */
    static nlohmann::json GetParamSchema();
protected:

    // If need to add sub-engines, first add string key name, then add to map TODO
    const std::string enginePre = "EnginePre"; // TODO
    const std::string engineSolve = "EngineSolve"; // TODO
    const std::string enginePost = "EnginePost"; // TODO
    
    std::map<std::string, EngineMainProcessVariant> subEnginesPool;

    std::vector<std::string> execution_order_ = {enginePre, engineSolve, enginePost}; // Only in EngineMainProcess subEngine order is specified.For other engines, the execution order of their lower-level engines or modules needs to be obtained from the json configuration file.
};

// --- Template helpers for engine and module management ---
/**
 * @brief Template helper to create the appropriate engine or module by name.
 * This template function maps a string identifier to the corresponding engine/module class.
 * @tparam EngineVariantType The std::variant type that will hold the created engine/module.
 * @param subEngineName The name of the engine/module to create.
 * @param config JSON configuration for the engine/module construction.
 * @return A variant containing the unique_ptr to the created engine/module.
 * @throw std::runtime_error if an unknown engine/module name is provided.
 */
#define CREATE_ENGINE_MAPPING(VariantType, EngineName, EngineClass) \
    if constexpr (std::is_same_v<EngineVariantType, VariantType>) { \
        if (subEngineName == EngineName) { \
            return std::make_unique<EngineClass>(config); \
        } \
    }

/**
 * @brief Template function for creating engine or module instances by name.
 * 
 * This template function maps string identifiers to their corresponding engine or module classes.
 * It's used throughout the component hierarchy to instantiate the appropriate objects based on
 * configuration data. The function uses compile-time type checking through constexpr and
 * std::is_same_v to ensure type safety when creating components.
 * 
 * @tparam EngineVariantType The std::variant type that will hold the created engine/module.
 * @param subEngineName The name of the engine/module to create.
 * @param config JSON configuration for the engine/module construction.
 * @return A variant containing the unique_ptr to the created engine/module.
 * @throw std::runtime_error if an unknown engine/module name is provided.
 */
template<typename EngineVariantType>
EngineVariantType CreateEngineByName(const std::string& subEngineName, const nlohmann::json& config)
{
    CREATE_ENGINE_MAPPING(EngineMainProcessVariant, "EnginePre", EnginePre)
    CREATE_ENGINE_MAPPING(EngineMainProcessVariant, "EngineSolve", EngineSolve)
    CREATE_ENGINE_MAPPING(EngineMainProcessVariant, "EnginePost", EnginePost)
    
    CREATE_ENGINE_MAPPING(EnginePreVariant, "EnginePreGrid", EnginePreGrid)
    CREATE_ENGINE_MAPPING(EngineSolveVariant, "EngineTurbulence", EngineTurbulence)
    CREATE_ENGINE_MAPPING(EnginePostVariant, "EngineFlowField", EngineFlowField)
    
    CREATE_ENGINE_MAPPING(EnginePreGridVariant, "ModulePreCGNS", ModulePreCGNS)
    CREATE_ENGINE_MAPPING(EnginePreGridVariant, "ModulePrePlot3D", ModulePrePlot3D)
    CREATE_ENGINE_MAPPING(EnginePreGridVariant, "ModulePreTecplot", ModulePreTecplot)
    
    CREATE_ENGINE_MAPPING(EngineTurbulenceVariant, "ModuleSA", ModuleSA)
    CREATE_ENGINE_MAPPING(EngineTurbulenceVariant, "ModuleSST", ModuleSST)
    CREATE_ENGINE_MAPPING(EngineTurbulenceVariant, "ModuleSSTWDF", ModuleSSTWDF)
    
    CREATE_ENGINE_MAPPING(EngineFlowFieldVariant, "ModulePostCGNS", ModulePostCGNS)
    CREATE_ENGINE_MAPPING(EngineFlowFieldVariant, "ModulePostPlot3D", ModulePostPlot3D)
    CREATE_ENGINE_MAPPING(EngineFlowFieldVariant, "ModulePostTecplot", ModulePostTecplot)
    
    throw std::runtime_error("Unknown engine/module name: " + subEngineName);
}

/**
 * @brief Template helper function to construct sub-engines or modules.
 * 
 * This function loads configurations for each sub-engine or module specified in the execution order,
 * instantiates them using the CreateEngineByName function, and stores them in the provided map.
 * 
 * @tparam EngineVariantType The variant type containing unique_ptr to engines or modules.
 * @param execution_order Vector of component names in their execution sequence.
 * @param subEnginesPool Map to store the created engine/module instances.
 */
template<typename EngineVariantType>
void ConstructSubEngines(
    const std::vector<std::string>& execution_order,
    std::map<std::string, EngineVariantType>& subEnginesPool) 
{
    for (const auto& subEngineName : execution_order)
    {
        // 1. Read JSON file based on subEngineName
        // 2. Pass JSON file to sub-engine for construction
        // 3. Put the following code in parent class, each engine calls this function in constructor and adds error handling
        // 4. Constructor first constructs execution_order_, then executes function in step 3
        nlohmann::json config;
        std::string configFileName = subEngineName + ".json";
        std::string baseEngineName = GetBaseName(subEngineName);
        LoadConfig(configFileName, config);
        subEnginesPool[subEngineName] = CreateEngineByName<EngineVariantType>(baseEngineName, config);
    }
}

/**
 * @brief Template helper function to initialize sub-engines or modules.
 * 
 * Calls the Initialize() method on each component in the specified execution order.
 * 
 * @tparam EngineVariantType The variant type containing unique_ptr to engines or modules.
 * @param execution_order Vector of component names in their execution sequence.
 * @param subEnginesPool Map containing the engine/module instances.
 */
template<typename EngineVariantType>
void InitializeSubEngines(
    const std::vector<std::string>& execution_order,
    std::map<std::string, EngineVariantType>& subEnginesPool) 
{
    for (const auto& subEngineName : execution_order)
    {
        std::visit([](auto&& engine_ptr) { if (engine_ptr) engine_ptr->Initialize(); }, subEnginesPool[subEngineName]);
    }
}

/**
 * @brief Template helper function to execute sub-engines or modules.
 * 
 * Calls the Execute() method on each component in the specified execution order.
 * 
 * @tparam EngineVariantType The variant type containing unique_ptr to engines or modules.
 * @param execution_order Vector of component names in their execution sequence.
 * @param subEnginesPool Map containing the engine/module instances.
 */
template<typename EngineVariantType>
void ExecuteSubEngines(
    const std::vector<std::string>& execution_order,
    std::map<std::string, EngineVariantType>& subEnginesPool) 
{
    for (const auto& subEngineName : execution_order)
    {
        std::visit([](auto&& engine_ptr) { if (engine_ptr) engine_ptr->Execute(); }, subEnginesPool[subEngineName]);
    }
}

/**
 * @brief Template helper function to release resources held by sub-engines or modules.
 * 
 * Calls the Release() method on each component in the specified execution order.
 * 
 * @tparam EngineVariantType The variant type containing unique_ptr to engines or modules.
 * @param execution_order Vector of component names in their execution sequence.
 * @param subEnginesPool Map containing the engine/module instances.
 */
template<typename EngineVariantType>
void ReleaseSubEngines(
    const std::vector<std::string>& execution_order,
    std::map<std::string, EngineVariantType>& subEnginesPool) 
{
    // Execute in reverse order for proper cleanup
    for (auto it = execution_order.rbegin(); it != execution_order.rend(); ++it)
    {
       std::visit([](auto&& engine_ptr) { if (engine_ptr) engine_ptr->Release(); }, subEnginesPool[*it]);
    }
}

// Check if engine and module classes have 4 common functions, if not, report error and exit at compile stage TODO

#endif // PARACONFIG_H