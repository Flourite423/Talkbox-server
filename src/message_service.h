#ifndef MESSAGE_SERVICE_H
#define MESSAGE_SERVICE_H

#include "common.h"
#include <string>
#include <vector>
#include <memory>

// 前向声明
class Database;
class UserManager;

class MessageService {
public:
    MessageService(Database* db, UserManager* user_manager);
    ~MessageService();
    
    // 消息处理API
    std::string send_message(const std::string& body, const std::string& token);
    std::string get_messages(const std::string& token);
    std::string get_contacts(const std::string& token);
    
    // 群组消息API
    std::string create_group(const std::string& body, const std::string& token);
    std::string join_group(const std::string& body, const std::string& token);
    std::string leave_group(const std::string& body, const std::string& token);
    std::string get_groups(const std::string& token);
    std::string get_group_messages(const std::string& body, const std::string& token);
    
private:
    Database* db;
    UserManager* user_manager;
    
    // 工具函数
    void broadcast_message(const Message& message);
};

#endif // MESSAGE_SERVICE_H
