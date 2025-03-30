#ifndef CONFIG_HPP
#define CONFIG_HPP
#include <fstream>
#include <algorithm>
#include <map>
#include <memory>
#include <sstream>
#include <string>
#include <optional>
namespace bre
{

class Config {
public:
    // 获取单例实例
    [[nodiscard]] static Config& getInstance() {
        static std::unique_ptr<Config> instance(new Config());
        return *instance;
    }

    // 获取配置项的值
    std::optional<std::string> Get(const std::string& key) const {
        auto it = configMap.find(key);
        if (it != configMap.end()) {
            return it->second;
        }
        return std::nullopt;
    }

private:
    // 构造函数私有化
    Config() {
        loadConfig();
    }

    // 禁止拷贝构造函数和赋值运算符
    Config(const Config&) = delete;
    Config& operator=(const Config&) = delete;

    // 存储配置项的map
    std::map<std::string, std::string> configMap;

    // 加载配置文件
    void loadConfig() {
        std::ifstream configFile("config.txt");
        if (!configFile.is_open()) {
            throw std::runtime_error("Failed to open config file!!");
            return;
        }

        std::string line;
        while (std::getline(configFile, line)) {
            std::istringstream iss(line);
            std::string key, value;
            if (std::getline(iss, key, ':') && std::getline(iss, value)) {
                removeAllSpace(key);
                removeAllSpace(value);
                configMap[key] = value;
            }
        }
    }

    void removeAllSpace(std::string& str) {
        str.erase(std::remove_if(str.begin(), str.end(), ::isspace), str.end());
    }
};

} // namespace bre

#endif // CONFIG_HPP
