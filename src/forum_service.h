#ifndef FORUM_SERVICE_H
#define FORUM_SERVICE_H

#include "common.h"
#include <string>
#include <vector>
#include <memory>

// 前向声明
class Database;
class UserManager;

class ForumService {
public:
    ForumService(Database* db, UserManager* user_manager);
    ~ForumService();
    
    // 论坛API
    std::string create_post(const std::string& body, const std::string& token);
    std::string get_posts(const std::string& body);
    std::string reply_post(const std::string& body, const std::string& token);
    std::string get_post_replies(const std::string& body);
    
private:
    Database* db;
    UserManager* user_manager;
};

#endif // FORUM_SERVICE_H
