#include "server/WebServer.hpp"
#include "config/Config.hpp"
#include "log/Log.hpp"
#include "http/HttpConn.hpp"

int main() {
    /* 守护进程 后台运行 */
    //daemon(1, 0); 
    bre::WebServer server;
    server.Start();
    return 0;
} 