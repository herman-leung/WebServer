
#ifndef HTTP_RESPONSE_HPP
#define HTTP_RESPONSE_HPP

#include "../buffer/Buffer.hpp"
#include "../mylog/Log.hpp"

#include <unordered_map>
#include <fcntl.h>    // open
#include <unistd.h>   // close
#include <sys/stat.h> // stat
#include <sys/mman.h> // mmap, munmap
#include <string>
#include <stdexcept>
#include <filesystem>

namespace bre
{
    using std::string;
    using std::unordered_map;
    using namespace std::filesystem;

    class HttpResponse
    {
    public:
        HttpResponse()
        {
            code = -1;
            path = srcDir = "";
            isKeepAlive = false;
            mmFile = nullptr;
            mmFileStat = {};
        }
        ~HttpResponse()
        {
            UnmapFile();
        }

        void Init(const std::string &srcDir, std::string &path, bool isKeepAlive = false, int code = -1)
        {
            if (srcDir.empty())
            {
                throw std::invalid_argument("srcDir is empty");
            }
            if (mmFile)
            {
                UnmapFile();
            }
            this->isKeepAlive = isKeepAlive;
            this->path = path;
            this->srcDir = srcDir;
            this->code = code;
            mmFile = nullptr;
            mmFileStat = {};
        }

        void MakeResponse(Buffer &buff)
        {
            if (stat((srcDir + path).data(), &mmFileStat) < 0 || S_ISDIR(mmFileStat.st_mode))
            {
                code = 404;
            }
            else if (!(mmFileStat.st_mode & S_IROTH))
            {
                code = 403;
            }
            else if (code == -1)
            {
                code = 200;
            }
            errorHtml();
            addStateLine(buff);
            addHeader(buff);
            addContent(buff);
        }

        void UnmapFile()
        {
            if (mmFile)
            {
                munmap(mmFile, mmFileStat.st_size);
                mmFile = nullptr;
            }
        }

        char *File()
        {
            return mmFile;
        }

        size_t FileLen() const
        {
            return mmFileStat.st_size;
        }

        void ErrorContent(Buffer &buff, std::string message)
        {
            string body;
            string status;
            body += "<html><title>Error</title>";
            body += "<body bgcolor=\"fff000\">";
            if (codePath.count(code) == 1)
            {
                status = codeStatus.find(code)->second;
                body += std::to_string(code) + " : " + status;
            }
            else
            {
                status = "Bad Request";
                body += "400 : Bad Request";
            }
            body += "<p>" + message + "</p>";
            body += "<hr><em>WebServer</em></body></html>";
            buff.Append("Content-length: " + std::to_string(body.size()) + "\r\n\r\n");
            buff.Append(body);
        }

        int Code() const { return code; }

        // private:
        void addStateLine(Buffer &buff)
        {
            string status;
            if (codeStatus.count(code) == 1)
            {
                status = codeStatus.find(code)->second;
            }
            else
            {
                code = 400;
                status = codeStatus.find(400)->second;
            }
            buff.Append("HTTP/1.1 " + std::to_string(code) + " " + status + "\r\n");
        }

        void addHeader(Buffer &buff)
        {
            buff.Append("Connection: ");
            if (isKeepAlive)
            {
                buff.Append("keep-alive\r\n");
                buff.Append("keep-alive: max=6, timeout=200\r\n");
            }
            else
            {
                buff.Append("close\r\n");
            }
            buff.Append("Content-type: " + getFileType() + "\r\n");
        }

        void addContent(Buffer &buff)
        {
            int srcFd = open((srcDir + path).data(), O_RDONLY);

            if (srcFd < 0)
            {
                ErrorContent(buff, "File NotFound!");
                return;
            }

            Log::debug("file path {}", (srcDir + path).data());

            // 使用mmap将文件映射到内存
            int *mmRet = (int *)mmap(0, mmFileStat.st_size, PROT_READ, MAP_PRIVATE, srcFd, 0);
            if (*mmRet == -1)
            {
                ErrorContent(buff, "File NotFound!");
                return;
            }
            mmFile = (char *)mmRet;
            close(srcFd);
            buff.Append("Content-length: " + std::to_string(mmFileStat.st_size) + "\r\n\r\n");
            buff.Append(mmFile, mmFileStat.st_size);
        }

        void errorHtml()
        {
            if (codePath.count(code) == 1)
            {
                path = codePath.find(code)->second;
                stat((srcDir + path).data(), &mmFileStat);
            }
        }

        std::string getFileType()
        {
            // 文件后缀
            std::string::size_type idx = path.find_last_of('.');
            if (idx == std::string::npos)
            {
                return "text/plain";
            }
            std::string suffix = path.substr(idx);
            if (suffixType.count(suffix) == 1)
            {
                return suffixType.find(suffix)->second;
            }
            return "text/plain";
        }

        int code;
        bool isKeepAlive = false;

        std::string path;
        std::string srcDir;

        char *mmFile = nullptr;
        struct stat mmFileStat{};

        static const std::unordered_map<std::string, std::string> suffixType;
        static const std::unordered_map<int, std::string> codeStatus;
        static const std::unordered_map<int, std::string> codePath;
    };

    const std::unordered_map<std::string, std::string> HttpResponse::suffixType{
        {".html", "text/html"},
        {".xml", "text/xml"},
        {".xhtml", "application/xhtml+xml"},
        {".html", "text/html"},
        {".xml", "text/xml"},
        {".xhtml", "application/xhtml+xml"},
        {".txt", "text/plain"},
        {".rtf", "application/rtf"},
        {".pdf", "application/pdf"},
        {".word", "application/nsword"},
        {".png", "image/png"},
        {".gif", "image/gif"},
        {".jpg", "image/jpeg"},
        {".jpeg", "image/jpeg"},
        {".au", "audio/basic"},
        {".mpeg", "video/mpeg"},
        {".mpg", "video/mpeg"},
        {".avi", "video/x-msvideo"},
        {".gz", "application/x-gzip"},
        {".tar", "application/x-tar"},
        {".css", "text/css "},
        {".js", "text/javascript "},
    };

    const unordered_map<int, string> HttpResponse::codeStatus = {
        {200, "OK"},
        {400, "Bad Request"},
        {403, "Forbidden"},
        {404, "Not Found"},
    };

    const unordered_map<int, string> HttpResponse::codePath = {
        {400, "/400.html"},
        {403, "/403.html"},
        {404, "/404.html"},
    };

} // namespace bre
#endif // HTTP_RESPONSE_HPP