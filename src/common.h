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
    int receiver_id;  // 私聊时使用
    int group_id;     // 群聊时使用，-1表示私聊
    std::string content;
    std::string timestamp;
    std::string type;  // "text", "file"
};

struct Post {
    int post_id;
    int user_id;
    std::string title;
    std::string content;
    std::string timestamp;
    std::vector<std::string> replies;
};

struct Reply {
    int reply_id;
    int post_id;
    int user_id;
    std::string content;
    std::string timestamp;
};

// 共享的工具函数
std::string get_current_timestamp();
std::string parse_json_value(const std::string& json, const std::string& key);
std::string create_json_response(const std::string& status, const std::string& data = "");

#endif // COMMON_H
