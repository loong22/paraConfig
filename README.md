# paraConfig

paraConfig是一个基于C++的模块化参数管理系统，通过JSON配置文件驱动各种计算模块的执行，适用于复杂的计算流程管理。

## 功能特点

- **基于JSON的灵活配置系统**：使用标准JSON格式定义所有参数和执行流程
- **模块化设计**：支持动态模块注册与管理，模块可独立开发和扩展
- **工厂模式支持**：提供"选择一个"或"按顺序执行"两种模块执行策略
- **嵌套引擎架构**：支持子引擎和主流程分离，灵活组织执行流程
- **完整的模块生命周期管理**：从创建、初始化、执行到释放的全生命周期跟踪
- **严格的参数验证**：内置参数类型、范围和枚举值验证
- **自动生成配置模板**：根据注册模块自动生成默认配置
- **支持Windows平台**：完整的UTF-8编码支持，确保中文显示正常

## 如何使用

### 生成配置模板

要生成默认的配置模板，请使用 get 参数：

```bash
./paraConfig get [输出文件名]
```

如果不指定输出文件名，将默认生成 `./config.json`。

### 使用现有配置运行

要使用现有配置文件运行程序：

```bash
./paraConfig [配置文件路径]
```

如果不指定配置文件路径，将默认使用 `./config.json`。

### 配置文件结构

配置文件采用JSON格式，主要包含以下部分：

- **config** - 全局配置参数和模块特定参数
  - 全局参数：solver, maxIterations, convergenceCriteria, time_step等
  - 模块特定参数：每个模块的配置参数
- **moduleFactories** - 模块工厂定义
  - 定义不同的模块工厂及其执行策略(CHOOSE_ONE/SEQUENTIAL_EXECUTION)
  - 为每个工厂指定包含的模块
- **engineFactoryBindings** - 引擎与工厂绑定关系
  - 定义每个引擎可以访问哪个工厂中的模块
- **engine** - 引擎执行流程配置
  - enginePool：定义所有引擎及其执行模块和子引擎
- **registry** - 注册模块配置
  - modules：所有可用模块及其启用状态和参数定义

实际使用的配置将保存为`[原始配置文件名]_used.json`，便于调试和追踪。

### 支持的模块

系统当前支持以下模块：

#### 预处理模块
- **PreCGNS**: CGNS格式网格处理
  - 参数：cgns_type (HDF5/ADF), cgns_value (1-100)
- **PrePlot3D**: Plot3D格式网格处理
  - 参数：plot3_type (ASCII/Binary), plot3_value (1-100)

#### 求解器模块
- **EulerSolver**: 欧拉方程求解器
  - 参数：euler_type (Standard/Other), euler_value (0-10)
- **SASolver**: Spalart-Allmaras湍流模型求解器
  - 参数：solver_type (Standard/SA-BC/SA-DDES/SA-IDDES), convergence_criteria (1e-10到1e-3), max_iterations (10-10000)
- **SSTSolver**: SST湍流模型求解器
  - 参数：solver_type (Standard/SST-CC/SA-DDES/SA-IDDES), convergence_criteria (1e-10到1e-3), max_iterations (10-10000)

#### 后处理模块
- **PostCGNS**: CGNS格式结果输出
  - 参数：cgns_type (HDF5/ADF), cgns_value (1-100)
- **PostPlot3D**: Plot3D格式结果输出
  - 参数：plot3_type (ASCII/Binary), plot3_value (1-100)

### 工厂执行策略

系统支持两种工厂执行策略：

- **CHOOSE_ONE**: 用户在运行时从工厂中选择一个模块执行
  - 一个引擎中只能启用该工厂的一个模块
  - 适用于多种实现方式中选择一种的场景
- **SEQUENTIAL_EXECUTION**: 按指定顺序依次执行工厂中的所有模块
  - 可以设置firstModule和executionOrder自定义执行顺序
  - 适用于需要按特定流程执行多个步骤的场景

### 引擎执行流程

默认配置包含以下引擎：

1. **PreGrid**: 网格预处理引擎，绑定preprocessing_factory工厂
2. **Solve**: 数值求解引擎，绑定solver_factory工厂
3. **Post**: 结果后处理引擎，绑定post_factory工厂
4. **mainProcess**: 主控制引擎，协调调用上述三个引擎

### 引擎与工厂绑定

系统通过engineFactoryBindings配置引擎与工厂的绑定关系：
- 每个引擎只能访问其绑定工厂中的模块
- 未明确绑定的引擎默认使用"default"工厂
- 引擎不能访问其绑定工厂之外的模块，确保模块访问控制

## 配置示例

```json
{
    "config": {
        "convergenceCriteria": 1e-06,
        "maxIterations": 1000,
        "solver": "SIMPLE",
        "time_step": 0.01,
        "EulerSolver": {
            "euler_type": "Standard",
            "euler_value": 0.5
        },
        "SASolver": {
            "solver_type": "SA-BC",
            "convergence_criteria": 1e-6,
            "max_iterations": 1000
        }
    },
    "moduleFactories": [
        {
            "name": "preprocessing_factory",
            "executionType": "CHOOSE_ONE",
            "modules": [
                {"name": "PreCGNS", "enabled": true},
                {"name": "PrePlot3D", "enabled": true}
            ]
        },
        {
            "name": "solver_factory",
            "executionType": "SEQUENTIAL_EXECUTION",
            "modules": [
                {"name": "EulerSolver", "enabled": true},
                {"name": "SASolver", "enabled": true},
                {"name": "SSTSolver", "enabled": true}
            ]
        },
        {
            "name": "post_factory",
            "executionType": "SEQUENTIAL_EXECUTION",
            "modules": [
                {"name": "PostCGNS", "enabled": true},
                {"name": "PostPlot3D", "enabled": true}
            ]
        }
    ],
    "engineFactoryBindings": [
        {"engineName": "PreGrid", "factoryName": "preprocessing_factory"},
        {"engineName": "Solve", "factoryName": "solver_factory"},
        {"engineName": "Post", "factoryName": "post_factory"},
        {"engineName": "mainProcess", "factoryName": "default"}
    ],
    "engine": {
        "enginePool": [
            {
                "name": "PreGrid",
                "description": "网格预处理引擎",
                "enabled": true,
                "modules": [
                    {"name": "PreCGNS", "enabled": true}
                ]
            },
            {
                "name": "Solve",
                "description": "求解引擎",
                "enabled": true,
                "modules": [
                    {"name": "EulerSolver", "enabled": true},
                    {"name": "SASolver", "enabled": true}
                ]
            },
            {
                "name": "Post",
                "description": "后处理引擎",
                "enabled": true,
                "modules": [
                    {"name": "PostCGNS", "enabled": true},
                    {"name": "PostPlot3D", "enabled": true}
                ]
            },
            {
                "name": "mainProcess",
                "description": "总控制引擎",
                "enabled": true,
                "subenginePool": ["PreGrid", "Solve", "Post"]
            }
        ]
    },
    "registry": {
        "modules": [
            {"name": "PreCGNS", "enabled": true},
            {"name": "PrePlot3D", "enabled": true},
            {"name": "EulerSolver", "enabled": true},
            {"name": "SASolver", "enabled": true},
            {"name": "SSTSolver", "enabled": true},
            {"name": "PostCGNS", "enabled": true},
            {"name": "PostPlot3D", "enabled": true}
        ]
    }
}
```

## 参数验证规则

系统在执行前会对配置进行严格验证，包括：

- 工厂定义的有效性检查（名称唯一性、执行策略有效性）
- 引擎定义的有效性检查（名称唯一性、子引擎存在性）
- 模块参数的类型和范围验证（基于模块声明的参数架构）
- 引擎与工厂绑定的有效性验证（工厂存在性、引擎存在性）
- 执行策略对应的模块配置验证（CHOOSE_ONE策略下一个引擎中只能启用一个模块）
- 循环依赖检测（防止引擎之间的循环引用）

验证失败会提供详细的错误信息，帮助用户快速定位配置问题。

## 编译

在Windows环境下，可以使用提供的批处理文件构建项目：

```bash
build_release.bat
```

或者使用CMake手动构建：

```bash
mkdir build
cd build
cmake ..
cmake --build . --config Release
```

## 依赖库

- **nlohmann/json** - 用于JSON处理
- **C++17标准库**

## 注意事项

- 程序使用UTF-8编码，确保在不同终端环境下都能正确显示中文字符
- 在Windows环境下，为确保正确显示中文，命令行可以使用`chcp 65001`设置代码页
- 所有配置路径均使用相对路径或绝对路径，支持跨平台使用
- 模块执行会严格检查生命周期，确保所有模块都正确释放资源
- CHOOSE_ONE工厂类型中，同一引擎只能启用一个模块，否则验证会失败
- 引擎只能访问其绑定工厂中的模块，确保模块访问控制