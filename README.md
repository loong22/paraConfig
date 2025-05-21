# ParaConfig - 可配置模块化CFD参数系统

[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](https://opensource.org/licenses/MIT)

ParaConfig是一个基于JSON配置的模块化框架，专为流体动力学(CFD)计算参数管理而设计，提供灵活的模块管理、动态配置和执行流程控制。

## 功能特点

- **模块化架构**：支持模块的独立开发、测试和部署
- **基于JSON的配置**：使用JSON格式定义模块参数和执行流程
- **参数校验体系**：提供完整的参数类型和范围检查
- **工厂绑定机制**：通过工厂绑定控制模块的访问权限
- **生命周期管理**：自动管理模块的创建、初始化、执行和释放
- **灵活的引擎系统**：支持嵌套引擎和子引擎执行
- **资源泄漏检测**：自动检测和报告未释放的模块
- **访问控制机制**：确保引擎只能访问其绑定工厂中的模块

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
./paraConfig --generate-templates ./configs
```

2. 编辑配置文件
修改生成的模板文件以适应您的需求。主要配置文件包括：
- `template_engine_mainProcess.json`：主引擎配置
- `template_engine_PreGrid.json`：网格预处理引擎配置
- `template_engine_Solve.json`：求解引擎配置
- `template_engine_Post.json`：后处理引擎配置

3. 运行程序
```bash
./paraConfig --config-dir ./configs
```

## 配置文件

系统使用JSON格式的配置文件，主要包含以下部分：

1. **引擎定义**：定义引擎及其子引擎和模块
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

2. **模块参数配置**：设置模块的运行参数
```json
"config": {
    "EulerSolver": {
        "euler_type": "Standard",
        "euler_value": 0.5
    }
}
```

3. **模块注册**：注册系统中可用的模块
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

### 添加新模块

要添加新模块，需完成以下步骤：

1. 定义模块类
```cpp
class MyNewModule {
public:
    explicit MyNewModule(const nlohmann::json& params);
    void initialize();
    void execute();
    void release();
    static nlohmann::json GetParamSchema();
private:
    // 私有成员变量
};
```

2. 实现参数特性
```cpp
template <> struct ModuleSystem::ModuleParamTraits<MyNewModule> {
    static nlohmann::json GetParamSchema() {
        return MyNewModule::GetParamSchema();
    }
};
```

3. 注册模块类型
```cpp
ModuleTypeRegistry::instance().registerType(
    "MyNewModule", 
    []() -> nlohmann::json { return MyNewModule::GetParamSchema(); }
);
```

4. 添加到模块工厂
```cpp
factory.registerModuleType("MyNewModule", 
    [](AdvancedRegistry* reg, const std::string& name) -> bool { 
        reg->Register<MyNewModule>(name);
        return true;
    });
```

5. 将模块关联到引擎
```cpp
moduleRegistryInit.assignModuleToEngine("MyNewModule", "YourEngine");
```

## 许可证

本项目采用MIT许可证 - 详见 [LICENSE](LICENSE) 文件
