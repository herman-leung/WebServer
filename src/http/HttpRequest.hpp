
#ifndef HTTP_REQUEST_H
#define HTTP_REQUEST_H

#include "../buffer/Buffer.hpp"
#include "../mylog/Log.hpp"
#include "../pool/Sqlconnpool.hpp"

// #include <mysql/jdbc.h>
#include <mysql_driver.h>

#include <unordered_map>
#include <unordered_set>
#include <string>
#include <string_view>
#include <regex>
// #include <prepared_statement>

namespace bre
{

    // GET /index.html HTTP/1.1
    // Host: example.com
    // User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/91.0.4472.124 Safari/537.36
    // Accept: text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,*/*;q=0.8
    // Accept-Language: en-US,en;q=0.5
    // Accept-Encoding: gzip, deflate, br
    // Connection: keep-alive

    // GET /images/6.jpg HTTP/1.1
    // Host: 127.0.0.1:5678
    // Connection: keep-alive
    // sec-ch-ua-platform: "Windows"
    // User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/131.0.0.0 Safari/537.36 Edg/131.0.0.0
    // sec-ch-ua: "Microsoft Edge";v="131", "Chromium";v="131", "Not_A Brand";v="24"
    // DNT: 1
    // sec-ch-ua-mobile: ?0
    // Accept: image/avif,image/webp,image/apng,image/svg+xml,image/*,*/*;q=0.8
    // Sec-Fetch-Site: same-origin
    // Sec-Fetch-Mode: no-cors
    // Sec-Fetch-Dest: image
    // Referer: http://127.0.0.1:5678/index.html
    // Accept-Encoding: gzip, deflate, br, zstd
    // Accept-Language: zh-CN,zh;q=0.9,en;q=0.8,en-GB;q=0.7,en-US;q=0.6

    class HttpRequest
    {
    public:
        enum class ParseState
        {
            RequestLine,
            Headers,
            Body,
            Finish
        };

        enum class HttpStatusCode
        {
            NoRequest,
            GetRequest,
            BadRequest,
            NoResource,
            ForbiddenRequest,
            FileRequest,
            InternalError,
            ClosedConnection
        };

        HttpRequest() { Init(); }
        ~HttpRequest() = default;

        void Init()
        {
            method = path = version = body = "";
            state = ParseState::RequestLine;
            header.clear();
            post.clear();
        }

        bool Parse(Buffer &buff)
        {
            const char CRLF[] = "\r\n";
            if (buff.ReadableBytes() < 1)
            {
                return false;
            }

            while (buff.ReadableBytes() && state != ParseState::Finish)
            {
                std::string line = buff.RetrieveUntil(CRLF);
                line = line.substr(0, line.size() - 2);

                switch (state)
                {
                case ParseState::RequestLine:
                    if (!parseRequestLine(line))
                    {
                        return false;
                    }
                    parsePath();
                    break;
                case ParseState::Headers:
                    parseHeader(line);
                    break;
                case ParseState::Body:
                    parseBody(line);
                    break;
                default:
                    break;
                }
                if (buff.ReadableBytes() < 1)
                {
                    std::cout << "buff.ReadableBytes() < 1" << std::endl;
                    break;
                }
            }
            Log::debug("method = {}, path = {}, version = {}",
                       method, path, version);
            return true;
        }

        std::string Path() const
        {
            return path;
        }
        std::string &Path()
        {
            return path;
        }
        std::string Method() const
        {
            return method;
        }
        std::string Version() const
        {
            return version;
        }
        std::string GetPost(const std::string &key) const
        {
            if (post.empty())
            {
                return "";
            }
            return post.find(key) == post.end() ? "" : post.at(key);
        }

        bool IsKeepAlive() const
        {
            if (header.count("Connection") == 1)
            {
                return header.find("Connection")->second == "keep-alive" && version == "1.1";
            }
            return false;
        }

        // private:

        bool parseRequestLine(const std::string &line)
        {
            // GET / HTTP/1.1
            //  ([^ ]*)  匹配任意不是空格的字符
            // ^ 表示匹配字符串的开始 $ 表示匹配字符串的结束

            std::regex patten("^([^ ]*) ([^ ]*) HTTP/([^ ]*)$");
            std::smatch subMatch;
            if (std::regex_match(line, subMatch, patten))
            {
                method = subMatch[1];
                path = subMatch[2];
                version = subMatch[3];
                state = ParseState::Headers;
                return true;
            }
            Log::err("RequestLine Error");
            return false;
        }

        void parseHeader(const std::string &line)
        {

            // Host: www.baidu.com
            // Accept: text/html等
            std::regex patten("^([^:]*): ?(.*)$");
            std::smatch subMatch;
            if (std::regex_match(line, subMatch, patten))
            {
                header[subMatch[1]] = subMatch[2];
                std::cout << "header[" << subMatch[1] << "] = " << subMatch[2] << std::endl;
            }
            else
            {
                std::cout << "change " << line << std::endl;
                state = ParseState::Body;
            }
        }

        void parseBody(const std::string &line)
        {
            body = line;
            parsePost();
            state = ParseState::Finish;
            Log::debug("Body = {}", body);
        }

        void parsePath()
        {
            // 设置默认主页
            if (path == "/")
            {
                path = "/index.html";
            }
            else
            {
                for (auto &item : defaultHtml)
                {
                    if (item == path)
                    {
                        path += ".html";
                        break;
                    }
                }
            }
        }

        void parsePost()
        {
            if (method != "POST")
            {
                return;
            }
            std::cout << "header[Content-Type]" << header["Content-Type"] << std::endl;
            // 登录注册
            if (method == "POST" && header["Content-Type"] == "application/x-www-form-urlencoded")
            {
                parseFromUrlencoded();
                Log::debug("username = {}, password = {}", post["username"], post["password"]);
                if (defaultHtmlTag.count(path))
                {
                    int tag = defaultHtmlTag.find(path)->second;
                    if (tag == 0 || tag == 1)
                    {
                        bool isLogin = tag;
                        if (userVerify(post["username"], post["password"], isLogin))
                        {
                            path = "/welcome.html";
                        }
                        else
                        {
                            path = "/error.html";
                        }
                    }
                }
            }
            // 其他请求
        }

        // 解析Url
        void parseFromUrlencoded()
        {
            if (body.empty())
            {
                return;
            }

            std::istringstream iss(body);
            std::string key, value, token;

            while (std::getline(iss, token, '&'))
            { // 分割字符串为键值对
                size_t equalPos = token.find('=');
                if (equalPos != std::string::npos)
                {
                    key = decodePercentEncoding(token.substr(0, equalPos));    // 解码键
                    value = decodePercentEncoding(token.substr(equalPos + 1)); // 解码值
                    // 去除加号
                    removePlus(key);
                    removePlus(value);
                    // 去除前后空格
                    trim(key);
                    trim(value);
                    // 将解码后的键值对存入 map
                    post[key] = value;
                    Log::debug("{} = {}", key, value);
                }
            }
        }
        void trim(std::string &str)
        {
            if (str.empty())
            {
                return;
            }
            str.erase(0, str.find_first_not_of(" "));
            str.erase(str.find_last_not_of(" ") + 1);
        }
        void removePlus(std::string &str)
        {
            auto new_end = std::remove(str.begin(), str.end(), '+');
            str.erase(new_end, str.end());
        }
        std::string decodePercentEncoding(const std::string &encodedStr)
        {
            std::string decodedStr;
            for (size_t i = 0; i < encodedStr.length(); ++i)
            {
                if (encodedStr[i] == '%' && i + 2 < encodedStr.length())
                {
                    int num = static_cast<int>(strtol(encodedStr.substr(i + 1, 2).c_str(), nullptr, 16));
                    decodedStr += static_cast<char>(num);
                    i += 2;
                }
                else
                {
                    decodedStr += encodedStr[i];
                }
            }
            return decodedStr;
        }

        // 用户验证
        bool userVerify(const std::string &name, const std::string &pwd, bool isLogin)
        {
            if (name.empty() || pwd.empty())
            {
                return false;
            }

            auto conn = MySqlPool::Instance().GetConn();
            if (conn == nullptr)
            {
                Log::err("No SQL connection in pool");
                return false;
            }

            bool flag = false;
            try
            {
                std::unique_ptr<sql::PreparedStatement> pstmt(conn->prepareStatement(
                    "SELECT username, password FROM user WHERE username = ? LIMIT 1"));

                pstmt->setString(1, name);
                std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());

                if (res->next())
                {
                    if (isLogin)
                    { // 登录
                        flag = res->getString("password") == pwd;
                    }
                    else
                    { // 注册
                        flag = false;
                    }
                }
                else
                {
                    if (!isLogin)
                    { // 注册
                        std::unique_ptr<sql::PreparedStatement> insertStmt(conn->prepareStatement(
                            "INSERT INTO user(username, password) VALUES(?, ?)"));

                        insertStmt->setString(1, name);
                        insertStmt->setString(2, pwd);
                        insertStmt->execute();
                        flag = true;
                    }
                }
            }
            catch (sql::SQLException &e)
            {
                Log::err("UserVerify error: {}", e.what());
                flag = false;
            }

            MySqlPool::Instance().FreeConn(std::move(conn));
            Log::info("UserVerify success!!");

            return flag;
        }

        ParseState state;
        std::string method, path, version, body;
        std::unordered_map<std::string, std::string> header;
        std::unordered_map<std::string, std::string> post;

        const std::unordered_set<std::string> defaultHtml{
            "/index", "/register", "/login",
            "/welcome", "/video", "/picture"};
        const std::unordered_map<std::string, int> defaultHtmlTag{
            {"/register.html", 0}, {"/login.html", 1}};
    };

} // namespace bre
#endif // HTTP_REQUEST_H

// ([^:]*): ([^ ]*) 匹配任意不是冒号的字符，冒号，空格，任意不是空格的字符
// std::regex patten("^([^:]*): ?(.*)$");
// //if (line.find('Content-Type') != std::string::npos)
// {
//     std::cout << "parseHeader: " << line << std::endl;
// }
// std::smatch subMatch;
// if (std::regex_match(line, subMatch, patten)) {
//     header[subMatch[1]] = subMatch[2];
// } else {
//     state = ParseState::Body;
// }