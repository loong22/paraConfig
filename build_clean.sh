#!/bin/bash
# Linux 版本的构建清理脚本

# 检查 build 目录是否存在，不存在则创建
if [ ! -d "build" ]; then
    mkdir build
    echo "创建 build 目录"
fi

# 进入 build 目录
cd build

# 运行 CMake 配置
cmake -G "Ninja" -DCMAKE_BUILD_TYPE=Release ..

# 执行清理操作
cmake --build . --config Release --target clean

echo "清理完成"