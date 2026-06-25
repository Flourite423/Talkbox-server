#include "forum_service.h"
#include "database.h"
#include "user_manager.h"
#include "common.h"
#include <iostream>
#include <sstream>

ForumService::ForumService(Database* db, UserManager* user_manager)
    : db(db), user_manager(user_manager) {
}

ForumService::~ForumService() {
}

std::string ForumService::create_post(const std::string& body) {
    std::string username = parse_json_value(body, "username");
    if (username.empty()) {
        return create_json_response("error", "用户名不能为空");
    }
    
    int user_id = user_manager->get_user_id_by_username(username);
    if (user_id == -1) {
        return create_json_response("error", "无效的用户名");
    }
    
    std::string title = parse_json_value(body, "title");
    std::string content = parse_json_value(body, "content");
    
    if (title.empty() || content.empty()) {
        return create_json_response("error", "标题和内容不能为空");
    }
    if (title.length() > 200) {
        return create_json_response("error", "标题不能超过200个字符");
    }
    if (content.length() > 50000) {
        return create_json_response("error", "内容不能超过50000个字符");
    }
    
    Post post;
    post.user_id = user_id;
    post.title = title;
    post.content = content;
    post.timestamp = get_current_timestamp();
    
    if (db->create_post(post)) {
        return create_json_response("success", "发帖成功");
    } else {
        return create_json_response("error", "发帖失败");
    }
}

std::string ForumService::get_posts(const std::string& query_string) {
    // 解析分页参数
    int page = 1;
    int page_size = 20;
    
    if (!query_string.empty()) {
        // 解析 page
        size_t pos = query_string.find("page=");
        if (pos != std::string::npos) {
            pos += 5;
            size_t end = query_string.find('&', pos);
            if (end == std::string::npos) end = query_string.length();
            try {
                page = safe_stoi(query_string.substr(pos, end - pos));
                if (page <= 0) page = 1;
            } catch (...) {}
        }
        
        // 解析 page_size
        pos = query_string.find("page_size=");
        if (pos != std::string::npos) {
            pos += 10;
            size_t end = query_string.find('&', pos);
            if (end == std::string::npos) end = query_string.length();
            try {
                page_size = safe_stoi(query_string.substr(pos, end - pos));
                if (page_size <= 0 || page_size > 100) page_size = 20;
            } catch (...) {}
        }
    }
    
    std::vector<Post> posts = db->get_posts(page, page_size);
    
    
    std::ostringstream json_array;
    json_array << "[";
    
    for (size_t i = 0; i < posts.size(); ++i) {
        if (i > 0) {
            json_array << ",";
        }
        
        // 获取用户名
        std::string username = db->get_username_by_id(posts[i].user_id);
        if (!username.empty()) {
            posts[i].username = username;
        }
        
        json_array << "{\"post_id\":" << posts[i].post_id
                  << ",\"user_id\":" << posts[i].user_id
                  << ",\"username\":\"" << escape_json_string(posts[i].username) << "\""
                  << ",\"title\":\"" << escape_json_string(posts[i].title) << "\""
                  << ",\"content\":\"" << escape_json_string(posts[i].content) << "\""
                  << ",\"timestamp\":\"" << escape_json_string(posts[i].timestamp) << "\"}";
    }
    
    json_array << "]";
    
    // 构建响应，包含分页信息
    std::string result = "{\"status\":\"success\",\"data\":" + json_array.str() + 
                         ",\"page\":" + std::to_string(page) +
                         ",\"page_size\":" + std::to_string(page_size) +
                         ",\"has_more\":" + (posts.size() >= (size_t)page_size ? "true" : "false") + "}";
    
    return "HTTP/1.1 200 OK\r\n"
           "Content-Type: application/json\r\n"
           "Content-Length: " + std::to_string(result.length()) + "\r\n"
           "Connection: close\r\n"
           "Access-Control-Allow-Origin: *\r\n"
           "\r\n" + result;
}

std::string ForumService::reply_post(const std::string& body) {
    std::string username = parse_json_value(body, "username");
    if (username.empty()) {
        return create_json_response("error", "用户名不能为空");
    }
    
    int user_id = user_manager->get_user_id_by_username(username);
    if (user_id == -1) {
        return create_json_response("error", "无效的用户名");
    }
    
    std::string post_id_str = parse_json_value(body, "post_id");
    std::string content = parse_json_value(body, "content");
    
    if (post_id_str.empty() || content.empty()) {
        return create_json_response("error", "帖子ID和回复内容不能为空");
    }
    
    int post_id = safe_stoi(post_id_str);
    std::string timestamp = get_current_timestamp();
    
    if (db->reply_post(post_id, user_id, content, timestamp)) {
        return create_json_response("success", "回帖成功");
    } else {
        return create_json_response("error", "回帖失败");
    }
}

std::string ForumService::get_post_replies(const std::string& query_string) {
    // 解析查询参数中的post_id
    std::string post_id_str = "";
    if (!query_string.empty()) {
        std::string search_key = "post_id=";
        size_t pos = query_string.find(search_key);
        if (pos != std::string::npos) {
            pos += search_key.length();
            size_t end = query_string.find('&', pos);
            if (end == std::string::npos) {
                end = query_string.length();
            }
            post_id_str = query_string.substr(pos, end - pos);
        }
    }
    
    if (post_id_str.empty()) {
        return create_json_response("error", "帖子ID不能为空");
    }
    
    int post_id = safe_stoi(post_id_str);
    std::vector<Reply> replies = db->get_post_replies(post_id);
    
    std::ostringstream json_array;
    json_array << "[";
    
    for (size_t i = 0; i < replies.size(); ++i) {
        if (i > 0) json_array << ",";
        
        // 获取用户名
        std::string username = db->get_username_by_id(replies[i].user_id);
        if (!username.empty()) {
            replies[i].username = username;
        }
        
        json_array << "{"
                   << "\"reply_id\":" << replies[i].reply_id << ","
                   << "\"post_id\":" << replies[i].post_id << ","
                   << "\"user_id\":" << replies[i].user_id << ","
                   << "\"username\":\"" << escape_json_string(replies[i].username) << "\","
                   << "\"content\":\"" << escape_json_string(replies[i].content) << "\","
                   << "\"timestamp\":\"" << escape_json_string(replies[i].timestamp) << "\""
                   << "}";
    }
    
    json_array << "]";
    
    return create_json_response("success", json_array.str());
}

std::string ForumService::get_post_detail(int post_id) {
    Post post = db->get_post_by_id(post_id);
    
    if (post.post_id == -1) {
        return create_json_response("error", "帖子不存在");
    }
    
    // 获取用户名
    std::string username = db->get_username_by_id(post.user_id);
    if (!username.empty()) {
        post.username = username;
    }
    
    std::ostringstream json;
    json << "{\"post_id\":" << post.post_id
         << ",\"user_id\":" << post.user_id
         << ",\"username\":\"" << post.username << "\""
         << ",\"title\":\"" << post.title << "\""
         << ",\"content\":\"" << post.content << "\""
         << ",\"timestamp\":\"" << post.timestamp << "\"";
    
    // 添加回复
    if (!post.replies.empty()) {
        json << ",\"replies\":[";
        for (size_t i = 0; i < post.replies.size(); ++i) {
            if (i > 0) {
                json << ",";
            }
            json << "\"" << post.replies[i] << "\"";
        }
        json << "]";
    }
    
    json << "}";
    
    return create_json_response("success", json.str());
}
