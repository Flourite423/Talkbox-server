#include "server.h"
#include "database.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <ctime>
#include <fstream>
#include <random>

Server::Server(int port) : port(port), server_fd(-1) {
    db = std::make_unique<Database>("talkbox.db");
    setup_server();
}

Server::~Server() {
    if (server_fd != -1) {
        close(server_fd);
    }
}

void Server::setup_server() {
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        throw std::runtime_error("创建socket失败");
    }
    
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);
    
    if (bind(server_fd, (sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        throw std::runtime_error("绑定端口失败");
    }
    
    if (listen(server_fd, 10) == -1) {
        throw std::runtime_error("监听失败");
    }
}

void Server::run() {
    while (true) {
        sockaddr_in client_addr{};
        socklen_t client_len = sizeof(client_addr);
        
        int client_fd = accept(server_fd, (sockaddr*)&client_addr, &client_len);
        if (client_fd == -1) {
            continue;
        }
        
        std::thread([this, client_fd]() {
            handle_client(client_fd);
        }).detach();
    }
}

void Server::handle_client(int client_fd) {
    char buffer[4096];
    
    while (true) {
        int bytes_read = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
        if (bytes_read <= 0) {
            break;
        }
        
        buffer[bytes_read] = '\0';
        std::string request(buffer);
        
        std::string response = handle_request(request, client_fd);
        send(client_fd, response.c_str(), response.length(), 0);
    }
    
    // 客户端断开连接，清理在线用户
    std::lock_guard<std::mutex> lock(users_mutex);
    for (auto it = online_users.begin(); it != online_users.end(); ++it) {
        if (it->second.socket_fd == client_fd) {
            it->second.online = false;
            online_users.erase(it);
            break;
        }
    }
    
    close(client_fd);
}

std::string Server::handle_request(const std::string& request, int client_fd) {
    // 解析HTTP请求
    size_t pos = request.find('\n');
    if (pos == std::string::npos) {
        return "HTTP/1.1 400 Bad Request\r\n\r\n";
    }
    
    std::string first_line = request.substr(0, pos);
    size_t method_end = first_line.find(' ');
    size_t path_end = first_line.find(' ', method_end + 1);
    
    if (method_end == std::string::npos || path_end == std::string::npos) {
        return "HTTP/1.1 400 Bad Request\r\n\r\n";
    }
    
    std::string method = first_line.substr(0, method_end);
    std::string path = first_line.substr(method_end + 1, path_end - method_end - 1);
    
    // 获取请求体
    size_t body_start = request.find("\r\n\r\n");
    std::string body;
    if (body_start != std::string::npos) {
        body = request.substr(body_start + 4);
    }
    
    // 提取token（除了注册和登录接口）
    std::string token = extract_token_from_request(request);
    
    std::string response_body;
    
    if (method == "POST") {
        if (path == "/api/register") {
            response_body = handle_register(body);
        } else if (path == "/api/login") {
            response_body = handle_login(body, client_fd);
        } else if (path == "/api/logout") {
            response_body = handle_logout(token, client_fd);
        } else if (path == "/api/send_message") {
            response_body = handle_send_message(body, token);
        } else if (path == "/api/create_post") {
            response_body = handle_create_post(body, token);
        } else if (path == "/api/reply_post") {
            response_body = handle_reply_post(body, token);
        } else if (path == "/api/upload_file") {
            response_body = handle_upload_file(body, token);
        } else if (path == "/api/create_group") {
            response_body = handle_create_group(body, token);
        } else if (path == "/api/join_group") {
            response_body = handle_join_group(body, token);
        } else if (path == "/api/leave_group") {
            response_body = handle_leave_group(body, token);
        } else {
            response_body = create_json_response("error", "未知的API路径");
        }
    } else if (method == "GET") {
        if (path == "/api/get_messages") {
            response_body = handle_get_messages(token);
        } else if (path == "/api/get_posts") {
            response_body = handle_get_posts(body);
        } else if (path == "/api/get_groups") {
            response_body = handle_get_groups(token);
        } else if (path == "/api/get_group_messages") {
            response_body = handle_get_group_messages(body, token);
        } else if (path.find("/api/download_file") == 0) {
            response_body = handle_download_file(path);
        } else {
            response_body = create_json_response("error", "未知的API路径");
        }
    } else {
        response_body = create_json_response("error", "不支持的HTTP方法");
    }
    
    std::string response = "HTTP/1.1 200 OK\r\n";
    response += "Content-Type: application/json\r\n";
    response += "Content-Length: " + std::to_string(response_body.length()) + "\r\n";
    response += "Access-Control-Allow-Origin: *\r\n";
    response += "\r\n";
    response += response_body;
    
    return response;
}

std::string Server::handle_register(const std::string& body) {
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
    }
    
    return create_json_response("error", "注册失败");
}

std::string Server::handle_login(const std::string& body, int client_fd) {
    std::string username = parse_json_value(body, "username");
    std::string password = parse_json_value(body, "password");
    
    User user;
    if (db->verify_user(username, password, user)) {
        std::string token = generate_token();
        
        std::lock_guard<std::mutex> lock(users_mutex);
        user.online = true;
        user.socket_fd = client_fd;
        user.token = token;
        online_users[user.user_id] = user;
        token_to_user_id[token] = user.user_id;
        
        return create_json_response("success", "{\"user_id\":" + std::to_string(user.user_id) + 
                                   ",\"username\":\"" + user.username + "\"" +
                                   ",\"token\":\"" + token + "\"}");
    }
    
    return create_json_response("error", "用户名或密码错误");
}

std::string Server::handle_logout(const std::string& token, int client_fd) {
    int user_id = get_user_id_by_token(token);
    if (user_id != -1) {
        std::lock_guard<std::mutex> lock(users_mutex);
        token_to_user_id.erase(token);
        online_users.erase(user_id);
        return create_json_response("success", "登出成功");
    }
    
    return create_json_response("error", "未登录");
}

std::string Server::handle_send_message(const std::string& body, const std::string& token) {
    int sender_id = get_user_id_by_token(token);
    if (sender_id == -1) {
        return create_json_response("error", "未登录");
    }
    
    std::string receiver_str = parse_json_value(body, "receiver_id");
    std::string group_str = parse_json_value(body, "group_id");
    std::string content = parse_json_value(body, "content");
    std::string type = parse_json_value(body, "type");
    
    Message message;
    message.sender_id = sender_id;
    message.content = content;
    message.type = type.empty() ? "text" : type;
    message.timestamp = get_current_timestamp();
    
    // 判断是私聊还是群聊
    if (!group_str.empty() && group_str != "-1") {
        // 群聊消息
        int group_id = std::stoi(group_str);
        if (!db->is_user_in_group(sender_id, group_id)) {
            return create_json_response("error", "您不是该群组成员");
        }
        message.group_id = group_id;
        message.receiver_id = -1;
    } else if (!receiver_str.empty() && receiver_str != "-1") {
        // 私聊消息
        message.receiver_id = std::stoi(receiver_str);
        message.group_id = -1;
    } else {
        return create_json_response("error", "请指定接收者或群组");
    }
    
    if (db->save_message(message)) {
        broadcast_message(message);
        return create_json_response("success", "消息发送成功");
    }
    
    return create_json_response("error", "消息发送失败");
}

std::string Server::handle_get_messages(const std::string& token) {
    int user_id = get_user_id_by_token(token);
    if (user_id == -1) {
        return create_json_response("error", "未登录");
    }
    
    std::vector<Message> messages = db->get_messages(user_id);
    
    std::string json_messages = "[";
    for (size_t i = 0; i < messages.size(); ++i) {
        if (i > 0) json_messages += ",";
        json_messages += "{";
        json_messages += "\"message_id\":" + std::to_string(messages[i].message_id) + ",";
        json_messages += "\"sender_id\":" + std::to_string(messages[i].sender_id) + ",";
        json_messages += "\"receiver_id\":" + std::to_string(messages[i].receiver_id) + ",";
        json_messages += "\"group_id\":" + std::to_string(messages[i].group_id) + ",";
        json_messages += "\"content\":\"" + messages[i].content + "\",";
        json_messages += "\"type\":\"" + messages[i].type + "\",";
        json_messages += "\"timestamp\":\"" + messages[i].timestamp + "\"";
        json_messages += "}";
    }
    json_messages += "]";
    
    return create_json_response("success", json_messages);
}

std::string Server::handle_create_post(const std::string& body, const std::string& token) {
    int user_id = get_user_id_by_token(token);
    if (user_id == -1) {
        return create_json_response("error", "未登录");
    }
    
    std::string title = parse_json_value(body, "title");
    std::string content = parse_json_value(body, "content");
    
    Post post;
    post.user_id = user_id;
    post.title = title;
    post.content = content;
    post.timestamp = get_current_timestamp();
    
    if (db->create_post(post)) {
        return create_json_response("success", "发帖成功");
    }
    
    return create_json_response("error", "发帖失败");
}

std::string Server::handle_get_posts(const std::string& body) {
    std::vector<Post> posts = db->get_posts();
    
    std::string json_posts = "[";
    for (size_t i = 0; i < posts.size(); ++i) {
        if (i > 0) json_posts += ",";
        json_posts += "{";
        json_posts += "\"post_id\":" + std::to_string(posts[i].post_id) + ",";
        json_posts += "\"user_id\":" + std::to_string(posts[i].user_id) + ",";
        json_posts += "\"title\":\"" + posts[i].title + "\",";
        json_posts += "\"content\":\"" + posts[i].content + "\",";
        json_posts += "\"timestamp\":\"" + posts[i].timestamp + "\"";
        json_posts += "}";
    }
    json_posts += "]";
    
    return create_json_response("success", json_posts);
}

std::string Server::handle_reply_post(const std::string& body, const std::string& token) {
    int user_id = get_user_id_by_token(token);
    if (user_id == -1) {
        return create_json_response("error", "未登录");
    }
    
    std::string post_id_str = parse_json_value(body, "post_id");
    std::string content = parse_json_value(body, "content");
    
    int post_id = std::stoi(post_id_str);
    
    if (db->reply_post(post_id, user_id, content, get_current_timestamp())) {
        return create_json_response("success", "回帖成功");
    }
    
    return create_json_response("error", "回帖失败");
}

std::string Server::handle_upload_file(const std::string& body, const std::string& token) {
    int user_id = get_user_id_by_token(token);
    if (user_id == -1) {
        return create_json_response("error", "未登录");
    }
    
    std::string filename = parse_json_value(body, "filename");
    std::string data = parse_json_value(body, "data");
    
    std::string filepath = "uploads/" + filename;
    std::ofstream file(filepath, std::ios::binary);
    if (file.is_open()) {
        file.write(data.c_str(), data.length());
        file.close();
        return create_json_response("success", "文件上传成功");
    }
    
    return create_json_response("error", "文件上传失败");
}

std::string Server::handle_download_file(const std::string& path) {
    size_t pos = path.find("filename=");
    if (pos == std::string::npos) {
        return create_json_response("error", "文件名参数缺失");
    }
    
    std::string filename = path.substr(pos + 9);
    std::string filepath = "uploads/" + filename;
    
    std::ifstream file(filepath, std::ios::binary);
    if (file.is_open()) {
        std::string content((std::istreambuf_iterator<char>(file)),
                           std::istreambuf_iterator<char>());
        file.close();
        return create_json_response("success", content);
    }
    
    return create_json_response("error", "文件不存在");
}

std::string Server::parse_json_value(const std::string& json, const std::string& key) {
    std::string search = "\"" + key + "\":\"";
    size_t start = json.find(search);
    if (start == std::string::npos) {
        return "";
    }
    
    start += search.length();
    size_t end = json.find("\"", start);
    if (end == std::string::npos) {
        return "";
    }
    
    return json.substr(start, end - start);
}

std::string Server::create_json_response(const std::string& status, const std::string& data) {
    std::string response = "{\"status\":\"" + status + "\"";
    if (!data.empty()) {
        // 检查data是否已经是JSON格式（以{或[开头）
        if (data[0] == '{' || data[0] == '[') {
            response += ",\"data\":" + data;
        } else {
            response += ",\"data\":\"" + data + "\"";
        }
    }
    response += "}";
    std::cout << "响应: " << response << std::endl;
    return response;
}

std::string Server::get_current_timestamp() {
    time_t now = time(0);
    char* timestr = ctime(&now);
    std::string result(timestr);
    result.pop_back(); // 移除换行符
    return result;
}

void Server::broadcast_message(const Message& message) {
    // 如果是群聊消息，广播给所有在线用户
    if (message.receiver_id == -1) {
        std::lock_guard<std::mutex> lock(users_mutex);
        for (const auto& pair : online_users) {
            if (pair.first != message.sender_id) {
                // 这里可以实现实时推送逻辑
            }
        }
    }
}

int Server::get_user_id_by_fd(int client_fd) {
    std::lock_guard<std::mutex> lock(users_mutex);
    for (const auto& pair : online_users) {
        if (pair.second.socket_fd == client_fd) {
            return pair.first;
        }
    }
    return -1;
}

int Server::get_user_id_by_token(const std::string& token) {
    if (token.empty()) return -1;
    
    std::lock_guard<std::mutex> lock(users_mutex);
    auto it = token_to_user_id.find(token);
    if (it != token_to_user_id.end()) {
        return it->second;
    }
    return -1;
}

std::string Server::generate_token() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 61);
    
    const std::string chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
    std::string token;
    for (int i = 0; i < 32; ++i) {
        token += chars[dis(gen)];
    }
    return token;
}

std::string Server::extract_token_from_request(const std::string& request) {
    // 从Authorization header中提取token: "Authorization: Bearer <token>"
    size_t auth_pos = request.find("Authorization: Bearer ");
    if (auth_pos != std::string::npos) {
        size_t token_start = auth_pos + 22; // 跳过"Authorization: Bearer "
        size_t token_end = request.find('\r', token_start);
        if (token_end == std::string::npos) {
            token_end = request.find('\n', token_start);
        }
        if (token_end != std::string::npos) {
            return request.substr(token_start, token_end - token_start);
        }
    }
    return "";
}

std::string Server::handle_create_group(const std::string& body, const std::string& token) {
    int user_id = get_user_id_by_token(token);
    if (user_id == -1) {
        return create_json_response("error", "未登录");
    }
    
    std::string group_name = parse_json_value(body, "group_name");
    std::string description = parse_json_value(body, "description");
    
    if (group_name.empty()) {
        return create_json_response("error", "群组名称不能为空");
    }
    
    Group group;
    group.group_name = group_name;
    group.description = description;
    group.creator_id = user_id;
    group.created_time = get_current_timestamp();
    
    if (db->create_group(group)) {
        return create_json_response("success", "群组创建成功");
    }
    
    return create_json_response("error", "群组创建失败");
}

std::string Server::handle_join_group(const std::string& body, const std::string& token) {
    int user_id = get_user_id_by_token(token);
    if (user_id == -1) {
        return create_json_response("error", "未登录");
    }
    
    std::string group_id_str = parse_json_value(body, "group_id");
    if (group_id_str.empty()) {
        return create_json_response("error", "群组ID不能为空");
    }
    
    int group_id = std::stoi(group_id_str);
    
    if (db->is_user_in_group(user_id, group_id)) {
        return create_json_response("error", "您已经是该群组成员");
    }
    
    if (db->join_group(user_id, group_id)) {
        return create_json_response("success", "加入群组成功");
    }
    
    return create_json_response("error", "加入群组失败");
}

std::string Server::handle_leave_group(const std::string& body, const std::string& token) {
    int user_id = get_user_id_by_token(token);
    if (user_id == -1) {
        return create_json_response("error", "未登录");
    }
    
    std::string group_id_str = parse_json_value(body, "group_id");
    if (group_id_str.empty()) {
        return create_json_response("error", "群组ID不能为空");
    }
    
    int group_id = std::stoi(group_id_str);
    
    if (!db->is_user_in_group(user_id, group_id)) {
        return create_json_response("error", "您不是该群组成员");
    }
    
    if (db->leave_group(user_id, group_id)) {
        return create_json_response("success", "退出群组成功");
    }
    
    return create_json_response("error", "退出群组失败");
}

std::string Server::handle_get_groups(const std::string& token) {
    int user_id = get_user_id_by_token(token);
    if (user_id == -1) {
        // 未登录用户也可以查看所有群组列表
        std::vector<Group> groups = db->get_all_groups();
        
        std::string json_groups = "[";
        for (size_t i = 0; i < groups.size(); ++i) {
            if (i > 0) json_groups += ",";
            json_groups += "{";
            json_groups += "\"group_id\":" + std::to_string(groups[i].group_id) + ",";
            json_groups += "\"group_name\":\"" + groups[i].group_name + "\",";
            json_groups += "\"description\":\"" + groups[i].description + "\",";
            json_groups += "\"creator_id\":" + std::to_string(groups[i].creator_id) + ",";
            json_groups += "\"created_time\":\"" + groups[i].created_time + "\",";
            json_groups += "\"is_member\":false";
            json_groups += "}";
        }
        json_groups += "]";
        
        return create_json_response("success", json_groups);
    } else {
        // 已登录用户获取所有群组，并标记是否已加入
        std::vector<Group> all_groups = db->get_all_groups();
        
        std::string json_groups = "[";
        for (size_t i = 0; i < all_groups.size(); ++i) {
            if (i > 0) json_groups += ",";
            json_groups += "{";
            json_groups += "\"group_id\":" + std::to_string(all_groups[i].group_id) + ",";
            json_groups += "\"group_name\":\"" + all_groups[i].group_name + "\",";
            json_groups += "\"description\":\"" + all_groups[i].description + "\",";
            json_groups += "\"creator_id\":" + std::to_string(all_groups[i].creator_id) + ",";
            json_groups += "\"created_time\":\"" + all_groups[i].created_time + "\",";
            json_groups += "\"is_member\":" + std::string(db->is_user_in_group(user_id, all_groups[i].group_id) ? "true" : "false");
            json_groups += "}";
        }
        json_groups += "]";
        
        return create_json_response("success", json_groups);
    }
}

std::string Server::handle_get_group_messages(const std::string& body, const std::string& token) {
    int user_id = get_user_id_by_token(token);
    if (user_id == -1) {
        return create_json_response("error", "未登录");
    }
    
    std::string group_id_str = parse_json_value(body, "group_id");
    if (group_id_str.empty()) {
        return create_json_response("error", "群组ID不能为空");
    }
    
    int group_id = std::stoi(group_id_str);
    
    if (!db->is_user_in_group(user_id, group_id)) {
        return create_json_response("error", "您不是该群组成员");
    }
    
    std::vector<Message> messages = db->get_group_messages(group_id);
    
    std::string json_messages = "[";
    for (size_t i = 0; i < messages.size(); ++i) {
        if (i > 0) json_messages += ",";
        json_messages += "{";
        json_messages += "\"message_id\":" + std::to_string(messages[i].message_id) + ",";
        json_messages += "\"sender_id\":" + std::to_string(messages[i].sender_id) + ",";
        json_messages += "\"group_id\":" + std::to_string(messages[i].group_id) + ",";
        json_messages += "\"content\":\"" + messages[i].content + "\",";
        json_messages += "\"type\":\"" + messages[i].type + "\",";
        json_messages += "\"timestamp\":\"" + messages[i].timestamp + "\"";
        json_messages += "}";
    }
    json_messages += "]";
    
    return create_json_response("success", json_messages);
}
