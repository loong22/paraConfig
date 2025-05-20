#include "preGrid.h"
#include <iostream>

namespace PREGRID {

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

void registerTypes() {
    // 在本地注册表中注册类型
    localTypeRegistryInstance.registerType("PreCGNS", 
        []() -> nlohmann::json { return PreCGNS::GetParamSchema(); });
    
    localTypeRegistryInstance.registerType("PrePlot3D", 
        []() -> nlohmann::json { return PrePlot3D::GetParamSchema(); });
    
    // 在本地工厂中注册创建函数
    localFactoryInstance.registerModuleType("PreCGNS", 
        [](ModuleSystem::AdvancedRegistry* reg, const std::string& name) -> bool { 
            reg->Register<PreCGNS>(name);
            return true;
        });
    
    localFactoryInstance.registerModuleType("PrePlot3D", 
        [](ModuleSystem::AdvancedRegistry* reg, const std::string& name) -> bool { 
            reg->Register<PrePlot3D>(name);
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
    std::cout << "导出网格预处理模块到全局注册表..." << std::endl;
#endif
    // 获取本组件特有的模块名称
    std::unordered_set<std::string> preGridModules = {"PreCGNS", "PrePlot3D"};
    
    // 使用新的导出方法将本地注册表导出到全局
    localTypeRegistryInstance.exportToGlobal();
    localFactoryInstance.exportToGlobal();
    
    // 确保我们使用的是全局实例来关联模块和引擎
    initializeFactory();
    
    // 将模块关联到对应的引擎 - 只关联成功导出的模块
    auto& moduleRegistryInit = ModuleSystem::ModuleRegistryInitializer::init();
    for (const auto& name : preGridModules) {
        moduleRegistryInit.assignModuleToEngine(name, "PreGrid");
#if DEBUG
        std::cout << "  - 已将模块 '" << name << "' 关联到 PreGrid 引擎" << std::endl;
#endif
    }
#if DEBUG
    std::cout << "网格预处理模块导出完成，共导出 " << preGridModules.size() << " 个类型和 " 
              << preGridModules.size() << " 个模块" << std::endl;
#endif
}

//============= PreCGNS 模块实现 =============//

PreCGNS::PreCGNS(const nlohmann::json& params) {
    cgns_type_ = params.value("cgns_type", "HDF5");
    cgns_value_ = params.value("cgns_value", 15);
}

void PreCGNS::initialize() {
    // PreCGNS 初始化逻辑
}

void PreCGNS::execute() {
    // PreCGNS 执行逻辑
}

void PreCGNS::release() {
    // PreCGNS 释放资源逻辑
}

nlohmann::json PreCGNS::GetParamSchema() {
    return {
        {"cgns_type", {
            {"type", "string"},
            {"description", "CGNS文件类型"},
            {"enum", {"HDF5", "ADF", "XML"}},
            {"default", "HDF5"}
        }},
        {"cgns_value", {
            {"type", "number"},
            {"description", "CGNS参数值"},
            {"minimum", 1},
            {"maximum", 100},
            {"default", 15}
        }}
    };
}

//============= PrePlot3D 模块实现 =============//

PrePlot3D::PrePlot3D(const nlohmann::json& params) {
    plot3_type_ = params.value("plot3_type", "ASCII");
    plot3d_value_ = params.value("plot3d_value", 30);
}

void PrePlot3D::initialize() {
    // PrePlot3D 初始化逻辑
}

void PrePlot3D::execute() {
    // PrePlot3D 执行逻辑
}

void PrePlot3D::release() {
    // PrePlot3D 释放资源逻辑
}

nlohmann::json PrePlot3D::GetParamSchema() {
    return {
        {"plot3_type", {
            {"type", "string"},
            {"description", "Plot3D文件类型"},
            {"enum", {"ASCII", "Binary", "Formatted"}},
            {"default", "ASCII"}
        }},
        {"plot3d_value", {
            {"type", "number"},
            {"description", "Plot3D参数值"},
            {"minimum", 1},
            {"maximum", 100},
            {"default", 30}
        }}
    };
}

} // namespace PREGRID


namespace ModuleSystem {

nlohmann::json ModuleParamTraits<PREGRID::PreCGNS>::GetParamSchema() {
    return PREGRID::PreCGNS::GetParamSchema();
}

nlohmann::json ModuleParamTraits<PREGRID::PrePlot3D>::GetParamSchema() {
    return PREGRID::PrePlot3D::GetParamSchema();
}

} // namespace ModuleSystem