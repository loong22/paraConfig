#!/bin/bash
# Linux 版本的构建脚本

# 检查 build 目录是否存在，不存在则创建
if [ ! -d "build" ]; then
    mkdir build
    echo "创建 build 目录"
fi

# 进入 build 目录
cd build

# 明确指定使用Unix Makefiles生成器
echo "使用Unix Makefiles构建工具"
cmake -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Release ..

# 执行构建操作
# 使用系统处理器核心数量作为并行编译的作业数
CORES=$(nproc)

# 使用make命令而不是cmake --build
if [ -f "Makefile" ]; then
    make -j $CORES
    echo "构建完成"
else
    echo "Makefile不存在，构建失败"
fi