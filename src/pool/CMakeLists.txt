cmake_minimum_required(VERSION 3.10)

project(testSqlconnpool VERSION 1.0 LANGUAGES CXX)

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_EXTENSIONS False)



# 查找 MySQL Connector/C++ 库
set(MYSQL_INCLUDE_DIR /usr/include)
set(MYSQL_LIBRARY /usr/lib/x86_64-linux-gnu/libmysqlcppconn.so)

set(SOURCE 
    testSqlconnpool.cpp
)

# 添加源文件
add_executable(${PROJECT_NAME} ${SOURCE})

target_compile_options(${PROJECT_NAME} PRIVATE "-Wall" "-Wextra")

target_include_directories(${PROJECT_NAME}
    PRIVATE
        ${MYSQL_INCLUDE_DIR}
)

# 链接 MySQL Connector/C++ 库
target_link_libraries(${PROJECT_NAME}
    PUBLIC
        ${MYSQL_LIBRARY}
)
