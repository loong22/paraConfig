# paraConfig

paraConfig是一个基于C++的高级模块化参数管理系统，通过JSON配置文件驱动各类计算模块的执行流程。系统采用工厂模式和嵌套引擎设计，特别适用于复杂的流体力学计算、网格处理和结果后处理等科学计算流程管理。

## 核心功能特点

- **完善的JSON配置系统**：使用结构化JSON格式定义模块参数、执行流程和工厂绑定关系
- **双层工厂模式**：支持"选择一个"或"按顺序执行"两种模块工厂策略
- **引擎与工厂绑定**：可将引擎绑定到特定工厂，实现模块访问权限控制
- **嵌套引擎架构**：支持主引擎和子引擎分层设计，灵活组织复杂执行流程
- **完整的模块生命周期管理**：自动处理模块的创建、初始化、执行和释放全流程
- **严格的参数验证机制**：内置参数类型检查、范围验证和枚举值验证
- **自动配置生成**：根据注册模块自动生成默认配置模板
- **完整UTF-8支持**：确保中文和其他Unicode字符正常显示
- **资源泄漏检测**：自动检测未释放的模块资源
- **循环依赖检测**：防止引擎之间的循环引用导致死循环
- **详细的错误报告**：提供准确的配置验证错误信息，帮助快速排查问题

## 如何使用

### 生成配置模板

生成默认配置模板使用`get`参数：

```bash
./paraConfig get [输出文件名]
```

如不指定输出文件名，将默认生成`./config.json`。

### 使用配置文件运行

使用指定配置文件运行程序：

```bash
./paraConfig [配置文件路径]
```

如不指定配置文件路径，将默认使用`./config.json`。

## 配置文件结构

配置文件采用JSON格式，主要包含以下核心部分：

### 1. config - 全局参数和模块参数配置

全局参数包括：
- `solver`: 求解器类型 (如"SIMPLE", "PISO", "PIMPLE", "Coupled")
- `maxIterations`: 最大迭代次数 (1-1000000)
- `convergenceCriteria`: 收敛判断标准 (0-1之间)
- `time_step`: 时间步长 (必须大于0)

模块特定参数采用嵌套对象格式：
```json
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
        "solver_type": "Standard",
        "convergence_criteria": 1e-06,
        "max_iterations": 1000
    }
}
```

### 2. moduleFactories - 模块工厂定义

定义模块工厂及其执行策略：
- `CHOOSE_ONE`：引擎在执行时，只能选择该工厂中的一个模块
- `SEQUENTIAL_EXECUTION`：按定义顺序依次执行工厂中的所有启用模块

```json
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
    }
]
```

### 3. engineFactoryBindings - 引擎与工厂绑定关系

定义每个引擎可访问的模块工厂，实现访问控制：

```json
"engineFactoryBindings": [
    {"engineName": "PreGrid", "factoryName": "preprocessing_factory"},
    {"engineName": "Solve", "factoryName": "solver_factory"},
    {"engineName": "Post", "factoryName": "post_factory"},
    {"engineName": "mainProcess", "factoryName": "default"}
]
```

### 4. engine - 引擎定义和执行流程

定义引擎池及各引擎中启用的模块或子引擎：

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
        },
        {
            "name": "mainProcess",
            "description": "总控制引擎",
            "enabled": true,
            "subenginePool": ["PreGrid", "Solve", "Post"]
        }
    ]
}
```

引擎定义可以包含：
- `modules`：直接使用的模块列表
- `subenginePool`：子引擎列表，用于构建复杂工作流
- `enabled`：每个引擎和模块都可以单独控制是否启用

### 5. registry - 模块注册信息

定义系统中所有可用模块及其启用状态：

```json
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
```

## 模块执行流程

系统遵循严格的模块生命周期管理：

1. **构造阶段**：系统依次创建所有启用模块的实例
2. **初始化阶段**：按创建顺序初始化所有模块
3. **执行阶段**：依次执行所有初始化完成的模块
4. **释放阶段**：按照逆序释放所有模块资源

生命周期状态转换：
```
CONSTRUCTED -> INITIALIZED -> EXECUTED -> RELEASED
```

模块执行过程中发生错误时，系统会尝试按逆序释放已创建的模块资源，确保系统不会发生资源泄漏。系统会自动跟踪每个模块的生命周期状态，并在发生状态转换错误时提供详细的错误信息。

## 工厂执行策略

系统支持两种工厂执行策略：

### CHOOSE_ONE 策略

- 用于从多个选项中选择一个模块执行
- 每个引擎中只能启用该工厂的一个模块
- 如果同一引擎中启用了多个模块，系统会在验证阶段报错
- 典型应用：多种网格输入格式中选择一种

### SEQUENTIAL_EXECUTION 策略

- 按顺序执行工厂中所有启用的模块
- 可以灵活地启用或禁用某些模块
- 典型应用：求解器多个阶段的连续执行

## 访问控制机制

系统通过引擎与工厂的绑定关系实现模块访问控制：

- 每个引擎只能访问其绑定工厂中的模块
- 未明确绑定的引擎默认使用"default"工厂
- 引擎尝试访问非绑定工厂的模块时会立即终止程序并提供明确的错误信息：
  ```
  访问控制错误: 引擎 'EngineName' 尝试创建不属于其绑定工厂的模块 'ModuleName'
  ```
- "CHOOSE_ONE"类型的工厂在同一引擎中只能启用一个模块

## 支持的模块

系统内置支持以下模块类型：

### 预处理模块
- **PreCGNS**: CGNS格式网格处理
  - 参数：cgns_type (HDF5/ADF), cgns_value (1-100)
- **PrePlot3D**: Plot3D格式网格处理
  - 参数：plot3_type (ASCII/Binary), plot3_value (1-100)

### 求解器模块
- **EulerSolver**: 欧拉方程求解器
  - 参数：euler_type (Standard/Other), euler_value (0-10)
- **SASolver**: Spalart-Allmaras湍流模型求解器
  - 参数：solver_type (Standard/SA-BC/SA-DDES/SA-IDDES), convergence_criteria (1e-10到1e-3), max_iterations (10-10000)
- **SSTSolver**: SST湍流模型求解器
  - 参数：solver_type (Standard/SST-CC/SA-DDES/SA-IDDES), convergence_criteria (1e-10到1e-3), max_iterations (10-10000)

### 后处理模块
- **PostCGNS**: CGNS格式结果输出
  - 参数：cgns_type (HDF5/ADF), cgns_value (1-100)
- **PostPlot3D**: Plot3D格式结果输出
  - 参数：plot3_type (ASCII/Binary), plot3_value (1-100)

## 配置验证机制

系统在执行前会进行多层次的配置验证：

1. **工厂定义验证**：检查工厂名称唯一性和执行策略有效性
2. **引擎定义验证**：检查引擎名称唯一性和子引擎存在性
3. **参数类型验证**：根据参数架构验证每个模块参数的类型和值范围
4. **模块依赖验证**：检查模块间依赖关系和引擎绑定的有效性
5. **循环依赖检测**：防止引擎之间的循环引用，确保执行路径是有向无环图
6. **访问控制验证**：确保引擎只访问其绑定工厂中的模块
7. **CHOOSE_ONE策略验证**：确保同一引擎中CHOOSE_ONE工厂只启用一个模块
8. **必需参数验证**：确保所有标记为required的参数都已提供

验证失败会提供详细的错误信息，帮助用户快速定位配置问题，例如：
```
错误: 引擎 'EngineName' 中模块 'ModuleName' 的参数 'paramName' 验证失败: 值必须大于等于10，但获取到5
```

## 配置文件保存

系统会将实际执行时使用的配置保存为`[原始配置文件名]_used.json`，方便问题追踪和执行记录。

## 资源泄漏检测

系统会在执行结束后检查是否有模块未被正确释放，防止资源泄漏：

```
警告: 检测到未释放的模块:
  - ModuleName (状态: EXECUTED)
```

- 自动追踪每个模块的生命周期状态
- 检测未释放的模块并提供详细报告
- 在执行流程中断时尝试释放已创建的模块资源

## 模块参数架构

模块参数使用JSON Schema风格定义，支持以下验证规则：

- **类型验证**：string, number, boolean, array, object
- **范围验证**：minimum, maximum (数值类型)
- **枚举验证**：enum (字符串类型)
- **必需性验证**：required
- **默认值**：default

例如，CGNS模块的参数架构：
```cpp
{
    "cgns_type": {
        "type": "string",
        "description": "Type of cgns file",
        "enum": ["HDF5", "ADF"],
        "default": "HDF5"
    },
    "cgns_value": {
        "type": "number",
        "description": "Number of cgns value",
        "minimum": 1,
        "maximum": 100,
        "default": 10
    }
}
```

## 编译与依赖

使用CMake构建项目：

```bash
mkdir build
cd build
cmake ..
cmake --build . --config Release
```

主要依赖：
- **nlohmann/json**：用于JSON处理
- **C++20或更高标准库**：使用了std::filesystem和其他现代C++特性

## 典型应用场景

### 1. CFD工作流管理
- 网格处理 → 流场求解 → 结果后处理的标准CFD工作流
- 不同阶段可以选择不同的模块实现

### 2. 参数扫描与优化
- 通过脚本生成多组配置文件
- 使用不同参数设置并行或串行执行多个任务

### 3. 多物理场耦合计算
- 使用多个求解器模块依次执行
- 在模块间传递中间结果

### 4. 定制工作流程
- 使用嵌套引擎设计复杂的计算流程
- 根据需要启用或禁用特定模块

## 注意事项

- 确保使用UTF-8编码保存配置文件，特别是包含中文字符时
- 在Windows环境下，程序会自动设置控制台代码页为UTF-8
- 所有模块参数都会进行类型和范围验证，请确保参数值在有效范围内
- CHOOSE_ONE类型的工厂中，同一引擎只能启用一个模块
- 引擎只能访问其绑定工厂中的模块，确保正确设置engineFactoryBindings
- 系统会检查未释放的模块资源，确保不发生资源泄漏
- 主进程引擎(mainProcess)必须存在且启用，作为程序的入口点
- 避免在引擎定义中创建循环依赖，这会导致验证失败