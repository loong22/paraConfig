#include "solve.h"
#include <iostream>

namespace SOLVE {

// 创建静态本地实例
static ModuleSystem::LocalTypeRegistry localTypeRegistryInstance;
static ModuleSystem::LocalFactory localFactoryInstance;

// 指向这些实例的指针
ModuleSystem::ModuleTypeRegistry* localTypeRegistry = nullptr;
ModuleSystem::ModuleFactory* localFactory = nullptr;

// 初始化函数 - 现在改为使用单例模式
void initializeFactory() {
    if (!localTypeRegistry) {
        localTypeRegistry = &ModuleSystem::ModuleTypeRegistry::instance();
    }
    if (!localFactory) {
        localFactory = &ModuleSystem::ModuleFactory::instance();
    }
}

/**
 * @brief 注册所有模块类型到本地注册表
 */
void registerTypes() {
    // 注册 EulerSolver 模块类型
    localTypeRegistryInstance.registerType("EulerSolver", 
        []() -> nlohmann::json { return EulerSolver::GetParamSchema(); });
    
    // 注册 SASolver 模块类型
    localTypeRegistryInstance.registerType("SASolver", 
        []() -> nlohmann::json { return SASolver::GetParamSchema(); });
    
    // 注册 SSTSolver 模块类型
    localTypeRegistryInstance.registerType("SSTSolver", 
        []() -> nlohmann::json { return SSTSolver::GetParamSchema(); });

    // 注册 EulerSolver 到工厂
    localFactoryInstance.registerModuleType("EulerSolver", 
        [](ModuleSystem::AdvancedRegistry* reg, const std::string& name) -> bool { 
            reg->Register<EulerSolver>(name);
            return true;
        });
    
    // 注册 SASolver 到工厂
    localFactoryInstance.registerModuleType("SASolver", 
        [](ModuleSystem::AdvancedRegistry* reg, const std::string& name) -> bool { 
            reg->Register<SASolver>(name);
            return true;
        });

    // 注册 SSTSolver 到工厂
    localFactoryInstance.registerModuleType("SSTSolver", 
        [](ModuleSystem::AdvancedRegistry* reg, const std::string& name) -> bool { 
            reg->Register<SSTSolver>(name);
            return true;
        });
}

/**
 * @brief 获取本地注册的模块类型集合
 * @return 模块类型信息集合
 */
const auto& getTypes() {
    return localTypeRegistry->getModuleTypes(); 
}

/**
 * @brief 获取本地模块工厂的创建函数集合
 * @return 模块创建函数映射
 */
const auto& getFactories() {
    return localFactory->getAllModuleCreators();
}

/**
 * @brief 将本地注册表中的类型和工厂导出到全局注册表
 */
void exportToGlobalRegistry() {
    // 确保模块类型已注册
    registerTypes();
#if DEBUG
    std::cout << "导出求解器模块到全局注册表..." << std::endl;
#endif
    // 获取本组件特有的模块名称
    std::unordered_set<std::string> solveModules = {"EulerSolver", "SASolver", "SSTSolver"};
    
    // 使用新的导出方法将本地注册表导出到全局
    localTypeRegistryInstance.exportToGlobal();
    localFactoryInstance.exportToGlobal();
    
    // 确保我们使用的是全局实例来关联模块和引擎
    initializeFactory();
    
    // 将模块关联到对应的引擎 - 只关联成功导出的模块
    auto& moduleRegistryInit = ModuleSystem::ModuleRegistryInitializer::init();
    for (const auto& name : solveModules) {
        moduleRegistryInit.assignModuleToEngine(name, "Solve");
#if DEBUG        
        std::cout << "  - 已将模块 '" << name << "' 关联到 Solve 引擎" << std::endl;
#endif        
    }
#if DEBUG
    std::cout << "网格预处理模块导出完成，共导出 " << solveModules.size() << " 个类型和 " 
              << solveModules.size() << " 个模块" << std::endl;
#endif              
}

//============= EulerSolver 模块实现 =============//

/**
 * @brief Constructor for EulerSolver module.
 * @param params The parameters for the module.
 */
EulerSolver::EulerSolver(const nlohmann::json& params) {
    euler_type_ = params.value("euler_type", "Standard");
    euler_value__ = params.value("euler_value", 0.5);
}

/**
 * @brief Initializes the EulerSolver module.
 */
void EulerSolver::initialize() {
}

/**
 * @brief Executes the EulerSolver module.
 */
void EulerSolver::execute() {
}

/**
 * @brief Releases the EulerSolver module.
 */
void EulerSolver::release() {
}

/**
 * @brief Gets the parameter schema for the EulerSolver module.
 * @return A JSON object representing the parameter schema.
 */
nlohmann::json EulerSolver::GetParamSchema() {
    return {
        {"euler_type", {
            {"type", "string"},
            {"description", "Type of euler file"},
            {"enum", {"Standard", "Other"}},
            {"default", "Standard"}
        }},
        {"euler_value", {
            {"type", "number"},
            {"description", "Number of euler value"},
            {"minimum", 0},
            {"maximum", 10},
            {"default", 0.5}
        }}
    };
}

//============= SASolver 模块实现 =============//

/**
 * @brief Constructor for SASolver module.
 * @param params The parameters for the module.
 */
SASolver::SASolver(const nlohmann::json& params) {
    sa_type_ = params.value("solver_type", "Standard");
    convergence_criteria_ = params.value("convergence_criteria", 1e-6);
    max_iterations_ = params.value("max_iterations", 1000);
}

/**
 * @brief Initializes the SASolver module.
 */
void SASolver::initialize() {
}

/**
 * @brief Executes the SASolver module.
 */
void SASolver::execute() {
}

/**
 * @brief Releases the SASolver module.
 */
void SASolver::release() {
}

/**
 * @brief Gets the parameter schema for the SASolver module.
 * @return A JSON object representing the parameter schema.
 */
nlohmann::json SASolver::GetParamSchema() {
    return {
        {"solver_type", {
            {"type", "string"},
            {"description", "Type of fluid solver"},
            {"enum", {"Standard", "SA-BC", "SA-DDES","SA-IDDES"}},
            {"default", "Standard"}
        }},
        {"convergence_criteria", {
            {"type", "number"},
            {"description", "Convergence criteria for solver"},
            {"minimum", 1e-10},
            {"maximum", 1e-3},
            {"default", 1e-6}
        }},
        {"max_iterations", {
            {"type", "number"},
            {"description", "Maximum number of iterations"},
            {"minimum", 10},
            {"maximum", 10000},
            {"default", 1000}
        }}
    };
}

//============= SSTSolver 模块实现 =============//

/**
 * @brief Constructor for SSTSolver module.
 * @param params The parameters for the module.
 */
SSTSolver::SSTSolver(const nlohmann::json& params) {
    sst_type_ = params.value("solver_type", "Standard");
    convergence_criteria_ = params.value("convergence_criteria", 1e-6);
    max_iterations_ = params.value("max_iterations", 1000);
}

/**
 * @brief Initializes the SSTSolver module.
 */
void SSTSolver::initialize() {
}

/**
 * @brief Executes the SSTSolver module.
 */
void SSTSolver::execute() {
}

/**
 * @brief Releases the SSTSolver module.
 */
void SSTSolver::release() {
}

/**
 * @brief Gets the parameter schema for the SSTSolver module.
 * @return A JSON object representing the parameter schema.
 */
nlohmann::json SSTSolver::GetParamSchema() {
    return {
        {"solver_type", {
            {"type", "string"},
            {"description", "Type of fluid solver"},
            {"enum", {"Standard", "SST-CC", "SA-DDES","SA-IDDES"}},
            {"default", "Standard"}
        }},
        {"convergence_criteria", {
            {"type", "number"},
            {"description", "Convergence criteria for solver"},
            {"minimum", 1e-10},
            {"maximum", 1e-3},
            {"default", 1e-6}
        }},
        {"max_iterations", {
            {"type", "number"},
            {"description", "Maximum number of iterations"},
            {"minimum", 10},
            {"maximum", 10000},
            {"default", 1000}
        }}
    };
}

} // namespace SOLVE

namespace ModuleSystem {
    nlohmann::json ModuleParamTraits<SOLVE::EulerSolver>::GetParamSchema() {
        return SOLVE::EulerSolver::GetParamSchema();
    }

    nlohmann::json ModuleParamTraits<SOLVE::SASolver>::GetParamSchema() {
        return SOLVE::SASolver::GetParamSchema();
    }

    nlohmann::json ModuleParamTraits<SOLVE::SSTSolver>::GetParamSchema() {
        return SOLVE::SSTSolver::GetParamSchema();
}

} // namespace ModuleSystem