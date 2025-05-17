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

#include "advanced_module_system.h"
#include <iostream>
#include <fstream>
#include <iomanip> // For std::setw
#include <filesystem> // For std::filesystem
#include <stdexcept> // For std::runtime_error
#include <algorithm> // For std::find
#include <cxxabi.h>

namespace ModuleSystem {

// 定义模块动作枚举
enum class ModuleAction {
    CREATE,
    INITIALIZE,
    EXECUTE,
    RELEASE,
    UNKNOWN
};

// 将字符串转换为 ModuleAction 枚举
ModuleAction stringToModuleAction(const std::string& actionStr) {
    static const std::unordered_map<std::string, ModuleAction> actionMap = {
        {"create", ModuleAction::CREATE},
        {"initialize", ModuleAction::INITIALIZE},
        {"execute", ModuleAction::EXECUTE},
        {"release", ModuleAction::RELEASE}
    };
    auto it = actionMap.find(actionStr);
    return (it != actionMap.end()) ? it->second : ModuleAction::UNKNOWN;
}

// Helper function to convert LifecycleStage to string for error messages
std::string LifecycleStageToString(LifecycleStage stage) {
    switch (stage) {
        case LifecycleStage::CONSTRUCTED: return "CONSTRUCTED";
        case LifecycleStage::INITIALIZED: return "INITIALIZED";
        case LifecycleStage::EXECUTED:    return "EXECUTED";
        case LifecycleStage::RELEASED:    return "RELEASED";
        default:                          return "UNKNOWN_STAGE (" + std::to_string(static_cast<int>(stage)) + ")";
    }
}

// AdvancedRegistry method definitions
void* AdvancedRegistry::Create(const std::string& name, const nlohmann::json& params) {
    auto it = modules_.find(name);
    if (it == modules_.end()) {
        throw std::runtime_error("Module not found: " + name);
    }
    
    void* instance = it->second.construct(params);
    if (!instance) {
        throw std::runtime_error("Failed to construct module: " + name);
    }

    auto lifecycleIt = lifecycle_.find(instance);
    // 如果实例已经存在于生命周期映射中
    if (lifecycleIt != lifecycle_.end()) {
        LifecycleStage currentStage = std::get<1>(lifecycleIt->second);
        if (currentStage != LifecycleStage::RELEASED) {
            // 如果状态不是RELEASED，需要发出警告
            std::cout << "警告: 模块 " << std::get<0>(lifecycleIt->second) 
                      << " 当前状态为 " << LifecycleStageToString(currentStage) 
                      << "，但正在重新创建。" << std::endl;
        }
    }

    lifecycle_.insert({
        instance, 
        {name, LifecycleStage::CONSTRUCTED, it->second.type}
    });
    
    return instance;
}

void AdvancedRegistry::Initialize(void* mod) { 
    if (mod) {
        auto it = lifecycle_.find(mod);
        if (it != lifecycle_.end()) {
            LifecycleStage currentStage = std::get<1>(it->second);
            // 只允许从 CONSTRUCTED, INITIALIZED, 或 EXECUTED 状态调用
            if (currentStage == LifecycleStage::RELEASED) {
                throw std::runtime_error("Module " + std::get<0>(it->second) + " is RELEASED and cannot be initialized. Current state: " + LifecycleStageToString(currentStage));
            }
            if (currentStage != LifecycleStage::CONSTRUCTED && currentStage != LifecycleStage::INITIALIZED && currentStage != LifecycleStage::EXECUTED) {
                throw std::runtime_error("Module " + std::get<0>(it->second) + " must be in CONSTRUCTED, INITIALIZED, or EXECUTED state to be initialized. Current state: " + LifecycleStageToString(currentStage));
            }
            modules_.at(std::get<0>(it->second)).initialize(mod);
            std::get<1>(it->second) = LifecycleStage::INITIALIZED;
        } else {
            // Handle error: module instance not found in lifecycle map
            throw std::runtime_error("Instance not found in lifecycle map for Initialize");
        }
    }
}

void AdvancedRegistry::Execute(void* mod) { 
    if (mod) {
        auto it = lifecycle_.find(mod);
        if (it != lifecycle_.end()) {
            LifecycleStage currentStage = std::get<1>(it->second);
            // 只允许从 INITIALIZED 或 EXECUTED 状态调用
            if (currentStage != LifecycleStage::INITIALIZED && currentStage != LifecycleStage::EXECUTED) {
                throw std::runtime_error("Module " + std::get<0>(it->second) + " must be in INITIALIZED or EXECUTED state to be executed. Current state: " + LifecycleStageToString(currentStage));
            }
            modules_.at(std::get<0>(it->second)).execute(mod);
            std::get<1>(it->second) = LifecycleStage::EXECUTED;
        } else {
            // Handle error: module instance not found in lifecycle map
            throw std::runtime_error("Instance not found in lifecycle map for Execute");
        }
    }
}

void AdvancedRegistry::Release(void* mod) { 
    if (mod) {
        auto it = lifecycle_.find(mod);
        if (it != lifecycle_.end()) {
            LifecycleStage currentStage = std::get<1>(it->second);
            if (currentStage == LifecycleStage::RELEASED) {
                throw std::runtime_error("Module " + std::get<0>(it->second) + " is already RELEASED and cannot be released again. Current state: " + LifecycleStageToString(currentStage));
            }
            // 允许从 CONSTRUCTED, INITIALIZED, 或 EXECUTED 状态调用
            if (currentStage != LifecycleStage::CONSTRUCTED && 
                currentStage != LifecycleStage::INITIALIZED && 
                currentStage != LifecycleStage::EXECUTED) {
                 // Should not happen if states are managed correctly, but as a safeguard:
                 throw std::runtime_error("Module " + std::get<0>(it->second) + " is in an invalid state for release: " + LifecycleStageToString(currentStage));
            }
            modules_.at(std::get<0>(it->second)).release(mod);
            std::get<1>(it->second) = LifecycleStage::RELEASED; // 标记为 RELEASED
            lifecycle_.erase(mod); // 然后从生命周期跟踪中移除
        } else {
            // Handle error: module instance not found in lifecycle map
            throw std::runtime_error("Instance not found in lifecycle map for Release");
        }
    }
}

std::vector<std::string> AdvancedRegistry::checkLeakedModules() const {
    std::vector<std::string> leakedModules;
    for (const auto& [instance, lifecycleData] : lifecycle_) {
        const std::string& moduleName = std::get<0>(lifecycleData);
        LifecycleStage stage = std::get<1>(lifecycleData);
        if (stage != LifecycleStage::RELEASED) {
            leakedModules.push_back(moduleName + " (状态: " + LifecycleStageToString(stage) + ")");
        }
    }
    return leakedModules;
}

// engineContext method definitions
engineContext::engineContext(AdvancedRegistry& reg, Nestedengine* engine) 
    : registry_(reg), engine_(engine) {}

void engineContext::setParameter(const std::string& name, const nlohmann::json& value) {
    parameters_[name] = value;
}

void engineContext::executeSubProcess(const std::string& name) {
    if (!engine_) {
        throw std::runtime_error("No engine available");
    }
    engine_->executeengine(name, nullptr); // Or pass shared_from_this() if appropriate
}

void* engineContext::createModule(const std::string& name, const nlohmann::json& params) {
    // 添加权限检查
    if (!canAccessModule(name)) {
        std::string errorMsg = "访问控制错误: 引擎 '" + engineName_ + "' 尝试创建不属于其绑定工厂的模块 '" + name + "'";
        std::cerr << errorMsg << std::endl;
        //throw std::runtime_error(errorMsg);
        exit(1);
    }

    if (modules_.count(name)) {
        std::cout << "警告: 模块 " << name << " 已存在。正在释放旧实例并创建新实例。" << std::endl;
        registry_.Release(modules_[name]); // This will also remove it from registry's lifecycle map
        modules_.erase(name);
    }
    void* instance = registry_.Create(name, params);
    modules_[name] = instance;
    return instance;
}

void engineContext::initializeModule(const std::string& name) {
    auto it = modules_.find(name);
    if (it != modules_.end()) {
        registry_.Initialize(it->second);
    } else {
        throw std::runtime_error("Module not found for initialize: " + name);
    }
}

void engineContext::executeModule(const std::string& name) {
    auto it = modules_.find(name);
    if (it != modules_.end()) {
        registry_.Execute(it->second);
    } else {
        throw std::runtime_error("Module not found for execute: " + name);
    }
}

void engineContext::releaseModule(const std::string& name) {
    auto it = modules_.find(name);
    if (it != modules_.end()) {
        try {
            registry_.Release(it->second);
            modules_.erase(it);
        } catch (const std::exception& e) {
            std::cerr << "错误: 释放模块 '" << name << "' 失败: " << e.what() << std::endl;
            throw; // 重新抛出异常以便上层处理
        }
    } else {
        std::cout << "错误: 尝试释放不存在的模块 '" << name 
                  << "'，可能该模块已经被释放或从未创建过" << std::endl;
        return;
    }
}

// 根据工厂执行逻辑执行模块
void engineContext::executeModulesAccordingToFactoryLogic(const std::string& factoryName) {
    // 获取工厂执行配置
    auto config = ModuleFactoryCollection::getFactoryExecutionConfig(factoryName);
    
    // 获取工厂中的模块
    auto factory = ModuleFactoryCollection::instance().getFactory(factoryName);
    auto& moduleCreators = factory->getAllModuleCreators();
    
    std::vector<std::string> moduleNames;
    for (const auto& [name, _] : moduleCreators) {
        moduleNames.push_back(name);
    }
    
    switch (config.type) {
        case FactoryExecutionType::CHOOSE_ONE: {
            // 多选一执行：让用户选择一个模块执行
            std::cout << "请选择要执行的模块 (从工厂 " << factoryName << "):" << std::endl;
            for (size_t i = 0; i < moduleNames.size(); ++i) {
                std::cout << i + 1 << ". " << moduleNames[i] << std::endl;
            }
            
            size_t choice = 0;
            std::cin >> choice;
            
            if (choice > 0 && choice <= moduleNames.size()) {
                std::string selectedModule = moduleNames[choice - 1];
                
                // 执行选中的模块
                void* moduleInstance = createModule(selectedModule, getParameter<nlohmann::json>(selectedModule));
                initializeModule(selectedModule);
                executeModule(selectedModule);
                releaseModule(selectedModule);
            }
            break;
        }
        
        case FactoryExecutionType::SEQUENTIAL_AND_CHOOSE: {
            // 修改为：按顺序执行所有模块
            std::cout << "按顺序执行工厂 " << factoryName << " 中的所有模块" << std::endl;
            
            // 使用工厂中的所有模块按顺序执行
            for (const auto& moduleName : moduleNames) {
                std::cout << "执行模块: " << moduleName << std::endl;
                
                // 执行当前模块
                void* moduleInstance = createModule(moduleName, getParameter<nlohmann::json>(moduleName));
                initializeModule(moduleName);
                executeModule(moduleName);
                releaseModule(moduleName);
            }
            break;
        }
        
        default: {
            // 未知执行类型，默认使用CHOOSE_ONE逻辑
            std::cerr << "警告: 工厂 '" << factoryName << "' 使用了未知的执行类型，将使用CHOOSE_ONE逻辑" << std::endl;
            // 复制CHOOSE_ONE的执行逻辑
            std::cout << "请选择要执行的模块 (从工厂 " << factoryName << "):" << std::endl;
            for (size_t i = 0; i < moduleNames.size(); ++i) {
                std::cout << i + 1 << ". " << moduleNames[i] << std::endl;
            }
            
            size_t choice = 0;
            std::cin >> choice;
            
            if (choice > 0 && choice <= moduleNames.size()) {
                std::string selectedModule = moduleNames[choice - 1];
                
                // 执行选中的模块
                void* moduleInstance = createModule(selectedModule, getParameter<nlohmann::json>(selectedModule));
                initializeModule(selectedModule);
                executeModule(selectedModule);
                releaseModule(selectedModule);
            }
            break;
        }
    }
}

// Nestedengine method definitions
Nestedengine::Nestedengine(AdvancedRegistry& reg) : registry_(reg) {}

void Nestedengine::Build(const nlohmann::json& config) {
    parameters_ = config;
}

void Nestedengine::defineengine(const std::string& name, 
                   std::function<void(engineContext&)> Engine) {
    enginePool_[name] = Engine;
}

const auto& Nestedengine::getengines() const { 
    return enginePool_; 
}

nlohmann::json Nestedengine::getDefaultConfig() const {
    nlohmann::json defaultConfig;
    defaultConfig["solver"] = "SIMPLE";
    defaultConfig["maxIterations"] = 1000;
    defaultConfig["convergenceCriteria"] = 1e-6;
    defaultConfig["time_step"] = 0.01;
    return defaultConfig;
}

void Nestedengine::executeengine(const std::string& name, std::shared_ptr<engineContext> parentContext) {
    auto it = enginePool_.find(name);
    if (it == enginePool_.end()) {
        throw std::runtime_error("engine: " + name);
    }

    std::shared_ptr<engineContext> contextToExecuteWith;

    if (parentContext) {
        contextToExecuteWith = parentContext;
    } else {
        contextToExecuteWith = std::make_shared<engineContext>(registry_, this);
    }
    
    // 设置引擎上下文的引擎名称
    contextToExecuteWith->setEngineName(name);
    
    // 获取引擎绑定的工厂名称
    std::string factoryName = getEngineFactoryBinding(name);
    
    // 如果工厂名称不是默认工厂，则设置访问权限
    if (factoryName != "default") {
        // 获取工厂中包含的所有模块
        auto factory = ModuleFactoryCollection::instance().getFactory(factoryName);
        auto& allModuleCreators = factory->getAllModuleCreators();
        
        // 创建允许访问的模块集合
        std::unordered_set<std::string> allowedModules; //在main.cpp中检测，把验证检测模块单独放出来。
        for (const auto& [moduleName, _] : allModuleCreators) {
            allowedModules.insert(moduleName);
        }
        
        // 设置引擎上下文的允许模块列表
        contextToExecuteWith->setAllowedModules(allowedModules);
    }
    
    contextToExecuteWith->engine_ = this; // Ensure the context knows about this engine
    it->second(*contextToExecuteWith); // Execute the engine's lambda with the chosen/created context
}

// Nestedengine方法实现（添加到现有代码中）
void Nestedengine::bindEngineToFactory(const std::string& engineName, const std::string& factoryName) {
    engineFactoryBindings_[engineName] = factoryName;
}

std::string Nestedengine::getEngineFactoryBinding(const std::string& engineName) const {
    auto it = engineFactoryBindings_.find(engineName);
    if (it != engineFactoryBindings_.end()) {
        return it->second;
    }
    return "default"; // 默认工厂
}

void Nestedengine::initializeEngineFactoryBindings(const nlohmann::json& config) {
    if (!config.contains("engineFactoryBindings") || !config["engineFactoryBindings"].is_array()) {
        return;
    }
    
    for (const auto& binding : config["engineFactoryBindings"]) {
        if (!binding.contains("engineName") || !binding.contains("factoryName")) {
            continue;
        }
        
        std::string engineName = binding["engineName"];
        std::string factoryName = binding["factoryName"];
        
        bindEngineToFactory(engineName, factoryName);
    }
}

// ModuleTypeInfo constructor
ModuleTypeInfo::ModuleTypeInfo(const std::string& n, std::function<nlohmann::json()> f)
    : name(n), getParamSchemaFunc(f) {}

// ModuleTypeRegistry method definitions
void ModuleTypeRegistry::registerType(const std::string& name, std::function<nlohmann::json()> schemaFunc) {
    moduleTypes_.emplace_back(name, schemaFunc);
}

const std::vector<ModuleTypeInfo>& ModuleTypeRegistry::getModuleTypes() const {
    return moduleTypes_;
}

ModuleRegistryInitializer& ModuleRegistryInitializer::init() {
    static ModuleRegistryInitializer initializer_instance; // Renamed to avoid conflict
    return initializer_instance;
}

// Static instance initialization (ensures registration before main)
static ModuleRegistryInitializer& moduleRegistryInit = ModuleRegistryInitializer::init();

// Definitions of functions from main.h
nlohmann::json createRegistryInfo() {
    static auto& _ = ModuleSystem::ModuleRegistryInitializer::init();
    
    nlohmann::json registryInfo;
    registryInfo["modules"] = nlohmann::json::array();
    
    for (const auto& moduleType : ModuleSystem::ModuleTypeRegistry::instance().getModuleTypes()) {
        nlohmann::json moduleInfo;
        moduleInfo["name"] = moduleType.name;
        moduleInfo["enabled"] = true;
        moduleInfo["parameters"] = moduleType.getParamSchemaFunc();
        registryInfo["modules"].push_back(moduleInfo);
    }
    
    return registryInfo;
}

nlohmann::json createengineInfo() {
    nlohmann::json engineInfo;
    engineInfo["enginePool"] = nlohmann::json::array();
    
    // PreGrid引擎
    nlohmann::json preGridEngine;
    preGridEngine["name"] = "PreGrid";
    preGridEngine["description"] = "网格预处理引擎";
    preGridEngine["enabled"] = true;
    preGridEngine["modules"] = nlohmann::json::array();
    preGridEngine["modules"].push_back({
        {"name", "PreCGNS"},
        {"enabled", true}
    });
    preGridEngine["modules"].push_back({
        {"name", "PrePlot3D"},
        {"enabled", false}
    });
    
    // Solve引擎
    nlohmann::json solveEngine;
    solveEngine["name"] = "Solve";
    solveEngine["description"] = "求解引擎";
    solveEngine["enabled"] = true;
    solveEngine["modules"] = nlohmann::json::array();
    solveEngine["modules"].push_back({
        {"name", "EulerSolver"},
        {"enabled", true}
    });
    solveEngine["modules"].push_back({
        {"name", "SASolver"},
        {"enabled", true}
    });
    solveEngine["modules"].push_back({
        {"name", "SSTSolver"},
        {"enabled", false}
    });
    
    // Post引擎
    nlohmann::json postEngine;
    postEngine["name"] = "Post";
    postEngine["description"] = "后处理引擎";
    postEngine["enabled"] = true;
    postEngine["modules"] = nlohmann::json::array();
    postEngine["modules"].push_back({
        {"name", "PostCGNS"},
        {"enabled", true}
    });
    postEngine["modules"].push_back({
        {"name", "PostPlot3D"},
        {"enabled", true}
    });
    
    // 主引擎
    nlohmann::json mainEngine;
    mainEngine["name"] = "mainProcess";
    mainEngine["description"] = "总控制引擎";
    mainEngine["enabled"] = true;
    mainEngine["subenginePool"] = {"PreGrid", "Solve", "Post"};
    
    engineInfo["enginePool"].push_back(preGridEngine);
    engineInfo["enginePool"].push_back(solveEngine);
    engineInfo["enginePool"].push_back(postEngine);
    engineInfo["enginePool"].push_back(mainEngine);
    
    return engineInfo;
}

// 在createengineInfo函数中添加

nlohmann::json createDefaultConfig() {
    nlohmann::json config;
    
    // 添加模块工厂定义
    config["moduleFactories"] = nlohmann::json::array();
    
    // 物理模块工厂
    nlohmann::json physicsFactory;
    physicsFactory["name"] = "physics_factory";
    physicsFactory["modules"] = nlohmann::json::array();
    physicsFactory["modules"].push_back({
        {"name", "FluidSolver"},
        {"enabled", true}
    });
    physicsFactory["modules"].push_back({
        {"name", "ThermalSolver"},
        {"enabled", true}
    });
    
    // 湍流模块工厂
    nlohmann::json turbulenceFactory;
    turbulenceFactory["name"] = "turbulence_factory";
    turbulenceFactory["modules"] = nlohmann::json::array();
    turbulenceFactory["modules"].push_back({
        {"name", "TurbulenceSolverSA"},
        {"enabled", true}
    });
    turbulenceFactory["modules"].push_back({
        {"name", "LaminarSolverEuler"},
        {"enabled", true}
    });
    
    // 添加工厂定义
    config["moduleFactories"].push_back(physicsFactory);
    config["moduleFactories"].push_back(turbulenceFactory);
    
    // 引擎与工厂的绑定
    config["engineFactoryBindings"] = nlohmann::json::array();
    config["engineFactoryBindings"].push_back({
        {"engineName", "engine1"},
        {"factoryName", "physics_factory"}
    });
    config["engineFactoryBindings"].push_back({
        {"engineName", "engine2"},
        {"factoryName", "turbulence_factory"}
    });
    config["engineFactoryBindings"].push_back({
        {"engineName", "engine3"},
        {"factoryName", "physics_factory"}
    });
    config["engineFactoryBindings"].push_back({
        {"engineName", "mainProcess"},
        {"factoryName", "default"}
    });
    
    return config;
}

void getConfigInfo(std::shared_ptr<ModuleSystem::AdvancedRegistry> registry, 
                   std::unique_ptr<ModuleSystem::Nestedengine>& engine, 
                   const std::string& outputFile) {

    // 创建一个有序的 JSON 对象来保证输出顺序
    nlohmann::ordered_json configInfo;

    // 按所需顺序添加节点
    configInfo["config"] = engine->getDefaultConfig();
    
    // 添加模块工厂定义
    nlohmann::json moduleFactories = nlohmann::json::array();
    
    // 创建三个工厂，分别对应不同的模块类型和引擎
    // 1. 预处理工厂 - 对应 PreGrid 引擎
    nlohmann::json preprocessingFactory;
    preprocessingFactory["name"] = "preprocessing_factory";
    preprocessingFactory["modules"] = nlohmann::json::array();
    preprocessingFactory["executionType"] = "CHOOSE_ONE";
    
    preprocessingFactory["modules"].push_back({
         {"name", "PreCGNS"},
         {"enabled", true}});

    preprocessingFactory["modules"].push_back({
        {"name", "PrePlot3D"},
        {"enabled", true}});
    
    // 2. 求解工厂 - 对应 Solve 引擎（修改：包含所有求解器模块）
    nlohmann::json solverFactory;
    solverFactory["name"] = "solver_factory";
    solverFactory["modules"] = nlohmann::json::array();
    solverFactory["executionType"] = "CHOOSE_ONE";
    
    // 添加所有与求解相关的模块到 solver_factory
    solverFactory["modules"].push_back({
         {"name", "EulerSolver"},
         {"enabled", true}});
    
    solverFactory["modules"].push_back({
         {"name", "SASolver"},
         {"enabled", true}});

    solverFactory["modules"].push_back({
        {"name", "SSTSolver"},
        {"enabled", true}});
    
    // 3. 不再单独使用turbulenceFactory（或将其用于其他用途）
    nlohmann::json turbulenceFactory;
    turbulenceFactory["name"] = "turbulence_factory";
    turbulenceFactory["modules"] = nlohmann::json::array();
    turbulenceFactory["executionType"] = "CHOOSE_ONE";
    
    // 可以将一些模块保留在turbulence_factory中用于其他用途
    
    // 4. 后处理工厂 - 对应 Post 引擎
    nlohmann::json postFactory;
    postFactory["name"] = "post_factory";
    postFactory["modules"] = nlohmann::json::array();
    postFactory["executionType"] = "CHOOSE_ONE";

    postFactory["modules"].push_back({
         {"name", "PostCGNS"},
         {"enabled", true}});

    postFactory["modules"].push_back({
        {"name", "PostPlot3D"},
        {"enabled", true}});

    // 添加所有工厂到配置
    moduleFactories.push_back(preprocessingFactory);
    moduleFactories.push_back(solverFactory);
    moduleFactories.push_back(turbulenceFactory);
    moduleFactories.push_back(postFactory);
    
    configInfo["moduleFactories"] = moduleFactories;
    
    // 修改：正确关联引擎与工厂的绑定关系
    nlohmann::json engineBindings = nlohmann::json::array();
    engineBindings.push_back({
         {"engineName", "PreGrid"},
         {"factoryName", "preprocessing_factory"}  // 关联到预处理工厂
     });
     engineBindings.push_back({
         {"engineName", "Solve"},
         {"factoryName", "solver_factory"}  // 关联到求解工厂
     });
     engineBindings.push_back({
         {"engineName", "Post"},
         {"factoryName", "post_factory"}  // 关联到后处理工厂
     });
     engineBindings.push_back({
         {"engineName", "mainProcess"},
         {"factoryName", "default"}
     });
     configInfo["engineFactoryBindings"] = engineBindings;
     
     // 添加引擎和模块注册信息
     configInfo["engine"] = createengineInfo();
     configInfo["registry"] = createRegistryInfo();
     
     // 添加模块默认配置参数
     for (const auto& moduleType : ModuleSystem::ModuleTypeRegistry::instance().getModuleTypes()) {
         nlohmann::json moduleParams = moduleType.getParamSchemaFunc();
         nlohmann::json moduleDefaults;
         for (auto& [paramName, paramInfo] : moduleParams.items()) {
             if (paramInfo.contains("default")) {
                 moduleDefaults[paramName] = paramInfo["default"];
             }
         }
         configInfo["config"][moduleType.name] = moduleDefaults;
     }
     
     // 保存配置文件
     std::ofstream file(outputFile);
     if (!file.is_open()) {
         throw std::runtime_error("无法创建配置文件: " + outputFile);
     }
     file << std::setw(4) << configInfo << std::endl;
     file.close();
     std::cout << "成功写入配置信息到 " << outputFile << std::endl;
}

void saveUsedConfig(const nlohmann::json& moduleConfig, 
                    const std::unordered_set<std::string>& enabledModules, 
                    const nlohmann::json& engine,
                    const nlohmann::json& globalParams,
                    const std::string& outputFile) {
    // 使用nlohmann::ordered_json来保持元素顺序
    nlohmann::ordered_json usedConfig;
    
    // 使用ordered_json来保留配置项的顺序
    usedConfig["config"] = nlohmann::ordered_json::object();
    
    // 先将globalParams按照原始顺序复制到usedConfig中
    for (auto it = globalParams.begin(); it != globalParams.end(); ++it) {
        usedConfig["config"][it.key()] = it.value();
    }
    
    // 然后按照moduleConfig中的顺序添加模块特定的配置
    // 但只添加已启用的模块
    for (auto it = moduleConfig.begin(); it != moduleConfig.end(); ++it) {
        const std::string& moduleName = it.key();
        if (enabledModules.count(moduleName) > 0) {
            usedConfig["config"][moduleName] = it.value();
        }
    }
    
    // 添加模块工厂定义
    if (engine.contains("moduleFactories")) {
        usedConfig["moduleFactories"] = engine["moduleFactories"];
    } 
    
    // 添加引擎与工厂的绑定关系
    if (engine.contains("engineFactoryBindings")) {
        usedConfig["engineFactoryBindings"] = engine["engineFactoryBindings"];
    } 
    
    // 设置engine部分
    usedConfig["engine"] = engine;

    // 设置registry部分
    usedConfig["registry"] = {{"modules", nlohmann::ordered_json::array()}};
    for (const auto& moduleName : enabledModules) {
        // 使用nlohmann::ordered_json代替nlohmann::json
        nlohmann::ordered_json moduleInfo = {
            {"enabled", true},
            {"name", moduleName}
        };
        usedConfig["registry"]["modules"].push_back(moduleInfo);
    }
    
    // 写入文件
    std::ofstream file(outputFile);
    if (!file.is_open()) {
        std::cerr << "无法创建配置文件: " << outputFile << std::endl;
        return;
    }
    file << std::setw(4) << usedConfig << std::endl;
    file.close();
    std::cout << "实际使用的配置已保存到: " << outputFile << std::endl;
}


std::string validateParam(const nlohmann::json& paramSchema, const nlohmann::json& value) {
    if (paramSchema.contains("type")) {
        std::string expectedType = paramSchema["type"];
        if (expectedType == "string") {
            if (!value.is_string()) return "期望字符串类型，但获取到" + value.dump();
            if (paramSchema.contains("enum")) {
                auto enumValues = paramSchema["enum"].get<std::vector<std::string>>();
                if (std::find(enumValues.begin(), enumValues.end(), value.get<std::string>()) == enumValues.end()) {
                    std::string errorMsg = "值必须是以下之一: ";
                    for (size_t i = 0; i < enumValues.size(); ++i) {
                        errorMsg += "\"" + enumValues[i] + "\"";
                        if (i < enumValues.size() - 1) errorMsg += ", ";
                    }
                    errorMsg += "，但获取到\"" + value.get<std::string>() + "\"";
                    return errorMsg;
                }
            }
        } else if (expectedType == "number") {
            if (!value.is_number()) return "期望数值类型，但获取到" + value.dump();
            double numValue = value.get<double>();
            if (paramSchema.contains("minimum") && numValue < paramSchema["minimum"].get<double>()) {
                return "值必须大于等于" + std::to_string(paramSchema["minimum"].get<double>()) + 
                       "，但获取到" + std::to_string(numValue);
            }
            if (paramSchema.contains("maximum") && numValue > paramSchema["maximum"].get<double>()) {
                return "值必须小于等于" + std::to_string(paramSchema["maximum"].get<double>()) + 
                       "，但获取到" + std::to_string(numValue);
            }
        } else if (expectedType == "boolean" && !value.is_boolean()) {
            return "期望布尔类型，但获取到" + value.dump();
        } else if (expectedType == "array" && !value.is_array()) {
            return "期望数组类型，但获取到" + value.dump();
        } else if (expectedType == "object" && !value.is_object()) {
            return "期望对象类型，但获取到" + value.dump();
        }
    }
    return "";
}

void validateModuleParams(const nlohmann::json& moduleParams, 
                          const std::string& moduleName, 
                          int rank) {
    nlohmann::json paramSchema;
    bool schemaFound = false;
    for (const auto& moduleType : ModuleSystem::ModuleTypeRegistry::instance().getModuleTypes()) {
        if (moduleType.name == moduleName) {
            paramSchema = moduleType.getParamSchemaFunc();
            schemaFound = true;
            break;
        }
    }
    if (!schemaFound) throw std::runtime_error("未找到模块 " + moduleName + " 的参数架构");

    for (auto& [paramName, paramValue] : moduleParams.items()) {
        if (!paramSchema.contains(paramName)) {
            throw std::runtime_error("模块 " + moduleName + " 不支持参数 '" + paramName + "'");
        }
        std::string errorMsg = validateParam(paramSchema[paramName], paramValue);
        if (!errorMsg.empty()) {
            throw std::runtime_error("模块 " + moduleName + " 的参数 '" + paramName + "' 无效: " + errorMsg);
        }
    }
    for (auto& [paramName, paramInfo] : paramSchema.items()) {
        if (!moduleParams.contains(paramName) && paramInfo.contains("required") && 
            paramInfo["required"].get<bool>()) {
            throw std::runtime_error("模块 " + moduleName + " 缺少必需参数 '" + paramName + "'");
        }
    }
}

// engineExecutionengine method definitions
engineExecutionengine::engineExecutionengine(const nlohmann::json& engines, 
                                             ModuleSystem::engineContext& context)
    : engines_(engines), context_(context) {}

bool engineExecutionengine::executeengine(const std::string& engineName) {
    if (visitedengines_.count(engineName)) {
        std::cerr << "错误: 检测到工作流循环依赖: " << engineName << std::endl;
        return false;
    }
    
    // 查找引擎定义
    auto it = std::find_if(engines_["enginePool"].begin(), engines_["enginePool"].end(), 
        [engineName](const nlohmann::json& eng) { 
            return eng["name"] == engineName && eng["enabled"].get<bool>(); 
        });
    
    if (it == engines_["enginePool"].end()) {
        std::cerr << "错误: 引擎 '" << engineName << "' 未找到或未启用" << std::endl;
        return false;
    }
    
    const nlohmann::json& engineDef = *it;
    
    visitedengines_.insert(engineName);
    std::cout << "开始 " << engineName << std::endl;
    
    // 处理子引擎
    if (engineDef.contains("subenginePool") && engineDef["subenginePool"].is_array()) {
        for (const auto& subEngineName : engineDef["subenginePool"]) {
            if (!executeengine(subEngineName.get<std::string>())) {
                std::cout << "警告: 子引擎 '" << subEngineName.get<std::string>() << "' 执行失败" << std::endl;
            }
        }
    }
    
     // 处理模块
    if (engineDef.contains("modules") && engineDef["modules"].is_array()) {
        for (const auto& moduleInfo : engineDef["modules"]) {
            if (!moduleInfo.contains("name")) {
                std::cerr << "错误: 模块定义缺少名称" << std::endl;
                return false;
            }
            
            const std::string& moduleName = moduleInfo["name"];
            bool moduleEnabled = moduleInfo.value("enabled", true);
            
            // 如果模块被禁用，则跳过
            if (!moduleEnabled) {
                std::cout << "跳过已禁用模块 '" << moduleName << "'" << std::endl;
                continue;
            }
            
            try {
                // 创建和执行模块
                std::cout << "创建模块: " << moduleName << std::endl;
                
                // 获取模块参数 - 使用之前定义的工具函数获取有效参数
                nlohmann::json moduleParams = getEffectiveModuleParams(
                    context_.getParameter<nlohmann::json>("config"), 
                    moduleName,
                    moduleInfo.contains("params") ? moduleInfo["params"] : nlohmann::json(nullptr)
                );
                
                void* moduleInstance = context_.createModule(moduleName, moduleParams);
                
                // 执行固定的四个动作：create, initialize, execute, release
                // (create 已经在上面执行)
                std::cout << "初始化模块: " << moduleName << std::endl;
                context_.initializeModule(moduleName);
                
                std::cout << "执行模块: " << moduleName << std::endl;
                context_.executeModule(moduleName);
                
                std::cout << "释放模块: " << moduleName << std::endl;
                context_.releaseModule(moduleName);
            } catch (const std::exception& e) {
                std::cerr << "错误: " << e.what() << std::endl;
                return false;
            }
        }
    }
    



    std::cout << "完成 " << engineName << std::endl;
    visitedengines_.erase(engineName);
    return true;
}

nlohmann::json getEffectiveModuleParams(
    const nlohmann::json& moduleConfig, 
    const std::string& moduleName,
    const nlohmann::json& engineSpecificParams) {
    nlohmann::json effectiveParams;
    if (moduleConfig.contains(moduleName)) {
        effectiveParams = moduleConfig[moduleName];
    }
    if (!engineSpecificParams.is_null()) {
        for (auto& [paramName, paramValue] : engineSpecificParams.items()) {
            effectiveParams[paramName] = paramValue;
        }
    }
    return effectiveParams;
}

// 实现 ModuleFactoryCollection::initializeFactoriesFromConfig
void ModuleFactoryCollection::initializeFactoriesFromConfig(const nlohmann::json& config) {
    if (!config.contains("moduleFactories") || !config["moduleFactories"].is_array()) {
        return;
    }
    
    // 清除现有工厂（保留默认工厂）
    for (auto it = factories_.begin(); it != factories_.end();) {
        if (it->first != defaultFactoryName_) {
            it = factories_.erase(it);
        } else {
            ++it;
        }
    }
    
    // 从配置创建工厂
    for (const auto& factory : config["moduleFactories"]) {
        if (!factory.contains("name") || !factory.contains("modules")) {
            continue;
        }
        
        std::string factoryName = factory["name"];
        auto factoryPtr = getFactory(factoryName);
        
        // 添加模块到工厂
        for (const auto& module : factory["modules"]) {
            if (module.contains("name") && module.contains("enabled") && module["enabled"].get<bool>()) {
                std::string moduleName = module["name"];
                addModuleToFactory(factoryName, moduleName);
            }
        }
    }
}

// 初始化静态工厂池
void ModuleFactoryCollection::initializeStaticFactoryPool(const nlohmann::json& factoryPool) {
    staticFactoryPool_ = factoryPool;
    factoryExecutionConfigs_.clear();
    
    // 解析每个工厂的执行逻辑
    for (const auto& factory : factoryPool) {
        if (factory.contains("name") && factory.contains("executionType")) {
            std::string factoryName = factory["name"];
            std::string executionTypeStr = factory["executionType"];
            
            FactoryExecutionConfig config;
            
            if (executionTypeStr == "CHOOSE_ONE") {
                config.type = FactoryExecutionType::CHOOSE_ONE;
            } else if (executionTypeStr == "SEQUENTIAL_AND_CHOOSE") {
                config.type = FactoryExecutionType::SEQUENTIAL_AND_CHOOSE;
                config.firstModule = factory.value("firstModule", "");
                
                if (factory.contains("remainingModules") && factory["remainingModules"].is_array()) {
                    for (const auto& module : factory["remainingModules"]) {
                        config.remainingModules.push_back(module);
                    }
                }
            } else if (executionTypeStr == "CHOOSE_ONE_AGAIN") {
                config.type = FactoryExecutionType::CHOOSE_ONE_AGAIN;
            } else {
                // 默认为CHOOSE_ONE
                config.type = FactoryExecutionType::CHOOSE_ONE;
            }
            
            factoryExecutionConfigs_[factoryName] = config;
        }
    }
}

// 获取静态工厂执行配置
FactoryExecutionConfig ModuleFactoryCollection::getFactoryExecutionConfig(const std::string& factoryName) {
    auto it = factoryExecutionConfigs_.find(factoryName);
    if (it != factoryExecutionConfigs_.end()) {
        return it->second;
    }
    
    // 默认返回CHOOSE_ONE类型的配置
    FactoryExecutionConfig defaultConfig;
    defaultConfig.type = FactoryExecutionType::CHOOSE_ONE;
    return defaultConfig;
}

void runWithConfig(const nlohmann::json& config, const std::string& outputFile) {
    std::string outputConfigFile = outputFile.empty() ? "config_.json" : outputFile;

    auto registry = std::make_shared<ModuleSystem::AdvancedRegistry>();
    auto engine = std::make_unique<ModuleSystem::Nestedengine>(*registry);

    nlohmann::json moduleConfig;
    nlohmann::json globalParams;
    
    // 定义已知模块集合
    std::unordered_set<std::string> knownModules;
    for (const auto& moduleType : ModuleSystem::ModuleTypeRegistry::instance().getModuleTypes()) {
        knownModules.insert(moduleType.name);
    }
    try {
        // 1. 初始化模块工厂集合
        ModuleFactoryCollection::instance().initializeFactoriesFromConfig(config);
        
        // 2. 初始化引擎与工厂的绑定关系
        engine->initializeEngineFactoryBindings(config);
                // 在注册模块之前验证引擎和工厂绑定与使用的模块的兼容性
        if (config.contains("engineFactoryBindings") && config.contains("moduleFactories")) {
            // 创建工厂到模块的映射
            std::unordered_map<std::string, std::unordered_set<std::string>> factoryToModules;
            
            // 首先收集每个工厂中包含的模块
            for (const auto& factory : config["moduleFactories"]) {
                if (!factory.contains("name") || !factory.contains("modules")) continue;
                
                std::string factoryName = factory["name"];
                std::unordered_set<std::string> modules;
                
                for (const auto& module : factory["modules"]) {
                    if (!module.contains("name") || !module.contains("enabled")) continue;
                    if (module["enabled"]) {
                        modules.insert(module["name"]);
                    }
                }
                
                factoryToModules[factoryName] = modules;
            }
            
            // 收集引擎到工厂的映射
            std::unordered_map<std::string, std::string> engineToFactory;
            for (const auto& binding : config["engineFactoryBindings"]) {
                if (!binding.contains("engineName") || !binding.contains("factoryName")) continue;
                
                engineToFactory[binding["engineName"]] = binding["factoryName"];
            }
            
            // 检查每个引擎使用的模块是否属于其绑定的工厂
            if (config["engine"].contains("enginePool")) {
                for (const auto& engineDef : config["engine"]["enginePool"]) {
                    if (!engineDef.contains("name") || !engineDef["enabled"].get<bool>()) continue;
                    
                    std::string engineName = engineDef["name"];
                    std::string boundFactoryName = "default"; // 默认工厂
                    
                    // 获取引擎绑定的工厂
                    if (engineToFactory.count(engineName)) {
                        boundFactoryName = engineToFactory[engineName];
                    }
                    
                    // 检查引擎使用的模块
                    if (engineDef.contains("modules") && engineDef["modules"].is_array()) {
                        for (const auto& moduleInfo : engineDef["modules"]) {
                            if (!moduleInfo.contains("name") || !moduleInfo.contains("enabled")) continue;
                            if (moduleInfo["enabled"]) {
                                std::string moduleName = moduleInfo["name"];
                                
                                // 如果工厂是默认工厂"default"，任何模块都可以使用
                                if (boundFactoryName != "default") {
                                    // 检查模块是否在绑定的工厂中
                                    if (!factoryToModules[boundFactoryName].count(moduleName)) {
                                        std::cerr << "错误: 引擎 '" << engineName 
                                                  << "' 尝试使用不属于其绑定工厂 '" << boundFactoryName 
                                                  << "' 的模块 '" << moduleName << "'" << std::endl;
                                        // 退出程序或抛出异常
                                        throw std::runtime_error("引擎与工厂绑定检查失败");
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
        // 验证配置中的工厂定义有效性
        if (config.contains("moduleFactories")) {
            std::unordered_set<std::string> factoryNames;
            for (const auto& factory : config["moduleFactories"]) {
                if (!factory.contains("name")) {
                    throw std::runtime_error("工厂定义错误: 缺少工厂名称");
                }
                
                std::string factoryName = factory["name"];
                if (!factoryNames.insert(factoryName).second) {
                    throw std::runtime_error("工厂定义错误: 发现重复的工厂名称 '" + factoryName + "'");
                }
                
                // 验证工厂中的模块是否是已知模块
                if (factory.contains("modules")) {
                    for (const auto& module : factory["modules"]) {
                        if (!module.contains("name")) {
                            throw std::runtime_error("模块定义错误: 工厂 '" + factoryName + "' 中的模块缺少名称");
                        }
                        
                        std::string moduleName = module["name"];
                        if (knownModules.find(moduleName) == knownModules.end()) {
                            throw std::runtime_error("工厂定义错误: 工厂 '" + factoryName + "' 中包含未知模块 '" + moduleName + "'");
                        }
                    }
                }
            }
            
            // 验证引擎与工厂绑定是否有效
            if (config.contains("engineFactoryBindings")) {
                for (const auto& binding : config["engineFactoryBindings"]) {
                    if (!binding.contains("engineName") || !binding.contains("factoryName")) {
                        throw std::runtime_error("引擎工厂绑定错误: 缺少引擎名称或工厂名称");
                    }
                    
                    std::string factoryName = binding["factoryName"];
                    if (factoryName != "default" && factoryNames.find(factoryName) == factoryNames.end()) {
                        throw std::runtime_error("引擎工厂绑定错误: 引用了未定义的工厂 '" + factoryName + "'");
                    }
                    
                    std::string engineName = binding["engineName"];
                    bool engineFound = false;
                    if (config.contains("engine") && config["engine"].contains("enginePool")) {
                        for (const auto& eng : config["engine"]["enginePool"]) {
                            if (eng.contains("name") && eng["name"] == engineName) {
                                engineFound = true;
                                break;
                            }
                        }
                    }
                    
                    if (!engineFound) {
                        throw std::runtime_error("引擎工厂绑定错误: 引用了未定义的引擎 '" + engineName + "'");
                    }
                }
            }
        }
        // 创建工厂到模块的映射
        std::unordered_map<std::string, std::unordered_set<std::string>> factoryToModules;
        
        // 首先收集每个工厂中包含的模块
        for (const auto& factory : config["moduleFactories"]) {
            if (!factory.contains("name") || !factory.contains("modules")) continue;
            
            std::string factoryName = factory["name"];
            std::unordered_set<std::string> modules;
            
            for (const auto& module : factory["modules"]) {
                if (!module.contains("name") || !module.contains("enabled")) continue;
                if (module["enabled"]) {
                    modules.insert(module["name"]);
                }
            }
            
            factoryToModules[factoryName] = modules;
        }
        
        // 收集引擎到工厂的映射
        std::unordered_map<std::string, std::string> engineToFactory;
        for (const auto& binding : config["engineFactoryBindings"]) {
            if (!binding.contains("engineName") || !binding.contains("factoryName")) continue;
            
            engineToFactory[binding["engineName"]] = binding["factoryName"];
        }
        
        // 检查每个引擎使用的模块是否属于其绑定的工厂
        if (config["engine"].contains("enginePool")) {
            for (const auto& engineDef : config["engine"]["enginePool"]) {
                if (!engineDef.contains("name") || !engineDef["enabled"].get<bool>()) continue;
                
                std::string engineName = engineDef["name"];
                std::string boundFactoryName = "default"; // 默认工厂
                
                // 获取引擎绑定的工厂
                if (engineToFactory.count(engineName)) {
                    boundFactoryName = engineToFactory[engineName];
                }
                
                // 检查引擎使用的模块
                if (engineDef.contains("modules") && engineDef["modules"].is_array()) {
                    for (const auto& moduleInfo : engineDef["modules"]) {
                        if (!moduleInfo.contains("name") || !moduleInfo["enabled"].get<bool>()) continue;
                        
                        std::string moduleName = moduleInfo["name"];
                        
                        // 如果工厂是默认工厂"default"，任何模块都可以使用
                        if (boundFactoryName != "default") {
                            // 检查模块是否在绑定的工厂中
                            if (!factoryToModules[boundFactoryName].count(moduleName)) {
                                std::cerr << "错误: 引擎 '" << engineName 
                                            << "' 尝试使用不属于其绑定工厂 '" << boundFactoryName 
                                            << "' 的模块 '" << moduleName << "'" << std::endl;
                                // 退出程序或抛出异常
                                throw std::runtime_error("引擎与工厂绑定检查失败");
                            }
                        }
                    }
                }
            }
        }

        if (config.contains("config")) {
            for (auto& [key, value] : config["config"].items()) {
                if (value.is_object()) moduleConfig[key] = value;
                else globalParams[key] = value;
            }
        }

        std::unordered_set<std::string> knownModules;
        for (const auto& moduleType : ModuleSystem::ModuleTypeRegistry::instance().getModuleTypes()) {
            knownModules.insert(moduleType.name);
        }

        std::unordered_set<std::string> moduleNamesCheck;
        if (config["registry"].contains("modules")) {
            for (const auto& module : config["registry"]["modules"]) {
                if (module.contains("name")) {
                    std::string moduleName = module["name"];
                    if (!moduleNamesCheck.insert(moduleName).second) { // 检查重复
                        std::cerr << "错误: 发现重复的模块名称: " << moduleName << std::endl;
                        return;
                    }
                    if (knownModules.find(moduleName) == knownModules.end()) {
                        std::cerr << "错误: 配置文件包含未知模块: " << moduleName << std::endl;
                        return;
                    }
                }
            }
        }

        std::unordered_set<std::string> enabledModules;
        if (config["registry"].contains("modules")) {
            for (const auto& module : config["registry"]["modules"]) {
                if (module.contains("name") && module.contains("enabled")) {
                    std::string moduleName = module["name"].get<std::string>();
                    bool isEnabled = module["enabled"].get<bool>();
                    if (knownModules.count(moduleName) && isEnabled) {
                        enabledModules.insert(moduleName);
                    } else if (!isEnabled) {
                        std::cout << "模块 " << moduleName << " 已禁用，不会被注册" << std::endl;
                    }
                }
            }
        }
        
        std::unordered_set<std::string> EngineNamesCheck;
        std::function<void(const nlohmann::json&)> collectEngineNames = 
            [&](const nlohmann::json& Engines) {
            if (!Engines.contains("enginePool")) return;
            for (const auto& Engine : Engines["enginePool"]) {
                if (!Engine.contains("name")) continue;
                std::string EngineName = Engine["name"].get<std::string>();
                if (!EngineNamesCheck.insert(EngineName).second) {
                    std::cerr << "错误: 发现重复的引擎名称: " << EngineName << std::endl;
                    return;
                }
            }
        };
        
        collectEngineNames(config["engine"]);
        if (EngineNamesCheck.empty()) {
            std::cerr << "错误: 配置中未找到有效的引擎定义" << std::endl;
            return;
        }

        std::unordered_set<std::string> validEnginePool;
        if (config["engine"].contains("enginePool")) {
            for (const auto& Engine : config["engine"]["enginePool"]) {
                if (Engine.contains("name") && Engine["enabled"].get<bool>()) {
                    std::string EngineName = Engine["name"].get<std::string>();
                    validEnginePool.insert(EngineName);
                    
                    if (Engine.contains("subEnginePool")) {
                        for (const auto& subEngine : Engine["subEnginePool"]) {
                            std::string subEngineName = subEngine.get<std::string>();
                            bool subEngineFound = false;
                            for (const auto& otherEngine : config["Engine"]["EnginePool"]) {
                                if (otherEngine.contains("name") && otherEngine["name"] == subEngineName) {
                                    subEngineFound = true; break;
                                }
                            }
                            if (!subEngineFound) {
                                std::cerr << "错误: 引擎 '" << EngineName << "' 引用了未定义的子引擎 '" << subEngineName << "'" << std::endl;
                                return;
                            }
                        }
                    }
                    
                    if (Engine.contains("modules") && Engine["modules"].is_array()) {
                        for (const auto& moduleInfo : Engine["modules"]) {
                            if (!moduleInfo.contains("name")) {
                                std::cerr << "错误: 引擎 '" << EngineName << "' 包含没有名称的模块定义" << std::endl;
                                return;
                            }
                            
                            // 如果模块禁用，则跳过对它的验证
                            if (moduleInfo.contains("enabled") && !moduleInfo["enabled"].get<bool>()) {
                                continue;
                            }
                            
                            const std::string& moduleName = moduleInfo["name"];
                            if (enabledModules.find(moduleName) == enabledModules.end()) {
                                std::cerr << "错误: 引擎 '" << EngineName 
                                    << "' 使用了未启用或未知的模块 '" << moduleName << "'" << std::endl;
                                return;
                            }
                        }
                    }
                }
            }
        } else {
            std::cerr << "错误: 配置中未找到 'EnginePool' 定义" << std::endl;
            return;
        }

        // 检测循环依赖
        std::unordered_map<std::string, std::vector<std::string>> engineDependencies;
        
        // 构建依赖图
        for (const auto& engineDef : config["engine"]["enginePool"]) {
            if (engineDef.contains("name") && engineDef.contains("subenginePool")) {
                std::string engineName = engineDef["name"];
                for (const auto& subengine : engineDef["subenginePool"]) {
                    engineDependencies[engineName].push_back(subengine);
                }
            }
        }
        
        std::function<bool(const std::string&, std::unordered_set<std::string>&, std::unordered_set<std::string>&)> 
        detectCycle = [&](const std::string& current, 
                        std::unordered_set<std::string>& visited, 
                        std::unordered_set<std::string>& recStack) -> bool {
            visited.insert(current);
            recStack.insert(current);
            
            for (const auto& dep : engineDependencies[current]) {
                if (!visited.count(dep)) {
                    if (detectCycle(dep, visited, recStack)) {
                        std::cerr << "发现循环依赖: " << current << " -> " << dep << std::endl;
                        return true;
                    }
                } else if (recStack.count(dep)) {
                    std::cerr << "发现循环依赖: " << current << " -> " << dep << std::endl;
                    return true;
                }
            }
            
            recStack.erase(current);
            return false;
        };
        
        // 遍历所有引擎检测循环
        std::unordered_set<std::string> visited, recStack;
        for (const auto& [engine, _] : engineDependencies) {
            if (!visited.count(engine)) {
                if (detectCycle(engine, visited, recStack)) {
                    return;
                }
            }
        }
        
        // 检查mainProcess引擎是否存在且启用
        bool hasMainProcess = false;
        if (config["engine"].contains("enginePool")) {
            for (const auto& Engine : config["engine"]["enginePool"]) {
                if (Engine.contains("name") && Engine["name"] == "mainProcess" && Engine["enabled"].get<bool>()) {
                    hasMainProcess = true; break;
                }
            }
        }
        
        if (!hasMainProcess) {
            std::cerr << "错误: 主引擎 'mainProcess' 不存在或已禁用" << std::endl;
            return;
        }
        
        // 保存实际使用的配置
        std::cout << "开始执行引擎..." << std::endl;
        
        // 收集所有启用的模块
        std::unordered_set<std::string> enabledModulesForSave;
        if (config["registry"].contains("modules")) {
            for (const auto& module : config["registry"]["modules"]) {
                if (module.contains("name") && module.contains("enabled") && module["enabled"].get<bool>()) {
                    enabledModulesForSave.insert(module["name"].get<std::string>());
                }
            }
        }
        
        // 在引擎定义前收集需要的引擎名称
        std::unordered_set<std::string> usedEngineNames;
        if (config["engine"].contains("enginePool")) {
            for (const auto& Engine : config["engine"]["enginePool"]) {
                if (!Engine.contains("name") || !Engine["enabled"].get<bool>()) continue;
                usedEngineNames.insert(Engine["name"].get<std::string>());
            }
        }
        
        // 根据使用的引擎名称和绑定关系，确定需要加载的模块工厂
        // 把requiredFactories作为引用的引擎成员变量，执行config的时候把工厂和配置参数
        std::unordered_set<std::string> requiredFactories;
        for (const auto& engineName : usedEngineNames) {
            std::string factoryName = engine->getEngineFactoryBinding(engineName);
            requiredFactories.insert(factoryName);
        }
        
        // 对每个需要的工厂，为registry注册相应模块
        for (const auto& factoryName : requiredFactories) {
            auto factory = ModuleFactoryCollection::instance().getFactory(factoryName);
            
            // 只注册这个工厂中包含且在enabledModules中的模块
            for (const auto& moduleName : enabledModules) {
                // 如果模块在当前工厂中注册，则添加到registry
                if (factory->registerModule(registry.get(), moduleName)) {
                    std::cout << "模块 " << moduleName << " 已从工厂 " << factoryName << " 注册" << std::endl;
                }
            }
        }
        
        saveUsedConfig(moduleConfig, enabledModulesForSave, config["engine"], globalParams, outputConfigFile);
    
        // 在这之后定义所有引擎
        if (config["engine"].contains("enginePool")) {
            for (const auto& EngineDef : config["engine"]["enginePool"]) {
                if (!EngineDef.contains("name") || !EngineDef["enabled"].get<bool>()) continue;
                std::string EngineName = EngineDef["name"].get<std::string>();

                engine->defineengine(EngineName, 
                    [EngineDef, EngineName, &config, &engine, &moduleConfig](ModuleSystem::engineContext& context) {
                    std::cout << "开始 " << EngineName << std::endl;

                    if (EngineDef.contains("subenginePool")) {
                        for (const auto& subEngineJson : EngineDef["subenginePool"]) {
                            std::string subEngineName = subEngineJson.get<std::string>();
                            
                            // 检查子引擎是否存在且是否被禁用
                            bool engineExists = false;
                            bool engineEnabled = false;
                            
                            // 查找子引擎定义
                            for (const auto& engDef : config["engine"]["enginePool"]) {
                                if (engDef.contains("name") && engDef["name"] == subEngineName) {
                                    engineExists = true;
                                    engineEnabled = engDef.value("enabled", true); // 默认为启用
                                    break;
                                }
                            }
                            
                            // 如果引擎不存在，输出警告并继续
                            if (!engineExists) {
                                std::cerr << "错误: 子引擎 '" << subEngineName << "' 未在配置中定义" << std::endl;
                                exit(1);
                            }
                            
                            // 如果引擎被禁用，输出警告并继续
                            if (!engineEnabled) {
                                std::cout << "警告: 子引擎 '" << subEngineName << "' 已禁用，将跳过此引擎" << std::endl;
                                continue;
                            }
                            
                            try {
                                engine->executeengine(subEngineName, context.shared_from_this());
                            } catch (const std::exception& e) {
                                std::cerr << "错误: 执行子引擎 '" << subEngineName << "' 时出错: " << e.what() << std::endl;
                                exit(1);
                            }
                        }
                    }

                    // 处理模块执行
                    if (EngineDef.contains("modules") && EngineDef["modules"].is_array()) {
                        // 按数组顺序执行模块
                        for (const auto& moduleInfo : EngineDef["modules"]) {
                            const std::string& moduleName = moduleInfo["name"];
                            bool moduleEnabled = moduleInfo.value("enabled", true); // 默认启用
                            
                            // 如果模块禁用，则跳过
                            if (!moduleEnabled) {
                                std::cout << "跳过禁用的模块 '" << moduleName << "'" << std::endl;
                                continue;
                            }
                            
                            try {
                                // 为模块准备参数
                                nlohmann::json params = getEffectiveModuleParams(moduleConfig, moduleName, moduleInfo.contains("params") ? moduleInfo["params"] : nlohmann::json(nullptr));
                                
                                //把四个动作拆开，分4个引擎来调用同一类型的模块函数

                                // 固定的模块生命周期：创建、初始化、执行、释放
                                std::cout << "创建模块: " << moduleName << std::endl;
                                context.createModule(moduleName, params);
                                
                                std::cout << "初始化模块: " << moduleName << std::endl;
                                context.initializeModule(moduleName);
                                
                                std::cout << "执行模块: " << moduleName << std::endl;
                                context.executeModule(moduleName);
                                
                                std::cout << "释放模块: " << moduleName << std::endl;
                                context.releaseModule(moduleName);
                            } catch (const std::exception& e) {
                                std::cerr << "错误: 模块 '" << moduleName << "' 执行过程中出错: " << e.what() << std::endl;
                                return;
                            }
                        }
                    }
                    std::cout << "完成 " << EngineName << std::endl;
                });
            }
        }

        if (config.contains("config")) engine->Build(config["config"]); // 使用"config"部分进行Build

        // 创建主上下文
        auto mainContext = std::make_shared<ModuleSystem::engineContext>(*registry, engine.get());
        if (config.contains("config")) {
            mainContext->setParameter("config", config["config"]); // 设置整个config块
            for (auto& [key, value] : config["config"].items()) { // 以及单独的参数
                if (!value.is_object()) mainContext->setParameter(key, value);
            }
            for (auto& [moduleName, params] : moduleConfig.items()) { // 以及模块特定的参数
                mainContext->setParameter(moduleName, params);
            }
        }

        // 执行主引擎
        try {
            engine->executeengine("mainProcess", mainContext);
            
            // 检查未释放的模块
            auto leakedModules = registry->checkLeakedModules();
            if (!leakedModules.empty()) {
                std::cerr << "错误: 发现未释放的模块:" << std::endl;
                for (const auto& module : leakedModules) {
                    std::cerr << "  - " << module << std::endl;
                }
                std::cerr << "所有模块必须在程序结束前释放。请检查'release'工作流是否正确配置和执行。" << std::endl;
                return;
            }
            
            std::cout << "引擎执行完成" << std::endl;
        } catch (const std::exception& e) {
            std::cerr << "错误：" << e.what() << std::endl;
            return;
        }

        std::cout << "成功完成引擎执行" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "验证配置时发生错误: " << e.what() << std::endl;
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 模块函数定义
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////

PreCGNS::PreCGNS(const nlohmann::json& params) {
    cgns_type_ = params.value("cgns_type", "HDF5");
    cgns_value_ = params.value("cgns_value", 15);
}

void PreCGNS::initialize() {
}

void PreCGNS::execute() {
}

void PreCGNS::release() {
}

nlohmann::json PreCGNS::GetParamSchema() {
    return {
        {"cgns_type", {
            {"type", "string"},
            {"description", "Type of cgns file"},
            {"enum", {"HDF5", "ADF"}},
            {"default", "HDF5"}
        }},
        {"cgns_value", {
            {"type", "number"},
            {"description", "Number of cgns value"},
            {"minimum", 1},
            {"maximum", 100},
            {"default", 10}
        }}
    };
}

nlohmann::json ModuleParamTraits<PreCGNS>::GetParamSchema() {
    return PreCGNS::GetParamSchema();
}

// PrePlot3D 模块
PrePlot3D::PrePlot3D(const nlohmann::json& params) {
    plot3_type_ = params.value("plot3_type", "ASCII");
    plot3d_value_ = params.value("plot3_value", 30);
}

void PrePlot3D::initialize() {
}

void PrePlot3D::execute() {
}

void PrePlot3D::release() {
}

nlohmann::json ModuleSystem::PrePlot3D::GetParamSchema() {
    return {
        {"plot3_type", {
            {"type", "string"},
            {"description", "Type of plot3 file"},
            {"enum", {"ASCII", "Binary"}},
            {"default", "ASCII"}
        }},
        {"plot3_value", {
            {"type", "number"},
            {"description", "Number of plot3 value"},
            {"minimum", 1},
            {"maximum", 100},
            {"default", 10}
        }}
    };
}

nlohmann::json ModuleSystem::ModuleParamTraits<ModuleSystem::PrePlot3D>::GetParamSchema() {
    return PrePlot3D::GetParamSchema();
}

// EulerSolver 模块
EulerSolver::EulerSolver(const nlohmann::json& params) {
    euler_type_ = params.value("euler_type", "Standard");
    euler_value__ = params.value("euler_value", 0.5);
}

void EulerSolver::initialize() {
}

void EulerSolver::execute() {
}

void EulerSolver::release() {
}

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

nlohmann::json ModuleParamTraits<EulerSolver>::GetParamSchema() {
    return EulerSolver::GetParamSchema();
}

// SASolver 模块
SASolver::SASolver(const nlohmann::json& params) {
    sa_type_ = params.value("solver_type", "Standard");
    convergence_criteria_ = params.value("convergence_criteria", 1e-6);
    max_iterations_ = params.value("max_iterations", 1000);
}

void SASolver::initialize() {
}

void SASolver::execute() {
}

void SASolver::release() {
}

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

nlohmann::json ModuleParamTraits<SASolver>::GetParamSchema() {
    return SASolver::GetParamSchema();
}

// SSTSolver 模块
SSTSolver::SSTSolver(const nlohmann::json& params) {
    sst_type_ = params.value("solver_type", "Standard");
    convergence_criteria_ = params.value("convergence_criteria", 1e-6);
    max_iterations_ = params.value("max_iterations", 1000);
}

void SSTSolver::initialize() {
}

void SSTSolver::execute() {
}

void SSTSolver::release() {
}

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

nlohmann::json ModuleParamTraits<SSTSolver>::GetParamSchema() {
    return SSTSolver::GetParamSchema();
}

// PostCGNS 模块
PostCGNS::PostCGNS(const nlohmann::json& params) {
    cgns_type_ = params.value("cgns_type", "HDF5");
    cgns_value_ = params.value("cgns_value", 15);
}

void PostCGNS::initialize() {
}

void PostCGNS::execute() {
}

void PostCGNS::release() {
}

nlohmann::json PostCGNS::GetParamSchema() {
    return {
        {"cgns_type", {
            {"type", "string"},
            {"description", "Type of cgns file"},
            {"enum", {"HDF5", "ADF"}},
            {"default", "HDF5"}
        }},
        {"cgns_value", {
            {"type", "number"},
            {"description", "Number of cgns value"},
            {"minimum", 1},
            {"maximum", 100},
            {"default", 10}
        }}
    };
}

nlohmann::json ModuleParamTraits<PostCGNS>::GetParamSchema() {
    return PostCGNS::GetParamSchema();
}

// PostPlot3D 模块
PostPlot3D::PostPlot3D(const nlohmann::json& params) {
    plot3d_type_ = params.value("plot3_type", "ASCII");
    plot3d_value_ = params.value("plot3_value", 30);
}

void PostPlot3D::initialize() {
}

void PostPlot3D::execute() {
}

void PostPlot3D::release() {
}

nlohmann::json PostPlot3D::GetParamSchema() {
    return {
        {"cgns_type", {
            {"type", "string"},
            {"description", "Type of plot3 file"},
            {"enum", {"ASCII", "Binary"}},
            {"default", "ASCII"}
        }},
        {"cgns_value", {
            {"type", "number"},
            {"description", "Number of plot3 value"},
            {"minimum", 1},
            {"maximum", 100},
            {"default", 30}
        }}
    };
}

nlohmann::json ModuleParamTraits<PostPlot3D>::GetParamSchema() {
    return PostPlot3D::GetParamSchema();
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 模块注册
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////

ModuleRegistryInitializer::ModuleRegistryInitializer() {
    ModuleTypeRegistry::instance().registerType(
        "PreCGNS", 
        []() -> nlohmann::json { return PreCGNS::GetParamSchema(); }
    );
    
    ModuleTypeRegistry::instance().registerType(
        "PrePlot3D", 
        []() -> nlohmann::json { return PrePlot3D::GetParamSchema(); }
    );
    
    ModuleTypeRegistry::instance().registerType(

        "EulerSolver", 
        []() -> nlohmann::json { return EulerSolver::GetParamSchema(); }
    );

    ModuleTypeRegistry::instance().registerType(
        "SASolver", 
        []() -> nlohmann::json { return SASolver::GetParamSchema(); }
    );

    ModuleTypeRegistry::instance().registerType(
        "SSTSolver", 
        []() -> nlohmann::json { return SSTSolver::GetParamSchema(); }
    );

    ModuleTypeRegistry::instance().registerType(
        "PostCGNS", 
        []() -> nlohmann::json { return PostCGNS::GetParamSchema(); }
    );

    ModuleTypeRegistry::instance().registerType(
        "PostPlot3D", 
        []() -> nlohmann::json { return PostPlot3D::GetParamSchema(); }
    );
}

// 初始化模块工厂，注册所有模块类型
void ModuleFactoryInitializer::init() {
    ModuleFactory& factory = ModuleFactory::instance();
    
    factory.registerModuleType("PreCGNS", 
        [](AdvancedRegistry* reg, const std::string& name) -> bool { 
            reg->Register<PreCGNS>(name);
            return true;
        });
    
    factory.registerModuleType("PrePlot3D", 
        [](AdvancedRegistry* reg, const std::string& name) -> bool { 
            reg->Register<PrePlot3D>(name);
            return true;
        });
    
    factory.registerModuleType("EulerSolver", 
        [](AdvancedRegistry* reg, const std::string& name) -> bool { 
            reg->Register<EulerSolver>(name);
            return true;
        });
    
    factory.registerModuleType("SASolver", 
        [](AdvancedRegistry* reg, const std::string& name) -> bool { 
            reg->Register<SASolver>(name);
            return true;
        });

    factory.registerModuleType("SSTSolver", 
        [](AdvancedRegistry* reg, const std::string& name) -> bool { 
            reg->Register<SSTSolver>(name);
            return true;
        });

    factory.registerModuleType("PostCGNS", 
        [](AdvancedRegistry* reg, const std::string& name) -> bool { 
            reg->Register<PostCGNS>(name);
            return true;
        });

    factory.registerModuleType("PostPlot3D", 
        [](AdvancedRegistry* reg, const std::string& name) -> bool { 
            reg->Register<PostPlot3D>(name);
            return true;
        });
}


struct ModuleFactoryInit {
        ModuleFactoryInit() {
            ModuleFactoryInitializer::init();
        }
};

// 全局静态实例定义
static ModuleSystem::ModuleFactoryInit moduleFactoryInitInstance;

// 初始化静态成员变量
nlohmann::json ModuleSystem::ModuleFactoryCollection::staticFactoryPool_ = nlohmann::json::array();
std::unordered_map<std::string, ModuleSystem::FactoryExecutionConfig> ModuleSystem::ModuleFactoryCollection::factoryExecutionConfigs_;
nlohmann::json ModuleSystem::Nestedengine::staticEnginePool_ = nlohmann::json::array();

// 初始化静态引擎池
void Nestedengine::initializeStaticEnginePool(const nlohmann::json& enginePool) {
    staticEnginePool_ = enginePool;
}

}
