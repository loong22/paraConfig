#!/bin/bash
# Linux 版本的构建清理脚本

# 检查 build 目录是否存在，不存在则创建
if [ ! -d "build" ]; then
    mkdir build
    echo "创建 build 目录"
fi

# 清除之前的build内容
rm -rf build/*
echo "已清除旧的build目录内容"

# 进入 build 目录
cd build

# 明确指定使用Unix Makefiles生成器
echo "使用Unix Makefiles构建工具"
cmake -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Release ..

# 如果需要清理，直接使用make clean而不是CMake命令
if [ -f "Makefile" ]; then
    make clean
    echo "清理完成"
else
    echo "Makefile不存在，无法清理"
fi