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
#include "preGrid.h"
#include "solve.h"
#include "post.h"
#include <iostream>
#include <fstream>
#include <iomanip>
#include <filesystem>
#include <stdexcept>
#include <algorithm>
#include <cxxabi.h>

namespace ModuleSystem {

/**
 * @brief Defines possible actions that can be performed on modules.
 */
enum class ModuleAction {
    CREATE,    /**< Create a module */
    INITIALIZE,/**< Initialize a module */
    EXECUTE,   /**< Execute a module */
    RELEASE,   /**< Release a module */
    UNKNOWN    /**< Unknown action */
};

/**
 * @brief Global vector to store the execution order of modules collected during validation.
 */
std::vector<ModuleExecInfo> collectedModules;

/**
 * @brief Converts a string to a ModuleAction enum value.
 * @param actionStr The string representation of the action.
 * @return The corresponding ModuleAction enum value, or UNKNOWN if not found.
 */
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

/**
 * @brief Converts a LifecycleStage enum value to its string representation.
 * @param stage The LifecycleStage enum value.
 * @return A string representing the lifecycle stage.
 */
std::string LifecycleStageToString(LifecycleStage stage) {
    switch (stage) {
        case LifecycleStage::CONSTRUCTED: return "CONSTRUCTED";
        case LifecycleStage::INITIALIZED: return "INITIALIZED";
        case LifecycleStage::EXECUTED:    return "EXECUTED";
        case LifecycleStage::RELEASED:    return "RELEASED";
        default:                          return "UNKNOWN_STAGE (" + std::to_string(static_cast<int>(stage)) + ")";
    }
}

/**
 * @brief Creates a new instance of a module.
 * @param name The name of the module to create.
 * @param params The parameters to pass to the module's constructor.
 * @return A pointer to the created module instance.
 * @throws std::runtime_error If the module is not found or cannot be constructed.
 */
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
    if (lifecycleIt != lifecycle_.end()) {
        LifecycleStage currentStage = std::get<1>(lifecycleIt->second);
        if (currentStage != LifecycleStage::RELEASED) {
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

/**
 * @brief Initializes a module instance.
 * @param mod Pointer to the module instance to initialize.
 * @throws std::runtime_error If the module is not in a valid state for initialization or not found in lifecycle map.
 */
void AdvancedRegistry::Initialize(void* mod) { 
    if (mod) {
        auto it = lifecycle_.find(mod);
        if (it != lifecycle_.end()) {
            LifecycleStage currentStage = std::get<1>(it->second);
            if (currentStage == LifecycleStage::RELEASED) {
                throw std::runtime_error("Module " + std::get<0>(it->second) + " is RELEASED and cannot be initialized. Current state: " + LifecycleStageToString(currentStage));
            }
            if (currentStage != LifecycleStage::CONSTRUCTED && currentStage != LifecycleStage::INITIALIZED && currentStage != LifecycleStage::EXECUTED) {
                throw std::runtime_error("Module " + std::get<0>(it->second) + " must be in CONSTRUCTED, INITIALIZED, or EXECUTED state to be initialized. Current state: " + LifecycleStageToString(currentStage));
            }
            modules_.at(std::get<0>(it->second)).initialize(mod);
            std::get<1>(it->second) = LifecycleStage::INITIALIZED;
        } else {
            throw std::runtime_error("Instance not found in lifecycle map for Initialize");
        }
    }
}

/**
 * @brief Executes a module instance.
 * @param mod Pointer to the module instance to execute.
 * @throws std::runtime_error If the module is not in a valid state for execution or not found in lifecycle map.
 */
void AdvancedRegistry::Execute(void* mod) { 
    if (mod) {
        auto it = lifecycle_.find(mod);
        if (it != lifecycle_.end()) {
            LifecycleStage currentStage = std::get<1>(it->second);
            if (currentStage != LifecycleStage::INITIALIZED && currentStage != LifecycleStage::EXECUTED) {
                throw std::runtime_error("Module " + std::get<0>(it->second) + " must be in INITIALIZED or EXECUTED state to be executed. Current state: " + LifecycleStageToString(currentStage));
            }
            modules_.at(std::get<0>(it->second)).execute(mod);
            std::get<1>(it->second) = LifecycleStage::EXECUTED;
        } else {
            throw std::runtime_error("Instance not found in lifecycle map for Execute");
        }
    }
}

/**
 * @brief Releases a module instance.
 * @param mod Pointer to the module instance to release.
 * @throws std::runtime_error If the module is already released, in an invalid state, or not found in lifecycle map.
 */
void AdvancedRegistry::Release(void* mod) { 
    if (mod) {
        auto it = lifecycle_.find(mod);
        if (it != lifecycle_.end()) {
            LifecycleStage currentStage = std::get<1>(it->second);
            if (currentStage == LifecycleStage::RELEASED) {
                throw std::runtime_error("Module " + std::get<0>(it->second) + " is already RELEASED and cannot be released again. Current state: " + LifecycleStageToString(currentStage));
            }
            if (currentStage != LifecycleStage::CONSTRUCTED && 
                currentStage != LifecycleStage::INITIALIZED && 
                currentStage != LifecycleStage::EXECUTED) {
                 throw std::runtime_error("Module " + std::get<0>(it->second) + " is in an invalid state for release: " + LifecycleStageToString(currentStage));
            }
            modules_.at(std::get<0>(it->second)).release(mod);
            std::get<1>(it->second) = LifecycleStage::RELEASED;
            lifecycle_.erase(mod);
        } else {
            throw std::runtime_error("Instance not found in lifecycle map for Release");
        }
    }
}

/**
 * @brief Checks for modules that have not been released.
 * @return A vector of strings describing the leaked modules.
 */
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

/**
 * @brief Constructor for engineContext.
 * @param reg Reference to the AdvancedRegistry.
 * @param engine Pointer to the Nestedengine, defaults to nullptr.
 */
engineContext::engineContext(AdvancedRegistry& reg, Nestedengine* engine) 
    : registry_(reg), engine_(engine) {}

/**
 * @brief Checks if the current engine can access a specific module.
 * @param moduleName The name of the module to check.
 * @return True if the engine can access the module, false otherwise.
 */   
bool engineContext::canAccessModule(const std::string& moduleName) const {
    // 如果是主引擎，允许访问所有模块
    if (engineName_ == "mainProcess") {
        return true;
    }
    
    // 如果模块未绑定到任何引擎，则所有引擎都可以访问它
    if (!EngineModuleMapping::instance().isModuleBoundToEngine(moduleName)) {
        return true;
    }
    
    // 获取模块所属引擎
    std::string moduleEngine = EngineModuleMapping::instance().getModuleEngine(moduleName);
    
    // 检查模块是否属于当前引擎或其子引擎
    auto& storage = ConfigurationStorage::instance();
    for (const auto& engine : storage.config["engine"]["enginePool"]) {
        if (engine["name"].get<std::string>() == engineName_) {
            // 检查模块是否直接属于该引擎
            if (moduleEngine == engineName_) {
                return true;
            }
            
            // 检查该引擎是否有子引擎
            if (engine.contains("subenginePool") && engine["subenginePool"].is_array()) {
                for (const auto& subEngine : engine["subenginePool"]) {
                    if (moduleEngine == subEngine.get<std::string>()) {
                        return true; // 模块属于子引擎
                    }
                }
            }
            break;
        }
    }
    
    return false;
}

/**
 * @brief Sets a parameter in the engine context.
 * @param name The name of the parameter.
 * @param value The value of the parameter.
 */
void engineContext::setParameter(const std::string& name, const nlohmann::json& value) {
    parameters_[name] = value;
}

/**
 * @brief Creates a new module instance.
 * @param name The name of the module to create.
 * @param params The parameters to pass to the module's constructor.
 * @return A pointer to the created module instance.
 * @throws std::runtime_error If the module cannot be created or already exists.
 */
void* engineContext::createModule(const std::string& name, const nlohmann::json& params) {
    if (!canAccessModule(name)) {
        std::string errorMsg = "访问控制错误: 引擎 '" + engineName_ + "' 尝试创建不属于其绑定工厂的模块 '" + name + "'";
        std::cerr << errorMsg << std::endl;
        exit(1);
    }

    if (modules_.count(name)) {
        std::cout << "警告: 模块 " << name << " 已存在。正在释放旧实例并创建新实例。" << std::endl;
        registry_.Release(modules_[name]);
        modules_.erase(name);
    }
    void* instance = registry_.Create(name, params);
    modules_[name] = instance;
    return instance;
}

/**
 * @brief Initializes a module by name.
 * @param name The name of the module to initialize.
 * @throws std::runtime_error If the module is not found.
 */
void engineContext::initializeModule(const std::string& name) {
    auto it = modules_.find(name);
    if (it != modules_.end()) {
        registry_.Initialize(it->second);
    } else {
        throw std::runtime_error("Module not found for initialize: " + name);
    }
}

/**
 * @brief Executes a module by name.
 * @param name The name of the module to execute.
 * @throws std::runtime_error If the module is not found.
 */
void engineContext::executeModule(const std::string& name) {
    auto it = modules_.find(name);
    if (it != modules_.end()) {
        registry_.Execute(it->second);
    } else {
        throw std::runtime_error("Module not found for execute: " + name);
    }
}

/**
 * @brief Releases a module by name.
 * @param name The name of the module to release.
 * @throws std::runtime_error If releasing the module fails.
 */
void engineContext::releaseModule(const std::string& name) {
    auto it = modules_.find(name);
    if (it != modules_.end()) {
        try {
            registry_.Release(it->second);
            modules_.erase(it);
        } catch (const std::exception& e) {
            std::cerr << "错误: 释放模块 '" << name << "' 失败: " << e.what() << std::endl;
            throw;
        }
    } else {
        std::cout << "错误: 尝试释放不存在的模块 '" << name 
                  << "'，可能该模块已经被释放或从未创建过" << std::endl;
        exit(1);
    }
}

/**
 * @brief Constructor for Nestedengine.
 * @param reg Reference to the AdvancedRegistry.
 */
Nestedengine::Nestedengine(AdvancedRegistry& reg) : registry_(reg) {}

/**
 * @brief Builds the engine with the given configuration.
 * @param config The configuration JSON object.
 */
void Nestedengine::Build(const nlohmann::json& config) {
    parameters_ = config;
}

/**
 * @brief Defines a new engine with the given name and function.
 * @param name The name of the engine.
 * @param Engine The function that defines the engine's behavior.
 */
void Nestedengine::defineengine(const std::string& name, 
                   std::function<void(engineContext&)> Engine) {
    enginePool_[name] = Engine;
}

/**
 * @brief Gets the engine pool.
 * @return A const reference to the engine pool.
 */
const auto& Nestedengine::getengines() const { 
    return enginePool_; 
}

/**
 * @brief Gets the default configuration for the engine.
 * @return A JSON object representing the default configuration.
 */
nlohmann::json Nestedengine::getDefaultConfig() const {
    nlohmann::json defaultConfig;
    defaultConfig["GlobalConfig"] = {
        {"solver", "SIMPLE"},
        {"maxIterations", 1000},
        {"convergenceCriteria", 1e-6},
        {"time_step", 0.01}
    };
    return defaultConfig;
}

/**
 * @brief 创建特定引擎的配置信息
 * @param engineName 引擎名称
 * @return 引擎特定的配置JSON对象
 */
nlohmann::json createEngineSpecificInfo(const std::string& engineName) {
    nlohmann::json engineInfo;
    engineInfo["engine"] = nlohmann::json::object();
    engineInfo["engine"]["enginePool"] = nlohmann::json::array();
    
    // 从全局引擎池中获取特定引擎信息
    auto fullEngineInfo = createengineInfo();
    
    // 查找指定的引擎
    for (auto& engine : fullEngineInfo["enginePool"]) {
        if (engine["name"] == engineName) {
            // 创建该引擎的特定信息
            engineInfo["engine"]["enginePool"].push_back(engine);
            
            // 处理子引擎
            if (engineName == "mainProcess" && engine.contains("subenginePool")) {
                // 主引擎只需要列出子引擎的基本信息，不包含子引擎的模块和子引擎的子引擎
                for (const auto& subEngineName : engine["subenginePool"]) {
                    for (auto& subEngine : fullEngineInfo["enginePool"]) {
                        if (subEngine["name"] == subEngineName) {
                            // 创建简化版本的子引擎信息
                            nlohmann::json simplifiedSubEngine;
                            simplifiedSubEngine["name"] = subEngine["name"];
                            simplifiedSubEngine["description"] = subEngine["description"];
                            simplifiedSubEngine["enabled"] = subEngine["enabled"];
                            
                            // 不添加modules和subenginePool
                            
                            engineInfo["engine"]["enginePool"].push_back(simplifiedSubEngine);
                            break;
                        }
                    }
                }
            } else if (engine.contains("subenginePool")) {
                // 非主引擎需要添加子引擎的完整信息
                for (const auto& subEngineName : engine["subenginePool"]) {
                    for (auto& subEngine : fullEngineInfo["enginePool"]) {
                        if (subEngine["name"] == subEngineName) {
                            engineInfo["engine"]["enginePool"].push_back(subEngine);
                            break;
                        }
                    }
                }
            }
            
            break;
        }
    }
    
    return engineInfo;
}

/**
 * @brief Creates a module configuration for the specified engine.
 * @param engineName The name of the engine.
 * @return A JSON object representing the module configuration.
 */
nlohmann::json createModuleConfigForEngine(const std::string& engineName) {
    nlohmann::json moduleConfig;
    moduleConfig["config"] = nlohmann::json::object();
    
    // 添加全局配置参数
    if (engineName == "mainProcess") {
        // 修复：使用临时对象直接调用函数，而不是尝试绑定到引用
        auto defaultConfig = ModuleSystem::Nestedengine(*(new ModuleSystem::AdvancedRegistry())).getDefaultConfig();
        moduleConfig["config"] = defaultConfig;
    }
    
    // 确定引擎包含的所有模块（不管是否启用）
    std::unordered_set<std::string> engineModules;
    
    // 从引擎配置中获取模块列表
    auto engineInfo = createEngineSpecificInfo(engineName);
    for (const auto& engine : engineInfo["engine"]["enginePool"]) {
        if (engine.contains("modules")) {
            for (const auto& module : engine["modules"]) {
                // 不再检查模块是否启用，只要存在就添加到模块列表中
                engineModules.insert(module["name"]);
            }
        }
    }
    
    // 为这些模块添加配置
    auto fullRegistry = createRegistryInfo();
    for (const auto& module : fullRegistry["modules"]) {
        std::string moduleName = module["name"];
        if (engineModules.find(moduleName) != engineModules.end()) {
            // 收集模块的所有参数
            for (const auto& param : module["parameters"].items()) {
                moduleConfig["config"][moduleName][param.key()] = param.value()["default"];
            }
        }
    }
    
    return moduleConfig;
}

/**
 * @brief 生成所有引擎和模块的配置模板
 * @param baseDir 模板文件保存的基础目录
 */
void generateTemplateConfigs(const std::string& baseDir) {
    std::cout << "正在生成引擎和模块配置模板..." << std::endl;
    
    std::filesystem::path dirPath(baseDir);
    std::filesystem::create_directories(dirPath);

    nlohmann::json fullRegistryInfo = createRegistryInfo();
    nlohmann::json globalConfigSchema;
    for (const auto& mod : fullRegistryInfo["modules"]) {
        if (mod["name"] == "GlobalConfig") {
            globalConfigSchema = mod["parameters"];
            break;
        }
    }

    // 生成主引擎配置
    nlohmann::json mainConfig;
    mainConfig["config"] = nlohmann::json::object();
    // 从 GlobalConfig schema 生成默认的 GlobalConfig 对象
    if (!globalConfigSchema.is_null()) {
        mainConfig["config"]["GlobalConfig"] = nlohmann::json::object();
        for (auto it = globalConfigSchema.begin(); it != globalConfigSchema.end(); ++it) {
            if (it.value().contains("default")) {
                mainConfig["config"]["GlobalConfig"][it.key()] = it.value()["default"];
            }
        }
    } else {
        // Fallback or error if GlobalConfig schema not found
         mainConfig["config"]["GlobalConfig"] = {
            {"solver", "SIMPLE"},
            {"maxIterations", 1000},
            {"convergenceCriteria", 1e-6},
            {"time_step", 0.01}
        };
        std::cerr << "警告: 未在 registry info 中找到 GlobalConfig schema，使用硬编码的默认值。" << std::endl;
    }
    
    mainConfig["engine"] = nlohmann::json::object();
    mainConfig["engine"]["enginePool"] = nlohmann::json::array();
    
    auto fullEngineInfo = createengineInfo(); // 假设这个函数返回所有引擎的定义
    for (const auto& engine : fullEngineInfo["enginePool"]) {
        if (engine["name"] == "mainProcess") {
            mainConfig["engine"]["enginePool"].push_back(engine);
            // 为 mainProcess 模板添加其直接模块的默认配置（如果需要）
            if (engine.contains("modules")) {
                for (const auto& modInfo : engine["modules"]) {
                    std::string moduleName = modInfo["name"].get<std::string>();
                    for (const auto& regModule : fullRegistryInfo["modules"]) {
                        if (regModule["name"] == moduleName && regModule.contains("parameters")) {
                            mainConfig["config"][moduleName] = nlohmann::json::object();
                            for (auto p_it = regModule["parameters"].begin(); p_it != regModule["parameters"].end(); ++p_it) {
                                if (p_it.value().contains("default")) {
                                    mainConfig["config"][moduleName][p_it.key()] = p_it.value()["default"];
                                }
                            }
                            break;
                        }
                    }
                }
            }
            break;
        }
    }
    
    std::filesystem::path mainFilePath = dirPath / "template_engine_mainProcess.json";
    std::ofstream mainFile(mainFilePath);
    if (mainFile.is_open()) {
        mainFile << std::setw(4) << mainConfig << std::endl;
        mainFile.close();
        std::cout << "已生成主引擎配置: " << mainFilePath.string() << std::endl;
    } else {
        std::cerr << "无法创建文件: " << mainFilePath.string() << std::endl;
    }

    // 为其他引擎生成配置模板
    std::vector<std::string> otherEngineNames = {"PreGrid", "Solve", "Post"};
    for (const std::string& engineName : otherEngineNames) {
        nlohmann::json engineSpecificConfig = createEngineSpecificInfo(engineName); // 获取引擎自身的定义
        nlohmann::json engineModuleParams = nlohmann::json::object();

        // 查找该引擎的定义以获取其模块列表
        nlohmann::json currentEngineDef;
        for (const auto& eng : fullEngineInfo["enginePool"]) {
            if (eng["name"] == engineName) {
                currentEngineDef = eng;
                break;
            }
        }

        if (currentEngineDef.is_null()) {
            std::cerr << "警告: 未找到引擎 '" << engineName << "' 的定义，跳过其模板生成。" << std::endl;
            continue;
        }
        
        // 为该引擎直接关联的模块添加默认参数
        if (currentEngineDef.contains("modules") && currentEngineDef["modules"].is_array()) {
            for (const auto& modInfo : currentEngineDef["modules"]) {
                if (modInfo.contains("name")) {
                    std::string moduleNameStr = modInfo["name"].get<std::string>();
                    for (const auto& regModule : fullRegistryInfo["modules"]) {
                        if (regModule["name"] == moduleNameStr && regModule.contains("parameters")) {
                            engineModuleParams[moduleNameStr] = nlohmann::json::object();
                            for (auto p_it = regModule["parameters"].begin(); p_it != regModule["parameters"].end(); ++p_it) {
                                if (p_it.value().contains("default")) {
                                    engineModuleParams[moduleNameStr][p_it.key()] = p_it.value()["default"];
                                }
                            }
                            break;
                        }
                    }
                }
            }
        }
        engineSpecificConfig["config"] = engineModuleParams; // 将模块参数添加到引擎特定配置中

        std::filesystem::path engineFilePath = dirPath / ("template_engine_" + engineName + ".json");
        std::ofstream engineFile(engineFilePath);
        if (engineFile.is_open()) {
            engineFile << std::setw(4) << engineSpecificConfig << std::endl;
            engineFile.close();
            std::cout << "已生成 " << engineName << " 引擎配置: " << engineFilePath.string() << std::endl;
        } else {
            std::cerr << "无法创建文件: " << engineFilePath.string() << std::endl;
        }
    }
    
    // 生成模块注册信息模板 (使用更新后的 createRegistryInfo)
    nlohmann::json registryConfigTemplate;
    registryConfigTemplate["registry"] = fullRegistryInfo; // fullRegistryInfo 已经包含了 GlobalConfig
    
    std::filesystem::path registryFilePath = dirPath / "template_registry_module.json";
    std::ofstream registryFile(registryFilePath);
    if (registryFile.is_open()) {
        registryFile << std::setw(4) << registryConfigTemplate << std::endl;
        registryFile.close();
        std::cout << "已生成模块注册信息模板: " << registryFilePath.string() << std::endl;
    } else {
        std::cerr << "无法创建文件: " << registryFilePath.string() << std::endl;
    }
    
    // 正常退出，避免访问ConfigurationStorage引起的崩溃
    exit(0); // 通常在生成模板后不需要退出程序，除非这是脚本的唯一目的
}

/**
 * @brief Saves the used configurations for each engine and module.
 * @param config The merged configuration JSON object.
 * @param configDir The directory where the configuration files will be saved.
 */
void saveUsedConfigs(const nlohmann::json& config, const std::string& configDir) {
    nlohmann::json usedConfig = config;
    std::vector<std::string> engineNames = {"mainProcess", "PreGrid", "Solve", "Post"};

    for (const auto& engineName : engineNames) {
        nlohmann::json engineConfig;
        engineConfig["engine"] = nlohmann::json::object();
        engineConfig["engine"]["enginePool"] = nlohmann::json::array();

        const nlohmann::json* currentEngineDefJsonPtr = nullptr;
        if (usedConfig.contains("engine") && usedConfig["engine"].is_object() &&
            usedConfig["engine"].contains("enginePool") && usedConfig["engine"]["enginePool"].is_array()) {
            for (const auto& engDef : usedConfig["engine"]["enginePool"]) {
                if (engDef.contains("name") && engDef["name"].get<std::string>() == engineName) {
                    currentEngineDefJsonPtr = &engDef;
                    break;
                }
            }
        }

        if (!currentEngineDefJsonPtr) {
            std::cerr << "警告: 在 saveUsedConfigs 中未找到引擎 '" << engineName << "' 的定义于 usedConfig。" << std::endl;
            continue; 
        }
        const nlohmann::json& currentEngineDef = *currentEngineDefJsonPtr;

        // Populate engineConfig["engine"]["enginePool"]
        if (engineName == "mainProcess") {
            // For mainProcess, save its own definition. Sub-engines are listed but not their full definitions here.
            engineConfig["engine"]["enginePool"].push_back(currentEngineDef);
        } else {
            // For other engines, save their own definition.
            engineConfig["engine"]["enginePool"].push_back(currentEngineDef);
            // And if they have sub-engines, save the full definitions of those direct sub-engines.
            if (currentEngineDef.contains("subenginePool") && currentEngineDef["subenginePool"].is_array()) {
                for (const auto& subEngineNameJson : currentEngineDef["subenginePool"]) {
                    std::string subEngineNameStr = subEngineNameJson.get<std::string>();
                    bool subEngineFound = false;
                    if (usedConfig.contains("engine") && usedConfig["engine"].is_object() &&
                        usedConfig["engine"].contains("enginePool") && usedConfig["engine"]["enginePool"].is_array()) {
                        for (const auto& subEngDef : usedConfig["engine"]["enginePool"]) {
                            if (subEngDef.contains("name") && subEngDef["name"].get<std::string>() == subEngineNameStr) {
                                engineConfig["engine"]["enginePool"].push_back(subEngDef);
                                subEngineFound = true;
                                break;
                            }
                        }
                    }
                    if (!subEngineFound) {
                         std::cerr << "警告: 在 saveUsedConfigs 中为引擎 '" << engineName << "' 的子引擎 '" << subEngineNameStr << "' 未找到定义。" << std::endl;
                    }
                }
            }
        }
        
        // Populate engineConfig["config"]
        engineConfig["config"] = nlohmann::json::object();
        if (usedConfig.contains("config") && usedConfig["config"].is_object()) {
            if (engineName == "mainProcess") {
                // mainProcess config file should contain the GlobalConfig
                if (usedConfig["config"].contains("GlobalConfig") && usedConfig["config"]["GlobalConfig"].is_object()) {
                    engineConfig["config"]["GlobalConfig"] = usedConfig["config"]["GlobalConfig"];
                }
            }
            
            // Add configs for modules directly listed under this engine's definition
            if (currentEngineDef.contains("modules") && currentEngineDef["modules"].is_array()) {
                for (const auto& moduleInfo : currentEngineDef["modules"]) {
                    if (moduleInfo.contains("name") && moduleInfo["name"].is_string()) {
                        std::string moduleNameStr = moduleInfo["name"].get<std::string>();
                        // Check if this module's config exists in the global 'config' section
                        if (usedConfig["config"].contains(moduleNameStr) && usedConfig["config"][moduleNameStr].is_object()) {
                            engineConfig["config"][moduleNameStr] = usedConfig["config"][moduleNameStr];
                        }
                    }
                }
            }
        }

        std::string outputFile = configDir + "config_engine_" + engineName + "_2.json";
        std::ofstream file(outputFile);
        if (file.is_open()) {
            file << std::setw(4) << engineConfig << std::endl;
            file.close();
            std::cout << "已保存引擎 " << engineName << " 的实际使用配置到 " << outputFile << std::endl;
        } else {
            std::cerr << "无法写入文件 " << outputFile << std::endl;
        }
    }
    
    // Save module registry info (this part seems unaffected by the GlobalConfig change)
    if (usedConfig.contains("registry")) {
        nlohmann::json registryConfig;
        registryConfig["registry"] = usedConfig["registry"];
        
        std::string registryFile = configDir + "config_registry_module_2.json";
        std::ofstream file(registryFile);
        if (file.is_open()) {
            file << std::setw(4) << registryConfig << std::endl;
            file.close();
            std::cout << "已保存模块注册信息到 " << registryFile << std::endl;
        } else {
            std::cerr << "无法写入文件 " << registryFile << std::endl;
        }
    } else {
        std::cerr << "警告: usedConfig 中缺少 'registry' 部分，无法保存模块注册信息。" << std::endl;
    }
}

/**
 * @brief Gets all modules for a specific engine.
 * @param engineName The name of the engine.
 * @param config The configuration JSON object.
 * @return A set of module names associated with the engine.
 */
std::unordered_set<std::string> getEngineModules(const std::string& engineName, const nlohmann::json& config) {
    std::unordered_set<std::string> modules;
    std::unordered_set<std::string> processedEngines;
    
    // 递归函数，收集引擎及其子引擎中的所有模块
    std::function<void(const std::string&)> collectModules = [&](const std::string& currEngineName) {
        // 防止循环引用
        if (processedEngines.find(currEngineName) != processedEngines.end()) {
            return;
        }
        processedEngines.insert(currEngineName);
        
        // 查找引擎
        for (const auto& engine : config["engine"]["enginePool"]) {
            if (engine["name"] == currEngineName) {
                // 收集当前引擎的模块
                if (engine.contains("modules")) {
                    for (const auto& module : engine["modules"]) {
                        // 无论模块是否启用，都添加到列表中
                        modules.insert(module["name"]);
                    }
                }
                
                // 递归处理子引擎
                if (engine.contains("subenginePool")) {
                    for (const auto& subEngineName : engine["subenginePool"]) {
                        collectModules(subEngineName);
                    }
                }
                
                break;
            }
        }
    };
    
    collectModules(engineName);
    return modules;
}

/**
 * @brief Executes the specified engine.
 * @param engineName The name of the engine to execute.
 * @return True if execution was successful, false otherwise.
 */
bool engineExecutionEngine::executeengine(const std::string& engineName, engineContext& parentContext) {
    auto& storage = ConfigurationStorage::instance();
    
    // 获取所有需要执行的模块（已在验证阶段收集并存储）
    std::vector<ModuleExecInfo> allModules;
    std::vector<std::string> executionOrder;
    
    // 查找当前引擎及其子引擎的执行顺序
    auto it = std::find(storage.engineExecutionOrder.begin(), storage.engineExecutionOrder.end(), engineName);
    if (it == storage.engineExecutionOrder.end()) {
        std::cerr << "错误：引擎 '" << engineName << "' 未在执行顺序中找到" << std::endl;
        return false;
    }
    
    // 收集引擎执行顺序和模块
    for (; it != storage.engineExecutionOrder.end(); ++it) {
        const std::string& engName = *it;
        executionOrder.push_back(engName);
        
        // 添加该引擎的模块
        auto moduleIt = storage.engineModules.find(engName);
        if (moduleIt != storage.engineModules.end()) {
            allModules.insert(allModules.end(), moduleIt->second.begin(), moduleIt->second.end());
        }
    }
    
    std::cout << "即将执行模块，共 " << allModules.size() << " 个模块..." << std::endl;
    
    // 初始化所有引擎的上下文
    for (const auto& engName : executionOrder) {
        // 确保引擎上下文存在
        if (storage.engineContexts.find(engName) == storage.engineContexts.end()) {
            std::cerr << "错误：找不到引擎 '" << engName << "' 的上下文" << std::endl;
            return false;
        }
        
        auto& engineContext = storage.engineContexts[engName];
        
        // 从父上下文继承参数
        if (engName != engineName) { // 不是主引擎时才需要继承
            for (const auto& [key, value] : parentContext.getParameters().items()) {
                engineContext->setParameter(key, value);
            }
        }
    }
    
    // 添加调试输出
    //std::cout << "引擎上下文初始化完毕，开始执行模块..." << std::endl;
    
    try {
        // 1. 全局构造阶段
        std::cout << "\n====== 全局构造阶段 ======" << std::endl;
        for (const auto& moduleInfo : allModules) {
            std::cout << "创建模块: " << moduleInfo.moduleName << " (引擎: " << moduleInfo.engineName << ")" << std::endl;
            try {
                if (storage.engineContexts.find(moduleInfo.engineName) == storage.engineContexts.end()) {
                    std::cerr << "错误：找不到引擎 '" << moduleInfo.engineName << "' 的上下文，无法创建模块 '" << moduleInfo.moduleName << "'" << std::endl;
                    throw std::runtime_error("引擎上下文不存在");
                }
                storage.engineContexts[moduleInfo.engineName]->createModule(moduleInfo.moduleName, moduleInfo.moduleParams);
            } catch (const std::exception& e) {
                std::cerr << "错误: 创建模块 '" << moduleInfo.moduleName << "' 失败: " << e.what() << std::endl;
                std::cerr << "确保模块已在正确的注册表中注册" << std::endl;
                throw;
            }
        }
        
        // 2. 全局初始化阶段
        std::cout << "\n====== 全局初始化阶段 ======" << std::endl;
        for (const auto& moduleInfo : allModules) {
            std::cout << "初始化模块: " << moduleInfo.moduleName << " (引擎: " << moduleInfo.engineName << ")" << std::endl;
            storage.engineContexts[moduleInfo.engineName]->initializeModule(moduleInfo.moduleName);
        }
        
        // 3. 全局执行阶段
        std::cout << "\n====== 全局执行阶段 ======" << std::endl;
        for (const auto& moduleInfo : allModules) {
            std::cout << "执行模块: " << moduleInfo.moduleName << " (引擎: " << moduleInfo.engineName << ")" << std::endl;
            storage.engineContexts[moduleInfo.engineName]->executeModule(moduleInfo.moduleName);
        }
        
        // 4. 全局释放阶段 (按构造的相反顺序)
        std::cout << "\n====== 全局释放阶段 ======" << std::endl;
        for (auto it = allModules.rbegin(); it != allModules.rend(); ++it) {
            std::cout << "释放模块: " << it->moduleName << " (引擎: " << it->engineName << ")" << std::endl;
            storage.engineContexts[it->engineName]->releaseModule(it->moduleName);
        }
        
    } catch (const std::exception& e) {
        std::cerr << "错误: " << e.what() << std::endl;
        std::cout << "尝试释放已创建的模块..." << std::endl;
        
        // 尝试释放已创建的模块 (按构造的相反顺序)
        for (auto it = allModules.rbegin(); it != allModules.rend(); ++it) {
            try {
                auto& ctx = storage.engineContexts[it->engineName];
                if (ctx->modules_.find(it->moduleName) != ctx->modules_.end()) {
                    std::cout << "释放模块: " << it->moduleName << " (引擎: " << it->engineName << ")" << std::endl;
                    ctx->releaseModule(it->moduleName);
                }
            } catch (const std::exception& e) {
                std::cerr << "释放模块 '" << it->moduleName << "' 时发生错误: " << e.what() << std::endl;
            }
        }
        return false;
    }
    
    // 添加调试输出
    //std::cout << "模块执行完毕，开始传递参数..." << std::endl;
    
    // 将所有子引擎的参数传回父上下文
    for (const auto& engName : executionOrder) {
        if (engName != engineName) { // 非主引擎
            for (const auto& [key, value] : storage.engineContexts[engName]->getParameters().items()) {
                parentContext.setParameter(key, value);
            }
        }
    }
    
    std::cout << "执行子引擎: " << engineName << " 完成" << std::endl;
    return true;
}

/**
 * @brief Constructor for ModuleTypeInfo.
 * @param n The module name.
 * @param f Function to get the parameter schema.
 */
ModuleTypeInfo::ModuleTypeInfo(const std::string& n, std::function<nlohmann::json()> f)
    : name(n), getParamSchemaFunc(f) {}

/**
 * @brief Registers a module type with the registry.
 * @param name The name of the module type.
 * @param schemaFunc Function to get the parameter schema.
 */
void ModuleTypeRegistry::registerType(const std::string& name, std::function<nlohmann::json()> schemaFunc) {
    moduleTypes_.emplace_back(name, schemaFunc);
}

/**
 * @brief Gets all registered module types.
 * @return A const reference to the vector of module type information.
 */
const std::vector<ModuleTypeInfo>& ModuleTypeRegistry::getModuleTypes() const {
    return moduleTypes_;
}

/**
 * @brief Gets the singleton instance of ModuleRegistryInitializer.
 * @return Reference to the singleton instance.
 */
ModuleRegistryInitializer& ModuleRegistryInitializer::init() {
    static ModuleRegistryInitializer initializer_instance;
    return initializer_instance;
}

/**
 * @brief Static instance to ensure module registration before main.
 */
static ModuleRegistryInitializer& moduleRegistryInit = ModuleRegistryInitializer::init();

/**
 * @brief Creates registry information as a JSON object.
 * @return A JSON object containing registry information.
 */
nlohmann::json createRegistryInfo() {
    nlohmann::json registry;
    registry["modules"] = nlohmann::json::array();

    // 添加 GlobalConfig 的定义
    nlohmann::json globalConfigModule;
    globalConfigModule["name"] = "GlobalConfig";
    globalConfigModule["enabled"] = true; // GlobalConfig 总是被认为是启用的
    globalConfigModule["description"] = "全局配置参数";
    globalConfigModule["parameters"] = {
        {"solver", {
            {"type", "string"},
            {"description", "求解器类型 (例如 SIMPLE, PISO)"},
            {"enum", {"SIMPLE", "PISO", "PIMPLE", "Coupled"}},
            {"default", "SIMPLE"}
        }},
        {"maxIterations", {
            {"type", "integer"},
            {"description", "求解过程中的最大迭代次数"},
            {"minimum", 1},
            {"maximum", 1000000},
            {"default", 1000}
        }},
        {"convergenceCriteria", {
            {"type", "number"},
            {"description", "求解收敛判断标准"},
            {"minimum", 0.0},
            {"exclusiveMinimum", true}, // 确保大于0
            {"maximum", 1.0},
            {"default", 1e-6}
        }},
        {"time_step", {
            {"type", "number"},
            {"description", "时间步长"},
            {"minimum", 0.0},
            {"exclusiveMinimum", true}, // 确保大于0
            {"default", 0.01}
        }}
        // 可以根据需要添加其他全局参数
    };
    registry["modules"].push_back(globalConfigModule);

    // 添加其他真实模块的注册信息
    auto& typeRegistry = ModuleSystem::ModuleTypeRegistry::instance();
    for (const auto& moduleType : typeRegistry.getModuleTypes()) {
        nlohmann::json mod;
        mod["name"] = moduleType.name;
        // 假设所有在类型注册表中的模块默认是可启用的，具体是否启用由配置文件决定
        mod["enabled"] = true; 
        mod["parameters"] = moduleType.getParamSchemaFunc();
        // 你可以在这里添加更多模块级别的元数据，如 "description"
        // mod["description"] = "模块 " + moduleType.name + " 的描述";
        registry["modules"].push_back(mod);
    }
    return registry;
}




/**
 * @brief Creates engine information as a JSON object.
 * @return A JSON object containing engine information.
 */
nlohmann::json createengineInfo() {
    nlohmann::json engineInfo;
    engineInfo["enginePool"] = nlohmann::json::array();
    
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

/**
 * @brief Prints the usage message for the program.
 * @param progName The name of the program.
 */
void printUsageMessage(const char* progName) {
    std::cerr << "\n用法: " << progName << " [选项]\n\n"
              << "选项:\n"
              << "  --config-dir <目录路径>\n"
              << "                           指定包含配置文件的目录。\n"
              << "                           如果未提供此选项，程序将默认尝试从 './config/' 目录加载配置。\n\n"
              << "  --generate-templates [目录路径]\n"
              << "                           生成所有必需的配置文件模板到指定的目录路径。\n"
              << "                           如果未提供目录路径，模板将生成在 './templates/' 目录中。\n"
              << "                           此选项执行后程序将退出。\n\n"
              << "  --help\n"
              << "                           显示此帮助信息并退出。\n\n"
              << "预期配置文件 (应位于配置目录中):\n"
              << "  - config_engine_mainProcess.json\n"
              << "  - config_engine_PreGrid.json\n"
              << "  - config_engine_Solve.json\n"
              << "  - config_engine_Post.json\n"
              << "  - config_registry_module.json\n\n"
              << "如果配置目录中未找到任何这些文件，程序将报错并退出。\n";
}

/**
 * @brief Generates a default configuration and saves it to a file.
 * @param argc The number of command line arguments.
 * @param argv The command line arguments.
 */
void getConfig(int argc, char* argv[]){
    std::string configDir = "./config/"; // 默认配置文件目录
    bool generateTemplates = false;
    std::string templateDir = "./templates/"; // 默认模板生成目录
    bool configDirSpecifiedByArg = false;

    // 解析命令行参数
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "--config-dir" && i + 1 < argc) {
            configDir = argv[++i];
            if (configDir.back() != '/') configDir += '/';
            configDirSpecifiedByArg = true;
        } else if (arg == "--generate-templates") {
            generateTemplates = true;
            if (i + 1 < argc && argv[i+1][0] != '-' && argv[i+1][0] != '\0') {
                templateDir = argv[++i];
                if (templateDir.back() != '/') templateDir += '/';
            }
            // 若未指定目录，则使用默认的 "./templates/"
        } else if (arg == "--help") {
            printUsageMessage(argv[0]);
            exit(0);
        } else {
            std::cerr << "错误: 未知参数或参数格式错误 '" << arg << "'\n";
            printUsageMessage(argv[0]);
            exit(1);
        }
    }
    
    if (generateTemplates) {
        try {
            if (!std::filesystem::exists(templateDir)) {
                if (std::filesystem::create_directories(templateDir)) {
                    std::cout << "已创建模板目录: " << templateDir << std::endl;
                } else {
                    std::cerr << "错误: 无法创建模板目录 '" << templateDir << "'。" << std::endl;
                    exit(1);
                }
            } else if (!std::filesystem::is_directory(templateDir)) {
                 std::cerr << "错误: 指定的模板路径 '" << templateDir << "' 已存在但不是一个目录。" << std::endl;
                 exit(1);
            }
        } catch (const std::filesystem::filesystem_error& e) {
            std::cerr << "文件系统错误 (操作模板目录 '" << templateDir << "'): " << e.what() << std::endl;
            exit(1);
        }
        generateTemplateConfigs(templateDir); // 此函数内部应有 exit(0)
        return; 
    }
    
    // 检查配置目录的有效性
    try {
        if (!std::filesystem::exists(configDir)) {
            if (configDirSpecifiedByArg) { 
                std::cout << "通知: 指定的配置目录 '" << configDir << "' 不存在，尝试创建..." << std::endl;
                if (!std::filesystem::create_directories(configDir)) {
                    std::cerr << "错误: 无法创建指定的配置目录 '" << configDir << "'。\n" << std::endl;
                    printUsageMessage(argv[0]);
                    exit(1);
                }
                std::cout << "已成功创建配置目录: " << configDir << std::endl;
            } else { 
                std::cerr << "错误: 默认配置目录 '" << configDir << "' 不存在。\n"
                          << "请创建该目录并放入配置文件，或使用 '--config-dir' 指定其他目录，或使用 '--generate-templates' 生成模板。\n" << std::endl;
                printUsageMessage(argv[0]);
                exit(1);
            }
        } else if (!std::filesystem::is_directory(configDir)) {
            std::cerr << "错误: 配置路径 '" << configDir << "' 已存在但不是一个目录。\n" << std::endl;
            printUsageMessage(argv[0]);
            exit(1);
        }
    } catch (const std::filesystem::filesystem_error& e) {
        std::cerr << "文件系统错误 (操作配置目录 '" << configDir << "'): " << e.what() << std::endl;
        exit(1);
    }
    
    nlohmann::json mergedConfig;
    std::vector<std::string> baseConfigFileNames = {
        "config_engine_mainProcess.json", "config_engine_PreGrid.json",
        "config_engine_Solve.json", "config_engine_Post.json",
        "config_registry_module.json"
    };
    std::vector<std::string> configFilesToLoad;
    for(const auto& name : baseConfigFileNames) {
        configFilesToLoad.push_back(configDir + name);
    }
    
    bool anyFileFound = false;
    
    for (const auto& filePath : configFilesToLoad) {
        std::ifstream configFileStream(filePath);
        if (configFileStream.is_open()) {
            anyFileFound = true;
            std::cout << "正在加载配置文件: " << filePath << std::endl;
            
            nlohmann::json fileConfig;
            try {
                configFileStream >> fileConfig;
                
                // 合并 engine 部分 (保持原始逻辑: 覆盖同名引擎，追加新引擎)
                if (fileConfig.contains("engine")) {
                    if (!mergedConfig.contains("engine")) {
                        mergedConfig["engine"] = fileConfig["engine"];
                    } else {
                        if (mergedConfig["engine"].is_object() && fileConfig["engine"].is_object() &&
                            fileConfig["engine"].contains("enginePool") && fileConfig["engine"]["enginePool"].is_array()) {
                            
                            if (!mergedConfig["engine"].contains("enginePool") || !mergedConfig["engine"]["enginePool"].is_array()) {
                                mergedConfig["engine"]["enginePool"] = nlohmann::json::array(); // 初始化/重置为数组
                            }

                            for (const auto& engineEntry : fileConfig["engine"]["enginePool"]) {
                                if (!engineEntry.contains("name") || !engineEntry["name"].is_string()) continue;
                                std::string engineName = engineEntry["name"].get<std::string>();
                                bool found = false;
                                for (auto& existingEngine : mergedConfig["engine"]["enginePool"]) {
                                    if (existingEngine.contains("name") && existingEngine["name"].is_string() &&
                                        existingEngine["name"].get<std::string>() == engineName) {
                                        existingEngine = engineEntry; 
                                        found = true;
                                        break;
                                    }
                                }
                                if (!found) {
                                    mergedConfig["engine"]["enginePool"].push_back(engineEntry);
                                }
                            }
                        } else if (mergedConfig["engine"].is_object() && fileConfig["engine"].is_object() &&
                                   !fileConfig["engine"].contains("enginePool") && fileConfig["engine"].size() > 0) {
                            // 如果 fileConfig["engine"] 不是空的但没有 enginePool，可能需要合并其他 engine 顶级字段
                            // 为简化，这里可以不做操作或用 merge_patch
                            // mergedConfig["engine"].merge_patch(fileConfig["engine"]);
                        }
                    }
                }
                
                // 合并 config 部分 (保持原始逻辑: 键值覆盖)
                if (fileConfig.contains("config")) {
                    if (!mergedConfig.contains("config") || !mergedConfig["config"].is_object()) {
                        mergedConfig["config"] = fileConfig["config"];
                    } else {
                         if (fileConfig["config"].is_object()) {
                            for (auto it = fileConfig["config"].begin(); it != fileConfig["config"].end(); ++it) {
                                mergedConfig["config"][it.key()] = it.value();
                            }
                        }
                    }
                }
                
                // 合并 registry 部分 (保持原始逻辑: 直接覆盖)
                if (fileConfig.contains("registry")) {
                    mergedConfig["registry"] = fileConfig["registry"];
                }
                
            } catch (const nlohmann::json::parse_error& e) {
                std::cerr << "错误: 解析JSON文件 '" << filePath << "' 失败: " << e.what()
                          << " (在字节偏移 " << e.byte << " 附近)。请检查文件格式。\n" << std::endl;
                // exit(1); // 关键配置文件解析失败，选择退出
            } catch (const std::exception& e) {
                std::cerr << "处理文件 '" << filePath << "' 时发生未知错误: " << e.what() << std::endl;
                // exit(1); 
            }
            configFileStream.close();
        }
    }
    
    if (!anyFileFound) {
        std::cerr << "错误: 在配置目录 '" << configDir << "' 中未找到任何有效的配置文件。\n"
                  << "请确保该目录下至少存在以下一个或多个配置文件，并且它们是可读的JSON格式：\n";
        for(size_t i = 0; i < baseConfigFileNames.size(); ++i) {
            std::cerr << "  - " << baseConfigFileNames[i] << " (期望路径: " << configFilesToLoad[i] << ")\n";
        }
        std::cerr << std::endl;
        printUsageMessage(argv[0]);
        exit(1);
    }
    
    if (!ModuleSystem::paramValidation(mergedConfig)) {
        std::cerr << "错误: 合并后的配置未能通过验证。请检查配置文件内容和结构是否符合规范。\n" << std::endl;
        exit(1);
    }
    
    saveUsedConfigs(mergedConfig, configDir);
}

/**
 * @brief Validates a parameter against its schema.
 * @param paramSchema The parameter schema JSON object.
 * @param value The parameter value JSON object.
 * @return An empty string if validation passes, otherwise an error message.
 */
std::string validateParam(const nlohmann::json& paramSchema, const nlohmann::json& value) {
    if (paramSchema.contains("type")) {
        std::string expectedType = paramSchema["type"];
        if (expectedType == "string") {
            if (!value.is_string()) return "期望字符串类型，但获取到" + value.dump();
            if (paramSchema.contains("enum")) {
                auto enumValues = paramSchema["enum"].get<std::vector<std::string>>();
                if (std::find(enumValues.begin(), enumValues.end(), value.get<std::string>()) == enumValues.end()) {
                    std::string errorMsg = "值必须是以下之一: ";
                    for (size_t i = 0; i < enumValues.size(); i++) {
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

/**
 * @brief Validates module parameters against their schema.
 * @param moduleParams The module parameters JSON object.
 * @param moduleName The name of the module.
 * @param rank The rank of the module, defaults to 0.
 * @throws std::runtime_error If the module schema is not found or parameters are invalid.
 */
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

/**
 * @brief Constructor for engineExecutionEngine.
 * @param engines The engine configuration JSON object.
 * @param context Reference to the engine context.
 */
engineExecutionEngine::engineExecutionEngine(const nlohmann::json& engines, 
                                             ModuleSystem::engineContext& context)
    : engines_(engines), context_(context) {}

/**
 * @brief Gets the effective parameters for a module.
 * @param moduleConfig The module configuration JSON object.
 * @param moduleName The name of the module.
 * @param engineSpecificParams Engine-specific parameters JSON object.
 * @return A JSON object with the effective module parameters.
 */
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

/**
 * @brief Initializes the engine contexts.
 */
void ConfigurationStorage::initializeEngineContexts() {
    if (!mainContext) {
        std::cerr << "错误：无法初始化引擎上下文，mainContext 未创建" << std::endl;
        return;
    }
    
    // 清除之前可能存在的上下文
    engineContexts.clear();
    
    // 递归函数，用于创建引擎及其子引擎的上下文
    std::function<void(const std::string&, std::shared_ptr<engineContext>)> 
    createContextsRecursively = [this, &createContextsRecursively](
        const std::string& engineName, 
        std::shared_ptr<engineContext> parentContext) {
        
        // 创建引擎上下文
        if (engineContexts.find(engineName) == engineContexts.end()) {
            engineContexts[engineName] = std::make_shared<engineContext>(*registry, engine.get());
            engineContexts[engineName]->setEngineName(engineName);
            
            // 从父上下文继承参数
            if (parentContext) {
                for (const auto& [key, value] : parentContext->getParameters().items()) {
                    engineContexts[engineName]->setParameter(key, value);
                }
            } else {
                // 从主上下文继承参数
                for (const auto& [key, value] : mainContext->getParameters().items()) {
                    engineContexts[engineName]->setParameter(key, value);
                }
            }
            
            // 设置允许的模块
            std::unordered_set<std::string> engineModules;
            auto modules = EngineModuleMapping::instance().getEngineModules(engineName);
            for (const auto& moduleName : modules) {
                engineModules.insert(moduleName);
            }

            // 添加未绑定到任何引擎的模块（全局可用模块）
            for (const auto& moduleName : ConfigurationStorage::instance().enabledModules) {
                if (!EngineModuleMapping::instance().isModuleBoundToEngine(moduleName)) {
                    engineModules.insert(moduleName);
                }
            }
            engineContexts[engineName]->setAllowedModules(engineModules);
            
            // 查找该引擎是否有子引擎
            for (const auto& engine : config["engine"]["enginePool"]) {
                if (engine["name"].get<std::string>() == engineName && 
                    engine.contains("subenginePool") && 
                    engine["subenginePool"].is_array()) {
                    
                    // 递归创建子引擎的上下文
                    for (const auto& subEngine : engine["subenginePool"]) {
                        std::string subEngineName = subEngine.get<std::string>();
                        createContextsRecursively(subEngineName, engineContexts[engineName]);
                    }
                    break;
                }
            }
        }
    };
    
    // 从主引擎开始创建上下文
    createContextsRecursively("mainProcess", nullptr);
}

/**
 * @brief Collects module information from a configuration for an engine.
 * @param config The configuration JSON object.
 * @param engineName The name of the engine.
 * @param visitedEngines Set of visited engines to detect circular dependencies.
 * @return True if collection is successful, false otherwise.
 */
bool collectModulesFromConfig(const nlohmann::json& config, 
                            const std::string& engineName,
                            std::unordered_set<std::string>& visitedEngines) {
    if (visitedEngines.count(engineName)) {
        std::cerr << "错误: 检测到工作流循环依赖: " << engineName << std::endl;
        return false;
    }
    
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
    
    if (engineDef.contains("subenginePool") && engineDef["subenginePool"].is_array()) {
        for (const auto& subEngineName : engineDef["subenginePool"]) {
            if (!collectModulesFromConfig(config, subEngineName.get<std::string>(), visitedEngines)) {
                return false;
            }
        }
    }
    
    if (engineDef.contains("modules") && engineDef["modules"].is_array()) {
        for (const auto& moduleInfo : engineDef["modules"]) {
            if (!moduleInfo.contains("name")) {
                std::cerr << "错误: 引擎 '" << engineName << "' 中的模块定义缺少名称" << std::endl;
                return false;
            }
            
            const std::string& moduleName = moduleInfo["name"];
            bool moduleEnabled = moduleInfo.value("enabled", true);
            
            if (!moduleEnabled) {
                continue;
            }
            
            nlohmann::json moduleParams = getEffectiveModuleParams(
                ConfigurationStorage::instance().config["config"], 
                moduleName,
                moduleInfo.contains("params") ? moduleInfo["params"] : nlohmann::json(nullptr)
            );
            
            collectedModules.push_back({engineName, moduleName, moduleParams});
            
            std::cout << "收集模块: " << moduleName << " 从引擎: " << engineName << std::endl;
        }
    }
    
    visitedEngines.erase(engineName);
    return true;
}

/**
 * @brief Validates the configuration parameters.
 * @param config The configuration JSON object.
 * @return True if validation passes, false otherwise.
 */
bool paramValidation(const nlohmann::json& config) {
    ConfigurationStorage::instance().clear();
    ConfigurationStorage::instance().initializeRegistryAndEngine();
    auto& storage = ConfigurationStorage::instance();
    
    storage.config = config; // 存储原始传入的完整配置

    // 1. 构建所有模块（包括GlobalConfig）的参数 Schema 映射 和 已知模块名称集合
    // 这个 registryInfoJson 是所有模块定义的权威来源，包括 GlobalConfig
    nlohmann::json registryInfoJson = createRegistryInfo(); 
    std::unordered_map<std::string, nlohmann::json> moduleParamSchemas;
    std::unordered_set<std::string> knownModulesFromRegistry;

    if (registryInfoJson.contains("modules") && registryInfoJson["modules"].is_array()) {
        for (const auto& moduleDef : registryInfoJson["modules"]) {
            if (moduleDef.contains("name") && moduleDef["name"].is_string()) {
                std::string moduleName = moduleDef["name"].get<std::string>();
                knownModulesFromRegistry.insert(moduleName);
                if (moduleDef.contains("parameters") && moduleDef["parameters"].is_object()) {
                    moduleParamSchemas[moduleName] = moduleDef["parameters"];
                } else {
                    moduleParamSchemas[moduleName] = nlohmann::json::object(); // 模块可能没有参数
                }
            }
        }
    }
    storage.knownModules = knownModulesFromRegistry; // 更新 storage 中的已知模块列表

    // 2. 验证引擎定义 (engine.enginePool)
    std::unordered_set<std::string> definedEngineNames;
    if (config.contains("engine") && config["engine"].is_object() && config["engine"].contains("enginePool") && config["engine"]["enginePool"].is_array()) {
        for (const auto& engineDef : config["engine"]["enginePool"]) {
            if (!engineDef.contains("name") || !engineDef["name"].is_string()) {
                std::cerr << "引擎定义错误: 存在未命名或名称非字符串的引擎。" << std::endl;
                return false;
            }
            std::string engineName = engineDef["name"].get<std::string>();
            if (!definedEngineNames.insert(engineName).second) {
                std::cerr << "引擎定义错误: 发现重复的引擎名称 '" << engineName << "'" << std::endl;
                return false;
            }

            if (engineDef.contains("modules") && engineDef["modules"].is_array()) {
                for (const auto& moduleInfo : engineDef["modules"]) {
                    if (!moduleInfo.contains("name") || !moduleInfo["name"].is_string()) {
                        std::cerr << "引擎 '" << engineName << "' 中的模块定义错误: 模块缺少名称或名称非字符串。" << std::endl;
                        return false;
                    }
                    std::string moduleNameInEngine = moduleInfo["name"].get<std::string>();
                    if (knownModulesFromRegistry.find(moduleNameInEngine) == knownModulesFromRegistry.end()) {
                        std::cerr << "引擎 '" << engineName << "' 中包含未知模块类型 '" << moduleNameInEngine << "'" << std::endl;
                        return false;
                    }
                    // 模块在引擎中声明的参数 (engineSpecificParams) 将在执行时与全局模块配置合并
                    // 此处的参数验证主要针对全局模块配置 (config.config.ModuleName)
                }
            }
        }
        // 验证子引擎是否存在
        for (const auto& engineDef : config["engine"]["enginePool"]) {
            if (engineDef.contains("subenginePool") && engineDef["subenginePool"].is_array()) {
                std::string parentEngineName = engineDef["name"].get<std::string>();
                for (const auto& subEngineNameJson : engineDef["subenginePool"]) {
                    if (!subEngineNameJson.is_string()) {
                        std::cerr << "引擎 '" << parentEngineName << "' 的子引擎池中存在非字符串条目。" << std::endl;
                        return false;
                    }
                    std::string subEngineName = subEngineNameJson.get<std::string>();
                    if (definedEngineNames.find(subEngineName) == definedEngineNames.end()) {
                        std::cerr << "错误: 引擎 '" << parentEngineName << "' 的子引擎 '" << subEngineName << "' 未在配置中定义。" << std::endl;
                        return false;
                    }
                }
            }
        }
    } else {
        std::cerr << "配置错误: 缺少 'engine.enginePool' 定义或格式不正确。" << std::endl;
        return false;
    }

    // 3. 验证配置参数 (config.config)
    storage.moduleConfig = nlohmann::json::object(); // 用于存储普通模块的配置
    storage.globalParams = nlohmann::json::object(); // 用于存储 GlobalConfig 的参数

    if (config.contains("config") && config["config"].is_object()) {
        for (auto it_cfg = config["config"].begin(); it_cfg != config["config"].end(); ++it_cfg) {
            const std::string& configKey = it_cfg.key();
            const nlohmann::json& configValue = it_cfg.value();

            if (!configValue.is_object()) {
                std::cerr << "配置错误: 'config." << configKey << "' 的值必须是一个JSON对象。" << std::endl;
                return false;
            }

            if (moduleParamSchemas.find(configKey) == moduleParamSchemas.end()) {
                std::cerr << "配置警告: 'config." << configKey << "' 不是一个已知的模块或GlobalConfig，将被忽略。" << std::endl;
                continue; 
            }
            
            const nlohmann::json& currentSchema = moduleParamSchemas.at(configKey);

            // 验证此配置块中的所有参数
            for (auto it_param = configValue.begin(); it_param != configValue.end(); ++it_param) {
                const std::string& paramName = it_param.key();
                const nlohmann::json& paramValue = it_param.value();

                if (!currentSchema.contains(paramName)) {
                    std::cerr << "配置错误: 在 '" << configKey << "' 中发现未知参数 '" << paramName << "'" << std::endl;
                    return false;
                }
                std::string errorMsg = validateParam(currentSchema[paramName], paramValue);
                if (!errorMsg.empty()) {
                    std::cerr << "配置错误: 在 '" << configKey << "." << paramName << "' 参数验证失败: " << errorMsg << std::endl;
                    return false;
                }
            }
            // 检查必需参数是否缺失 (根据 schema 中的 "required" 字段，如果存在的话)
            // JSON schema standard typically uses a "required" array at the object level.
            // Our current per-parameter schema might have a "required": true boolean.
            for(auto schema_it = currentSchema.begin(); schema_it != currentSchema.end(); ++schema_it) {
                const std::string& schemaParamName = schema_it.key();
                const nlohmann::json& schemaParamDef = schema_it.value();
                if (schemaParamDef.contains("required") && schemaParamDef["required"].get<bool>()) {
                    if (!configValue.contains(schemaParamName)) {
                        std::cerr << "配置错误: '" << configKey << "' 缺少必需参数 '" << schemaParamName << "'" << std::endl;
                        return false;
                    }
                }
            }

            if (configKey == "GlobalConfig") {
                storage.globalParams = configValue;
            } else {
                storage.moduleConfig[configKey] = configValue;
            }
        }
    } else {
        std::cout << "警告: 配置中没有 'config' 节点。如果模块需要参数，请提供。" << std::endl;
        // 如果 GlobalConfig 是必需的，这里可能需要错误处理或加载默认值
        if (moduleParamSchemas.count("GlobalConfig")) {
             const nlohmann::json& gcSchema = moduleParamSchemas.at("GlobalConfig");
             for(auto it = gcSchema.begin(); it != gcSchema.end(); ++it) {
                 if (it.value().contains("default")) {
                     storage.globalParams[it.key()] = it.value()["default"];
                 } else if (it.value().contains("required") && it.value()["required"].get<bool>()) {
                     std::cerr << "配置错误: 缺少必需的 GlobalConfig 参数 '" << it.key() << "' 且无默认值。" << std::endl;
                     return false;
                 }
             }
             std::cout << "提示: 已为 GlobalConfig 加载默认参数。" << std::endl;
        }
    }
    
    // 4. 验证模块注册表 (config.registry)
    std::unordered_set<std::string> enabledModulesFromConfig;
    if (config.contains("registry") && config["registry"].is_object() && config["registry"].contains("modules") && config["registry"]["modules"].is_array()) {
        for (const auto& moduleRegEntry : config["registry"]["modules"]) {
            if (!moduleRegEntry.contains("name") || !moduleRegEntry["name"].is_string()) {
                std::cerr << "模块注册表错误: 存在未命名或名称非字符串的模块条目。" << std::endl;
                return false;
            }
            std::string moduleName = moduleRegEntry["name"].get<std::string>();
            
            // GlobalConfig 在注册表模板中可能出现，但其启用状态不由用户配置文件控制，它总是“存在”
            if (moduleName == "GlobalConfig") continue; 

            if (knownModulesFromRegistry.find(moduleName) == knownModulesFromRegistry.end()) {
                std::cerr << "模块注册表错误: 模块 '" << moduleName << "' 未在已知模块类型中定义。" << std::endl;
                return false;
            }
            if (moduleRegEntry.contains("enabled") && moduleRegEntry["enabled"].is_boolean() && moduleRegEntry["enabled"].get<bool>()) {
                enabledModulesFromConfig.insert(moduleName);
            }
            // 此处可以添加对注册表中 "parameters" 覆盖的验证，但不推荐在运行时修改基础 schema
        }
    } else {
        std::cerr << "配置错误: 缺少 'registry.modules' 定义或格式不正确。" << std::endl;
        return false;
    }
    storage.enabledModules = enabledModulesFromConfig;

    // 5. 检查引擎引用的模块是否已在注册表中启用
    for (const auto& engineDef : config["engine"]["enginePool"]) {
        if (!engineDef.value("enabled", false)) continue; // 只检查启用的引擎
        std::string engineName = engineDef["name"].get<std::string>();
        if (engineDef.contains("modules") && engineDef["modules"].is_array()) {
            for (const auto& moduleInfo : engineDef["modules"]) {
                 if (!moduleInfo.value("enabled", false)) continue; // 只检查引擎中启用的模块实例
                std::string moduleNameInEngine = moduleInfo["name"].get<std::string>();
                if (moduleNameInEngine == "GlobalConfig") { // GlobalConfig 不应作为普通模块列在引擎中
                     std::cerr << "配置错误: GlobalConfig不应作为模块列在引擎 '" << engineName << "' 中。" << std::endl;
                     return false;
                }
                if (enabledModulesFromConfig.find(moduleNameInEngine) == enabledModulesFromConfig.end()) {
                    std::cerr << "错误: 引擎 '" << engineName << "' 引用了模块 '" << moduleNameInEngine << "', 但该模块未在注册表中启用或定义。" << std::endl;
                    return false;
                }
            }
        }
    }
    
    // 6. 检查主引擎 'mainProcess' 是否存在且已启用
    bool mainProcessExistsAndEnabled = false;
    if (config.contains("engine") && config["engine"].is_object() && config["engine"].contains("enginePool")) {
        for (const auto& engineDef : config["engine"]["enginePool"]) {
            if (engineDef.contains("name") && engineDef["name"].get<std::string>() == "mainProcess" &&
                engineDef.contains("enabled") && engineDef["enabled"].get<bool>()) {
                mainProcessExistsAndEnabled = true;
                break;
            }
        }
    }
    if (!mainProcessExistsAndEnabled) {
        std::cerr << "错误: 主引擎 'mainProcess' 未定义或未在配置中启用。" << std::endl;
        return false;
    }

    // 7. 检查引擎依赖循环 (与之前逻辑类似，此处省略以保持简洁，假设它已正确实现)
    std::function<bool(const std::string&, 
                       std::unordered_set<std::string>&, 
                       std::unordered_set<std::string>&, 
                       const nlohmann::json&)>
    detectCycle = 
        [&](const std::string& currentEngineName, 
            std::unordered_set<std::string>& visited, 
            std::unordered_set<std::string>& recursionStack,
            const nlohmann::json& enginePool) -> bool {
        
        visited.insert(currentEngineName);
        recursionStack.insert(currentEngineName);

        for (const auto& engineDef : enginePool) {
            if (engineDef.contains("name") && engineDef["name"].get<std::string>() == currentEngineName) {
                if (engineDef.contains("subenginePool") && engineDef["subenginePool"].is_array()) {
                    for (const auto& subEngineNameJson : engineDef["subenginePool"]) {
                        std::string subEngineName = subEngineNameJson.get<std::string>();
                        
                        // 检查子引擎是否在定义的引擎列表中
                        bool subEngineDefined = false;
                        for (const auto& eng : enginePool) {
                            if (eng.contains("name") && eng["name"].get<std::string>() == subEngineName) {
                                subEngineDefined = true;
                                break;
                            }
                        }
                        if (!subEngineDefined) {
                            std::cerr << "错误: 引擎 '" << currentEngineName << "' 的子引擎 '" << subEngineName << "' 未在引擎池中定义。" << std::endl;
                            // 或者可以决定是否要因为这个错误而返回 true (表示检测到问题)
                            // return true; 
                            continue; // 当前跳过未定义的子引擎
                        }


                        if (recursionStack.count(subEngineName)) {
                            std::cerr << "错误: 检测到引擎间的循环依赖: ";
                            // 可以选择打印更详细的循环路径，但这会增加复杂性
                            // 简单打印涉及的引擎
                            std::string path_str;
                            for(const std::string& node : recursionStack){
                                path_str += node + " -> ";
                            }
                            path_str += subEngineName;
                            std::cerr << path_str << std::endl;
                            return true; // 发现循环
                        }
                        if (!visited.count(subEngineName)) {
                            if (detectCycle(subEngineName, visited, recursionStack, enginePool)) {
                                return true; // 从递归调用中发现循环
                            }
                        }
                    }
                }
                break; // 找到了当前引擎的定义，处理完毕
            }
        }
        recursionStack.erase(currentEngineName); // 从递归栈中移除当前引擎
        return false; // 未在此路径发现循环
    };

    if (config.contains("engine") && config["engine"].is_object() && config["engine"].contains("enginePool")) {
        const auto& enginePool = config["engine"]["enginePool"];
        std::unordered_set<std::string> visitedEnginesForCycleCheck;
        std::unordered_set<std::string> recursionStackForCycleCheck;
        for (const auto& engineDef : enginePool) {
            if (engineDef.contains("name") && engineDef["name"].is_string()) {
                std::string engineName = engineDef["name"].get<std::string>();
                if (!visitedEnginesForCycleCheck.count(engineName)) {
                    if (detectCycle(engineName, visitedEnginesForCycleCheck, recursionStackForCycleCheck, enginePool)) {
                        return false; // 如果检测到循环，验证失败
                    }
                }
            }
        }
    }

    // 8. 注册模块和定义引擎 (与之前逻辑类似)
    auto& registry_ptr = storage.registry; // 使用 .get() 获取裸指针如果需要
    auto& engine_ptr_storage = storage.engine; 

    // 注册启用的模块到 AdvancedRegistry
    for (const auto& moduleName : storage.enabledModules) {
        ModuleFactory::instance().registerModule(registry_ptr.get(), moduleName);
        // 实际的注册（如 reg->Register<Type>(name)）应该由 ModuleFactory 内部处理
        // 这里我们假设 AdvancedRegistry 能够通过名称查找并创建模块，
        // 或者 ModuleFactory 已经用某种方式填充了 AdvancedRegistry
    }
    
    if (config.contains("engine") && config["engine"].contains("enginePool")) {
        for (const auto& EngineDef : config["engine"]["enginePool"]) {
            if (!EngineDef.contains("name") || !EngineDef.value("enabled", false)) continue;
            std::string EngineName = EngineDef["name"].get<std::string>();
            engine_ptr_storage->defineengine(EngineName, 
                [EngineDef, EngineName](ModuleSystem::engineContext& context_lambda) {
                    // Placeholder
                });
        }
        storage.enginesAreDefined = true;
    }
    
    // 将解析后的配置（包含GlobalConfig和模块特定配置）传递给引擎构建
    // Nestedengine::Build 可能需要调整以区分全局参数和模块参数，或期望特定结构
    if (config.contains("config")) {
         engine_ptr_storage->Build(config["config"]); 
    }
    
    // 初始化主上下文并设置参数
    storage.mainContext = std::make_shared<ModuleSystem::engineContext>(*registry_ptr, engine_ptr_storage.get());
    if (config.contains("config")) {
        // 将整个 config.config (包含 GlobalConfig 和模块配置) 设置为一个名为 "config" 的参数
        storage.mainContext->setParameter("config", config["config"]);

        // 单独设置解析后的全局参数到主上下文 (方便直接访问)
        for (auto& [gKey, gValue] : storage.globalParams.items()) {
            storage.mainContext->setParameter(gKey, gValue);
        }
        // 单独设置模块特定配置到主上下文 (方便直接访问)
        // 注意：模块实例创建时会接收更具体的参数
        for (auto& [moduleName, params] : storage.moduleConfig.items()) {
            storage.mainContext->setParameter(moduleName, params);
        }
    }
    
    // 9. 收集模块执行顺序 (与之前逻辑类似)
    collectedModules.clear();
    std::unordered_set<std::string> visitedEnginesForCollection;
    if (!collectModulesFromConfig(config["engine"], "mainProcess", visitedEnginesForCollection)) {
        std::cerr << "错误: 收集模块执行顺序失败。" << std::endl;
        return false;
    }
    
    // 10. 存储引擎模块和执行顺序 (与之前逻辑类似)
    storage.engineModules.clear();
    storage.engineExecutionOrder.clear();
    std::vector<std::string> tempExecutionOrder;
    std::unordered_set<std::string> processedEnginesForOrder;
    
    std::function<void(const std::string&)> collectEngineOrder = 
        [&](const std::string& engineName_co) {
            // ... (之前的 collectEngineOrder 逻辑) ...
            if (processedEnginesForOrder.count(engineName_co) > 0) return;
            processedEnginesForOrder.insert(engineName_co);
            tempExecutionOrder.push_back(engineName_co);
            storage.engineModules[engineName_co] = {};
            for (const auto& moduleInfo : collectedModules) {
                if (moduleInfo.engineName == engineName_co) {
                    storage.engineModules[engineName_co].push_back(moduleInfo);
                }
            }
            if (config.contains("engine") && config["engine"].contains("enginePool")) {
                for (const auto& engine_def_co : config["engine"]["enginePool"]) {
                    if (engine_def_co.contains("name") && engine_def_co["name"].get<std::string>() == engineName_co && 
                        engine_def_co.contains("subenginePool") && 
                        engine_def_co["subenginePool"].is_array()) {
                        for (const auto& subEngineJson_co : engine_def_co["subenginePool"]) {
                            std::string subEngineName_co = subEngineJson_co.get<std::string>();
                            collectEngineOrder(subEngineName_co);
                        }
                        break;
                    }
                }
            }
        };
    
    collectEngineOrder("mainProcess");
    storage.engineExecutionOrder = tempExecutionOrder;
    storage.initializeEngineContexts();

    std::cout << "参数验证通过。" << std::endl;
    return true;
}

/**
 * @brief Runs the module system by executing the main process engine.
 */
void run() {
    auto& storage = ConfigurationStorage::instance();
    const nlohmann::json& config = storage.config;
    
    auto registry = storage.registry;
    auto context = storage.mainContext;

    engineExecutionEngine executionEngine(config["engine"], *context);
    
    std::cout << "开始执行主引擎工作流..." << std::endl;
    if (!executionEngine.executeengine("mainProcess", *context)) {
        std::cerr << "引擎执行失败" << std::endl;
    } else {
        std::cout << "引擎执行成功" << std::endl;
    }
    
    auto leakedModules = registry->checkLeakedModules();
    if (!leakedModules.empty()) {
        std::cerr << "警告: 检测到未释放的模块:" << std::endl;
        for (const auto& module : leakedModules) {
            std::cerr << "  - " << module << std::endl;
        }
    }
}

/**
 * @brief Tests the module system by manually executing selected modules.
 */
void test() {
    auto& storage = ConfigurationStorage::instance();
    
    if (!storage.registry || !storage.engine || !storage.mainContext || !storage.enginesAreDefined) {
        std::cerr << "错误: 测试前请先运行 paramValidation 函数" << std::endl;
        return;
    }
    
    std::cout << "\n======== 开始模块手动测试 ========\n" << std::endl;
    
    try {
        if (storage.enabledModules.find("PreCGNS") != storage.enabledModules.end()) {
            std::cout << "测试 PreCGNS 模块..." << std::endl;

            // 使用 PreGrid 引擎的上下文
            auto& engineContext = storage.engineContexts["PreGrid"];

            nlohmann::json moduleParams;
            if (storage.moduleConfig.contains("PreCGNS")) {
                moduleParams = storage.moduleConfig["PreCGNS"];
            } else {
                moduleParams = {
                    {"cgns_type", "HDF5"},
                    {"cgns_value", 20}
                };
            }
            
            void* moduleInstance = engineContext->createModule("PreCGNS", moduleParams);
            std::cout << " - 创建 PreCGNS 成功" << std::endl;
            
            engineContext->initializeModule("PreCGNS");
            std::cout << " - 初始化 PreCGNS 成功" << std::endl;
            
            engineContext->executeModule("PreCGNS");
            std::cout << " - 执行 PreCGNS 成功" << std::endl;
            
            engineContext->releaseModule("PreCGNS");
            std::cout << " - 释放 PreCGNS 成功" << std::endl;
        }
        
        if (storage.enabledModules.find("EulerSolver") != storage.enabledModules.end()) {
            std::cout << "\n测试 EulerSolver 模块..." << std::endl;
            
            auto& engineContext = storage.engineContexts["Solve"];

            nlohmann::json moduleParams;
            if (storage.moduleConfig.contains("EulerSolver")) {
                moduleParams = storage.moduleConfig["EulerSolver"];
            } else {
                moduleParams = {
                    {"euler_type", "Standard"},
                    {"euler_value", 0.7}
                };
            }
            
            void* moduleInstance = engineContext->createModule("EulerSolver", moduleParams);
            std::cout << " - 创建 EulerSolver 成功" << std::endl;
            
            engineContext->initializeModule("EulerSolver");
            std::cout << " - 初始化 EulerSolver 成功" << std::endl;
            
            engineContext->executeModule("EulerSolver");
            std::cout << " - 执行 EulerSolver 成功" << std::endl;
            
            engineContext->releaseModule("EulerSolver");
            std::cout << " - 释放 EulerSolver 成功" << std::endl;
        }
        
    } catch (const std::exception& e) {
        std::cerr << "测试过程中发生错误: " << e.what() << std::endl;
    }
    
    std::cout << "\n======== 模块手动测试结束 ========\n" << std::endl;
    
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

/**
 * @brief Constructor for ModuleRegistryInitializer, registers all module types.
 */
ModuleRegistryInitializer::ModuleRegistryInitializer() {
    // ModuleTypeRegistry::instance().registerType(
    //     "PreCGNS", 
    //     []() -> nlohmann::json { return PreCGNS::GetParamSchema(); }
    // );
    
    // ModuleTypeRegistry::instance().registerType(
    //     "PrePlot3D", 
    //     []() -> nlohmann::json { return PrePlot3D::GetParamSchema(); }
    // );
    
    // ModuleTypeRegistry::instance().registerType(
    //     "EulerSolver", 
    //     []() -> nlohmann::json { return EulerSolver::GetParamSchema(); }
    // );

    // ModuleTypeRegistry::instance().registerType(
    //     "SASolver", 
    //     []() -> nlohmann::json { return SASolver::GetParamSchema(); }
    // );

    // ModuleTypeRegistry::instance().registerType(
    //     "SSTSolver", 
    //     []() -> nlohmann::json { return SSTSolver::GetParamSchema(); }
    // );

    // ModuleTypeRegistry::instance().registerType(
    //     "PostCGNS", 
    //     []() -> nlohmann::json { return PostCGNS::GetParamSchema(); }
    // );

    // ModuleTypeRegistry::instance().registerType(
    //     "PostPlot3D", 
    //     []() -> nlohmann::json { return PostPlot3D::GetParamSchema(); }
    // );

    // 将模块关联到对应的引擎
    //assignModuleToEngine("PreCGNS", "PreGrid");
    //assignModuleToEngine("PrePlot3D", "PreGrid");
    
    // assignModuleToEngine("EulerSolver", "Solve");
    // assignModuleToEngine("SASolver", "Solve");
    // assignModuleToEngine("SSTSolver", "Solve");
    
    // assignModuleToEngine("PostCGNS", "Post");
    // assignModuleToEngine("PostPlot3D", "Post");
}

/**
 * @brief Initializes the ModuleFactory by registering all module types.
 */
void ModuleFactoryInitializer::init() {
    ModuleFactory& factory = ModuleFactory::instance();
    
    // factory.registerModuleType("PreCGNS", 
    //     [](AdvancedRegistry* reg, const std::string& name) -> bool { 
    //         reg->Register<PreCGNS>(name);
    //         return true;
    //     });
    
    // factory.registerModuleType("PrePlot3D", 
    //     [](AdvancedRegistry* reg, const std::string& name) -> bool { 
    //         reg->Register<PrePlot3D>(name);
    //         return true;
    //     });
    
    // factory.registerModuleType("EulerSolver", 
    //     [](AdvancedRegistry* reg, const std::string& name) -> bool { 
    //         reg->Register<EulerSolver>(name);
    //         return true;
    //     });
    
    // factory.registerModuleType("SASolver", 
    //     [](AdvancedRegistry* reg, const std::string& name) -> bool { 
    //         reg->Register<SASolver>(name);
    //         return true;
    //     });

    // factory.registerModuleType("SSTSolver", 
    //     [](AdvancedRegistry* reg, const std::string& name) -> bool { 
    //         reg->Register<SSTSolver>(name);
    //         return true;
    //     });

    // factory.registerModuleType("PostCGNS", 
    //     [](AdvancedRegistry* reg, const std::string& name) -> bool { 
    //         reg->Register<PostCGNS>(name);
    //         return true;
    //     });

    // factory.registerModuleType("PostPlot3D", 
    //     [](AdvancedRegistry* reg, const std::string& name) -> bool { 
    //         reg->Register<PostPlot3D>(name);
    //         return true;
    //     });
}

/**
 * @brief Helper struct to initialize the module factory before main.
 */
struct ModuleFactoryInit {
    /**
     * @brief Constructor that triggers module factory initialization.
     */
    ModuleFactoryInit() {
        ModuleFactoryInitializer::init();
    }
};

/**
 * @brief Static instance to initialize the module factory before main.
 */
static ModuleSystem::ModuleFactoryInit moduleFactoryInitInstance;

}