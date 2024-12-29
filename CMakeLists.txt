cmake_minimum_required(VERSION 3.20)

project(breWebserver VERSION 1.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 设置输出路径
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/run)

# 使用仅头文件的好处是减少库和文件的依次添加
add_executable(Webserver src/main.cpp)

# 注意mysql-connector-c++文件路径，因为window安装使用jdbc，请根据自己的选项写入
# 网上很多资料的 mysql-connector-c++ 很老，路径很多不正确
target_include_directories(Webserver PRIVATE /usr/include/mysql-connector-c++)
find_package(Threads REQUIRED)

target_link_libraries(Webserver PRIVATE 
    -lmysqlcppconn
    Threads::Threads
)

# 复制con.txt到输出目录
file(COPY ${CMAKE_SOURCE_DIR}/src/config.txt DESTINATION ${CMAKE_SOURCE_DIR}/run)

# 复制src到输出目录
execute_process( COMMAND ${CMAKE_COMMAND} -E copy_directory 
        ${CMAKE_CURRENT_SOURCE_DIR}/src/resources 
        ${CMAKE_SOURCE_DIR}/run/resources)