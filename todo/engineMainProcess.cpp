#include "engineMainProcess.h"
#include "enginePre.h"
#include "engineSolve.h"
#include "enginePost.h"

EngineMainProcess::EngineMainProcess(const nlohmann::json& config)
{
    InitializeFactory();
    ParamValidation(config);
    ConstructSubEngines<EngineMainProcessVariant>(execution_order_, subEnginesPool, factory_);
}

void EngineMainProcess::InitializeFactory()
{
    factory_.RegisterCreator("EnginePre", 
        [](const nlohmann::json& config) -> EngineMainProcessVariant {
            return std::make_unique<EnginePre>(config);
        });
    factory_.RegisterCreator("EngineSolve", 
        [](const nlohmann::json& config) -> EngineMainProcessVariant {
            return std::make_unique<EngineSolve>(config);
        });
    factory_.RegisterCreator("EnginePost", 
        [](const nlohmann::json& config) -> EngineMainProcessVariant {
            return std::make_unique<EnginePost>(config);
        });
}

void EngineMainProcess::Initialize()
{
    InitializeSubEngines<EngineMainProcessVariant>(execution_order_, subEnginesPool);
}

void EngineMainProcess::Execute()
{
    ExecuteSubEngines<EngineMainProcessVariant>(execution_order_, subEnginesPool);
}

void EngineMainProcess::Release()
{
    ReleaseSubEngines<EngineMainProcessVariant>(execution_order_, subEnginesPool);
}

void EngineMainProcess::ParamValidation(const nlohmann::json& params)
{
    // 主流程引擎的参数验证逻辑
    if (params.contains("execution_order")) {
        if (!params.at("execution_order").is_array()) {
            throw std::runtime_error("EngineMainProcess: 'execution_order' must be an array.");
        }
    }
}

nlohmann::json EngineMainProcess::GetParamSchema()
{
    return {
        {"execution_order", {
            {"type", "array"},
            {"description", "Execution order of main process engines"},
            {"items", {"type", "string"}},
            {"default", {"EnginePre", "EngineSolve", "EnginePost"}}
        }}
    };
}
