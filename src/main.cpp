#include "server.h"
#include <iostream>
#include <signal.h>

Server* g_server = nullptr;

void signal_handler(int signal) {
    if (g_server) {
        std::cout << "\n正在关闭服务器..." << std::endl;
        delete g_server;
        g_server = nullptr;
    }
    exit(0);
}

int main(int argc, char* argv[]) {
    int port = 8080;
    
    if (argc > 1) {
        port = std::atoi(argv[1]);
    }
    
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    try {
        g_server = new Server(port);
        std::cout << "服务器启动在端口: " << port << std::endl;
        g_server->run();
    } catch (const std::exception& e) {
        std::cerr << "服务器错误: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
