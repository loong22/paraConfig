@echo off
chcp 65001

REM 检查 build 目录是否存在，不存在则创建
if not exist build (
    mkdir build
    echo 创建 build 目录
)

REM 进入 build 目录
cd build

REM 运行 CMake 配置
cmake -G "Ninja" -DCMAKE_BUILD_TYPE=Release ..

REM 执行清理操作
cmake --build . --config Release --target clean

echo 清理完成