# ParaConfig - 模块化计算流体力学(CFD)配置系统

一个基于C++和JSON的模块化CFD预处理、求解和后处理配置管理系统。

## 项目概述

ParaConfig是一个灵活的配置管理系统，用于管理复杂的CFD工作流程。系统采用分层架构设计，支持模块化配置和动态组合。

### 主要特性

- **模块化设计**: 支持预处理、湍流模型、后处理等各种模块
- **分层引擎**: 从主进程到子引擎的多层次管理
- **JSON配置**: 基于JSON Schema的配置验证和默认值生成
- **动态组合**: 支持模块实例的灵活组合和重用
- **类型安全**: 使用C++17的std::variant确保类型安全

## 系统架构

```mermaid
graph TD
    A[EngineMainProcess 主进程引擎]
    A --> B[EnginePre 预处理引擎]
    B --> B1[EnginePreGrid 网格预处理引擎]
    B1 --> B11[ModulePreCGNS CGNS预处理模块]
    B1 --> B12[ModulePrePlot3D Plot3D预处理模块]
    B1 --> B13[ModulePreTecplot Tecplot预处理模块]

    A --> C[EngineSolve 求解引擎]
    C --> C1[EngineTurbulence 湍流引擎]
    C1 --> C11[ModuleSA Spalart-Allmaras模型]
    C1 --> C12[ModuleSST SST k-ω模型]
    C1 --> C13[ModuleSSTWDF SST壁面阻尼函数模型]

    A --> D[EnginePost 后处理引擎]
    D --> D1[EngineFlowField 流场引擎]
    D1 --> D11[ModulePostCGNS CGNS后处理模块]
    D1 --> D12[ModulePostPlot3D Plot3D后处理模块]
    D1 --> D13[ModulePostTecplot Tecplot后处理模块]
```

## 程序执行流程图

```mermaid
graph TD
    A[程序启动] --> B{解析命令行参数}
    
    B -->|--write-config| C[生成默认配置文件]
    C --> D[创建config.json]
    C --> E[创建register.json]
    D --> F[程序结束]
    E --> F
    
    B -->|--config| G[加载配置文件]
    G --> H[验证JSON格式]
    H --> I[检查EngineMainProcess配置]
    I --> J[创建EngineMainProcess实例]
    
    J --> K[Initialize阶段]
    K --> L[初始化EnginePre]
    K --> M[初始化EngineSolve]
    K --> N[初始化EnginePost]
    
    L --> L1[初始化EnginePreGrid]
    L1 --> L2[创建预处理模块实例]
    L2 --> L3[ModulePreCGNS::Initialize]
    L2 --> L4[ModulePrePlot3D::Initialize]
    L2 --> L5[ModulePreTecplot::Initialize]
    
    M --> M1[初始化EngineTurbulence]
    M1 --> M2[创建湍流模块实例]
    M2 --> M3[ModuleSA::Initialize]
    M2 --> M4[ModuleSST::Initialize]
    M2 --> M5[ModuleSSTWDF::Initialize]
    
    N --> N1[初始化EngineFlowField]
    N1 --> N2[创建后处理模块实例]
    N2 --> N3[ModulePostCGNS::Initialize]
    N2 --> N4[ModulePostPlot3D::Initialize]
    N2 --> N5[ModulePostTecplot::Initialize]
    
    L3 --> O[Execute阶段]
    L4 --> O
    L5 --> O
    M3 --> O
    M4 --> O
    M5 --> O
    N3 --> O
    N4 --> O
    N5 --> O
    
    O --> P[执行EnginePre]
    P --> P1[执行EnginePreGrid]
    P1 --> P2[按module_execution_order顺序执行]
    P2 --> P3[ModulePreCGNS::Execute]
    P3 --> P4[ModulePrePlot3D::Execute]
    P4 --> P5[ModulePreTecplot::Execute]
    
    P5 --> S[执行EngineSolve]
    S --> S1[执行EngineTurbulence]
    S1 --> S2[按module_execution_order顺序执行]
    S2 --> S3[ModuleSA::Execute]
    S3 --> S4[ModuleSST::Execute]
    S4 --> S5[ModuleSSTWDF::Execute]
    
    S5 --> V[执行EnginePost]
    V --> V1[执行EngineFlowField]
    V1 --> V2[按module_execution_order顺序执行]
    V2 --> V3[ModulePostCGNS::Execute]
    V3 --> V4[ModulePostPlot3D::Execute]
    V4 --> V5[ModulePostTecplot::Execute]
    
    V5 --> Y[Release阶段]
    Y --> Y1[按逆序释放EnginePost]
    Y1 --> Y2[按逆序释放EngineFlowField]
    Y2 --> Y3[按逆序释放后处理模块]
    Y3 --> Y4[ModulePostTecplot::Release]
    Y4 --> Y5[ModulePostPlot3D::Release]
    Y5 --> Y6[ModulePostCGNS::Release]
    
    Y6 --> Y7[按逆序释放EngineSolve]
    Y7 --> Y8[按逆序释放EngineTurbulence]
    Y8 --> Y9[按逆序释放湍流模块]
    Y9 --> Y10[ModuleSSTWDF::Release]
    Y10 --> Y11[ModuleSST::Release]
    Y11 --> Y12[ModuleSA::Release]
    
    Y12 --> Y13[按逆序释放EnginePre]
    Y13 --> Y14[按逆序释放EnginePreGrid]
    Y14 --> Y15[按逆序释放预处理模块]
    Y15 --> Y16[ModulePreTecplot::Release]
    Y16 --> Y17[ModulePrePlot3D::Release]
    Y17 --> Y18[ModulePreCGNS::Release]
    
    Y18 --> AA[程序结束]
    
    B -->|无效参数| AB[显示使用说明]
    AB --> AC[程序异常退出]
    
    style A fill:#e1f5fe
    style F fill:#c8e6c9
    style AA fill:#c8e6c9
    style AC fill:#ffcdd2
    
    style P3 fill:#e8f5e8
    style P4 fill:#e8f5e8
    style P5 fill:#e8f5e8
    style S3 fill:#fff3e0
    style S4 fill:#fff3e0
    style S5 fill:#fff3e0
    style V3 fill:#f3e5f5
    style V4 fill:#f3e5f5
    style V5 fill:#f3e5f5
```

## 快速开始

### 编译要求

- C++17或更高版本
- nlohmann/json库
- CMake 3.10+

### 编译步骤

```bash
mkdir build
cd build
cmake ..
make
```

### 基本使用

1. **生成默认配置文件**:
```bash
./paraconfig --write-config ./config
```

这将在`./config`目录下生成:
- `config.json` - 包含所有模块和引擎的默认参数
- `register.json` - 包含所有组件的JSON Schema定义

2. **运行配置**:
```bash
./paraconfig --config ./config/config.json
```

## 配置文件结构

### config.json 示例结构

```json
{
  "EngineMainProcess": {
    "execution_order": ["EnginePre", "EngineSolve", "EnginePost"]
  },
  "EnginePre": {
    "execution_order": ["EnginePreGrid"]
  },
  "EnginePreGrid": {
    "module_execution_order": ["ModulePreCGNS"]
  },
  "ModulePreCGNS": {
    "cgns_type": "HDF5",
    "cgns_value": 15.0
  }
}
```

### 支持的模块类型

#### 预处理模块
- **ModulePreCGNS**: CGNS文件预处理
- **ModulePrePlot3D**: Plot3D文件预处理  
- **ModulePreTecplot**: Tecplot文件预处理

#### 湍流模型模块
- **ModuleSA**: Spalart-Allmaras一方程模型
- **ModuleSST**: SST k-ω两方程模型
- **ModuleSSTWDF**: 带壁面阻尼函数的SST模型

#### 后处理模块
- **ModulePostCGNS**: CGNS文件输出
- **ModulePostPlot3D**: Plot3D文件输出
- **ModulePostTecplot**: Tecplot文件输出

## 高级用法

### 模块实例重用

支持同一模块类型的多个实例:

```json
{
  "EngineFlowField": {
    "module_execution_order": ["ModulePostPlot3D", "ModulePostPlot3D_1", "ModulePostPlot3D_2"]
  },
  "ModulePostPlot3D": {
    "write_q_file": true
  },
  "ModulePostPlot3D_1": {
    "write_q_file": false
  },
  "ModulePostPlot3D_2": {
    "write_q_file": true
  }
}
```

### 引擎重用

支持引擎实例的重用，比如多个后处理阶段:

```json
{
  "EngineMainProcess": {
    "execution_order": ["EnginePre", "EngineSolve", "EnginePost", "EnginePost_1"]
  }
}
```

## 文件结构

```
001/
├── include/
│   └── paraConfig.h          # 主头文件，包含所有类声明
├── source/
│   ├── paraConfig.cpp        # 主实现文件
│   └── main.cpp             # 程序入口点
├── README.md                # 项目文档
└── build/                   # 编译输出目录
```

## API文档

### 核心类

- **EngineMainProcess**: 主进程管理器，协调整个工作流程
- **EnginePre/EngineSolve/EnginePost**: 各阶段的顶层引擎
- **EnginePreGrid/EngineTurbulence/EngineFlowField**: 具体功能引擎
- **Module***: 各种具体功能模块

### 关键方法

每个引擎和模块都实现了统一的接口:
- `Initialize()`: 初始化资源
- `Execute()`: 执行主要逻辑
- `Release()`: 释放资源
- `GetParamSchema()`: 返回参数JSON Schema

## 贡献指南

1. Fork本项目
2. 创建功能分支 (`git checkout -b feature/新功能`)
3. 提交更改 (`git commit -am '添加新功能'`)
4. 推送到分支 (`git push origin feature/新功能`)
5. 创建Pull Request

## 许可证

本项目采用MIT许可证 - 查看[LICENSE](LICENSE)文件了解详情。

## 联系方式

如有问题或建议，请通过GitHub Issues联系。


