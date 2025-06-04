cmake_minimum_required(VERSION 3.10)
project(paraConfig VERSION 2.0)

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 设置输出目录
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# 查找源文件
file(GLOB SOURCES "source/*.cpp")

# 创建可执行文件
add_executable(paraConfig ${SOURCES})

# 添加头文件路径
target_include_directories(paraConfig PUBLIC ${PROJECT_SOURCE_DIR}/include)
target_include_directories(paraConfig PUBLIC ${PROJECT_SOURCE_DIR}/packages)

# 添加安装目标
install(TARGETS paraConfig DESTINATION bin)