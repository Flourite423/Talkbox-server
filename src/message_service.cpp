#include "message_service.h"
#include "database.h"
#include "user_manager.h"
#include "common.h"
#include <iostream>
#include <sstream>

MessageService::MessageService(Database* db, UserManager* user_manager)
    : db(db), user_manager(user_manager) {
}

MessageService::~MessageService() {
}

std::string MessageService::send_message(const std::string& body, const std::string& token) {
    int sender_id = user_manager->get_user_id_by_token(token);
    if (sender_id == -1) {
        return create_json_response("error", "无效的token");
    }
    
    std::string receiver_id_str = parse_json_value(body, "receiver_id");
    std::string group_id_str = parse_json_value(body, "group_id");
    std::string content = parse_json_value(body, "content");
    std::string type = parse_json_value(body, "type");
    
    if (content.empty()) {
        return create_json_response("error", "消息内容不能为空");
    }
    
    if (type.empty()) {
        type = "text";
    }
    
    Message message;
    message.sender_id = sender_id;
    message.content = content;
    message.timestamp = get_current_timestamp();
    message.type = type;
    
    // 私聊消息
    if (!receiver_id_str.empty()) {
        int receiver_id = std::stoi(receiver_id_str);
        message.receiver_id = receiver_id;
        message.group_id = -1;
    }
    // 群聊消息
    else if (!group_id_str.empty()) {
        int group_id = std::stoi(group_id_str);
        message.receiver_id = -1;
        message.group_id = group_id;
        
        if (!db->is_user_in_group(sender_id, group_id)) {
            return create_json_response("error", "您不是该群组的成员");
        }
    }
    else {
        return create_json_response("error", "必须提供接收者ID或群组ID");
    }
    
    if (db->save_message(message)) {
        broadcast_message(message);
        return create_json_response("success", "消息发送成功");
    } else {
        return create_json_response("error", "消息发送失败");
    }
}

std::string MessageService::get_messages(const std::string& token) {
    int user_id = user_manager->get_user_id_by_token(token);
    if (user_id == -1) {
        return create_json_response("error", "无效的token");
    }
    
    std::vector<Message> messages = db->get_messages(user_id);
    
    std::ostringstream json_array;
    json_array << "[";
    
    for (size_t i = 0; i < messages.size(); ++i) {
        if (i > 0) {
            json_array << ",";
        }
        
        // 获取发送者用户名
        std::string sender_username = db->get_username_by_id(messages[i].sender_id);
        if (!sender_username.empty()) {
            messages[i].sender_username = sender_username;
        }
        
        json_array << "{\"message_id\":" << messages[i].message_id
                  << ",\"sender_id\":" << messages[i].sender_id
                  << ",\"sender_username\":\"" << messages[i].sender_username << "\""
                  << ",\"receiver_id\":" << messages[i].receiver_id
                  << ",\"group_id\":" << messages[i].group_id
                  << ",\"content\":\"" << messages[i].content << "\""
                  << ",\"type\":\"" << messages[i].type << "\""
                  << ",\"timestamp\":\"" << messages[i].timestamp << "\"}";
    }
    
    json_array << "]";
    
    return create_json_response("success", json_array.str());
}

std::string MessageService::get_contacts(const std::string& token) {
    int user_id = user_manager->get_user_id_by_token(token);
    if (user_id == -1) {
        return create_json_response("error", "无效的token");
    }
    
    std::vector<User> contacts = db->get_user_contacts(user_id);
    
    std::ostringstream json_array;
    json_array << "[";
    
    for (size_t i = 0; i < contacts.size(); ++i) {
        if (i > 0) {
            json_array << ",";
        }
        
        json_array << "{\"user_id\":" << contacts[i].user_id
                  << ",\"username\":\"" << contacts[i].username << "\""
                  << ",\"online\":" << (contacts[i].online ? "true" : "false")
                  << "}";
    }
    
    json_array << "]";
    
    return create_json_response("success", json_array.str());
}

std::string MessageService::create_group(const std::string& body, const std::string& token) {
    int user_id = user_manager->get_user_id_by_token(token);
    if (user_id == -1) {
        return create_json_response("error", "无效的token");
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
    } else {
        return create_json_response("error", "群组创建失败");
    }
}

std::string MessageService::join_group(const std::string& body, const std::string& token) {
    int user_id = user_manager->get_user_id_by_token(token);
    if (user_id == -1) {
        return create_json_response("error", "无效的token");
    }
    
    std::string group_id_str = parse_json_value(body, "group_id");
    if (group_id_str.empty()) {
        return create_json_response("error", "群组ID不能为空");
    }
    
    int group_id = std::stoi(group_id_str);
    
    if (db->is_user_in_group(user_id, group_id)) {
        return create_json_response("error", "您已经是该群组的成员");
    }
    
    if (db->join_group(user_id, group_id)) {
        return create_json_response("success", "加入群组成功");
    } else {
        return create_json_response("error", "加入群组失败");
    }
}

std::string MessageService::leave_group(const std::string& body, const std::string& token) {
    int user_id = user_manager->get_user_id_by_token(token);
    if (user_id == -1) {
        return create_json_response("error", "无效的token");
    }
    
    std::string group_id_str = parse_json_value(body, "group_id");
    if (group_id_str.empty()) {
        return create_json_response("error", "群组ID不能为空");
    }
    
    int group_id = std::stoi(group_id_str);
    
    if (!db->is_user_in_group(user_id, group_id)) {
        return create_json_response("error", "您不是该群组的成员");
    }
    
    if (db->leave_group(user_id, group_id)) {
        return create_json_response("success", "退出群组成功");
    } else {
        return create_json_response("error", "退出群组失败");
    }
}

std::string MessageService::get_groups(const std::string& token) {
    std::vector<Group> groups;
    int user_id = -1;
    
    if (!token.empty()) {
        user_id = user_manager->get_user_id_by_token(token);
    }
    
    if (user_id != -1) {
        groups = db->get_user_groups(user_id);
    } else {
        groups = db->get_all_groups();
    }
    
    std::ostringstream json_array;
    json_array << "[";
    
    for (size_t i = 0; i < groups.size(); ++i) {
        if (i > 0) {
            json_array << ",";
        }
        
        json_array << "{\"group_id\":" << groups[i].group_id
                  << ",\"group_name\":\"" << groups[i].group_name << "\""
                  << ",\"description\":\"" << groups[i].description << "\""
                  << ",\"creator_id\":" << groups[i].creator_id
                  << ",\"created_time\":\"" << groups[i].created_time << "\"";
        
        if (user_id != -1) {
            bool is_member = db->is_user_in_group(user_id, groups[i].group_id);
            json_array << ",\"is_member\":" << (is_member ? "true" : "false");
        }
        
        json_array << "}";
    }
    
    json_array << "]";
    
    return create_json_response("success", json_array.str());
}

std::string MessageService::get_group_messages(const std::string& body, const std::string& token) {
    int user_id = user_manager->get_user_id_by_token(token);
    if (user_id == -1) {
        return create_json_response("error", "无效的token");
    }
    
    std::string group_id_str = parse_json_value(body, "group_id");
    if (group_id_str.empty()) {
        return create_json_response("error", "群组ID不能为空");
    }
    
    int group_id = std::stoi(group_id_str);
    
    if (!db->is_user_in_group(user_id, group_id)) {
        return create_json_response("error", "您不是该群组的成员");
    }
    
    std::vector<Message> messages = db->get_group_messages(group_id);
    
    std::ostringstream json_array;
    json_array << "[";
    
    for (size_t i = 0; i < messages.size(); ++i) {
        if (i > 0) {
            json_array << ",";
        }
        
        // 获取发送者用户名
        std::string sender_username = db->get_username_by_id(messages[i].sender_id);
        if (!sender_username.empty()) {
            messages[i].sender_username = sender_username;
        }
        
        json_array << "{\"message_id\":" << messages[i].message_id
                  << ",\"sender_id\":" << messages[i].sender_id
                  << ",\"sender_username\":\"" << messages[i].sender_username << "\""
                  << ",\"group_id\":" << messages[i].group_id
                  << ",\"content\":\"" << messages[i].content << "\""
                  << ",\"type\":\"" << messages[i].type << "\""
                  << ",\"timestamp\":\"" << messages[i].timestamp << "\"}";
    }
    
    json_array << "]";
    
    return create_json_response("success", json_array.str());
}

void MessageService::broadcast_message(const Message& message) {
    // 获取在线用户
    auto& online_users = user_manager->get_online_users();
    auto& users_mutex = user_manager->get_users_mutex();
    
    std::lock_guard<std::mutex> lock(users_mutex);
    
    // 私聊消息
    if (message.group_id == -1) {
        auto it = online_users.find(message.receiver_id);
        if (it != online_users.end() && it->second.online) {
            std::string notification = "新消息: " + message.content;
            // 可以在这里实现实时推送
        }
    }
    // 群聊消息
    else {
        // 获取群组所有成员并推送消息
        // 这里可以扩展实现实时推送功能
    }
}
