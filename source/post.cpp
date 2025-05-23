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

#include "post.h"
#include <iostream>

namespace POST {

// 创建静态本地实例
static ModuleSystem::LocalTypeRegistry localTypeRegistryInstance;
static ModuleSystem::LocalFactory localFactoryInstance;

void registerTypes() {
    // 在本地注册表中注册类型
    localTypeRegistryInstance.registerType("PostCGNS", 
        []() -> nlohmann::json { return PostCGNS::GetParamSchema(); });
    
    localTypeRegistryInstance.registerType("PostPlot3D", 
        []() -> nlohmann::json { return PostPlot3D::GetParamSchema(); });
    
    // 在本地工厂中注册创建函数
    localFactoryInstance.registerModuleType("PostCGNS", 
        [](ModuleSystem::AdvancedRegistry* reg, const std::string& name) -> bool { 
            reg->Register<PostCGNS>(name);
            return true;
        });
    
    localFactoryInstance.registerModuleType("PostPlot3D", 
        [](ModuleSystem::AdvancedRegistry* reg, const std::string& name) -> bool { 
            reg->Register<PostPlot3D>(name);
            return true;
        });
}

const auto& getTypes() { 
    return localTypeRegistryInstance.getModuleTypes(); 
}

const auto& getFactories() {
    return localFactoryInstance.getAllModuleCreators();
}

void exportToGlobalRegistry() {
    // 确保模块类型已注册
    registerTypes();
#if DEBUG
    std::cout << "导出后处理模块到全局注册表..." << std::endl;
#endif
    // 获取本组件特有的模块名称
    std::unordered_set<std::string> postModules = {"PostCGNS", "PostPlot3D"};
    
    // 使用新的导出方法将本地注册表导出到全局
    localTypeRegistryInstance.exportToGlobal();
    localFactoryInstance.exportToGlobal();

    // 将模块关联到对应的引擎 - 只关联成功导出的模块
    auto& moduleRegistryInit = ModuleSystem::ModuleRegistryInitializer::init();
    for (const auto& name : postModules) {
        moduleRegistryInit.assignModuleToEngine(name, "Post");
#if DEBUG        
        std::cout << "  - 已将模块 '" << name << "' 关联到 Post 引擎" << std::endl;
#endif
    }
#if DEBUG
    std::cout << "后处理模块导出完成，共导出 " << postModules.size() << " 个类型和 " 
              << postModules.size() << " 个模块" << std::endl;
#endif
}

//============= PostCGNS 模块实现 =============//

/**
 * @brief Constructor for PostCGNS module.
 * @param params The parameters for the module.
 */
PostCGNS::PostCGNS(const nlohmann::json& params) {
    cgns_type_ = params.value("cgns_type", "HDF5");
    cgns_value_ = params.value("cgns_value", 15);
}

/**
 * @brief Initializes the PostCGNS module.
 */
void PostCGNS::initialize() {
}

/**
 * @brief Executes the PostCGNS module.
 */
void PostCGNS::execute() {
}

/**
 * @brief Releases the PostCGNS module.
 */
void PostCGNS::release() {
}

/**
 * @brief Gets the parameter schema for the PostCGNS module.
 * @return A JSON object representing the parameter schema.
 */
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

//============= PostPlot3D 模块实现 =============//

/**
 * @brief Constructor for PostPlot3D module.
 * @param params The parameters for the module.
 */
PostPlot3D::PostPlot3D(const nlohmann::json& params) {
    plot3d_type_ = params.value("plot3_type", "ASCII");
    plot3d_value_ = params.value("plot3_value", 30);
}

/**
 * @brief Initializes the PostPlot3D module.
 */
void PostPlot3D::initialize() {
}

/**
 * @brief Executes
 */
void PostPlot3D::execute() {
}

/**
 * @brief Releases the PostPlot3D module.
 */
void PostPlot3D::release() {
}

/**
 * @brief Gets the parameter schema for the PostPlot3D module.
 * @return A JSON object representing the parameter schema.
 */
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

} // namespace POST


namespace ModuleSystem {

nlohmann::json ModuleParamTraits<POST::PostCGNS>::GetParamSchema() {
    return POST::PostCGNS::GetParamSchema();
}

nlohmann::json ModuleParamTraits<POST::PostPlot3D>::GetParamSchema() {
    return POST::PostPlot3D::GetParamSchema();
}

} // namespace ModuleSystem