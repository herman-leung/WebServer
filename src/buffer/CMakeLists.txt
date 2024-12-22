cmake_minimum_required(VERSION 3.10)

# 设置项目名称和版本号
project(TestBuffer VERSION 1.0)

# 指定输出目录为 build 目录，并确保构建文件不会污染源码树
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR})

# 添加可执行文件，并指定源文件
add_executable(${PROJECT_NAME} testBuffer.cpp)


