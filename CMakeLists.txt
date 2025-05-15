cmake_minimum_required(VERSION 3.10)
project(paraConfig VERSION 1.0)

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# 删除 MPI C++ 编译器设置
# set(CMAKE_CXX_COMPILER "mpicxx")

# 设置输出目录
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# 查找源文件
file(GLOB SOURCES "source/*.cpp")

# 创建可执行文件
add_executable(paraConfig ${SOURCES})

# 添加头文件路径
target_include_directories(paraConfig PUBLIC ${PROJECT_SOURCE_DIR}/include)
target_include_directories(paraConfig PUBLIC ${PROJECT_SOURCE_DIR}/packages)
# 删除 MPI 相关头文件路径
# target_include_directories(paraConfig PUBLIC ${PROJECT_SOURCE_DIR}/packages/openmpi-5.0.4/include)

# 删除查找 MPI 库
# find_library(MPI_LIB NAMES mpi PATHS ${PROJECT_SOURCE_DIR}/packages/openmpi-5.0.4/lib)

# 删除链接 MPI 库
# target_link_libraries(paraConfig PRIVATE ${MPI_LIB})

# 添加安装目标
install(TARGETS paraConfig DESTINATION bin)