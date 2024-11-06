cmake_minimum_required(VERSION 3.10)

# 设置项目名称
project(ObjectStorageProject)

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# 添加源文件
set(SOURCE_FILES mian.cpp)

# 添加可执行文件
add_executable(object_storage ${SOURCE_FILES})

# 链接 pthread 库（用于多线程支持）
find_package(Threads REQUIRED)
target_link_libraries(object_storage Threads::Threads)
