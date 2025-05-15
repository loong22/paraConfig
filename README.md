# paraConfig
C++ 参数数据库，基于 JSON 配置文件驱动的模块化系统

## 功能特点
基于 JSON 格式的灵活配置系统
模块化设计，支持动态模块注册与管理
嵌套引擎架构，支持子过程执行
完整的模块生命周期管理
自动生成配置模板功能
支持 Windows 平台，UTF-8 编码兼容

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
配置文件采用 JSON 格式，主要包含三个部分：

- **config** - 全局配置参数和模块特定参数
  - 全局参数：solver, maxIterations, convergenceCriteria, time_step 等
  - 模块特定参数：每个模块的配置参数，如 TurbulenceSolverSA, LaminarSolverEuler 等
- **registry** - 注册模块配置
  - modules：所有可用模块及其启用状态和参数定义
- **engine** - 引擎执行流程配置
  - enginePool：定义所有引擎（包括子引擎）及其执行模块

实际使用的配置将保存为 `[原始配置文件名]_used.json`，便于调试和追踪。

### 配置示例
配置文件示例可参考程序根目录下的 `config.json`，包含了所有模块的参数设置和引擎执行流程定义。

## 编译
在 Windows 环境下，可以使用提供的批处理文件构建项目：

```bash
build_release.bat
```

这将使用 CMake 构建项目的 Release 版本。

## 依赖库
- nlohmann/json - 用于 JSON 处理
- C++17 标准库

## 注意事项
程序使用 UTF-8 编码，确保在不同终端环境下都能正确显示中文字符
在 Windows 环境下，为确保正确显示中文，命令行可以使用 chcp 65001 设置代码页