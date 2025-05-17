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
#include <memory>
#include <fstream>
#include <string>
#include <nlohmann/json.hpp>
#include <filesystem>

#ifdef _WIN32
#include <windows.h>
#include <io.h>
#include <fcntl.h>
#endif

// 新增函数：配置参数验证
bool paramValidation(const nlohmann::json& config) {
    try {
        // 收集已知模块集合
        std::unordered_set<std::string> knownModules;
        for (const auto& moduleType : ModuleSystem::ModuleTypeRegistry::instance().getModuleTypes()) {
            knownModules.insert(moduleType.name);
        }

        // 验证工厂定义
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
                
                // 验证执行逻辑
                if (!factory.contains("executionType")) {
                    throw std::runtime_error("工厂定义错误: 工厂 '" + factoryName + "' 缺少执行逻辑定义");
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
                
                // 如果是SEQUENTIAL_AND_CHOOSE类型，需要验证firstModule和remainingModules
                if (factory["executionType"] == "SEQUENTIAL_AND_CHOOSE") {
                    if (!factory.contains("firstModule")) {
                        throw std::runtime_error("工厂定义错误: 工厂 '" + factoryName + "' 使用SEQUENTIAL_AND_CHOOSE逻辑但未指定firstModule");
                    }
                    if (!factory.contains("remainingModules") || !factory["remainingModules"].is_array()) {
                        throw std::runtime_error("工厂定义错误: 工厂 '" + factoryName + "' 使用SEQUENTIAL_AND_CHOOSE逻辑但未正确指定remainingModules");
                    }
                    
                    // 验证firstModule是否在工厂的modules中
                    std::string firstModule = factory["firstModule"];
                    bool firstModuleFound = false;
                    for (const auto& module : factory["modules"]) {
                        if (module["name"] == firstModule) {
                            firstModuleFound = true;
                            break;
                        }
                    }
                    if (!firstModuleFound) {
                        throw std::runtime_error("工厂定义错误: 工厂 '" + factoryName + "' 的firstModule '" + firstModule + "' 不在工厂模块列表中");
                    }
                    
                    // 验证remainingModules是否都在工厂的modules中
                    for (const auto& moduleName : factory["remainingModules"]) {
                        bool moduleFound = false;
                        for (const auto& module : factory["modules"]) {
                            if (module["name"] == moduleName) {
                                moduleFound = true;
                                break;
                            }
                        }
                        if (!moduleFound) {
                            throw std::runtime_error("工厂定义错误: 工厂 '" + factoryName + "' 的remainingModules中模块 '" + moduleName.get<std::string>() + "' 不在工厂模块列表中");
                        }
                    }
                }
            }
            
            // 存储验证通过的工厂定义
            ModuleSystem::ModuleFactoryCollection::initializeStaticFactoryPool(config["moduleFactories"]);
        }
        
        // 验证引擎定义
        if (config.contains("engine") && config["engine"].contains("enginePool")) {
            std::unordered_set<std::string> engineNames;
            for (const auto& engine : config["engine"]["enginePool"]) {
                if (!engine.contains("name")) {
                    throw std::runtime_error("引擎定义错误: 缺少引擎名称");
                }
                
                std::string engineName = engine["name"];
                if (!engineNames.insert(engineName).second) {
                    throw std::runtime_error("引擎定义错误: 发现重复的引擎名称 '" + engineName + "'");
                }
            }
            
            // 检查是否有必需的引擎
            if (engineNames.find("mainProcess") == engineNames.end()) {
                throw std::runtime_error("引擎定义错误: 缺少必需的mainProcess引擎");
            }
            if (engineNames.find("PreGrid") == engineNames.end()) {
                throw std::runtime_error("引擎定义错误: 缺少必需的PreGrid引擎");
            }
            if (engineNames.find("Solve") == engineNames.end()) {
                throw std::runtime_error("引擎定义错误: 缺少必需的Solve引擎");
            }
            if (engineNames.find("Post") == engineNames.end()) {
                throw std::runtime_error("引擎定义错误: 缺少必需的Post引擎");
            }
            
            // 验证主引擎的子引擎列表
            for (const auto& engine : config["engine"]["enginePool"]) {
                if (engine["name"] == "mainProcess") {
                    if (!engine.contains("subenginePool") || !engine["subenginePool"].is_array()) {
                        throw std::runtime_error("引擎定义错误: mainProcess引擎缺少子引擎定义");
                    }
                    
                    std::vector<std::string> requiredSubEngines = {"PreGrid", "Solve", "Post"};
                    std::vector<std::string> missingEngines;
                    
                    for (const auto& requiredEngine : requiredSubEngines) {
                        bool found = false;
                        for (const auto& subEngine : engine["subenginePool"]) {
                            if (subEngine == requiredEngine) {
                                found = true;
                                break;
                            }
                        }
                        if (!found) {
                            missingEngines.push_back(requiredEngine);
                        }
                    }
                    
                    if (!missingEngines.empty()) {
                        std::string errorMsg = "引擎定义错误: mainProcess引擎缺少以下必需的子引擎: ";
                        for (size_t i = 0; i < missingEngines.size(); ++i) {
                            errorMsg += missingEngines[i];
                            if (i < missingEngines.size() - 1) {
                                errorMsg += ", ";
                            }
                        }
                        throw std::runtime_error(errorMsg);
                    }
                    
                    break;
                }
            }
            
            // 存储验证通过的引擎定义
            ModuleSystem::Nestedengine::initializeStaticEnginePool(config["engine"]["enginePool"]);
        } else {
            throw std::runtime_error("配置错误: 缺少引擎定义");
        }
        
        // 验证引擎与工厂绑定
        if (config.contains("engineFactoryBindings")) {
            for (const auto& binding : config["engineFactoryBindings"]) {
                if (!binding.contains("engineName") || !binding.contains("factoryName")) {
                    throw std::runtime_error("引擎工厂绑定错误: 缺少引擎名称或工厂名称");
                }
            }
        }
        
        return true;
    } catch (const std::exception& e) {
        std::cerr << "参数验证失败: " << e.what() << std::endl;
        return false;
    }
}

int main(int argc, char* argv[]) {
#ifdef _WIN32
    // 设置控制台代码页为 UTF-8
    SetConsoleOutputCP(CP_UTF8);
    _setmode(_fileno(stdout), _O_BINARY);
    std::cout.setf(std::ios::unitbuf);
    std::cerr.setf(std::ios::unitbuf);
    setvbuf(stdout, nullptr, _IONBF, 0);
    setvbuf(stderr, nullptr, _IONBF, 0);
#endif

    try {
        std::string configFile = "./config.json";
        bool generateConfig = false;
        
        if (argc > 1) {
            if (std::string(argv[1]) == "get") {
                generateConfig = true;
                if (argc > 2) {
                    configFile = argv[2];
                    if (configFile.find('.') == std::string::npos) configFile += ".json";
                }
            } else {
                configFile = argv[1];
                if (configFile.find('.') == std::string::npos) configFile += ".json";
            }
        }
        
        if (generateConfig) {
            auto registry = std::make_shared<ModuleSystem::AdvancedRegistry>();
            auto engine = std::make_unique<ModuleSystem::Nestedengine>(*registry);
            
            // 注册模块
            for (const auto& moduleType : ModuleSystem::ModuleTypeRegistry::instance().getModuleTypes()) {
                if (!ModuleSystem::ModuleFactory::instance().registerModule(registry.get(), moduleType.name)) {
                    std::cerr << "警告: 无法通过工厂注册模块 '" << moduleType.name << "'" << std::endl;
                }
            }
            
            std::cout << "生成默认配置模板到 " << configFile << "..." << std::endl;
            ModuleSystem::getConfigInfo(registry, engine, configFile);
        } else {
            nlohmann::json config;

            std::cout << "正在加载配置文件: " << configFile << std::endl;
            
            std::ifstream file(configFile);
            if (!file.is_open()) {
                std::cerr << "无法打开配置文件: " << configFile << std::endl;
                std::cerr << "请确认文件路径正确且文件存在。" << std::endl;
                std::cerr << "使用 './programName get " << configFile << "' 生成默认配置，或者 './programName' 使用默认配置文件路径。" << std::endl;
                return 1;
            }
            
            try {
                file >> config;
            } catch (const nlohmann::json::exception& e) {
                std::cerr << "解析配置文件失败: " << e.what() << std::endl;
                return 1;
            }
            file.close();
            
            std::string outputFile;
            std::filesystem::path configPath(configFile);
            std::filesystem::path parentDir = configPath.parent_path();
            if (parentDir.empty()) parentDir = ".";
            std::filesystem::path outputPath = parentDir / (configPath.stem().string() + "_used" + configPath.extension().string());
            outputFile = outputPath.string();
            
            std::cout << "实际使用的配置将保存到: " << outputFile << std::endl;
            
            // 进行参数验证
            if (!paramValidation(config)) {
                std::cerr << "配置文件验证失败，程序退出" << std::endl;
                return 1;
            }
            
            // 验证通过后执行配置
            ModuleSystem::runWithConfig(config, outputFile);
        }
    } catch (const std::exception& e) {
        std::cerr << "错误: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}