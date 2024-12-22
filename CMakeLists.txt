cmake_minimum_required(VERSION 3.20)

project(breWebserver VERSION 1.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

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