#include "user_manager.h"
#include "database.h"
#include "common.h"
#include <iostream>
#include <random>
#include <algorithm>

UserManager::UserManager(Database* db) : db(db) {
}

UserManager::~UserManager() {
}

std::string UserManager::register_user(const std::string& body) {
    std::string username = parse_json_value(body, "username");
    std::string password = parse_json_value(body, "password");
    
    if (username.empty() || password.empty()) {
        return create_json_response("error", "用户名和密码不能为空");
    }
    
    if (db->user_exists(username)) {
        return create_json_response("error", "用户名已存在");
    }
    
    if (db->create_user(username, password)) {
        return create_json_response("success", "注册成功");
    } else {
        return create_json_response("error", "注册失败");
    }
}

std::string UserManager::login_user(const std::string& body, int client_fd) {
    std::string username = parse_json_value(body, "username");
    std::string password = parse_json_value(body, "password");
    
    if (username.empty() || password.empty()) {
        return create_json_response("error", "用户名和密码不能为空");
    }
    
    User user;
    if (!db->verify_user(username, password, user)) {
        return create_json_response("error", "用户名或密码错误");
    }
    
    std::lock_guard<std::mutex> lock(users_mutex);
    
    // 更新用户信息
    user.online = true;
    user.socket_fd = client_fd;
    // 不再需要token，清空token字段
    user.token = "";
    online_users[user.user_id] = user;
    
    // 返回登录成功的用户信息（不包含token）
    std::string data = "{\"user_id\":" + std::to_string(user.user_id) + 
                       ",\"username\":\"" + user.username + "\"}";
    
    return create_json_response("success", data);
}

std::string UserManager::logout_user(const std::string& body, int client_fd) {
    (void)client_fd;  // 参数未使用，保留为了接口兼容性
    std::string username = parse_json_value(body, "username");
    
    if (username.empty()) {
        return create_json_response("error", "用户名不能为空");
    }
    
    int user_id = get_user_id_by_username(username);
    if (user_id == -1) {
        return create_json_response("error", "无效的用户名");
    }
    
    std::lock_guard<std::mutex> lock(users_mutex);
    
    if (online_users.find(user_id) != online_users.end()) {
        online_users[user_id].online = false;
        online_users.erase(user_id);
    }
    
    return create_json_response("success", "登出成功");
}

int UserManager::get_user_id_by_token(const std::string& token) {
    std::lock_guard<std::mutex> lock(users_mutex);
    
    auto it = token_to_user_id.find(token);
    if (it != token_to_user_id.end()) {
        return it->second;
    }
    
    return -1;
}

int UserManager::get_user_id_by_fd(int client_fd) {
    std::lock_guard<std::mutex> lock(users_mutex);
    
    for (const auto& pair : online_users) {
        if (pair.second.socket_fd == client_fd) {
            return pair.first;
        }
    }
    
    return -1;
}

std::string UserManager::generate_token() {
    static const char alphanum[] =
        "0123456789"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz";
        
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, sizeof(alphanum) - 2);
    
    std::string token;
    token.reserve(32);
    for (int i = 0; i < 32; ++i) {
        token += alphanum[dis(gen)];
    }
    
    return token;
}

bool UserManager::is_valid_token(const std::string& token) {
    return get_user_id_by_token(token) != -1;
}

std::unordered_map<int, User>& UserManager::get_online_users() {
    return online_users;
}

std::mutex& UserManager::get_users_mutex() {
    return users_mutex;
}

// 新增：获取用户信息API实现
std::string UserManager::get_user_profile(const std::string& body) {
    std::string username = parse_json_value(body, "username");
    if (username.empty()) {
        return create_json_response("error", "用户名不能为空");
    }
    
    int user_id = get_user_id_by_username(username);
    if (user_id == -1) {
        return create_json_response("error", "无效的用户名");
    }
    
    std::lock_guard<std::mutex> lock(users_mutex);
    auto it = online_users.find(user_id);
    if (it != online_users.end()) {
        const User& user = it->second;
        std::string data = "{\"user_id\":" + std::to_string(user.user_id) + 
                          ",\"username\":\"" + user.username + "\"}";
        return create_json_response("success", data);
    }
    
    return create_json_response("error", "用户不在线");
}

std::string UserManager::get_username_by_id(int user_id) {
    // 首先检查在线用户
    std::lock_guard<std::mutex> lock(users_mutex);
    auto it = online_users.find(user_id);
    if (it != online_users.end()) {
        return it->second.username;
    }
    
    // 如果不在线，从数据库查询
    return db->get_username_by_id(user_id);
}

int UserManager::get_user_id_by_username(const std::string& username) {
    std::lock_guard<std::mutex> lock(users_mutex);
    
    for (const auto& pair : online_users) {
        if (pair.second.username == username && pair.second.online) {
            return pair.first;
        }
    }
    
    return -1;
}
