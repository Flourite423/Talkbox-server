#ifndef USER_MANAGER_H
#define USER_MANAGER_H

#include "common.h"
#include <string>
#include <unordered_map>
#include <mutex>
#include <memory>

// 前向声明
class Database;

class UserManager {
public:
    UserManager(Database* db);
    ~UserManager();
    
    // 用户管理API
    std::string register_user(const std::string& body);
    std::string login_user(const std::string& body, int client_fd);
    std::string logout_user(const std::string& token, int client_fd);
    
    // 新增：获取用户信息API
    std::string get_user_profile(const std::string& token);
    std::string get_username_by_id(int user_id);
    
    // 工具函数
    int get_user_id_by_token(const std::string& token);
    int get_user_id_by_fd(int client_fd);
    std::string generate_token();
    bool is_valid_token(const std::string& token);
    
    // 获取用户信息
    std::unordered_map<int, User>& get_online_users();
    std::mutex& get_users_mutex();
    
private:
    Database* db;
    std::unordered_map<int, User> online_users;
    std::unordered_map<std::string, int> token_to_user_id;  // token到用户ID的映射
    std::mutex users_mutex;
};

#endif // USER_MANAGER_H
