# 定义变量
CXX = g++
CXXFLAGS = -std=c++20 -Wall -Wextra
TARGET = testSqlconnpool
SRC = testSqlconnpool.cpp
OBJ = $(SRC:.cpp=.o)

# 默认目标
all: $(TARGET)

# 链接目标
$(TARGET): $(OBJ)
	$(CXX) $(CXXFLAGS) -o $@ $^ -L/usr/lib/mysql-connector-c++ -lmysqlcppconn

# 编译源文件
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# 清理目标
clean:
	rm -f $(OBJ) $(TARGET)

.PHONY: all clean