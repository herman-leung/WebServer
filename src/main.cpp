#include "server/WebServer.hpp"


int main() {
    /* 守护进程 后台运行 切断控制台 */
    // daemon(1, 0); 
    bre::WebServer server;
    server.Start();
    return 0;
} 