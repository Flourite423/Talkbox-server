#ifndef SERVER_H
#define SERVER_H

#include <string>
#include <unordered_map>
#include <vector>
#include <memory>
#include <thread>
#include <mutex>

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

class Database;

class Server {
public:
    Server(int port);
    ~Server();
    
    void run();
    
private:
    int server_fd;
    int port;
    std::unique_ptr<Database> db;
    std::unordered_map<int, User> online_users;
    std::unordered_map<std::string, int> token_to_user_id;  // token到用户ID的映射
    std::mutex users_mutex;
    
    void setup_server();
    void handle_client(int client_fd);
    std::string handle_request(const std::string& request, int client_fd);
    
    // HTTP请求处理
    std::string handle_register(const std::string& body);
    std::string handle_login(const std::string& body, int client_fd);
    std::string handle_logout(const std::string& token, int client_fd);
    std::string handle_send_message(const std::string& body, const std::string& token);
    std::string handle_get_messages(const std::string& token);
    std::string handle_create_post(const std::string& body, const std::string& token);
    std::string handle_get_posts(const std::string& body);
    std::string handle_reply_post(const std::string& body, const std::string& token);
    std::string handle_upload_file(const std::string& body, const std::string& token);
    std::string handle_download_file(const std::string& body);
    
    // 群组管理
    std::string handle_create_group(const std::string& body, const std::string& token);
    std::string handle_join_group(const std::string& body, const std::string& token);
    std::string handle_leave_group(const std::string& body, const std::string& token);
    std::string handle_get_groups(const std::string& token);
    std::string handle_get_group_messages(const std::string& body, const std::string& token);
    
    // 工具函数
    std::string parse_json_value(const std::string& json, const std::string& key);
    std::string create_json_response(const std::string& status, const std::string& data = "");
    std::string get_current_timestamp();
    void broadcast_message(const Message& message);
    int get_user_id_by_fd(int client_fd);
    int get_user_id_by_token(const std::string& token);
    std::string generate_token();
    std::string extract_token_from_request(const std::string& request);
};

#endif
