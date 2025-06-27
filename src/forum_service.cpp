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

std::string ForumService::get_posts(const std::string& body) {
    (void)body;  // 参数未使用，获取所有帖子时不需要特定参数
    std::vector<Post> posts = db->get_posts();
    
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
                  << ",\"username\":\"" << posts[i].username << "\""
                  << ",\"title\":\"" << posts[i].title << "\""
                  << ",\"content\":\"" << posts[i].content << "\""
                  << ",\"timestamp\":\"" << posts[i].timestamp << "\"";
        
        // 添加回复
        if (!posts[i].replies.empty()) {
            json_array << ",\"replies\":[";
            for (size_t j = 0; j < posts[i].replies.size(); ++j) {
                if (j > 0) {
                    json_array << ",";
                }
                json_array << "\"" << posts[i].replies[j] << "\"";
            }
            json_array << "]";
        }
        
        json_array << "}";
    }
    
    json_array << "]";
    
    return create_json_response("success", json_array.str());
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
    
    int post_id = std::stoi(post_id_str);
    std::string timestamp = get_current_timestamp();
    
    if (db->reply_post(post_id, user_id, content, timestamp)) {
        return create_json_response("success", "回帖成功");
    } else {
        return create_json_response("error", "回帖失败");
    }
}

std::string ForumService::get_post_replies(const std::string& body) {
    std::string post_id_str = parse_json_value(body, "post_id");
    
    if (post_id_str.empty()) {
        return create_json_response("error", "帖子ID不能为空");
    }
    
    int post_id = std::stoi(post_id_str);
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
                   << "\"username\":\"" << replies[i].username << "\","
                   << "\"content\":\"" << replies[i].content << "\","
                   << "\"timestamp\":\"" << replies[i].timestamp << "\""
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
