#ifndef SERVER_H
#define SERVER_H

#include <string>
#include <memory>
#include <thread>
#include "common.h"
#include "user_manager.h"
#include "message_service.h"
#include "forum_service.h"
#include "file_manager.h"

// 前向声明
class Database;

class Server {
public:
    Server(int port);
    ~Server();
    
    void run();
    
private:
    int server_fd;
    int port;
    
    // 数据库
    std::unique_ptr<Database> db;
    
    // 功能模块
    std::unique_ptr<UserManager> user_manager;
    std::unique_ptr<MessageService> message_service;
    std::unique_ptr<ForumService> forum_service;
    std::unique_ptr<FileManager> file_manager;
    
    // 核心服务器功能
    void setup_server();
    void handle_client(int client_fd);
    std::string handle_request(const std::string& request, int client_fd);
    std::string extract_token_from_request(const std::string& request);
    std::string parse_query_param(const std::string& query_string, const std::string& key);
};

#endif // SERVER_H
