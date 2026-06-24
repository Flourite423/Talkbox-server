#include "server.h"
#include "database.h"
#include "common.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <iostream>
#include <sstream>

Server::Server(int port) : server_fd(-1), port(port) {
    // 初始化数据库
    db = std::make_unique<Database>("talkbox.db");
    
    // 初始化各个服务模块
    user_manager = std::make_unique<UserManager>(db.get());
    message_service = std::make_unique<MessageService>(db.get(), user_manager.get());
    forum_service = std::make_unique<ForumService>(db.get(), user_manager.get());
    file_manager = std::make_unique<FileManager>("uploads", user_manager.get());
    
    // 设置服务器
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
        close(server_fd);
        server_fd = -1;
        throw std::runtime_error("绑定端口失败");
    }
    
    if (listen(server_fd, 10) == -1) {
        close(server_fd);
        server_fd = -1;
        throw std::runtime_error("监听失败");
    }
}

void Server::run() {
    std::cout << "服务器已启动，监听端口: " << port << std::endl;
    
    while (g_running) {
        sockaddr_in client_addr{};
        socklen_t client_len = sizeof(client_addr);
        
        int client_fd = accept(server_fd, (sockaddr*)&client_addr, &client_len);
        if (client_fd == -1) {
            if (!g_running) break;
            continue;
        }
        
        std::thread([this, client_fd]() {
            handle_client(client_fd);
        }).detach();
    }
    
    std::cout << "服务器正在关闭..." << std::endl;
}

void Server::handle_client(int client_fd) {
    // 增大缓冲区以支持大文件上传 (64KB)
    char buffer[65536];
    
    while (true) {
        int bytes_read = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
        if (bytes_read <= 0) {
            break;
        }
        
        buffer[bytes_read] = '\0';
        std::string request(buffer);
        
        std::string response = handle_request(request, client_fd);
        
        // 检查 send 返回值，处理部分写入
        size_t total_sent = 0;
        size_t to_send = response.length();
        while (total_sent < to_send) {
            ssize_t sent = send(client_fd, response.c_str() + total_sent, to_send - total_sent, 0);
            if (sent <= 0) {
                break;  // 发送失败或连接关闭
            }
            total_sent += sent;
        }
    }
    
    // 客户端断开连接，清理在线状态
    user_manager->remove_online_user_by_fd(client_fd);
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
    if (method_end == std::string::npos) {
        return "HTTP/1.1 400 Bad Request\r\n\r\n";
    }
    
    std::string method = first_line.substr(0, method_end);
    size_t path_start = first_line.find(' ', method_end) + 1;
    size_t path_end = first_line.find(' ', path_start);
    if (path_start == std::string::npos || path_end == std::string::npos) {
        return "HTTP/1.1 400 Bad Request\r\n\r\n";
    }
    
    std::string full_path = first_line.substr(path_start, path_end - path_start);
    
    // 分离路径和查询参数
    std::string path = full_path;
    std::string query_string;
    size_t query_start = full_path.find('?');
    if (query_start != std::string::npos) {
        path = full_path.substr(0, query_start);
        query_string = full_path.substr(query_start + 1);
    }
    
    // 查找请求体
    size_t body_start = request.find("\r\n\r\n");
    std::string body;
    if (body_start != std::string::npos) {
        body = request.substr(body_start + 4);
    }
    
    // 提取token
    std::string token = extract_token_from_request(request);
    
    // 认证辅助函数：验证用户是否存在（已注册）
    auto verify_authenticated = [&](const std::string& username) -> bool {
        if (username.empty()) return false;
        int user_id = user_manager->get_user_id_by_username(username);
        return user_id != -1;  // 只检查用户是否存在，不检查是否在线
    };
    
    // 从 body 或 query_string 提取 username
    auto extract_username = [&](const std::string& body, const std::string& query) -> std::string {
        std::string username = parse_json_value(body, "username");
        if (username.empty()) {
            size_t pos = query.find("username=");
            if (pos != std::string::npos) {
                pos += 8;
                size_t end = query.find('&', pos);
                if (end == std::string::npos) end = query.length();
                username = query.substr(pos, end - pos);
            }
        }
        return username;
    };
    
    // 根据路径和方法处理请求
    // 不需要认证的接口
    if (path == "/api/register" && method == "POST") {
        return user_manager->register_user(body);
    } else if (path == "/api/login" && method == "POST") {
        return user_manager->login_user(body, client_fd);
    } else if (path == "/api/get_posts" && method == "GET") {
        return forum_service->get_posts(query_string);
    } else if (path.find("/api/post/") == 0 && method == "GET") {
        std::string post_id_str = path.substr(10);
        if (!post_id_str.empty()) {
            int post_id = safe_stoi(post_id_str);
            if (post_id == -1) {
                return create_json_response("error", "无效的帖子ID");
            }
            return forum_service->get_post_detail(post_id);
        }
        return create_json_response("error", "缺少帖子ID");
    } else if (path == "/api/get_groups" && method == "GET") {
        return message_service->get_groups(query_string);
    }
    
    // 以下接口需要认证
    std::string username = extract_username(body, query_string);
    if (!verify_authenticated(username)) {
        return create_json_response("error", "未登录或会话已过期");
    }
    
    // 需要认证的接口
    if (path == "/api/logout" && method == "POST") {
        return user_manager->logout_user(body, client_fd);
    } else if (path == "/api/user/profile" && method == "GET") {
        return user_manager->get_user_profile(query_string);
    } else if (path == "/api/send_message" && method == "POST") {
        return message_service->send_message(body);
    } else if (path == "/api/get_messages" && method == "GET") {
        return message_service->get_messages(query_string);
    } else if (path == "/api/get_contacts" && method == "GET") {
        return message_service->get_contacts(query_string);
    } else if (path == "/api/create_post" && method == "POST") {
        return forum_service->create_post(body);
    } else if (path == "/api/reply_post" && method == "POST") {
        return forum_service->reply_post(body);
    } else if (path == "/api/get_post_replies" && method == "GET") {
        return forum_service->get_post_replies(query_string);
    } else if (path == "/api/upload_file" && method == "POST") {
        return file_manager->upload_file(body);
    } else if (path == "/api/download_file" && method == "GET") {
        return file_manager->download_file(query_string);
    } else if (path == "/api/create_group" && method == "POST") {
        return message_service->create_group(body);
    } else if (path == "/api/join_group" && method == "POST") {
        return message_service->join_group(body);
    } else if (path == "/api/leave_group" && method == "POST") {
        return message_service->leave_group(body);
    } else if (path == "/api/get_group_messages" && method == "GET") {
        return message_service->get_group_messages(query_string);
    } else if (path == "/api/heartbeat" && method == "POST") {
        return create_json_response("success", "heartbeat_ok");
    } else if (method != "GET" && method != "POST") {
        return create_json_response("error", "不支持的HTTP方法: " + method);
    } else {
        return create_json_response("error", "无效的API路径: " + path + " " + method);
    }
}

std::string Server::extract_token_from_request(const std::string& request) {
    std::string auth_header = "Authorization: Bearer ";
    size_t pos = request.find(auth_header);
    if (pos == std::string::npos) {
        return "";
    }
    
    pos += auth_header.length();
    size_t end = request.find("\r\n", pos);
    if (end == std::string::npos) {
        return "";
    }
    
    return request.substr(pos, end - pos);
}

std::string Server::parse_query_param(const std::string& query_string, const std::string& key) {
    if (query_string.empty()) {
        return "";
    }
    
    std::string search_key = key + "=";
    size_t pos = query_string.find(search_key);
    if (pos == std::string::npos) {
        return "";
    }
    
    pos += search_key.length();
    size_t end = query_string.find('&', pos);
    if (end == std::string::npos) {
        end = query_string.length();
    }
    
    return query_string.substr(pos, end - pos);
}
