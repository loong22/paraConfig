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
    defaultConfig["solver"] = "SIMPLE";
    defaultConfig["maxIterations"] = 1000;
    defaultConfig["convergenceCriteria"] = 1e-6;
    defaultConfig["time_step"] = 0.01;
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
    
    // 确保目标目录存在 - 支持相对路径和绝对路径
    std::filesystem::path dirPath(baseDir);
    std::filesystem::create_directories(dirPath);
    
    // 生成主引擎配置 - 只包含mainProcess引擎的信息和全局配置
    nlohmann::json mainConfig;
    
    // 全局配置参数
    mainConfig["config"] = ModuleSystem::Nestedengine(*(new ModuleSystem::AdvancedRegistry())).getDefaultConfig();
    
    // 仅添加主引擎信息，不包含其他引擎
    mainConfig["engine"] = nlohmann::json::object();
    mainConfig["engine"]["enginePool"] = nlohmann::json::array();
    
    // 查找主引擎并添加
    auto fullEngineInfo = createengineInfo();
    for (auto& engine : fullEngineInfo["enginePool"]) {
        if (engine["name"] == "mainProcess") {
            mainConfig["engine"]["enginePool"].push_back(engine);
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
    
    // 生成PreGrid引擎配置
    nlohmann::json preGridConfig = createEngineSpecificInfo("PreGrid");
    nlohmann::json preGridModuleConfig = createModuleConfigForEngine("PreGrid");
    preGridConfig["config"] = preGridModuleConfig["config"];
    
    std::filesystem::path preGridFilePath = dirPath / "template_engine_PreGrid.json";
    std::ofstream preGridFile(preGridFilePath);
    if (preGridFile.is_open()) {
        preGridFile << std::setw(4) << preGridConfig << std::endl;
        preGridFile.close();
        std::cout << "已生成PreGrid引擎配置: " << preGridFilePath.string() << std::endl;
    } else {
        std::cerr << "无法创建文件: " << preGridFilePath.string() << std::endl;
    }
    
    // 生成Solve引擎配置
    nlohmann::json solveConfig = createEngineSpecificInfo("Solve");
    nlohmann::json solveModuleConfig = createModuleConfigForEngine("Solve");
    solveConfig["config"] = solveModuleConfig["config"];
    
    std::filesystem::path solveFilePath = dirPath / "template_engine_Solve.json";
    std::ofstream solveFile(solveFilePath);
    if (solveFile.is_open()) {
        solveFile << std::setw(4) << solveConfig << std::endl;
        solveFile.close();
        std::cout << "已生成Solve引擎配置: " << solveFilePath.string() << std::endl;
    } else {
        std::cerr << "无法创建文件: " << solveFilePath.string() << std::endl;
    }
    
    // 生成Post引擎配置
    nlohmann::json postConfig = createEngineSpecificInfo("Post");
    nlohmann::json postModuleConfig = createModuleConfigForEngine("Post");
    postConfig["config"] = postModuleConfig["config"];
    
    std::filesystem::path postFilePath = dirPath / "template_engine_Post.json";
    std::ofstream postFile(postFilePath);
    if (postFile.is_open()) {
        postFile << std::setw(4) << postConfig << std::endl;
        postFile.close();
        std::cout << "已生成Post引擎配置: " << postFilePath.string() << std::endl;
    } else {
        std::cerr << "无法创建文件: " << postFilePath.string() << std::endl;
    }
    
    // 生成模块注册信息
    nlohmann::json registryConfig;
    registryConfig["registry"] = createRegistryInfo();
    
    std::filesystem::path registryFilePath = dirPath / "template_registry_module.json";
    std::ofstream registryFile(registryFilePath);
    if (registryFile.is_open()) {
        registryFile << std::setw(4) << registryConfig << std::endl;
        registryFile.close();
        std::cout << "已生成模块注册配置: " << registryFilePath.string() << std::endl;
    } else {
        std::cerr << "无法创建文件: " << registryFilePath.string() << std::endl;
    }
    
    // 正常退出，避免访问ConfigurationStorage引起的崩溃
    exit(0);
}

/**
 * @brief Saves the used configurations for each engine and module.
 * @param config The merged configuration JSON object.
 * @param configDir The directory where the configuration files will be saved.
 */
void saveUsedConfigs(const nlohmann::json& config, const std::string& configDir) {
    auto& storage = ConfigurationStorage::instance();
    
    // 获取全部配置
    nlohmann::json usedConfig = config;
    
    // 根据引擎分离配置
    std::vector<std::string> engineNames = {"mainProcess", "PreGrid", "Solve", "Post"};
    
    for (const auto& engineName : engineNames) {
        nlohmann::json engineConfig;
        
        // 获取引擎特定信息
        engineConfig["engine"] = nlohmann::json::object();
        engineConfig["engine"]["enginePool"] = nlohmann::json::array();
        
        // 对于主引擎，简化处理方式
        if (engineName == "mainProcess") {
            // 查找主引擎
            for (const auto& engine : usedConfig["engine"]["enginePool"]) {
                if (engine["name"] == engineName) {
                    // 只添加主引擎本身，不添加子引擎作为独立条目
                    engineConfig["engine"]["enginePool"].push_back(engine);
                    break;
                }
            }
            
            // 复制全局配置参数（不是对象的参数）
            engineConfig["config"] = nlohmann::json::object();
            for (auto it = usedConfig["config"].begin(); it != usedConfig["config"].end(); ++it) {
                if (!it.key().empty() && it.key()[0] != '_' && !it.value().is_object()) {
                    engineConfig["config"][it.key()] = it.value();
                }
            }
        } 
        // 对于非主引擎，保持原有处理方式
        else {
            // 查找当前引擎
            for (const auto& engine : usedConfig["engine"]["enginePool"]) {
                if (engine["name"] == engineName) {
                    // 添加当前引擎信息
                    engineConfig["engine"]["enginePool"].push_back(engine);
                    
                    // 如果有子引擎，也添加子引擎的详细信息
                    if (engine.contains("subenginePool")) {
                        for (const auto& subEngineName : engine["subenginePool"]) {
                            for (const auto& subEngine : usedConfig["engine"]["enginePool"]) {
                                if (subEngine["name"] == subEngineName) {
                                    engineConfig["engine"]["enginePool"].push_back(subEngine);
                                    break;
                                }
                            }
                        }
                    }
                    
                    break;
                }
            }
            
            // 为引擎相关模块获取配置
            engineConfig["config"] = nlohmann::json::object();
            std::unordered_set<std::string> engineModules = getEngineModules(engineName, usedConfig);
            for (const auto& moduleName : engineModules) {
                if (usedConfig["config"].contains(moduleName)) {
                    engineConfig["config"][moduleName] = usedConfig["config"][moduleName];
                }
            }
        }
        
        // 写入文件
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
    
    // 保存模块注册信息
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
 * @brief Generates a default configuration and saves it to a file.
 * @param argc The number of command line arguments.
 * @param argv The command line arguments.
 */
void getConfig(int argc, char* argv[]){
    std::string configDir = "./";
    bool generateTemplates = false;
    std::string templateDir = "./";

    // 解析命令行参数
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "--config-dir" && i + 1 < argc) {
            configDir = argv[++i];
            if (configDir.back() != '/') configDir += '/';
        } else if (arg == "--generate-templates") {
            generateTemplates = true;
            // 检查是否有指定目录参数
            if (i + 1 < argc && argv[i+1][0] != '-') {
                templateDir = argv[++i];
            }
        }
    }
    
    if (generateTemplates) {
        generateTemplateConfigs(templateDir);
        return;
    }
    
    // 确保配置目录存在
    std::filesystem::create_directories(configDir);
    
    // 合并配置文件
    nlohmann::json mergedConfig;
    
    // 定义需要读取的配置文件列表
    std::vector<std::string> configFiles = {
        configDir + "config_engine_mainProcess.json",
        configDir + "config_engine_PreGrid.json",
        configDir + "config_engine_Solve.json",
        configDir + "config_engine_Post.json",
        configDir + "config_registry_module.json"
    };
    
    bool anyFileFound = false;
    
    // 读取并合并所有配置文件
    for (const auto& file : configFiles) {
        std::ifstream configFile(file);
        if (configFile.is_open()) {
            anyFileFound = true;
            std::cout << "正在加载配置文件: " << file << std::endl;
            
            nlohmann::json fileConfig;
            try {
                configFile >> fileConfig;
                
                // 合并到主配置
                if (fileConfig.contains("engine")) {
                    if (!mergedConfig.contains("engine")) {
                        mergedConfig["engine"] = fileConfig["engine"];
                    } else {
                        // 合并engine.enginePool数组
                        if (fileConfig["engine"].contains("enginePool")) {
                            for (const auto& engine : fileConfig["engine"]["enginePool"]) {
                                bool found = false;
                                for (auto& existingEngine : mergedConfig["engine"]["enginePool"]) {
                                    if (existingEngine["name"] == engine["name"]) {
                                        existingEngine = engine;  // 覆盖现有引擎
                                        found = true;
                                        break;
                                    }
                                }
                                if (!found) {
                                    mergedConfig["engine"]["enginePool"].push_back(engine);
                                }
                            }
                        }
                    }
                }
                
                // 合并config部分
                if (fileConfig.contains("config")) {
                    if (!mergedConfig.contains("config")) {
                        mergedConfig["config"] = fileConfig["config"];
                    } else {
                        // 合并配置参数
                        for (auto it = fileConfig["config"].begin(); it != fileConfig["config"].end(); ++it) {
                            mergedConfig["config"][it.key()] = it.value();
                        }
                    }
                }
                
                // 合并registry部分
                if (fileConfig.contains("registry")) {
                    mergedConfig["registry"] = fileConfig["registry"];
                }
                
            } catch (const std::exception& e) {
                std::cerr << "解析文件 " << file << " 失败: " << e.what() << std::endl;
            }
            
            configFile.close();
        }
    }
    
    // 如果没有找到任何配置文件，生成默认配置
    if (!anyFileFound) {
        std::cout << "未找到任何配置文件，将生成默认配置模板" << std::endl;
        generateTemplateConfigs(configDir);
        
        // 生成默认配置后退出
        std::cout << "请编辑生成的配置模板，然后重新运行程序" << std::endl;
        exit(0);
    }
    
    // 验证并处理合并后的配置
    if (!ModuleSystem::paramValidation(mergedConfig)) {
        std::cerr << "配置验证失败，请检查配置文件" << std::endl;
        exit(1);
    }
    
    // 保存使用的配置
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
    
    storage.config = config;
    
    std::unordered_set<std::string> knownModules;
    for (const auto& moduleType : ModuleSystem::ModuleTypeRegistry::instance().getModuleTypes()) {
        knownModules.insert(moduleType.name);
    }
    storage.knownModules = knownModules;

    std::unordered_map<std::string, nlohmann::json> moduleParamSchemas;
    for (const auto& moduleType : ModuleSystem::ModuleTypeRegistry::instance().getModuleTypes()) {
        moduleParamSchemas[moduleType.name] = moduleType.getParamSchemaFunc();
    }

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
            
            if (engine.contains("modules") && engine["modules"].is_array()) {
                for (const auto& moduleInfo : engine["modules"]) {
                    if (!moduleInfo.contains("name")) {
                        std::cerr << "引擎 '" << engineName << "' 中的模块定义错误: 缺少模块名称" << std::endl;
                        return false;
                    }
                    
                    std::string moduleName = moduleInfo["name"];
                    if (knownModules.find(moduleName) == knownModules.end()) {
                        std::cerr << "引擎 '" << engineName << "' 中包含未知模块 '" << moduleName << "'" << std::endl;
                        return false;
                    }
                    
                    if (moduleInfo.contains("params")) {
                        if (!moduleInfo["params"].is_object()) {
                            std::cerr << "引擎 '" << engineName << "' 中模块 '" << moduleName << "' 的参数必须是对象" << std::endl;
                            return false;
                        }
                        
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
    
    if (config.contains("config")) {
        nlohmann::json moduleConfig;
        nlohmann::json globalParams;
        
        for (auto& [key, value] : config["config"].items()) {
            if (value.is_object()) {
                moduleConfig[key] = value;
                if (knownModules.find(key) != knownModules.end()) {
                    const nlohmann::json& moduleParams = value;
                    const nlohmann::json& paramSchema = moduleParamSchemas[key];
                    
                    for (auto it = moduleParams.begin(); it != moduleParams.end(); ++it) {
                        const std::string& paramName = it.key();
                        const nlohmann::json& paramValue = it.value();
                        
                        if (!paramSchema.contains(paramName)) {
                            std::cerr << "模块 '" << key << "' 的未知参数: '" << paramName << "'" << std::endl;
                            return false;
                        }
                        
                        std::string errorMsg = validateParam(paramSchema[paramName], paramValue);
                        if (!errorMsg.empty()) {
                            std::cerr << "模块 '" << key << "' 的参数 '" << paramName 
                                      << "' 验证失败: " << errorMsg << std::endl;
                            return false;
                        }
                    }
                    
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
                globalParams[key] = value;
                
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
        
        storage.moduleConfig = moduleConfig;
        storage.globalParams = globalParams;
    } else {
        std::cout << "警告: 配置中没有 'config' 节点，使用默认参数" << std::endl;
        storage.moduleConfig = nlohmann::json::object();
        storage.globalParams = nlohmann::json::object();
    }
    
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
    
    std::unordered_set<std::string> usedEngineNames;
    if (config["engine"].contains("enginePool")) {
        for (const auto& Engine : config["engine"]["enginePool"]) {
            if (!Engine.contains("name") || !Engine["enabled"].get<bool>()) continue;
            usedEngineNames.insert(Engine["name"].get<std::string>());
        }
    }
    storage.usedEngineNames = usedEngineNames;
    
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
        
        for (const auto& engine : enginePool) {
            if (engine.contains("name") && engine["name"] == current &&
                engine.contains("subenginePool") && engine["subenginePool"].is_array()) {
                
                for (const auto& subEngine : engine["subenginePool"]) {
                    std::string subEngineName = subEngine.get<std::string>();
                    
                    if (inStack.count(subEngineName)) {
                        std::cerr << "错误: 检测到循环依赖: " << current << " -> " << subEngineName << std::endl;
                        return true;
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
    
    auto& registry = storage.registry;
    auto& engine = storage.engine;
    
    // 为每个引擎注册其关联的模块以及所有未绑定的模块
    for (const auto& engineName : storage.usedEngineNames) {
        // 注册引擎特定的模块
        auto modules = EngineModuleMapping::instance().getEngineModules(engineName);
        for (const auto& moduleName : modules) {
            if (storage.enabledModules.count(moduleName) > 0) {
                // 直接注册模块
                if (ModuleFactory::instance().registerModule(registry.get(), moduleName)) {
                    std::cout << "模块 " << moduleName << " 已为引擎 " << engineName << " 注册" << std::endl;
                }
            }
        }
    }

    // 注册所有未绑定的模块 - 对所有引擎可用
    for (const auto& moduleName : storage.enabledModules) {
        if (!EngineModuleMapping::instance().isModuleBoundToEngine(moduleName)) {
            if (ModuleFactory::instance().registerModule(registry.get(), moduleName)) {
                std::cout << "全局模块 " << moduleName << " 已注册（对所有引擎可用）" << std::endl;
            }
        }
    }
    
    if (config["engine"].contains("enginePool")) {
        for (const auto& EngineDef : config["engine"]["enginePool"]) {
            if (!EngineDef.contains("name") || !EngineDef["enabled"].get<bool>()) continue;
            std::string EngineName = EngineDef["name"].get<std::string>();

            engine->defineengine(EngineName, 
                [EngineDef, EngineName](ModuleSystem::engineContext& context) {
                });
        }
        storage.enginesAreDefined = true;
    }
    
    if (config.contains("config")) {
        engine->Build(config["config"]);
    }
    
    storage.mainContext = std::make_shared<ModuleSystem::engineContext>(*registry, engine.get());
    if (config.contains("config")) {
        storage.mainContext->setParameter("config", config["config"]);
        for (auto& [key, value] : config["config"].items()) {
            if (!value.is_object()) storage.mainContext->setParameter(key, value);
        }
        for (auto& [moduleName, params] : storage.moduleConfig.items()) {
            storage.mainContext->setParameter(moduleName, params);
        }
    }

    collectedModules.clear();

    if (!config.contains("engine") || !config["engine"].contains("enginePool") || 
        !config["engine"]["enginePool"].is_array()) {
        std::cerr << "错误: 配置缺少有效的 engine.enginePool 数组" << std::endl;
        return false;
    }

    if (!config.contains("config")) {
        std::cerr << "警告: 配置缺少 config 对象，可能缺少全局模块参数" << std::endl;
    }

    bool hasDefaultEngine = false;
    for (const auto& engine : config["engine"]["enginePool"]) {
        if (!engine.contains("name") || !engine["name"].is_string()) {
            std::cerr << "错误: 引擎定义缺少有效的名称" << std::endl;
            return false;
        }
        
        if (engine["name"] == "mainProcess" && engine.value("enabled", true)) {
            hasDefaultEngine = true;
        }
    }

    std::unordered_set<std::string> visitedEngines;
    if (!collectModulesFromConfig(config["engine"], "mainProcess", visitedEngines)) {
        std::cerr << "错误: 收集模块执行顺序失败" << std::endl;
        return false;
    }
    
    // 收集完成后，整理并将结果存储到 ConfigurationStorage
    storage.engineModules.clear();
    storage.engineExecutionOrder.clear();
    
    // 创建引擎执行顺序的临时存储
    std::vector<std::string> tempExecutionOrder;
    std::unordered_set<std::string> processedEngines;
    
    // 递归函数，收集引擎及其子引擎的执行顺序
    std::function<void(const std::string&)> collectEngineOrder = 
        [&](const std::string& engineName) {
            if (processedEngines.count(engineName) > 0) return;
            
            processedEngines.insert(engineName);
            tempExecutionOrder.push_back(engineName);
            
            // 为该引擎创建模块列表
            storage.engineModules[engineName] = {};
            
            // 查找该引擎的模块
            for (const auto& moduleInfo : collectedModules) {
                if (moduleInfo.engineName == engineName) {
                    storage.engineModules[engineName].push_back(moduleInfo);
                }
            }
            
            // 检查子引擎
            for (const auto& engine : config["engine"]["enginePool"]) {
                if (engine["name"].get<std::string>() == engineName && 
                    engine.contains("subenginePool") && 
                    engine["subenginePool"].is_array()) {
                    
                    for (const auto& subEngine : engine["subenginePool"]) {
                        std::string subEngineName = subEngine.get<std::string>();
                        collectEngineOrder(subEngineName);
                    }
                    break;
                }
            }
        };
    
    // 从主引擎开始收集
    collectEngineOrder("mainProcess");
    
    // 存储收集到的执行顺序
    storage.engineExecutionOrder = tempExecutionOrder;
    
    // 初始化所有引擎上下文
    storage.initializeEngineContexts();

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

// /**
//  * @brief Constructor for PreCGNS module.
//  * @param params The parameters for the module.
//  */
// PreCGNS::PreCGNS(const nlohmann::json& params) {
//     cgns_type_ = params.value("cgns_type", "HDF5");
//     cgns_value_ = params.value("cgns_value", 15);
// }

// /**
//  * @brief Initializes the PreCGNS module.
//  */
// void PreCGNS::initialize() {
// }

// /**
//  * @brief Executes the PreCGNS module.
//  */
// void PreCGNS::execute() {
// }

// /**
//  * @brief Releases the PreCGNS module.
//  */
// void PreCGNS::release() {
// }

// /**
//  * @brief Gets the parameter schema for the PreCGNS module.
//  * @return A JSON object representing the parameter schema.
//  */
// nlohmann::json PreCGNS::GetParamSchema() {
//     return {
//         {"cgns_type", {
//             {"type", "string"},
//             {"description", "Type of cgns file"},
//             {"enum", {"HDF5", "ADF"}},
//             {"default", "HDF5"}
//         }},
//         {"cgns_value", {
//             {"type", "number"},
//             {"description", "Number of cgns value"},
//             {"minimum", 1},
//             {"maximum", 100},
//             {"default", 10}
//         }}
//     };
// }

// /**
//  * @brief Gets the parameter schema for PreCGNS.
//  * @return A JSON object representing the parameter schema.
//  */
// nlohmann::json ModuleParamTraits<PreCGNS>::GetParamSchema() {
//     return PreCGNS::GetParamSchema();
// }

// /**
//  * @brief Constructor for PrePlot3D module.
//  * @param params The parameters for the module.
//  */
// PrePlot3D::PrePlot3D(const nlohmann::json& params) {
//     plot3_type_ = params.value("plot3_type", "ASCII");
//     plot3d_value_ = params.value("plot3_value", 30);
// }

// /**
//  * @brief Initializes the PrePlot3D module.
//  */
// void PrePlot3D::initialize() {
// }

// /**
//  * @brief Executes the PrePlot3D module.
//  */
// void PrePlot3D::execute() {
// }

// /**
//  * @brief Releases the PrePlot3D module.
//  */
// void PrePlot3D::release() {
// }

// /**
//  * @brief Gets the parameter schema for the PrePlot3D module.
//  * @return A JSON object representing the parameter schema.
//  */
// nlohmann::json PrePlot3D::GetParamSchema() {
//     return {
//         {"plot3_type", {
//             {"type", "string"},
//             {"description", "Type of plot3 file"},
//             {"enum", {"ASCII", "Binary"}},
//             {"default", "ASCII"}
//         }},
//         {"plot3_value", {
//             {"type", "number"},
//             {"description", "Number of plot3 value"},
//             {"minimum", 1},
//             {"maximum", 100},
//             {"default", 10}
//         }}
//     };
// }

// /**
//  * @brief Gets the parameter schema for PrePlot3D.
//  * @return A JSON object representing the parameter schema.
//  */
// nlohmann::json ModuleParamTraits<PrePlot3D>::GetParamSchema() {
//     return PrePlot3D::GetParamSchema();
// }

// /**
//  * @brief Constructor for EulerSolver module.
//  * @param params The parameters for the module.
//  */
// EulerSolver::EulerSolver(const nlohmann::json& params) {
//     euler_type_ = params.value("euler_type", "Standard");
//     euler_value__ = params.value("euler_value", 0.5);
// }

// /**
//  * @brief Initializes the EulerSolver module.
//  */
// void EulerSolver::initialize() {
// }

// /**
//  * @brief Executes the EulerSolver module.
//  */
// void EulerSolver::execute() {
// }

// /**
//  * @brief Releases the EulerSolver module.
//  */
// void EulerSolver::release() {
// }

// /**
//  * @brief Gets the parameter schema for the EulerSolver module.
//  * @return A JSON object representing the parameter schema.
//  */
// nlohmann::json EulerSolver::GetParamSchema() {
//     return {
//         {"euler_type", {
//             {"type", "string"},
//             {"description", "Type of euler file"},
//             {"enum", {"Standard", "Other"}},
//             {"default", "Standard"}
//         }},
//         {"euler_value", {
//             {"type", "number"},
//             {"description", "Number of euler value"},
//             {"minimum", 0},
//             {"maximum", 10},
//             {"default", 0.5}
//         }}
//     };
// }

// /**
//  * @brief Gets the parameter schema for EulerSolver.
//  * @return A JSON object representing the parameter schema.
//  */
// nlohmann::json ModuleParamTraits<EulerSolver>::GetParamSchema() {
//     return EulerSolver::GetParamSchema();
// }

// /**
//  * @brief Constructor for SASolver module.
//  * @param params The parameters for the module.
//  */
// SASolver::SASolver(const nlohmann::json& params) {
//     sa_type_ = params.value("solver_type", "Standard");
//     convergence_criteria_ = params.value("convergence_criteria", 1e-6);
//     max_iterations_ = params.value("max_iterations", 1000);
// }

// /**
//  * @brief Initializes the SASolver module.
//  */
// void SASolver::initialize() {
// }

// /**
//  * @brief Executes the SASolver module.
//  */
// void SASolver::execute() {
// }

// /**
//  * @brief Releases the SASolver module.
//  */
// void SASolver::release() {
// }

// /**
//  * @brief Gets the parameter schema for the SASolver module.
//  * @return A JSON object representing the parameter schema.
//  */
// nlohmann::json SASolver::GetParamSchema() {
//     return {
//         {"solver_type", {
//             {"type", "string"},
//             {"description", "Type of fluid solver"},
//             {"enum", {"Standard", "SA-BC", "SA-DDES","SA-IDDES"}},
//             {"default", "Standard"}
//         }},
//         {"convergence_criteria", {
//             {"type", "number"},
//             {"description", "Convergence criteria for solver"},
//             {"minimum", 1e-10},
//             {"maximum", 1e-3},
//             {"default", 1e-6}
//         }},
//         {"max_iterations", {
//             {"type", "number"},
//             {"description", "Maximum number of iterations"},
//             {"minimum", 10},
//             {"maximum", 10000},
//             {"default", 1000}
//         }}
//     };
// }

// /**
//  * @brief Gets the parameter schema for SASolver.
//  * @return A JSON object representing the parameter schema.
//  */
// nlohmann::json ModuleParamTraits<SASolver>::GetParamSchema() {
//     return SASolver::GetParamSchema();
// }

// /**
//  * @brief Constructor for SSTSolver module.
//  * @param params The parameters for the module.
//  */
// SSTSolver::SSTSolver(const nlohmann::json& params) {
//     sst_type_ = params.value("solver_type", "Standard");
//     convergence_criteria_ = params.value("convergence_criteria", 1e-6);
//     max_iterations_ = params.value("max_iterations", 1000);
// }

// /**
//  * @brief Initializes the SSTSolver module.
//  */
// void SSTSolver::initialize() {
// }

// /**
//  * @brief Executes the SSTSolver module.
//  */
// void SSTSolver::execute() {
// }

// /**
//  * @brief Releases the SSTSolver module.
//  */
// void SSTSolver::release() {
// }

// /**
//  * @brief Gets the parameter schema for the SSTSolver module.
//  * @return A JSON object representing the parameter schema.
//  */
// nlohmann::json SSTSolver::GetParamSchema() {
//     return {
//         {"solver_type", {
//             {"type", "string"},
//             {"description", "Type of fluid solver"},
//             {"enum", {"Standard", "SST-CC", "SA-DDES","SA-IDDES"}},
//             {"default", "Standard"}
//         }},
//         {"convergence_criteria", {
//             {"type", "number"},
//             {"description", "Convergence criteria for solver"},
//             {"minimum", 1e-10},
//             {"maximum", 1e-3},
//             {"default", 1e-6}
//         }},
//         {"max_iterations", {
//             {"type", "number"},
//             {"description", "Maximum number of iterations"},
//             {"minimum", 10},
//             {"maximum", 10000},
//             {"default", 1000}
//         }}
//     };
// }

// /**
//  * @brief Gets the parameter schema for SSTSolver.
//  * @return A JSON object representing the parameter schema.
//  */
// nlohmann::json ModuleParamTraits<SSTSolver>::GetParamSchema() {
//     return SSTSolver::GetParamSchema();
// }

// /**
//  * @brief Constructor for PostCGNS module.
//  * @param params The parameters for the module.
//  */
// PostCGNS::PostCGNS(const nlohmann::json& params) {
//     cgns_type_ = params.value("cgns_type", "HDF5");
//     cgns_value_ = params.value("cgns_value", 15);
// }

// /**
//  * @brief Initializes the PostCGNS module.
//  */
// void PostCGNS::initialize() {
// }

// /**
//  * @brief Executes the PostCGNS module.
//  */
// void PostCGNS::execute() {
// }

// /**
//  * @brief Releases the PostCGNS module.
//  */
// void PostCGNS::release() {
// }

// /**
//  * @brief Gets the parameter schema for the PostCGNS module.
//  * @return A JSON object representing the parameter schema.
//  */
// nlohmann::json PostCGNS::GetParamSchema() {
//     return {
//         {"cgns_type", {
//             {"type", "string"},
//             {"description", "Type of cgns file"},
//             {"enum", {"HDF5", "ADF"}},
//             {"default", "HDF5"}
//         }},
//         {"cgns_value", {
//             {"type", "number"},
//             {"description", "Number of cgns value"},
//             {"minimum", 1},
//             {"maximum", 100},
//             {"default", 10}
//         }}
//     };
// }

// /**
//  * @brief Gets the parameter schema for PostCGNS.
//  * @return A JSON object representing the parameter schema.
//  */
// nlohmann::json ModuleParamTraits<PostCGNS>::GetParamSchema() {
//     return PostCGNS::GetParamSchema();
// }

// /**
//  * @brief Constructor for PostPlot3D module.
//  * @param params The parameters for the module.
//  */
// PostPlot3D::PostPlot3D(const nlohmann::json& params) {
//     plot3d_type_ = params.value("plot3_type", "ASCII");
//     plot3d_value_ = params.value("plot3_value", 30);
// }

// /**
//  * @brief Initializes the PostPlot3D module.
//  */
// void PostPlot3D::initialize() {
// }

// /**
//  * @brief Executes
//  */
// void PostPlot3D::execute() {
// }

// /**
//  * @brief Releases the PostPlot3D module.
//  */
// void PostPlot3D::release() {
// }

// /**
//  * @brief Gets the parameter schema for the PostPlot3D module.
//  * @return A JSON object representing the parameter schema.
//  */
// nlohmann::json PostPlot3D::GetParamSchema() {
//     return {
//         {"plot3_type", {
//             {"type", "string"},
//             {"description", "Type of plot3 file"},
//             {"enum", {"ASCII", "Binary"}},
//             {"default", "ASCII"}
//         }},
//         {"plot3_value", {
//             {"type", "number"},
//             {"description", "Number of plot3 value"},
//             {"minimum", 1},
//             {"maximum", 100},
//             {"default", 30}
//         }}
//     };
// }

// /**
//  * @brief Gets the parameter schema for PostPlot3D.
//  * @return A JSON object representing the parameter schema.
//  */
// nlohmann::json ModuleParamTraits<PostPlot3D>::GetParamSchema() {
//     return PostPlot3D::GetParamSchema();
// }

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