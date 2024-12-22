# 定义变量
CXX = g++
CXXFLAGS = -std=c++20 -Wall -Wextra
TARGET = test_heapTimer
SRC = testHeapTimer.cpp
OBJ = $(SRC:.cpp=.o)

# 默认目标
all: $(TARGET)

# 链接目标
$(TARGET): $(OBJ)
	$(CXX) $(CXXFLAGS) -o $@ $^

# 编译源文件
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# 清理目标
clean:
	rm -f $(OBJ) $(TARGET)

.PHONY: all clean
