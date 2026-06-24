#ifndef COMMON_H
#define COMMON_H

#include <string>
#include <vector>

// 所有模块共享的数据结构
struct User {
    int user_id;
    std::string username;
    std::string password;
    bool online;
    int socket_fd;
    std::string token;
};

struct Group {
    int group_id;
    std::string group_name;
    std::string description;
    int creator_id;
    std::string created_time;
};

struct Message {
    int message_id;
    int sender_id;
    std::string sender_username;
    int receiver_id;  // 私聊时使用
    int group_id;     // 群聊时使用，-1表示私聊
    std::string content;
    std::string timestamp;
    std::string type;  // "text", "file"
};

struct Post {
    int post_id;
    int user_id;
    std::string username;
    std::string title;
    std::string content;
    std::string timestamp;
    std::vector<std::string> replies;
};

struct Reply {
    int reply_id;
    int post_id;
    int user_id;
    std::string username;
    std::string content;
    std::string timestamp;
};

// 共享的工具函数
std::string get_current_timestamp();
std::string parse_json_value(const std::string& json, const std::string& key);
std::string create_json_response(const std::string& status, const std::string& data = "");

// 安全的字符串转整数（防止 stoi 异常）
int safe_stoi(const std::string& s, int default_val = -1);

// JSON 字符串转义（防止 JSON 注入）
std::string escape_json_string(const std::string& s);

// 密码哈希函数 (PBKDF2-HMAC-SHA256)
std::string hash_password(const std::string& password);
bool verify_password(const std::string& password, const std::string& stored_hash);
#endif // COMMON_H
