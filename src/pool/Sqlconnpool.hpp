#ifndef SQLCONNPOOL_H
#define SQLCONNPOOL_H

#include "../config/Config.hpp"
#include <mysql/jdbc.h>
#include <string>
#include <queue>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <chrono>
#include <memory>

namespace bre {
	using namespace std::chrono_literals;
	using namespace std::chrono;

	using std::unique_ptr;
	using std::string;

	class MySqlPool {
	public:
		static MySqlPool& Instance() {
			static MySqlPool instance;
			static std::once_flag flag;
			std::call_once(flag, [&]() {
				try {
					auto& config = Config::getInstance();

					std::string url = config.Get("DB_URL").value();
					std::string user = config.Get("USER").value();
					std::string pass = config.Get("PASS").value();	
					std::string schame = config.Get("SCHAME").value();
					int poolSize = std::stoi(config.Get("POOL_SIZE").value_or("8"));
					instance.Init(url, user, pass, schame, poolSize);
				} catch(const std::exception& e) {
					std::cerr << "sql read config has no value: " <<e.what() << '\n';
					throw;
				}
			});
			return instance;
		}

		MySqlPool() = default;

		void Init(const std::string& Url, const std::string& User, const std::string& Pass,
			const std::string& Schema, int PoolSize = 8) {
			url = Url;
			user = User;
			pass = Pass; 
			schema = Schema;
			poolSize = PoolSize;
			b_stop = false; 
			try {
				for (int i = 0; i < poolSize; ++i) {
					sql::mysql::MySQL_Driver* driver = sql::mysql::get_mysql_driver_instance();
					sql::Connection* con = driver->connect(url.c_str(), user.c_str(), pass.c_str());
					con->setSchema(schema);
					pool.emplace(con);
				}
			}
			catch (sql::SQLException& e) {
				std::cout << "sql::SQLException, error is " << e.what() << std::endl;
			}
			catch (std::exception& e) {
				std::cout << "mysql pool init failed, error is " << e.what() << std::endl;
			}
		}


		std::unique_ptr<sql::Connection> GetConn() {
			std::unique_lock<std::mutex> lock(mux);
			cond.wait(lock, [this] {
				if (b_stop) {
					return true;
				}
				return !pool.empty(); });
			if (b_stop) {
				return nullptr;
			}
			std::unique_ptr<sql::Connection> con(std::move(pool.front()));
			pool.pop();
			return con;
		}


		void FreeConn(std::unique_ptr<sql::Connection> con) {
			std::unique_lock<std::mutex> lock(mux);
			if (b_stop) {
				return;
			}
			pool.push(std::move(con));
			cond.notify_one();
		}


		void Close() {
			b_stop = true;
			cond.notify_all();
		}


		~MySqlPool() {
			std::unique_lock<std::mutex> lock(mux);
			if (!b_stop) {
				Close();
			}

			while (!pool.empty()) {
				pool.pop();
			}
		}
	private:
		void checkConnection() {
			std::lock_guard lock(mux);
			for (int i = 0; i < static_cast<int>(pool.size()); i++) {
				auto con = std::move(pool.front());
				pool.pop();

				try {
					std::cout << "checkConnection" << i << "\n";
					std::unique_ptr<sql::Statement> stmt(con->createStatement());
					stmt->executeQuery("SELECT 1");
				}
				catch (sql::SQLException& e) {
					con->close();
					std::cout << "Error keeping connection alive: " << e.what() << std::endl;
					// 重新创建连接并替换旧的连接
					sql::mysql::MySQL_Driver* driver = sql::mysql::get_mysql_driver_instance();
					auto* newcon = driver->connect(url, user, pass);
					newcon->setSchema(schema);
					con.reset(newcon);
				}
				pool.push(std::move(con));
			}
		}

	private:
		std::string url;
		std::string user;
		std::string pass;
		std::string schema;
		
		int poolSize;
		std::queue<std::unique_ptr<sql::Connection>> pool;
		std::mutex mux;
		std::condition_variable cond;
		std::atomic<bool> b_stop;
		// std::thread check_thread;
	};

} // namespace bre
#endif // SQLCONNPOOL_H