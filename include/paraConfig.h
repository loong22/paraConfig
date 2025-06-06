#pragma once

#include "nlohmann/json.hpp"
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <variant>
#include <stdexcept>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <functional>

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

// --- Factory Pattern for Engine/Module Creation ---
/**
 * @brief Template factory class for creating engines and modules.
 * @tparam VariantType The std::variant type that will hold the created engine/module.
 */
template<typename VariantType>
class ComponentFactory {
public:
    using CreatorFunc = std::function<VariantType(const nlohmann::json&)>;
    
    /**
     * @brief Register a component creator function.
     * @param name Component name.
     * @param creator Function that creates the component.
     */
    void RegisterCreator(const std::string& name, CreatorFunc creator) {
        creators_[name] = creator;
    }
    
    /**
     * @brief Create a component by name.
     * @param name Component name.
     * @param config Configuration for the component.
     * @return Created component wrapped in variant.
     */
    VariantType Create(const std::string& name, const nlohmann::json& config) {
        auto it = creators_.find(name);
        if (it != creators_.end()) {
            return it->second(config);
        }
        throw std::runtime_error("Unknown component name: " + name);
    }
    
    /**
     * @brief Get singleton instance of factory.
     * @return Reference to factory instance.
     */
    static ComponentFactory& Instance() {
        static ComponentFactory instance;
        return instance;
    }

private:
    std::map<std::string, CreatorFunc> creators_;
};

// Factory type aliases
using MainProcessFactory = ComponentFactory<EngineMainProcessVariant>;
using PreFactory = ComponentFactory<EnginePreVariant>;
using SolveFactory = ComponentFactory<EngineSolveVariant>;
using PostFactory = ComponentFactory<EnginePostVariant>;
using PreGridFactory = ComponentFactory<EnginePreGridVariant>;
using TurbulenceFactory = ComponentFactory<EngineTurbulenceVariant>;
using FlowFieldFactory = ComponentFactory<EngineFlowFieldVariant>;

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
     * @brief Validate parameters for this module.
     * @param params JSON parameters to validate.
     * @throw std::runtime_error If validation fails.
     */
    void ParamValidation(const nlohmann::json& params);
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
     * @brief Validate parameters for this module.
     * @param params JSON parameters to validate.
     * @throw std::runtime_error If validation fails.
     */
    void ParamValidation(const nlohmann::json& params);
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
     * @brief Validate parameters for this module.
     * @param params JSON parameters to validate.
     * @throw std::runtime_error If validation fails.
     */
    void ParamValidation(const nlohmann::json& params);
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
     * @brief Validate parameters for this module.
     * @param params JSON parameters to validate.
     * @throw std::runtime_error If validation fails.
     */
    void ParamValidation(const nlohmann::json& params);
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
     * @brief Validate parameters for this module.
     * @param params JSON parameters to validate.
     * @throw std::runtime_error If validation fails.
     */
    void ParamValidation(const nlohmann::json& params);
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
     * @brief Validate parameters for this module.
     * @param params JSON parameters to validate.
     * @throw std::runtime_error If validation fails.
     */
    void ParamValidation(const nlohmann::json& params);
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
     * @brief Validate parameters for this module.
     * @param params JSON parameters to validate.
     * @throw std::runtime_error If validation fails.
     */
    void ParamValidation(const nlohmann::json& params);
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
     * @brief Validate parameters for this module.
     * @param params JSON parameters to validate.
     * @throw std::runtime_error If validation fails.
     */
    void ParamValidation(const nlohmann::json& params);
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
     * @brief Validate parameters for this module.
     * @param params JSON parameters to validate.
     * @throw std::runtime_error If validation fails.
     */
    void ParamValidation(const nlohmann::json& params);
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
     * @brief Validate parameters for this engine.
     * @param params JSON parameters to validate.
     * @throw std::runtime_error If validation fails.
     */
    void ParamValidation(const nlohmann::json& params);
    /**
     * @brief Get JSON schema for this engine's parameters.
     * @return nlohmann::json object representing the schema.
     */
    static nlohmann::json GetParamSchema();

    // Static factory instance for this engine
    static PreFactory factory_;

protected:
    const std::string modulePreCGNS = "ModulePreCGNS";
    const std::string modulePrePlot3D = "ModulePrePlot3D";
    const std::string modulePreTecplot = "ModulePreTecplot";
    
    std::map<std::string, EnginePreGridVariant> subModulesPool;

    std::vector<std::string> execution_order_;

private:
    /**
     * @brief Create module instance by name
     * @param module_name Module name to create
     * @param config Module configuration
     * @return Created module wrapped in variant
     */
    EnginePreGridVariant CreateModule(const std::string& module_name, const nlohmann::json& config);
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
     * @brief Validate parameters for this engine.
     * @param params JSON parameters to validate.
     * @throw std::runtime_error If validation fails.
     */
    void ParamValidation(const nlohmann::json& params);
    /**
     * @brief Get JSON schema for this engine's parameters.
     * @return nlohmann::json object representing the schema.
     */
    static nlohmann::json GetParamSchema();
    
    // Static factory instance for this engine
    static SolveFactory factory_;
    
protected:
    const std::string moduleSA = "ModuleSA";
    const std::string moduleSST = "ModuleSST";
    const std::string moduleSSTWDF = "ModuleSSTWDF";
    
    std::map<std::string, EngineTurbulenceVariant> subModulesPool;

    std::vector<std::string> execution_order_;

private:
    /**
     * @brief Create module instance by name
     * @param module_name Module name to create
     * @param config Module configuration
     * @return Created module wrapped in variant
     */
    EngineTurbulenceVariant CreateModule(const std::string& module_name, const nlohmann::json& config);
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
     * @brief Validate parameters for this engine.
     * @param params JSON parameters to validate.
     * @throw std::runtime_error If validation fails.
     */
    void ParamValidation(const nlohmann::json& params);
    /**
     * @brief Get JSON schema for this engine's parameters.
     * @return nlohmann::json object representing the schema.
     */
    static nlohmann::json GetParamSchema();
    
    // Static factory instance for this engine
    static PostFactory factory_;
    
protected:
    const std::string modulePostCGNS = "ModulePostCGNS";
    const std::string modulePostPlot3D = "ModulePostPlot3D";
    const std::string modulePostTecplot = "ModulePostTecplot";
    
    std::map<std::string, EngineFlowFieldVariant> subModulesPool;

    std::vector<std::string> execution_order_;

private:
    /**
     * @brief Create module instance by name
     * @param module_name Module name to create
     * @param config Module configuration
     * @return Created module wrapped in variant
     */
    EngineFlowFieldVariant CreateModule(const std::string& module_name, const nlohmann::json& config);
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
     * @brief Validate parameters for this engine.
     * @param params JSON parameters to validate.
     * @throw std::runtime_error If validation fails.
     */
    void ParamValidation(const nlohmann::json& params);
    /**
     * @brief Get JSON schema for this engine's parameters.
     * @return nlohmann::json object representing the schema.
     */
    static nlohmann::json GetParamSchema();
    
    // Static factory instance for this engine
    static MainProcessFactory factory_;
    
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
     * @brief Validate parameters for this engine.
     * @param params JSON parameters to validate.
     * @throw std::runtime_error If validation fails.
     */
    void ParamValidation(const nlohmann::json& params);
    /**
     * @brief Get JSON schema for this engine's parameters.
     * @return nlohmann::json object representing the schema.
     */
    static nlohmann::json GetParamSchema();
    
    // Static factory instance for this engine
    static MainProcessFactory factory_;
    
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
     * @brief Validate parameters for this engine.
     * @param params JSON parameters to validate.
     * @throw std::runtime_error If validation fails.
     */
    void ParamValidation(const nlohmann::json& params);
    /**
     * @brief Get JSON schema for this engine's parameters.
     * @return nlohmann::json object representing the schema.
     */
    static nlohmann::json GetParamSchema();
    
    // Static factory instance for this engine
    static MainProcessFactory factory_;
    
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
    void ParamValidation(const nlohmann::json& params);
    
    /**
     * @brief Register all engines with their respective factories.
     * This ensures factories are ready before any engine instantiation.
     */
    static void RegisterAllEngines();
    
protected:
    const std::string enginePre = "EnginePre";
    const std::string engineSolve = "EngineSolve";
    const std::string enginePost = "EnginePost";
    
    std::map<std::string, EngineMainProcessVariant> subEnginesPool;

    std::vector<std::string> execution_order_ = {enginePre, engineSolve, enginePost}; // Only in EngineMainProcess subEngine order is specified.For other engines, the execution order of their lower-level engines or modules needs to be obtained from the json configuration file.
};

// --- Auxiliary function declarations ---
/*
 * @param j_obj The nlohmann::json object to write.
 */
void WriteJsonFile(const std::string& dir_path, const std::string& filename, const nlohmann::json& j_obj);

/**
 * @brief Template specializations for factory access - avoiding runtime if-else checks
 */
template<typename VariantType>
struct FactoryProvider;

template<>
struct FactoryProvider<EngineMainProcessVariant> {
    static auto& GetFactory() { return MainProcessFactory::Instance(); }
};

template<>
struct FactoryProvider<EnginePreVariant> {
    static auto& GetFactory() { return PreFactory::Instance(); }
};

template<>
struct FactoryProvider<EngineSolveVariant> {
    static auto& GetFactory() { return SolveFactory::Instance(); }
};

template<>
struct FactoryProvider<EnginePostVariant> {
    static auto& GetFactory() { return PostFactory::Instance(); }
};

template<>
struct FactoryProvider<EnginePreGridVariant> {
    static auto& GetFactory() { return PreGridFactory::Instance(); }
};

template<>
struct FactoryProvider<EngineTurbulenceVariant> {
    static auto& GetFactory() { return TurbulenceFactory::Instance(); }
};

template<>
struct FactoryProvider<EngineFlowFieldVariant> {
    static auto& GetFactory() { return FlowFieldFactory::Instance(); }
};

/**
 * @brief Template helper function to construct sub-engines (for engine-to-engine relationships).
 * 
 * This function loads configurations for each sub-engine specified in the execution order,
 * instantiates them using the factory pattern, and stores them in the provided map.
 * 
 * @tparam EngineVariantType The variant type containing unique_ptr to engines.
 * @param execution_order Vector of component names in their execution sequence.
 * @param subEnginesPool Map to store the created engine instances.
 */
template<typename EngineVariantType>
void ConstructSubEngines(
    const std::vector<std::string>& execution_order,
    std::map<std::string, EngineVariantType>& subEnginesPool) 
{
    auto& factory = FactoryProvider<EngineVariantType>::GetFactory();
    
    for (const auto& subEngineName : execution_order)
    {
        nlohmann::json config;
        std::string configFileName = subEngineName + ".json";
        std::string baseEngineName = GetBaseName(subEngineName);
        LoadConfig(configFileName, config);
        
        subEnginesPool[subEngineName] = factory.Create(baseEngineName, config);
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
