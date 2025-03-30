#include "Sqlconnpool.hpp"

#include <iostream>
#include <vector>
#include <thread>
#include <chrono>

using namespace std;
using namespace bre;
const std::string DB_URL = "192.168.37.132";
const std::string USER = "breeze";
const std::string PASS = "abc123";

//测试用的数据库连接信息
void insertData(bre::MySqlPool& pool,int id) {
    std::unique_ptr<sql::Connection> conn = pool.GetConn();
    std::unique_ptr<sql::Statement> stmt(conn->createStatement());

    for (int i = 0; i < 10; ++i) {
        stmt->execute("INSERT INTO tmp (id, name) VALUES (" + std::to_string(i*id) + ", 'Test Name')");
        std::this_thread::sleep_for(std::chrono::milliseconds(1)); // 模拟耗时操作
    }

    pool.FreeConn(std::move(conn));
}
void queryData(bre::MySqlPool& pool) {
    std::unique_ptr<sql::Connection> conn = pool.GetConn();
    std::unique_ptr<sql::Statement> stmt(conn->createStatement());
    std::unique_ptr<sql::ResultSet> res(stmt->executeQuery("SELECT * FROM tmp"));

    while (res->next()) {
        std::cout << "Found record with ID: " << res->getInt("id")
            << " and Name: " << res->getString("name") << std::endl;
    }

    pool.FreeConn(std::move(conn));
}
void deleteData(bre::MySqlPool& pool) {
    std::unique_ptr<sql::Connection> conn = pool.GetConn();
    std::unique_ptr<sql::Statement> stmt(conn->createStatement());

    for (int i = 0; i < 10; ++i) {
        stmt->execute("DELETE FROM tmp WHERE id=" + std::to_string(i));
        std::this_thread::sleep_for(std::chrono::milliseconds(100)); // 模拟耗时操作
    }

    pool.FreeConn(std::move(conn));
}

void testMySqlPool() {
    // 创建连接池实例
    auto& pool = bre::MySqlPool::Instance();

    // 创建线程
    std::vector<std::thread> insertThreads;
    std::vector<std::thread> queryThreads;
    std::vector<std::thread> deleteThread;

    // 创建两个插入线程
    for (int i = 1; i < 3; ++i) {
        insertThreads.push_back(thread{ [&]() {
            insertData(std::ref(pool), i);
            } });
    }
    std::cout << "insertThreads.size() = " << insertThreads.size() << std::endl;


    std::cout << "queryThreads.size() = " << queryThreads.size() << std::endl;
    // 创建一个删除线程
    deleteThread.emplace_back(deleteData, std::ref(pool));
	this_thread::sleep_for(1s);
    for (int i = 0; i < 1; ++i) {
        queryThreads.emplace_back(queryData, std::ref(pool));
    }
    // 等待所有线程完成
    for (auto& t : insertThreads) {
        t.join();
    }

    for (auto& t : queryThreads) {
        t.join();
    }

    deleteThread.front().join();

    // 关闭连接池
    pool.Close();
}

void sampleTest() {
    auto& pool = bre::MySqlPool::Instance();

    std::unique_ptr<sql::Connection> conn = pool.GetConn();
    std::unique_ptr<sql::Statement> stmt(conn->createStatement());

    for (int i = 0; i < 10; ++i) {
        stmt->execute("INSERT INTO tmp (id, name) VALUES (" + std::to_string(i) + ", 'Test Name')");
        std::this_thread::sleep_for(std::chrono::milliseconds(1)); // 模拟耗时操作

    }

    pool.FreeConn(std::move(conn));
	pool.Close();
    std::cout << "insert data finished!" << std::endl;
    this_thread::sleep_for(1s);
}




int main() {
    std::cout << "================== test sqlpool! ==============" << std::endl;
    try {
        testMySqlPool();
         //sampleTest();
    }
    catch (const std::exception& e) {
        std::cerr << "Exception caught: " << e.what() << std::endl;
        return 1;
    }
    std::cout << "================== test sqlpool end! ==============" << std::endl;
    return 0;
}
