#include "HttpRequest.hpp"

using namespace bre;
using namespace std;


void testFuncPrase() {
    Buffer buffer;
    HttpRequest request;
    string str = "GET / HTTP/1.1\r\n"
                    "Host: www.baidu.com\r\n"
                    "Connection: keep-alive\r\n"
                    "Cache-Control: max-age=0\r\n"
                    "Upgrade-Insecure-Requests: 1\r\n"
                    "User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/58.0.3029.110 Safari/537.36 Edge/16.16299\r\n"
                    "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,image/apng,*/*;q=0.8\r\n"
                    "Accept-Encoding: gzip, deflate, br\r\n"
                    "Accept-Language: zh-CN,zh;q=0.9\r\n"
                    "Content-Type: application/x-www-form-urlencoded\r\n"
                    "Content-Length: 23\r\n"
                    "key1=value1&key2=value2\r\n"
                    "\r\n";
    buffer.Append(str);
    request.Parse(buffer);

    cout << "header size: " << request.header.size() << "\n";
    for(auto it = request.header.begin(); it != request.header.end(); it++) {
        cout << it->first << " : " << it->second << "\n";
    }
    cout << "\n\n";
}

void testParseFromUrlencoded() {
HttpRequest request;

// 测试数据
std::vector<std::pair<std::string, std::unordered_map<std::string, std::string>>> testData = {
{"", {}},
{"key1=value1", {{"key1", "value1"}}},
{"key1=value1&key2=value2", {{"key1", "value1"}, {"key2", "value2"}}},
{"key1=value1", {{"key1", "value1"}}},
{"key1=%20value1", {{"key1", "value1"}}},
{"key1=%20value1&key2=+value2", {{"key1", "value1"}, {"key2", "value2"}}},
{"key1=%E4%B8%AD%E6%96%87", {{"key1", "中文"}}},
{"key1=%22value1%2Cvalue2%22", {{"key1", "\"value1,value2\""}}},
{"key1=value1&key1=value2", {{"key1", "value2"}}}, // 只保留最后一个值
{"key1=value%20with%20space", {{"key1", "value with space"}}},
{"key1=%E4%B8%AD%E6%96%87%E5%AD%97%E7%AC%A6", {{"key1", "中文字符"}}},
{"key1=", {{"key1", ""}}}, // 没有值
{"key1=verylongvalue1234567890verylongvalue1234567890verylongvalue", {{"key1", "verylongvalue1234567890verylongvalue1234567890verylongvalue"}}}, // 非常长的值
};

for (const auto& [input, expectedOutput] : testData) {
    request.body = input;
    request.parseFromUrlencoded();
    // 验证结果
    bool isCorrect = true;
    for (const auto& [expectedKey, expectedValue] : expectedOutput) {
        if (request.post.count(expectedKey) == 0 || request.post[expectedKey] != expectedValue) {
            isCorrect = false;
            break;
        }
    }

    if (isCorrect) {
        std::cout << "Test passed for input: " << input << std::endl;
    } else {
        std::cout << "Test failed for input: " << input << std::endl;
    }
}
}

void testuserVerify() {
std::cout << "Test userVerify" << std::endl;
HttpRequest request;
cout << request.userVerify("admin", "admin", false) << "\n";
cout << request.userVerify("admin", "admin", true) << "\n";
cout << request.userVerify("test", "123456", true) << "\n";
cout << request.userVerify("test", "666666", true) << "\n";
}


void testParseRequestLine() {
    HttpRequest request;
    std::cout << request.parseRequestLine("GET / HTTP/1.1") << "\n";
    std::cout << request.method << "\n";
    std::cout << request.path << "\n";
    std::cout << request.version << "\n";

}

void testParseHeader() {
    std::cout << "Test parseHeader" << std::endl;
    HttpRequest request;
    request.parseHeader("Host: www.baidu.com");
    cout << request.header.size() << "\n";
    cout << request.header["Host"] << "\n";
}

void testParseBody() {
    HttpRequest request;
    request.parseBody("key1=value1&key2=value2");
    cout << request.body << "\n";

}

void testParsePath() {
    HttpRequest request;
    request.path = "/";
    request.parsePath();
    cout << request.path << "\n";

    request.path = "/login";
    request.parsePath();
    cout << request.path << "\n";
}

void testParsePost() {
    bre::HttpRequest request;
    request.method = "POST";
    request.header["Content-Type"] = "application/x-www-form-urlencoded";
    request.body = "username=user&password=pass&key1=value1&key2=value2";
    request.parsePost();
    std::cout << request.post.size() << "\n";
    std::cout << request.post["key1"] << "\n";
    std::cout << request.post["key2"] << "\n";
}


int main() {
    testFuncPrase();
    // testParseFromUrlencoded();
    // testuserVerify();
    // testParseRequestLine();
    // testParseHeader();
    // testParseBody();
    // testParsePath();
    // testParsePost();


return 0;
}