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
        return 0;
    } 

    nlohmann::json config;
    std::cout << "正在加载配置文件: " << configFile << std::endl;
    
    std::ifstream file(configFile);
    if (!file.is_open()) {
        std::cerr << "无法打开配置文件: " << configFile << std::endl;
        std::cerr << "请确认文件路径正确且文件存在。" << std::endl;
        std::cerr << "使用 './programName get " << configFile << "' 生成默认配置，或者 './programName' 使用默认配置文件路径。" << std::endl;
        return 1;
    }
    
    // 解析JSON文件
    file >> config;
    file.close();
    
    std::string outputFile;
    std::filesystem::path configPath(configFile);
    std::filesystem::path parentDir = configPath.parent_path();
    if (parentDir.empty()) parentDir = ".";
    std::filesystem::path outputPath = parentDir / (configPath.stem().string() + "_used" + configPath.extension().string());
    outputFile = outputPath.string();
    
    // 进行参数验证
    if (!ModuleSystem::paramValidation(config)) {
        std::cerr << "配置文件验证失败，程序退出" << std::endl;
        return 1;
    }
    
    // 保存使用的配置
    ModuleSystem::saveUsedConfig(config, outputFile);
    
    // 验证通过后执行配置
    ModuleSystem::run();
    
    //test
    ModuleSystem::test();

    return 0;
}