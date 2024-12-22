#include "HttpResponse.hpp"
#include "../buffer/Buffer.hpp"
#include "../log/Log.hpp"
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <iostream>
#include <cassert>

using namespace bre;
using namespace std;


void test_init_and_destruct() {
    Buffer buff;
    HttpResponse resp;
    try {
        string path = "/index.html";
        resp.Init(".", path);
        assert(resp.path == "/index.html");
        assert(resp.srcDir == ".");
        assert(!resp.mmFile); // 没有调用MakeResponse前，mmFile应为nullptr
    } catch (...) {
        assert(false);
    }
    resp.MakeResponse(buff);
    assert(resp.mmFile);

    string content = buff.RetrieveAll();
    cout << content << endl;

    std::cout << "Test init and destruct success!" << std::endl;
}

void test_add_state_line() {
    Buffer buff;
    HttpResponse resp;
    string path = "/test.html";
    resp.Init(".", path, true, 200);
    resp.addStateLine(buff);
    string content = buff.RetrieveAll();
    cout << content << endl;
    assert(content == "HTTP/1.1 200 OK\r\n");

    resp.code = 404;
    resp.addStateLine(buff);
    content = buff.RetrieveAll();
    cout << content << endl;
    assert(content == "HTTP/1.1 404 Not Found\r\n");
}

void test_add_header() {
    Buffer buff;
    HttpResponse resp;
    string path = "/test.html";
    resp.Init(".", path, true, 200);
    resp.addHeader(buff);
    string content = buff.RetrieveAll();
    cout << content << endl;
    assert(content.find("Connection: keep-alive") != std::string::npos);
    resp.isKeepAlive = false;
    resp.addHeader(buff);
    content = buff.RetrieveAll();
    cout  << content << endl;
    assert(content.find("Connection: close") != std::string::npos);
    std::cout << "Test add header success!" << std::endl;
}

void test_get_file_type() {
    HttpResponse resp;
    resp.path = "/test.html";
    assert(resp.getFileType() == "text/html");
    resp.path = "/test.txt";
    assert(resp.getFileType() == "text/plain");
    resp.path = "/unknown";
    assert(resp.getFileType() == "text/plain");

    std::cout << "Test get file type success!" << std::endl;
}

void test_error_content() {
    Buffer buff;
    HttpResponse resp;
    resp.ErrorContent(buff, "File NotFound!");
    std::string content = buff.RetrieveAll();
    cout << content << endl;
    assert(content.find("<html><title>Error</title>") != std::string::npos);
    assert(content.find("<p>File NotFound!</p>") != std::string::npos);

    std::cout << "Test error content success!" << std::endl;
}

int main() {
    test_init_and_destruct();
    test_add_state_line();
    test_add_header();
    test_get_file_type();
    test_error_content();
    return 0;
}