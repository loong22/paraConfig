#pragma once

#include "paraConfig.h"
#include <memory>
#include <map>
#include <vector>
#include <variant>

// Forward declarations
class EnginePre;
class EngineSolve;
class EnginePost;

//If adding modules or engines, change here
using EngineMainProcessVariant = std::variant<
        std::unique_ptr<EnginePre>,
        std::unique_ptr<EngineSolve>,
        std::unique_ptr<EnginePost>>;

// Factory type alias
using MainProcessFactory = ComponentFactory<EngineMainProcessVariant>;

/**
 * @brief Main process engine that coordinates the overall workflow.
 * Manages the sequence of major processing stages (preprocessing, solving, postprocessing).
 */
class EngineMainProcess {
public:
    EngineMainProcess(const nlohmann::json& config);
    void Initialize();
    void Execute();
    void Release();
    static nlohmann::json GetParamSchema();
    void ParamValidation(const nlohmann::json& params);
    
protected:
    const std::string enginePre = "EnginePre";
    const std::string engineSolve = "EngineSolve";
    const std::string enginePost = "EnginePost";
    
    std::map<std::string, EngineMainProcessVariant> subEnginesPool;
    std::vector<std::string> execution_order_ = {enginePre, engineSolve, enginePost};
    ComponentFactory<EngineMainProcessVariant> factory_;
    void InitializeFactory();
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
    std::map<std::string, EngineVariantType>& subEnginesPool,
    ComponentFactory<EngineVariantType>& factory) 
{
    for (const auto& subEngineName : execution_order)
    {
        nlohmann::json config;
        std::string configFileName = subEngineName + ".json";
        std::string baseEngineName = GetBaseName(subEngineName);
        LoadConfig(configFileName, config);
        subEnginesPool[subEngineName] = factory.Create(baseEngineName, config);
    }
}

// Template specializations for factory access - avoiding runtime if-else checks
template<typename VariantType>
struct FactoryProvider;

template<>
struct FactoryProvider<EngineMainProcessVariant> {
    static auto& GetFactory() { return MainProcessFactory::Instance(); }
};
