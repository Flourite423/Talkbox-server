#ifndef DATABASE_H
#define DATABASE_H

#include <sqlite3.h>
#include <string>
#include <vector>
#include "common.h"

class Database {
public:
    Database(const std::string& db_path);
    ~Database();
    
    bool create_user(const std::string& username, const std::string& password);
    bool user_exists(const std::string& username);
    bool verify_user(const std::string& username, const std::string& password, User& user);
    
    bool save_message(const Message& message);
    std::vector<Message> get_messages(int user_id);
    std::vector<Message> get_group_messages(int group_id);
    
    // 群组管理
    bool create_group(const Group& group);
    std::vector<Group> get_all_groups();
    std::vector<Group> get_user_groups(int user_id);
    bool join_group(int user_id, int group_id);
    bool leave_group(int user_id, int group_id);
    bool is_user_in_group(int user_id, int group_id);
    
    bool create_post(const Post& post);
    std::vector<Post> get_posts();
    bool reply_post(int post_id, int user_id, const std::string& content, const std::string& timestamp);
    
private:
    sqlite3* db;
    bool init_database();
    bool execute_sql(const std::string& sql);
};

#endif
