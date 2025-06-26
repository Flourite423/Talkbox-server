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

std::string ForumService::create_post(const std::string& body, const std::string& token) {
    int user_id = user_manager->get_user_id_by_token(token);
    if (user_id == -1) {
        return create_json_response("error", "无效的token");
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
    std::vector<Post> posts = db->get_posts();
    
    std::ostringstream json_array;
    json_array << "[";
    
    for (size_t i = 0; i < posts.size(); ++i) {
        if (i > 0) {
            json_array << ",";
        }
        
        json_array << "{\"post_id\":" << posts[i].post_id
                  << ",\"user_id\":" << posts[i].user_id
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

std::string ForumService::reply_post(const std::string& body, const std::string& token) {
    int user_id = user_manager->get_user_id_by_token(token);
    if (user_id == -1) {
        return create_json_response("error", "无效的token");
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
        json_array << "{"
                   << "\"reply_id\":" << replies[i].reply_id << ","
                   << "\"post_id\":" << replies[i].post_id << ","
                   << "\"user_id\":" << replies[i].user_id << ","
                   << "\"content\":\"" << replies[i].content << "\","
                   << "\"timestamp\":\"" << replies[i].timestamp << "\""
                   << "}";
    }
    
    json_array << "]";
    
    return create_json_response("success", json_array.str());
}
