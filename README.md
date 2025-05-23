# ParaConfig - 可配置模块化CFD参数系统

[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](https://opensource.org/licenses/MIT)

ParaConfig是一个基于JSON配置的模块化框架，专为流体动力学(CFD)计算参数管理而设计，提供灵活的模块管理、动态配置和执行流程控制。

## 功能特点

- **模块化架构**：支持模块的独立开发、测试和部署。
- **组件化开发**：通过本地注册表和导出机制，支持将模块按功能组件进行组织和管理。
- **基于JSON的配置**：使用JSON格式定义模块参数、引擎行为和执行流程。
- **参数校验体系**：提供完整的参数类型、范围和枚举值检查。
- **工厂绑定机制**：通过工厂绑定控制模块的访问权限（未来增强）。
- **生命周期管理**：自动管理模块的创建、初始化、执行和释放。
- **灵活的引擎系统**：支持嵌套引擎和子引擎执行，定义复杂的计算工作流。
- **资源泄漏检测**：自动检测和报告未释放的模块实例。
- **访问控制机制**：确保引擎只能访问其绑定工厂中的模块（未来增强）。
- **命令行接口**：提供生成配置模板、指定配置文件目录等便捷操作。

## 系统架构

系统由以下核心组件构成：

- **AdvancedRegistry**：模块注册表，负责模块的生命周期管理
- **engineContext**：引擎上下文，提供模块执行环境和参数访问
- **Nestedengine**：嵌套引擎，支持定义和执行引擎工作流
- **ModuleFactory**：模块工厂，负责创建特定类型的模块实例
- **ModuleTypeRegistry**：模块类型注册表，存储可用模块类型及参数架构
- **engineExecutionEngine**：引擎执行引擎，处理引擎工作流的执行

### 组件关系图

```
                     ┌───────────────────┐
                     │   ModuleSystem    │
                     └───────────────────┘
                               │
         ┌───────────┬─────────┼─────────┬───────────┐
         │           │         │         │           │
┌────────▼───────┐   │  ┌──────▼─────┐   │  ┌────────▼────────────┐
│AdvancedRegistry│◄──┘  │Nestedengine│   └─►│engineExecutionEngine│
└────────┬───────┘      └──────┬─────┘      └────────┬────────────┘
         │                     │                     │
         │               ┌─────▼──────┐              │
         └───────────────►engineContext◄─────────────┘
                         └────────────┘
```

## 快速开始

### 安装

1. 克隆仓库
```bash
git clone https://github.com/yourusername/paraConfig.git
cd paraConfig
```

2. 创建构建目录并编译
```bash
mkdir build && cd build
cmake ..
make
```

### 使用示例

1. 生成配置模板
```bash
./paraConfig --generate-templates ./templates
```
   如果未指定目录，则默认生成到 `./templates/`。

2. 编辑配置文件
修改生成的模板文件以适应您的需求。主要配置文件包括：
- `template_engine_mainProcess.json`：主引擎配置，定义全局参数和主工作流。
- `template_engine_PreGrid.json`：网格预处理引擎配置。
- `template_engine_Solve.json`：求解引擎配置。
- `template_engine_Post.json`：后处理引擎配置。
- `template_registry_module.json`：模块注册表配置，定义系统中所有可用模块及其默认启用状态。

3. 运行程序
```bash
./paraConfig --config-dir ./config
```
   如果未指定目录，则默认从 `./config/` 加载。

## 配置文件

系统使用JSON格式的配置文件，主要包含以下部分：

1. **引擎定义** (`config_engine_*.json` 中的 `"engine"` 部分): 定义引擎及其子引擎和直接关联的模块
```json
"engine": {
    "enginePool": [
        {
            "name": "PreGrid",
            "description": "网格预处理引擎",
            "enabled": true,
            "modules": [
                {"name": "PreCGNS", "enabled": true},
                {"name": "PrePlot3D", "enabled": false}
            ]
        }
    ]
}
```

2. **模块参数配置** (`config_engine_*.json` 中的 `"config"` 部分): 设置模块的运行参数。也包括全局配置 `GlobalConfig`。
```json
"config": {
    "GlobalConfig": {
        "solver": "SIMPLE",
        "maxIterations": 1000,
        "convergenceCriteria": 1e-6,
        "time_step": 0.01
    },
    "EulerSolver": {
        "euler_type": "Standard",
        "euler_value": 0.5
    },
    "PreCGNS": {
        "cgns_type": "HDF5",
        "cgns_value": 20
    }
}
```

3. **模块注册** (`config_registry_module.json` 中的 `"registry"` 部分): 注册系统中所有可用的模块及其元数据（通常由程序自动生成模板，用户可修改启用状态）。
```json
"registry": {
    "modules": [
        {"name": "PreCGNS", "enabled": true},
        {"name": "EulerSolver", "enabled": true}
    ]
}
```

## 可用模块

### 预处理模块
- **PreCGNS**：CGNS格式数据预处理
- **PrePlot3D**：Plot3D格式数据预处理

### 求解器模块
- **EulerSolver**：欧拉方程求解器
- **SASolver**：Spalart-Allmaras湍流模型求解器
- **SSTSolver**：SST湍流模型求解器

### 后处理模块
- **PostCGNS**：CGNS格式结果后处理
- **PostPlot3D**：Plot3D格式结果后处理

## 扩展系统

### 添加新模块（作为新组件的一部分）

推荐将新模块组织在独立的组件中，每个组件负责管理一组相关模块的注册和导出。

1.  **创建组件命名空间和文件**：
    *   例如，为新组件 `MYCOMPONENT` 创建 `my_component.h` 和 `my_component.cpp`。

2.  **定义模块类** (在 `my_component.h` 中):
    ```cpp
    // filepath: /home/cfd/paraConfig/include/my_component.h
    #pragma once
    #include "advanced_module_system.h" // 包含核心系统头文件
    #include <string>
    #include <nlohmann/json.hpp>

    namespace MYCOMPONENT {

    class MyNewModule {
    public:
        explicit MyNewModule(const nlohmann::json& params);
        void initialize();
        void execute();
        void release();
        static nlohmann::json GetParamSchema();
    private:
        // 私有成员变量
        std::string example_param_;
    };

    // 声明导出函数
    void exportToGlobalRegistry();

    } // namespace MYCOMPONENT

    // ModuleParamTraits 特化 (在头文件的 ModuleSystem 命名空间中)
    namespace ModuleSystem {
    template <> struct ModuleParamTraits<MYCOMPONENT::MyNewModule> {
        static nlohmann::json GetParamSchema() {
            return MYCOMPONENT::MyNewModule::GetParamSchema();
        }
    };
    } // namespace ModuleSystem
    ```

3.  **实现模块逻辑和本地注册** (在 `my_component.cpp` 中):
    ```cpp
    // filepath: /home/cfd/paraConfig/source/my_component.cpp
    #include "my_component.h" // 包含组件头文件
    #include <iostream> // 用于调试输出

    namespace MYCOMPONENT {

    // --- MyNewModule 实现 ---
    MyNewModule::MyNewModule(const nlohmann::json& params) {
        example_param_ = params.value("example_param", "default_value");
        // 根据 params 初始化模块
    #if DEBUG
        std::cout << "MyNewModule constructed with param: " << example_param_ << std::endl;
    #endif
    }

    void MyNewModule::initialize() {
        // 初始化逻辑
    #if DEBUG
        std::cout << "MyNewModule initialized." << std::endl;
    #endif
    }

    void MyNewModule::execute() {
        // 执行逻辑
    #if DEBUG
        std::cout << "MyNewModule executed." << std::endl;
    #endif
    }

    void MyNewModule::release() {
        // 释放资源逻辑
    #if DEBUG
        std::cout << "MyNewModule released." << std::endl;
    #endif
    }

    nlohmann::json MyNewModule::GetParamSchema() {
        return {
            {"example_param", {
                {"type", "string"},
                {"description", "An example parameter for MyNewModule."},
                {"default", "default_value"}
            }}
            // 定义其他参数...
        };
    }

    // --- 组件注册与导出 ---
    static ModuleSystem::LocalTypeRegistry localTypeRegistryInstance;
    static ModuleSystem::LocalFactory localFactoryInstance;

    // 内部辅助函数，用于注册本组件的所有模块类型和工厂方法
    void registerComponentModules() {
        // 1. 注册到本地类型注册表
        localTypeRegistryInstance.registerType("MyNewModule", 
            []() -> nlohmann::json { return MyNewModule::GetParamSchema(); });
        // 如果有其他模块，也在此注册...

        // 2. 注册到本地工厂
        localFactoryInstance.registerModuleType("MyNewModule", 
            [](ModuleSystem::AdvancedRegistry* reg, const std::string& name) -> bool { 
                reg->Register<MyNewModule>(name); // 使用实际模块类
                return true; 
            });
        // 如果有其他模块的工厂方法，也在此注册...
    }

    void exportToGlobalRegistry() {
        // a. 确保本组件内的模块已在本地注册
        registerComponentModules();

    #if DEBUG
        std::cout << "Exporting MYCOMPONENT modules to global registry..." << std::endl;
    #endif

        // b. 将本地注册表和工厂导出到全局
        localTypeRegistryInstance.exportToGlobal();
        localFactoryInstance.exportToGlobal();

        // c. 将模块关联到对应的引擎（如果适用）
        // 这是配置驱动的，但可以在此为模块建议一个默认引擎
        // 实际关联在 paramValidation 中根据配置文件进行
        // ModuleSystem::ModuleRegistryInitializer::init().assignModuleToEngine("MyNewModule", "TargetEngineName");
        // 示例：假设 MyNewModule 属于 "Solve" 引擎
        ModuleSystem::ModuleRegistryInitializer::init().assignModuleToEngine("MyNewModule", "Solve");


    #if DEBUG
        std::cout << "MYCOMPONENT modules export complete." << std::endl;
    #endif
    }

    } // namespace MYCOMPONENT
    ```

4.  **在主程序中导出组件** (修改 `main.cpp`):
    *   包含新组件的头文件: `#include "my_component.h"`
    *   在 `main` 函数中调用导出函数: `MYCOMPONENT::exportToGlobalRegistry();`
    ```cpp
    // filepath: /home/cfd/paraConfig/source/main.cpp
    // ...existing code...
    #include "post.h"
    #include "my_component.h" // 假设新组件头文件
    // ...existing code...
    int main(int argc, char* argv[]) {
    // ...existing code...
        SOLVE::exportToGlobalRegistry();
        POST::exportToGlobalRegistry();
        MYCOMPONENT::exportToGlobalRegistry(); // 导出新组件的模块

        // 获取配置
    // ...existing code...
    ```

5.  **更新CMakeLists.txt** (如果适用):
    *   将新的 `.cpp` 和 `.h` 文件添加到编译目标中。

6.  **更新配置文件模板和文档**：
    *   如果模块引入了新的引擎或重要的全局配置，考虑更新 `createengineInfo()` 和 `createRegistryInfo()` 中的默认模板生成逻辑。
    *   在 `template_registry_module.json` 中应能看到新模块（如果 `createRegistryInfo` 被正确更新或手动添加）。
    *   在 `config_engine_*.json` 中，可以将新模块添加到相关引擎的 `modules` 列表，并配置其参数。

## 许可证

本项目采用MIT许可证 - 详见 [LICENSE](LICENSE) 文件
