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

// 存储验证阶段收集的模块执行顺序
std::vector<ModuleExecInfo> collectedModules;

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

bool engineExecutionEngine::executeengine(const std::string& engineName) {
    // 如果收集的模块列表为空，则返回
    if (collectedModules.empty()) {
        std::cout << "没有启用的模块需要执行" << std::endl;
        return true;
    }
    
    try {
        // 第一阶段：构造 - 创建所有模块
        std::cout << "====== 构造阶段 ======" << std::endl;
        for (const auto& moduleInfo : collectedModules) {
            std::cout << "创建模块: " << moduleInfo.moduleName << " (引擎: " << moduleInfo.engineName << ")" << std::endl;
            
            // 确保模块名称在registry中存在
            try {
                // 使用公共接口创建模块，不再直接访问私有成员
                context_.createModule(moduleInfo.moduleName, moduleInfo.moduleParams);
            } catch (const std::exception& e) {
                std::cerr << "错误: 创建模块 '" << moduleInfo.moduleName << "' 失败: " << e.what() << std::endl;
                std::cerr << "确保模块已在正确的注册表中注册" << std::endl;
                throw;
            }
        }
        
        // 第二阶段：初始化 - 初始化所有模块
        std::cout << "====== 初始化阶段 ======" << std::endl;
        for (const auto& moduleInfo : collectedModules) {
            std::cout << "初始化模块: " << moduleInfo.moduleName << " (引擎: " << moduleInfo.engineName << ")" << std::endl;
            context_.initializeModule(moduleInfo.moduleName);
        }
        
        // 第三阶段：执行 - 执行所有模块
        std::cout << "====== 执行阶段 ======" << std::endl;
        for (const auto& moduleInfo : collectedModules) {
            std::cout << "执行模块: " << moduleInfo.moduleName << " (引擎: " << moduleInfo.engineName << ")" << std::endl;
            context_.executeModule(moduleInfo.moduleName);
        }
        
        // 第四阶段：释放 - 按反序释放所有模块
        std::cout << "====== 释放阶段 ======" << std::endl;
        for (auto it = collectedModules.rbegin(); it != collectedModules.rend(); ++it) {
            std::cout << "释放模块: " << it->moduleName << " (引擎: " << it->engineName << ")" << std::endl;
            context_.releaseModule(it->moduleName);
        }
        
    } catch (const std::exception& e) {
        std::cerr << "错误: " << e.what() << std::endl;
        
        // 错误处理：尝试释放已创建但未释放的模块
        std::cout << "尝试释放已创建的模块..." << std::endl;
        for (auto it = collectedModules.rbegin(); it != collectedModules.rend(); ++it) {
            try {
                context_.releaseModule(it->moduleName);
            } catch (const std::exception& e) {
                // 忽略释放过程中的错误，以便尽可能多地释放资源
                std::cerr << "释放模块 '" << it->moduleName << "' 时发生错误: " << e.what() << std::endl;
            }
        }
        
        return false;
    }
    
    std::cout << "引擎执行完成" << std::endl;
    return true;
}

void Nestedengine::executeengine(const std::string& name, std::shared_ptr<engineContext> parentContext) {
    auto it = enginePool_.find(name);
    if (it == enginePool_.end()) {
        throw std::runtime_error("engine: " + name + " not found");
    }
    
    // 创建上下文或使用父上下文
    std::shared_ptr<engineContext> context = parentContext ? parentContext : 
        std::make_shared<engineContext>(registry_, this);
    
    // 添加工厂访问权限控制
    context->setEngineName(name);
    
    // 设置当前引擎可访问的模块列表，基于工厂绑定
    std::string factoryName = getEngineFactoryBinding(name);
    if (factoryName != "default") {
        auto factory = ModuleFactoryCollection::instance().getFactory(factoryName);
        std::unordered_set<std::string> allowedModules;
        
        for (const auto& [moduleName, _] : factory->getAllModuleCreators()) {
            allowedModules.insert(moduleName);
        }
        
        context->setAllowedModules(allowedModules);
    }
    
    // 在调用引擎函数前，创建工作流执行引擎
    engineExecutionEngine executor(
        ConfigurationStorage::instance().config["engine"], 
        *context
    );
    
    // 先调用引擎函数（可能包含自定义逻辑）
    enginePool_[name](*context);
    
    // 执行此引擎定义的工作流
    executor.executeengine(name);
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
    preprocessingFactory["executionType"] = "CHOOSE_ONE";  // 保持为多选一类型
    
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
    solverFactory["executionType"] = "SEQUENTIAL_EXECUTION";  // 改为按顺序执行
    
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
    
    // 3. 后处理工厂 - 对应 Post 引擎，修改为按顺序执行
    nlohmann::json postFactory;
    postFactory["name"] = "post_factory";
    postFactory["modules"] = nlohmann::json::array();
    postFactory["executionType"] = "SEQUENTIAL_EXECUTION";  // 改为按顺序执行

    postFactory["modules"].push_back({
         {"name", "PostCGNS"},
         {"enabled", true}});

    postFactory["modules"].push_back({
        {"name", "PostPlot3D"},
        {"enabled", true}});

    // 添加所有工厂到配置
    moduleFactories.push_back(preprocessingFactory);
    moduleFactories.push_back(solverFactory);
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

engineExecutionEngine::engineExecutionEngine(const nlohmann::json& engines, 
                                             ModuleSystem::engineContext& context)
    : engines_(engines), context_(context) {}

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

// 辅助函数：收集引擎中所有模块信息
bool collectModulesFromConfig(const nlohmann::json& config, 
                            const std::string& engineName,
                            std::unordered_set<std::string>& visitedEngines) {
    // 检查循环依赖
    if (visitedEngines.count(engineName)) {
        std::cerr << "错误: 检测到工作流循环依赖: " << engineName << std::endl;
        return false;
    }
    
    // 查找引擎定义
    auto it = std::find_if(config["enginePool"].begin(), config["enginePool"].end(), 
        [engineName](const nlohmann::json& eng) { 
            return eng["name"] == engineName && eng["enabled"].get<bool>(); 
        });
    
    if (it == config["enginePool"].end()) {
        std::cerr << "错误: 引擎 '" << engineName << "' 未找到或未启用" << std::endl;
        return false;
    }
    
    const nlohmann::json& engineDef = *it;
    visitedEngines.insert(engineName);
    
    // 首先处理子引擎
    if (engineDef.contains("subenginePool") && engineDef["subenginePool"].is_array()) {
        for (const auto& subEngineName : engineDef["subenginePool"]) {
            if (!collectModulesFromConfig(config, subEngineName.get<std::string>(), visitedEngines)) {
                return false;
            }
        }
    }
    
    // 处理当前引擎的模块
    if (engineDef.contains("modules") && engineDef["modules"].is_array()) {
        for (const auto& moduleInfo : engineDef["modules"]) {
            if (!moduleInfo.contains("name")) {
                std::cerr << "错误: 引擎 '" << engineName << "' 中的模块定义缺少名称" << std::endl;
                return false;
            }
            
            const std::string& moduleName = moduleInfo["name"];
            bool moduleEnabled = moduleInfo.value("enabled", true);
            
            // 如果模块被禁用，则跳过
            if (!moduleEnabled) {
                continue;
            }
            
            // 获取模块参数
            nlohmann::json moduleParams = getEffectiveModuleParams(
                ConfigurationStorage::instance().config["config"], 
                moduleName,
                moduleInfo.contains("params") ? moduleInfo["params"] : nlohmann::json(nullptr)
            );
            
            // 存储模块信息
            collectedModules.push_back({engineName, moduleName, moduleParams});
            
            // 调试输出
            std::cout << "收集模块: " << moduleName << " 从引擎: " << engineName << std::endl;
        }
    }
    
    visitedEngines.erase(engineName);
    return true;
}

bool paramValidation(const nlohmann::json& config) {
    // 清除之前的配置
    ConfigurationStorage::instance().clear();
    
    // 初始化注册表和引擎
    ConfigurationStorage::instance().initializeRegistryAndEngine();
    auto& storage = ConfigurationStorage::instance();
    
    // 存储整个配置
    storage.config = config;
    
    // 收集已知模块集合
    std::unordered_set<std::string> knownModules;
    for (const auto& moduleType : ModuleSystem::ModuleTypeRegistry::instance().getModuleTypes()) {
        knownModules.insert(moduleType.name);
    }
    storage.knownModules = knownModules;

    // 存储模块参数模式
    std::unordered_map<std::string, nlohmann::json> moduleParamSchemas;
    for (const auto& moduleType : ModuleSystem::ModuleTypeRegistry::instance().getModuleTypes()) {
        moduleParamSchemas[moduleType.name] = moduleType.getParamSchemaFunc();
    }

    // 验证工厂定义
    if (config.contains("moduleFactories")) {
        std::unordered_set<std::string> factoryNames;
        for (const auto& factory : config["moduleFactories"]) {
            if (!factory.contains("name")) {
                std::cerr << "工厂定义错误: 缺少工厂名称" << std::endl;
                return false;
            }
            
            std::string factoryName = factory["name"];
            if (!factoryNames.insert(factoryName).second) {
                std::cerr << "工厂定义错误: 发现重复的工厂名称 '" << factoryName << "'" << std::endl;
                return false;
            }
            
            // 验证执行逻辑
            if (!factory.contains("executionType")) {
                std::cerr << "工厂定义错误: 工厂 '" << factoryName << "' 缺少执行逻辑定义" << std::endl;
                return false;
            }

            std::string executionType = factory["executionType"];
            if (executionType != "CHOOSE_ONE" && executionType != "SEQUENTIAL_EXECUTION") {
                std::cerr << "工厂定义错误: 工厂 '" << factoryName << "' 使用了无效的执行逻辑类型 '" << executionType << "'" << std::endl;
                return false;
            }

            // 如果是SEQUENTIAL_EXECUTION类型，验证firstModule和executionOrder（如果存在）
            if (executionType == "SEQUENTIAL_EXECUTION") {
                if (factory.contains("firstModule")) {
                    if (!factory["firstModule"].is_string()) {
                        std::cerr << "工厂定义错误: 工厂 '" << factoryName << "' 的firstModule必须是字符串" << std::endl;
                        return false;
                    }
                    
                    // 验证firstModule是否是有效模块
                    std::string firstModule = factory["firstModule"];
                    bool moduleExists = false;
                    for (const auto& module : factory["modules"]) {
                        if (module.contains("name") && module["name"] == firstModule) {
                            moduleExists = true;
                            break;
                        }
                    }
                    
                    if (!moduleExists) {
                        std::cerr << "工厂定义错误: 工厂 '" << factoryName << "' 的firstModule '" << firstModule << "' 不在工厂模块列表中" << std::endl;
                        return false;
                    }
                }
                
                if (factory.contains("executionOrder") && factory["executionOrder"].is_array()) {
                    std::unordered_set<std::string> moduleNames;
                    for (const auto& module : factory["modules"]) {
                        if (module.contains("name")) {
                            moduleNames.insert(module["name"]);
                        }
                    }
                    
                    for (const auto& moduleName : factory["executionOrder"]) {
                        if (!moduleName.is_string()) {
                            std::cerr << "工厂定义错误: 工厂 '" << factoryName << "' 的executionOrder中必须都是字符串" << std::endl;
                            return false;
                        }
                        
                        if (moduleNames.find(moduleName) == moduleNames.end()) {
                            std::cerr << "工厂定义错误: 工厂 '" << factoryName << "' 的executionOrder中的模块 '" 
                                << moduleName.get<std::string>() << "' 不在工厂模块列表中" << std::endl;
                        }
                    }
                }
            }
            
            // 验证工厂中的模块是否是已知模块
            if (factory.contains("modules")) {
                for (const auto& module : factory["modules"]) {
                    if (!module.contains("name")) {
                        std::cerr << "模块定义错误: 工厂 '" << factoryName << "' 中的模块缺少名称" << std::endl;
                        return false;
                    }
                    
                    std::string moduleName = module["name"];
                    if (knownModules.find(moduleName) == knownModules.end()) {
                        std::cerr << "工厂定义错误: 工厂 '" << factoryName << "' 中包含未知模块 '" << moduleName << "'" << std::endl;
                        return false;
                    }
                }
            }
        }
    }
    
    // 验证引擎定义和引擎名称
    std::unordered_set<std::string> engineNames;
    if (config.contains("engine") && config["engine"].contains("enginePool")) {
        for (const auto& engine : config["engine"]["enginePool"]) {
            if (!engine.contains("name")) {
                std::cerr << "引擎定义错误: 缺少引擎名称" << std::endl;
                return false;
            }
            
            std::string engineName = engine["name"];
            if (!engineNames.insert(engineName).second) {
                std::cerr << "引擎定义错误: 发现重复的引擎名称 '" << engineName << "'" << std::endl;
                return false;
            }
            
            // 验证引擎的模块参数
            if (engine.contains("modules") && engine["modules"].is_array()) {
                for (const auto& moduleInfo : engine["modules"]) {
                    if (!moduleInfo.contains("name")) {
                        std::cerr << "引擎 '" << engineName << "' 中的模块定义错误: 缺少模块名称" << std::endl;
                        return false;
                    }
                    
                    std::string moduleName = moduleInfo["name"];
                    
                    // 验证模块是否存在
                    if (knownModules.find(moduleName) == knownModules.end()) {
                        std::cerr << "引擎 '" << engineName << "' 中包含未知模块 '" << moduleName << "'" << std::endl;
                        return false;
                    }
                    
                    // 验证模块参数（如果提供了params）
                    if (moduleInfo.contains("params")) {
                        if (!moduleInfo["params"].is_object()) {
                            std::cerr << "引擎 '" << engineName << "' 中模块 '" << moduleName << "' 的参数必须是对象" << std::endl;
                            return false;
                        }
                        
                        // 检查模块参数是否符合模式
                        const nlohmann::json& params = moduleInfo["params"];
                        const nlohmann::json& paramSchema = moduleParamSchemas[moduleName];
                        
                        for (auto it = params.begin(); it != params.end(); ++it) {
                            const std::string& paramName = it.key();
                            const nlohmann::json& paramValue = it.value();
                            
                            if (!paramSchema.contains(paramName)) {
                                std::cerr << "引擎 '" << engineName << "' 中模块 '" << moduleName 
                                          << "' 的未知参数: '" << paramName << "'" << std::endl;
                                return false;
                            }
                            
                            // 验证参数值
                            std::string errorMsg = validateParam(paramSchema[paramName], paramValue);
                            if (!errorMsg.empty()) {
                                std::cerr << "引擎 '" << engineName << "' 中模块 '" << moduleName 
                                          << "' 的参数 '" << paramName << "' 验证失败: " << errorMsg << std::endl;
                                return false;
                            }
                        }
                    }
                }
            }
        }
        
        // 添加对子引擎存在性的验证
        for (const auto& engine : config["engine"]["enginePool"]) {
            if (engine.contains("subenginePool") && engine["subenginePool"].is_array()) {
                std::string parentEngineName = engine["name"];
                for (const auto& subEngineJson : engine["subenginePool"]) {
                    if (!subEngineJson.is_string()) {
                        std::cerr << "引擎定义错误: 引擎 '" << parentEngineName 
                                  << "' 的子引擎必须是字符串" << std::endl;
                        return false;
                    }
                    
                    std::string subEngineName = subEngineJson.get<std::string>();
                    if (engineNames.find(subEngineName) == engineNames.end()) {
                        std::cerr << "错误: 子引擎 '" << subEngineName << "' 未在配置中定义" << std::endl;
                        return false;
                    }
                }
            }
        }
    } else {
        std::cerr << "配置错误: 缺少引擎定义" << std::endl;
        return false;
    }
    
    // 从配置中提取参数
    if (config.contains("config")) {
        nlohmann::json moduleConfig;
        nlohmann::json globalParams;
        
        for (auto& [key, value] : config["config"].items()) {
            if (value.is_object()) {
                moduleConfig[key] = value;
                
                // 验证模块配置参数
                if (knownModules.find(key) != knownModules.end()) {
                    const nlohmann::json& moduleParams = value;
                    const nlohmann::json& paramSchema = moduleParamSchemas[key];
                    
                    // 检查模块的每个参数
                    for (auto it = moduleParams.begin(); it != moduleParams.end(); ++it) {
                        const std::string& paramName = it.key();
                        const nlohmann::json& paramValue = it.value();
                        
                        // 检查参数是否在模式中定义
                        if (!paramSchema.contains(paramName)) {
                            std::cerr << "模块 '" << key << "' 的未知参数: '" << paramName << "'" << std::endl;
                            return false;
                        }
                        
                        // 验证参数值
                        std::string errorMsg = validateParam(paramSchema[paramName], paramValue);
                        if (!errorMsg.empty()) {
                            std::cerr << "模块 '" << key << "' 的参数 '" << paramName 
                                      << "' 验证失败: " << errorMsg << std::endl;
                            return false;
                        }
                    }
                    
                    // 检查是否缺少必需的参数
                    for (auto it = paramSchema.begin(); it != paramSchema.end(); ++it) {
                        const std::string& paramName = it.key();
                        const nlohmann::json& paramInfo = it.value();
                        
                        if (paramInfo.contains("required") && paramInfo["required"].get<bool>() && 
                            !moduleParams.contains(paramName)) {
                            std::cerr << "模块 '" << key << "' 缺少必需参数: '" << paramName << "'" << std::endl;
                            return false;
                        }
                    }
                }
            }
            else {
                // 这是全局参数
                globalParams[key] = value;
                
                // 验证全局参数的合理值（如果有特定要求）
                if (key == "maxIterations") {
                    if (!value.is_number_integer()) {
                        std::cerr << "全局参数 'maxIterations' 必须是整数" << std::endl;
                        return false;
                    }
                    int maxIter = value.get<int>();
                    if (maxIter <= 0 || maxIter > 1000000) {
                        std::cerr << "全局参数 'maxIterations' 的值必须在范围 [1, 1000000] 内，当前值为 " 
                                  << maxIter << std::endl;
                        return false;
                    }
                }
                else if (key == "convergenceCriteria") {
                    if (!value.is_number()) {
                        std::cerr << "全局参数 'convergenceCriteria' 必须是数值" << std::endl;
                        return false;
                    }
                    double criteria = value.get<double>();
                    if (criteria <= 0 || criteria > 1) {
                        std::cerr << "全局参数 'convergenceCriteria' 的值必须在范围 (0, 1] 内，当前值为 " 
                                  << criteria << std::endl;
                        return false;
                    }
                }
                else if (key == "time_step") {
                    if (!value.is_number()) {
                        std::cerr << "全局参数 'time_step' 必须是数值" << std::endl;
                        return false;
                    }
                    double timeStep = value.get<double>();
                    if (timeStep <= 0) {
                        std::cerr << "全局参数 'time_step' 必须是正数，当前值为 " << timeStep << std::endl;
                        return false;
                    }
                }
                else if (key == "solver") {
                    if (!value.is_string()) {
                        std::cerr << "全局参数 'solver' 必须是字符串" << std::endl;
                        return false;
                    }
                    
                    // 检查求解器类型是否是允许的枚举值
                    std::string solver = value.get<std::string>();
                    const std::unordered_set<std::string> allowedSolvers = {"SIMPLE", "PISO", "PIMPLE", "Coupled"};
                    if (allowedSolvers.find(solver) == allowedSolvers.end()) {
                        std::cerr << "全局参数 'solver' 的值必须是以下之一: ";
                        bool first = true;
                        for (const auto& s : allowedSolvers) {
                            if (!first) std::cerr << ", ";
                            std::cerr << "'" << s << "'";
                            first = false;
                        }
                        std::cerr << "，当前值为 '" << solver << "'" << std::endl;
                        return false;
                    }
                }
            }
        }
        
        // 存储提取的参数
        storage.moduleConfig = moduleConfig;
        storage.globalParams = globalParams;
    } else {
        // config节点是可选的，但我们应该至少警告一下
        std::cout << "警告: 配置中没有 'config' 节点，使用默认参数" << std::endl;
        storage.moduleConfig = nlohmann::json::object();
        storage.globalParams = nlohmann::json::object();
    }
    
    // 收集启用的模块
    std::unordered_set<std::string> enabledModules;
    if (config["registry"].contains("modules")) {
        for (const auto& module : config["registry"]["modules"]) {
            if (module.contains("name") && module.contains("enabled")) {
                std::string moduleName = module["name"].get<std::string>();
                bool isEnabled = module["enabled"].get<bool>();
                if (knownModules.count(moduleName) && isEnabled) {
                    enabledModules.insert(moduleName);
                }
            } else {
                std::cerr << "模块注册表错误: 模块定义必须包含 'name' 和 'enabled' 属性" << std::endl;
                return false;
            }
        }
    } else {
        std::cerr << "配置错误: 缺少模块注册表" << std::endl;
        return false;
    }
    storage.enabledModules = enabledModules;
    
    // 验证所有引用的模块是否在注册表中并启用
    for (const auto& engineDef : config["engine"]["enginePool"]) {
        if (!engineDef.contains("name") || !engineDef["enabled"].get<bool>()) continue;
        
        if (engineDef.contains("modules") && engineDef["modules"].is_array()) {
            for (const auto& moduleInfo : engineDef["modules"]) {
                if (!moduleInfo.contains("name") || !moduleInfo["enabled"].get<bool>()) continue;
                
                std::string moduleName = moduleInfo["name"];
                if (enabledModules.find(moduleName) == enabledModules.end()) {
                    std::cerr << "错误: 引擎 '" << engineDef["name"] 
                              << "' 引用了未在注册表中启用的模块 '" << moduleName << "'" << std::endl;
                    return false;
                }
            }
        }
    }
    
    // 收集需要的引擎名称
    std::unordered_set<std::string> usedEngineNames;
    if (config["engine"].contains("enginePool")) {
        for (const auto& Engine : config["engine"]["enginePool"]) {
            if (!Engine.contains("name") || !Engine["enabled"].get<bool>()) continue;
            usedEngineNames.insert(Engine["name"].get<std::string>());
        }
    }
    storage.usedEngineNames = usedEngineNames;
    
    // 验证引擎与工厂绑定是否有效
    if (config.contains("moduleFactories") && config.contains("engineFactoryBindings")) {
        std::unordered_set<std::string> factoryNames;
        for (const auto& factory : config["moduleFactories"]) {
            if (factory.contains("name")) {
                factoryNames.insert(factory["name"]);
            }
        }
        
        for (const auto& binding : config["engineFactoryBindings"]) {
            if (!binding.contains("engineName") || !binding.contains("factoryName")) {
                std::cerr << "引擎工厂绑定错误: 绑定必须包含 'engineName' 和 'factoryName' 属性" << std::endl;
                return false;
            }
            
            std::string factoryName = binding["factoryName"];
            if (factoryName != "default" && factoryNames.find(factoryName) == factoryNames.end()) {
                std::cerr << "引擎工厂绑定错误: 引用了未定义的工厂 '" << factoryName << "'" << std::endl;
                return false;
            }
            
            std::string engineName = binding["engineName"];
            bool engineFound = false;
            if (config.contains("engine") && config["engine"].contains("enginePool")) {
                for (const auto& engDef : config["engine"]["enginePool"]) {
                    if (engDef.contains("name") && engDef["name"] == engineName) {
                        engineFound = true;
                        break;
                    }
                }
            }
            
            if (!engineFound) {
                std::cerr << "引擎工厂绑定错误: 引用了未定义的引擎 '" << engineName << "'" << std::endl;
                return false;
            }
        }
    } else if (!config.contains("engineFactoryBindings")) {
        std::cerr << "配置错误: 缺少引擎工厂绑定定义" << std::endl;
        return false;
    }
    
    // 验证模块兼容性
    if (config.contains("engineFactoryBindings") && config.contains("moduleFactories") && 
        config.contains("engine") && config["engine"].contains("enginePool")) {
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
        for (const auto& engineDef : config["engine"]["enginePool"]) {
            if (!engineDef.contains("name") || !engineDef.contains("enabled") || 
                !engineDef["enabled"].get<bool>()) continue;
            
            std::string engineName = engineDef["name"];
            std::string boundFactoryName = "default"; // 默认工厂
            
            // 获取引擎绑定的工厂
            if (engineToFactory.count(engineName)) {
                boundFactoryName = engineToFactory[engineName];
            }
            
            // 检查引擎使用的模块
            if (engineDef.contains("modules") && engineDef["modules"].is_array()) {
                for (const auto& moduleInfo : engineDef["modules"]) {
                    if (!moduleInfo.contains("name") || 
                        (moduleInfo.contains("enabled") && !moduleInfo["enabled"].get<bool>())) continue;
                    
                    std::string moduleName = moduleInfo["name"];
                    
                    // 如果工厂是默认工厂"default"，任何模块都可以使用
                    if (boundFactoryName != "default" && factoryToModules.count(boundFactoryName)) {
                        // 检查模块是否在绑定的工厂中
                        if (!factoryToModules[boundFactoryName].count(moduleName)) {
                            std::cerr << "错误: 引擎 '" << engineName 
                                      << "' 尝试使用不属于其绑定工厂 '" << boundFactoryName 
                                      << "' 的模块 '" << moduleName << "'" << std::endl;
                            return false;
                        }
                    }
                }
            }
        }

        // 新增：验证工厂类型为CHOOSE_ONE时，同一工厂的多个模块不能在同一引擎中同时启用
        std::unordered_map<std::string, std::string> factoryExecutionTypes;
        for (const auto& factory : config["moduleFactories"]) {
            if (!factory.contains("name") || !factory.contains("executionType")) continue;
            
            std::string factoryName = factory["name"];
            std::string executionType = factory["executionType"];
            factoryExecutionTypes[factoryName] = executionType;
        }
        
        for (const auto& engineDef : config["engine"]["enginePool"]) {
            if (!engineDef.contains("name") || !engineDef.contains("enabled") || 
                !engineDef["enabled"].get<bool>()) continue;
            
            std::string engineName = engineDef["name"];
            std::string boundFactoryName = "default";
            
            // 获取引擎绑定的工厂
            if (engineToFactory.count(engineName)) {
                boundFactoryName = engineToFactory[engineName];
            }
            
            // 如果工厂类型是CHOOSE_ONE，检查该引擎中启用的模块数量
            if (boundFactoryName != "default" && 
                factoryExecutionTypes.count(boundFactoryName) && 
                factoryExecutionTypes[boundFactoryName] == "CHOOSE_ONE") {
                
                // 收集该引擎中所有启用的模块
                std::vector<std::string> enabledModulesInEngine;
                
                if (engineDef.contains("modules") && engineDef["modules"].is_array()) {
                    for (const auto& moduleInfo : engineDef["modules"]) {
                        if (moduleInfo.contains("name") && moduleInfo.contains("enabled") && 
                            moduleInfo["enabled"].get<bool>()) {
                            
                            std::string moduleName = moduleInfo["name"];
                            
                            // 检查模块是否属于绑定的工厂
                            if (factoryToModules.count(boundFactoryName) && 
                                factoryToModules[boundFactoryName].count(moduleName)) {
                                enabledModulesInEngine.push_back(moduleName);
                            }
                        }
                    }
                }
                
                // 如果启用的模块数量大于1，报错
                if (enabledModulesInEngine.size() > 1) {
                    std::cerr << "工厂验证错误: 引擎 '" << engineName 
                              << "' 绑定了类型为CHOOSE_ONE的工厂 '" << boundFactoryName 
                              << "'，但同时启用了该工厂中的多个模块: ";
                    
                    for (size_t i = 0; i < enabledModulesInEngine.size(); ++i) {
                        if (i > 0) std::cerr << ", ";
                        std::cerr << "'" << enabledModulesInEngine[i] << "'";
                    }
                    std::cerr << std::endl;
                    std::cerr << "对于CHOOSE_ONE类型的工厂，一个引擎中只能启用该工厂的一个模块" << std::endl;
                    return false;
                }
            }
        }
    }
    
    // 检查主进程引擎是否存在并启用
    bool mainProcessExists = false;
    for (const auto& engine : config["engine"]["enginePool"]) {
        if (engine.contains("name") && engine["name"] == "mainProcess" && 
            engine.contains("enabled") && engine["enabled"].get<bool>()) {
            mainProcessExists = true;
            break;
        }
    }
    
    if (!mainProcessExists) {
        std::cerr << "错误: 主处理引擎 'mainProcess' 未定义或已禁用" << std::endl;
        return false;
    }
    
    // 检查是否存在循环依赖
    std::function<bool(const std::string&, 
                       std::unordered_set<std::string>&, 
                       std::unordered_set<std::string>&, 
                       const nlohmann::json&)> 
    detectCycle = [&](const std::string& current, 
                       std::unordered_set<std::string>& visited, 
                       std::unordered_set<std::string>& inStack,
                       const nlohmann::json& enginePool) -> bool {
        visited.insert(current);
        inStack.insert(current);
        
        // 查找当前引擎的子引擎
        for (const auto& engine : enginePool) {
            if (engine.contains("name") && engine["name"] == current &&
                engine.contains("subenginePool") && engine["subenginePool"].is_array()) {
                
                for (const auto& subEngine : engine["subenginePool"]) {
                    std::string subEngineName = subEngine.get<std::string>();
                    
                    if (inStack.count(subEngineName)) {
                        std::cerr << "错误: 检测到循环依赖: " << current << " -> " << subEngineName << std::endl;
                        return true; // 发现循环
                    }
                    
                    if (!visited.count(subEngineName)) {
                        if (detectCycle(subEngineName, visited, inStack, enginePool)) {
                            return true;
                        }
                    }
                }
                
                break;
            }
        }
        
        inStack.erase(current);
        return false;
    };
    
    // 检测每个引擎是否存在循环依赖
    std::unordered_set<std::string> visited, inStack;
    for (const auto& engine : config["engine"]["enginePool"]) {
        if (!engine.contains("name")) continue;
        
        std::string engineName = engine["name"];
        if (!visited.count(engineName)) {
            if (detectCycle(engineName, visited, inStack, config["engine"]["enginePool"])) {
                return false;
            }
        }
    }
    
    /////////////////////////////////////////////////////////////////////////////
    // 验证通过后，进行工厂模块加载、工厂和引擎绑定、模块注册和引擎定义
    /////////////////////////////////////////////////////////////////////////////
    
    auto& registry = storage.registry;
    auto& engine = storage.engine;
    
    // 1. 初始化模块工厂集合（从配置加载工厂定义）
    ModuleFactoryCollection::instance().initializeFactoriesFromConfig(config);
    
    // 2. 初始化引擎与工厂的绑定关系
    engine->initializeEngineFactoryBindings(config);
    
    // 3. 根据使用的引擎名称和绑定关系，确定需要加载的模块工厂
    std::unordered_set<std::string> requiredFactories;
    for (const auto& engineName : storage.usedEngineNames) {
        std::string factoryName = engine->getEngineFactoryBinding(engineName);
        requiredFactories.insert(factoryName);
    }
    storage.requiredFactories = requiredFactories;
    
    // 4. 对每个需要的工厂，为registry注册相应模块
    for (const auto& factoryName : requiredFactories) {
        auto factory = ModuleFactoryCollection::instance().getFactory(factoryName);
        
        // 只注册这个工厂中包含且在enabledModules中的模块
        for (const auto& moduleName : storage.enabledModules) {
            // 如果模块在当前工厂中注册，则添加到registry
            if (factory->registerModule(registry.get(), moduleName)) {
                std::cout << "模块 " << moduleName << " 已从工厂 " << factoryName << " 注册" << std::endl;
            }
        }
    }
    
    // 5. 定义所有引擎
    if (config["engine"].contains("enginePool")) {
        for (const auto& EngineDef : config["engine"]["enginePool"]) {
            if (!EngineDef.contains("name") || !EngineDef["enabled"].get<bool>()) continue;
            std::string EngineName = EngineDef["name"].get<std::string>();

            engine->defineengine(EngineName, 
                [EngineDef, EngineName](ModuleSystem::engineContext& context) {
                    // 创建engineExecutionEngine实例来处理这个引擎中的所有模块的执行
                    // engineExecutionEngine executor(
                    //     ConfigurationStorage::instance().config["engine"], 
                    //     context
                    // );
                    // 执行工作流
                    //executor.executeengine(EngineName);
                });
        }
        storage.enginesAreDefined = true;
    }
    
    // 6. 构建引擎
    if (config.contains("config")) {
        engine->Build(config["config"]); // 使用"config"部分进行Build
    }
    
    // 7. 创建主上下文
    storage.mainContext = std::make_shared<ModuleSystem::engineContext>(*registry, engine.get());
    if (config.contains("config")) {
        storage.mainContext->setParameter("config", config["config"]); // 设置整个config块
        for (auto& [key, value] : config["config"].items()) { // 以及单独的参数
            if (!value.is_object()) storage.mainContext->setParameter(key, value);
        }
        for (auto& [moduleName, params] : storage.moduleConfig.items()) { // 以及模块特定的参数
            storage.mainContext->setParameter(moduleName, params);
        }
    }

    // 清空之前收集的模块信息
    collectedModules.clear();

    // 验证配置结构
    if (!config.contains("engine") || !config["engine"].contains("enginePool") || 
        !config["engine"]["enginePool"].is_array()) {
        std::cerr << "错误: 配置缺少有效的 engine.enginePool 数组" << std::endl;
        return false;
    }

    if (!config.contains("moduleFactories") || !config["moduleFactories"].is_array()) {
        std::cerr << "警告: 配置缺少 moduleFactories 数组，将使用默认工厂" << std::endl;
    }

    if (!config.contains("config")) {
        std::cerr << "警告: 配置缺少 config 对象，可能缺少全局模块参数" << std::endl;
    }

    // 验证引擎定义
    bool hasDefaultEngine = false;
    for (const auto& engine : config["engine"]["enginePool"]) { // 修正路径
        if (!engine.contains("name") || !engine["name"].is_string()) {
            std::cerr << "错误: 引擎定义缺少有效的名称" << std::endl;
            return false;
        }
        
        if (engine["name"] == "mainProcess" && engine.value("enabled", true)) {
            hasDefaultEngine = true;
        }
    }

    // 收集模块执行顺序
    std::unordered_set<std::string> visitedEngines;
    if (!collectModulesFromConfig(config["engine"], "mainProcess", visitedEngines)) { // 修正传入参数
        std::cerr << "错误: 收集模块执行顺序失败" << std::endl;
        return false;
    }

    // 验证模块参数
    for (const auto& moduleInfo : collectedModules) {
        // 验证模块参数
        try {
            validateModuleParams(moduleInfo.moduleParams, moduleInfo.moduleName, 0);
        } catch (const std::exception& e) {
            std::cerr << "错误: 模块 '" << moduleInfo.moduleName << "' (引擎: " << moduleInfo.engineName 
                    << ") 的参数验证失败: " << e.what() << std::endl;
            return false;
        }
    }

    // 如果没有收集到任何模块，发出警告
    if (collectedModules.empty()) {
        std::cerr << "警告: 没有找到任何启用的模块" << std::endl;
    }
    
    std::cout << "配置验证通过，模块注册完成" << std::endl;
    return true;
}


void saveUsedConfig(const nlohmann::json& config, const std::string& outputFile) {
    // 解析配置，提取需要的部分
    nlohmann::json moduleConfig;
    nlohmann::json globalParams;
    
    // 从配置中提取参数信息
    if (config.contains("config")) {
        for (auto& [key, value] : config["config"].items()) {
            if (value.is_object()) moduleConfig[key] = value;
            else globalParams[key] = value;
        }
    }
    
    // 确定启用的模块集合
    std::unordered_set<std::string> enabledModulesForSave;
    if (config["registry"].contains("modules")) {
        for (const auto& module : config["registry"]["modules"]) {
            if (module.contains("name") && module.contains("enabled") && module["enabled"].get<bool>()) {
                enabledModulesForSave.insert(module["name"].get<std::string>());
            }
        }
    }
    
    // 调用ModuleSystem中的保存函数
    ModuleSystem::saveUsedConfig(moduleConfig, enabledModulesForSave, 
                                config["engine"], globalParams, outputFile);
}

void run() {
    // 获取已验证的配置和已创建的注册表
    auto& storage = ConfigurationStorage::instance();
    const nlohmann::json& config = storage.config;
    
    // 使用验证阶段创建的注册表和引擎，而不是创建新的
    auto registry = storage.registry;
    auto engine = std::move(storage.engine);
    auto context = storage.mainContext;
    
    // 创建工作流引擎
    engineExecutionEngine executionEngine(config["engine"], *context);
    
    // 执行主引擎
    std::cout << "开始执行主引擎工作流..." << std::endl;
    if (!executionEngine.executeengine("mainProcess")) {
        std::cerr << "引擎执行失败" << std::endl;
    } else {
        std::cout << "引擎执行成功" << std::endl;
    }
    
    // 检查是否有模块泄漏
    auto leakedModules = registry->checkLeakedModules();
    if (!leakedModules.empty()) {
        std::cerr << "警告: 检测到未释放的模块:" << std::endl;
        for (const auto& module : leakedModules) {
            std::cerr << "  - " << module << std::endl;
        }
    }
    
    // 将engine所有权归还给storage，以便后续使用
    storage.engine = std::move(engine);
}

void test() {
    auto& storage = ConfigurationStorage::instance();
    
    // 确保已经初始化了注册表和引擎
    if (!storage.registry || !storage.engine || !storage.mainContext || !storage.enginesAreDefined) {
        std::cerr << "错误: 测试前请先运行 paramValidation 函数" << std::endl;
        return;
    }
    
    std::cout << "\n======== 开始模块手动测试 ========\n" << std::endl;
    
    // 测试手动调用模块
    try {
        // 测试一个预处理模块
        if (storage.enabledModules.find("PreCGNS") != storage.enabledModules.end()) {
            std::cout << "测试 PreCGNS 模块..." << std::endl;
            
            // 准备参数
            nlohmann::json moduleParams;
            if (storage.moduleConfig.contains("PreCGNS")) {
                moduleParams = storage.moduleConfig["PreCGNS"];
            } else {
                moduleParams = {
                    {"cgns_type", "HDF5"},
                    {"cgns_value", 20}
                };
            }
            
            // 执行完整模块生命周期
            void* moduleInstance = storage.mainContext->createModule("PreCGNS", moduleParams);
            std::cout << " - 创建 PreCGNS 成功" << std::endl;
            
            storage.mainContext->initializeModule("PreCGNS");
            std::cout << " - 初始化 PreCGNS 成功" << std::endl;
            
            storage.mainContext->executeModule("PreCGNS");
            std::cout << " - 执行 PreCGNS 成功" << std::endl;
            
            storage.mainContext->releaseModule("PreCGNS");
            std::cout << " - 释放 PreCGNS 成功" << std::endl;
        }
        
        // 测试一个求解器模块
        if (storage.enabledModules.find("EulerSolver") != storage.enabledModules.end()) {
            std::cout << "\n测试 EulerSolver 模块..." << std::endl;
            
            // 准备参数
            nlohmann::json moduleParams;
            if (storage.moduleConfig.contains("EulerSolver")) {
                moduleParams = storage.moduleConfig["EulerSolver"];
            } else {
                moduleParams = {
                    {"euler_type", "Standard"},
                    {"euler_value", 0.7}
                };
            }
            
            // 执行完整模块生命周期
            void* moduleInstance = storage.mainContext->createModule("EulerSolver", moduleParams);
            std::cout << " - 创建 EulerSolver 成功" << std::endl;
            
            storage.mainContext->initializeModule("EulerSolver");
            std::cout << " - 初始化 EulerSolver 成功" << std::endl;
            
            storage.mainContext->executeModule("EulerSolver");
            std::cout << " - 执行 EulerSolver 成功" << std::endl;
            
            storage.mainContext->releaseModule("EulerSolver");
            std::cout << " - 释放 EulerSolver 成功" << std::endl;
        }
        
    } catch (const std::exception& e) {
        std::cerr << "测试过程中发生错误: " << e.what() << std::endl;
    }
    
    std::cout << "\n======== 模块手动测试结束 ========\n" << std::endl;
    
    // 检查是否有模块泄漏
    auto leakedModules = storage.registry->checkLeakedModules();
    if (!leakedModules.empty()) {
        std::cerr << "测试后发现未释放的模块:" << std::endl;
        for (const auto& module : leakedModules) {
            std::cerr << "  - " << module << std::endl;
        }
    } else {
        std::cout << "测试完成，没有模块泄漏" << std::endl;
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
nlohmann::json ModuleSystem::Nestedengine::staticEnginePool_ = nlohmann::json::array();

// 初始化静态引擎池
void Nestedengine::initializeStaticEnginePool(const nlohmann::json& enginePool) {
    staticEnginePool_ = enginePool;
}

}
