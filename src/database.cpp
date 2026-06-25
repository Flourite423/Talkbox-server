#include "database.h"
#include <iostream>
#include <algorithm>

// 安全获取 sqlite3 文本字段
static std::string safe_sqlite3_text(sqlite3_stmt* stmt, int col) {
    const char* text = reinterpret_cast<const char*>(sqlite3_column_text(stmt, col));
    return text ? std::string(text) : std::string();
}
Database::Database(const std::string& db_path) {
    int rc = sqlite3_open(db_path.c_str(), &db);
    if (rc) {
        std::cerr << "无法打开数据库: " << sqlite3_errmsg(db) << std::endl;
        db = nullptr;
        return;
    }
    
    init_database();
}

Database::~Database() {
    if (db) {
        sqlite3_close(db);
    }
}

bool Database::init_database() {
    std::string create_users_table = R"(
        CREATE TABLE IF NOT EXISTS users (
            user_id INTEGER PRIMARY KEY AUTOINCREMENT,
            username TEXT UNIQUE NOT NULL,
            password TEXT NOT NULL,
            online INTEGER DEFAULT 0
        );
    )";
    
    std::string create_messages_table = R"(
        CREATE TABLE IF NOT EXISTS messages (
            message_id INTEGER PRIMARY KEY AUTOINCREMENT,
            sender_id INTEGER NOT NULL,
            receiver_id INTEGER,
            group_id INTEGER,
            content TEXT NOT NULL,
            type TEXT DEFAULT 'text',
            timestamp TEXT NOT NULL,
            FOREIGN KEY (sender_id) REFERENCES users(user_id)
        );
    )";
    
    std::string create_groups_table = R"(
        CREATE TABLE IF NOT EXISTS groups (
            group_id INTEGER PRIMARY KEY AUTOINCREMENT,
            group_name TEXT NOT NULL,
            description TEXT,
            creator_id INTEGER NOT NULL,
            created_time TEXT NOT NULL,
            FOREIGN KEY (creator_id) REFERENCES users(user_id)
        );
    )";
    
    std::string create_group_members_table = R"(
        CREATE TABLE IF NOT EXISTS group_members (
            member_id INTEGER PRIMARY KEY AUTOINCREMENT,
            group_id INTEGER NOT NULL,
            user_id INTEGER NOT NULL,
            joined_time TEXT NOT NULL,
            FOREIGN KEY (group_id) REFERENCES groups(group_id),
            FOREIGN KEY (user_id) REFERENCES users(user_id),
            UNIQUE(group_id, user_id)
        );
    )";
    
    std::string create_posts_table = R"(
        CREATE TABLE IF NOT EXISTS posts (
            post_id INTEGER PRIMARY KEY AUTOINCREMENT,
            user_id INTEGER NOT NULL,
            title TEXT NOT NULL,
            content TEXT NOT NULL,
            timestamp TEXT NOT NULL,
            FOREIGN KEY (user_id) REFERENCES users(user_id)
        );
    )";
    
    std::string create_replies_table = R"(
        CREATE TABLE IF NOT EXISTS replies (
            reply_id INTEGER PRIMARY KEY AUTOINCREMENT,
            post_id INTEGER NOT NULL,
            user_id INTEGER NOT NULL,
            content TEXT NOT NULL,
            timestamp TEXT NOT NULL,
            FOREIGN KEY (post_id) REFERENCES posts(post_id),
            FOREIGN KEY (user_id) REFERENCES users(user_id)
        );
    )";
    
    return execute_sql(create_users_table) &&
           execute_sql(create_messages_table) &&
           execute_sql(create_groups_table) &&
           execute_sql(create_group_members_table) &&
           execute_sql(create_posts_table) &&
           execute_sql(create_replies_table);
}

bool Database::execute_sql(const std::string& sql) {
    if (!db) {
        std::cerr << "数据库连接未初始化" << std::endl;
        return false;
    }
    
    char* error_msg = nullptr;
    int rc = sqlite3_exec(db, sql.c_str(), nullptr, nullptr, &error_msg);
    
    if (rc != SQLITE_OK) {
        std::cerr << "SQL错误: " << error_msg << std::endl;
        sqlite3_free(error_msg);
        return false;
    }
    
    return true;
}

bool Database::create_user(const std::string& username, const std::string& password) {
    std::lock_guard<std::mutex> lock(db_mutex);
    if (!db) {
        std::cerr << "数据库连接未初始化" << std::endl;
        return false;
    }
    
    // 对密码进行哈希处理
    std::string hashed_password = hash_password(password);
    if (hashed_password.empty()) {
        std::cerr << "密码哈希失败" << std::endl;
        return false;
    }
    
    std::string sql = "INSERT INTO users (username, password) VALUES (?, ?);";
    
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }
    
    sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, hashed_password.c_str(), -1, SQLITE_STATIC);
    
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    return rc == SQLITE_DONE;
}

bool Database::user_exists(const std::string& username) {
    if (!db) {
        std::cerr << "数据库连接未初始化" << std::endl;
        return false;
    }
    
    std::string sql = "SELECT user_id FROM users WHERE username = ?;";
    
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }
    
    sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_STATIC);
    
    bool exists = (sqlite3_step(stmt) == SQLITE_ROW);
    sqlite3_finalize(stmt);
    
    return exists;
}

bool Database::verify_user(const std::string& username, const std::string& password, User& user) {
    if (!db) {
        std::cerr << "数据库连接未初始化" << std::endl;
        return false;
    }
    
    // 先根据用户名查询用户
    std::string sql = "SELECT user_id, username, password FROM users WHERE username = ?;";
    
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }
    
    sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_STATIC);
    
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        user.user_id = sqlite3_column_int(stmt, 0);
        user.username = safe_sqlite3_text(stmt, 1);
        std::string stored_hash = safe_sqlite3_text(stmt, 2);
        sqlite3_finalize(stmt);
        
        // 验证密码
        if (verify_password(password, stored_hash)) {
            user.password = stored_hash;
            return true;
        }
    } else {
        sqlite3_finalize(stmt);
    }
    
    return false;
}

bool Database::save_message(const Message& message) {
    std::lock_guard<std::mutex> lock(db_mutex);
    std::string sql = "INSERT INTO messages (sender_id, receiver_id, group_id, content, type, timestamp) VALUES (?, ?, ?, ?, ?, ?);";
    
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }
    
    sqlite3_bind_int(stmt, 1, message.sender_id);
    if (message.receiver_id == -1) {
        sqlite3_bind_null(stmt, 2);
    } else {
        sqlite3_bind_int(stmt, 2, message.receiver_id);
    }
    
    if (message.group_id == -1) {
        sqlite3_bind_null(stmt, 3);
    } else {
        sqlite3_bind_int(stmt, 3, message.group_id);
    }
    
    sqlite3_bind_text(stmt, 4, message.content.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 5, message.type.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 6, message.timestamp.c_str(), -1, SQLITE_STATIC);
    
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    return rc == SQLITE_DONE;
}

std::vector<Message> Database::get_messages(int user_id, int limit, int before_id) {
    std::lock_guard<std::mutex> lock(db_mutex);
    std::vector<Message> messages;
    std::string sql;
    
    if (!db) {
        std::cerr << "数据库连接未初始化" << std::endl;
        return messages;
    }
    
    if (before_id > 0) {
        // 获取指定消息ID之前的消息（用于无限滚动加载）
        sql = "SELECT message_id, sender_id, receiver_id, group_id, content, type, timestamp "
              "FROM messages WHERE (sender_id = ? OR receiver_id = ?) AND message_id < ? "
              "ORDER BY message_id DESC LIMIT ?;";
    } else {
        // 获取最新的消息
        sql = "SELECT message_id, sender_id, receiver_id, group_id, content, type, timestamp "
              "FROM messages WHERE sender_id = ? OR receiver_id = ? "
              "ORDER BY message_id DESC LIMIT ?;";
    }
    
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        return messages;
    }
    
    sqlite3_bind_int(stmt, 1, user_id);
    sqlite3_bind_int(stmt, 2, user_id);
    
    if (before_id > 0) {
        sqlite3_bind_int(stmt, 3, before_id);
        sqlite3_bind_int(stmt, 4, limit);
    } else {
        sqlite3_bind_int(stmt, 3, limit);
    }
    
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        Message message;
        message.message_id = sqlite3_column_int(stmt, 0);
        message.sender_id = sqlite3_column_int(stmt, 1);
        
        if (sqlite3_column_type(stmt, 2) == SQLITE_NULL) {
            message.receiver_id = -1;
        } else {
            message.receiver_id = sqlite3_column_int(stmt, 2);
        }
        
        if (sqlite3_column_type(stmt, 3) == SQLITE_NULL) {
            message.group_id = -1;
        } else {
            message.group_id = sqlite3_column_int(stmt, 3);
        }
        
        message.content = safe_sqlite3_text(stmt, 4);
        message.type = safe_sqlite3_text(stmt, 5);
        message.timestamp = safe_sqlite3_text(stmt, 6);
        
        messages.push_back(message);
    }
    
    sqlite3_finalize(stmt);
    
    // 反转顺序，使消息按时间正序排列
    std::reverse(messages.begin(), messages.end());
    
    return messages;
}

std::vector<Message> Database::get_messages_before(int user_id, int before_id, int limit) {
    return get_messages(user_id, limit, before_id);
}

std::vector<User> Database::get_user_contacts(int user_id) {
    std::vector<User> contacts;
    
    // 获取所有用户（除了当前用户自己）
    std::string sql = R"(
        SELECT user_id, username, online
        FROM users
        WHERE user_id != ?
        ORDER BY username;
    )";
    
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        return contacts;
    }
    
    sqlite3_bind_int(stmt, 1, user_id);
    
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        User contact;
        contact.user_id = sqlite3_column_int(stmt, 0);
        contact.username = safe_sqlite3_text(stmt, 1);
        contact.online = sqlite3_column_int(stmt, 2) ? true : false;
        contact.password = ""; // 不返回密码信息
        contact.socket_fd = -1;
        contact.token = "";
        
        contacts.push_back(contact);
    }
    
    sqlite3_finalize(stmt);
    return contacts;
}

bool Database::create_post(const Post& post) {
    std::lock_guard<std::mutex> lock(db_mutex);
    std::string sql = "INSERT INTO posts (user_id, title, content, timestamp) VALUES (?, ?, ?, ?);";
    
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }
    
    sqlite3_bind_int(stmt, 1, post.user_id);
    sqlite3_bind_text(stmt, 2, post.title.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, post.content.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 4, post.timestamp.c_str(), -1, SQLITE_STATIC);
    
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    return rc == SQLITE_DONE;
}
std::vector<Post> Database::get_posts(int page, int page_size) {
    std::lock_guard<std::mutex> lock(db_mutex);
    std::vector<Post> posts;
    
    if (!db) {
        std::cerr << "数据库连接未初始化" << std::endl;
        return posts;
    }
    
    // 计算偏移量
    int offset = (page - 1) * page_size;
    if (offset < 0) offset = 0;
    
    std::string sql = "SELECT post_id, user_id, title, content, timestamp FROM posts "
                      "ORDER BY post_id DESC LIMIT ? OFFSET ?;";
    
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        return posts;
    }
    
    sqlite3_bind_int(stmt, 1, page_size);
    sqlite3_bind_int(stmt, 2, offset);
    
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        Post post;
        post.post_id = sqlite3_column_int(stmt, 0);
        post.user_id = sqlite3_column_int(stmt, 1);
        post.title = safe_sqlite3_text(stmt, 2);
        post.content = safe_sqlite3_text(stmt, 3);
        post.timestamp = safe_sqlite3_text(stmt, 4);
        
        posts.push_back(post);
    }
    
    sqlite3_finalize(stmt);
    return posts;
}

std::vector<Post> Database::get_posts_page(int page, int page_size) {
    return get_posts(page, page_size);
}

bool Database::reply_post(int post_id, int user_id, const std::string& content, const std::string& timestamp) {
    std::string sql = "INSERT INTO replies (post_id, user_id, content, timestamp) VALUES (?, ?, ?, ?);";
    
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }
    
    sqlite3_bind_int(stmt, 1, post_id);
    sqlite3_bind_int(stmt, 2, user_id);
    sqlite3_bind_text(stmt, 3, content.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 4, timestamp.c_str(), -1, SQLITE_STATIC);
    
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    return rc == SQLITE_DONE;
}

std::vector<Reply> Database::get_post_replies(int post_id) {
    std::vector<Reply> replies;
    std::string sql = "SELECT reply_id, post_id, user_id, content, timestamp FROM replies WHERE post_id = ? ORDER BY timestamp;";
    
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        return replies;
    }
    
    sqlite3_bind_int(stmt, 1, post_id);
    
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        Reply reply;
        reply.reply_id = sqlite3_column_int(stmt, 0);
        reply.post_id = sqlite3_column_int(stmt, 1);
        reply.user_id = sqlite3_column_int(stmt, 2);
        reply.content = safe_sqlite3_text(stmt, 3);
        reply.timestamp = safe_sqlite3_text(stmt, 4);
        replies.push_back(reply);
    }
    
    sqlite3_finalize(stmt);
    return replies;
}

std::vector<Message> Database::get_group_messages(int group_id, int limit, int before_id) {
    std::vector<Message> messages;
    std::string sql;
    
    if (!db) {
        std::cerr << "数据库连接未初始化" << std::endl;
        return messages;
    }
    
    if (before_id > 0) {
        // 获取指定消息ID之前的消息（用于无限滚动加载）
        sql = "SELECT message_id, sender_id, receiver_id, group_id, content, type, timestamp "
              "FROM messages WHERE group_id = ? AND message_id < ? "
              "ORDER BY message_id DESC LIMIT ?;";
    } else {
        // 获取最新的消息
        sql = "SELECT message_id, sender_id, receiver_id, group_id, content, type, timestamp "
              "FROM messages WHERE group_id = ? "
              "ORDER BY message_id DESC LIMIT ?;";
    }
    
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        return messages;
    }
    
    sqlite3_bind_int(stmt, 1, group_id);
    
    if (before_id > 0) {
        sqlite3_bind_int(stmt, 2, before_id);
        sqlite3_bind_int(stmt, 3, limit);
    } else {
        sqlite3_bind_int(stmt, 2, limit);
    }
    
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        Message message;
        message.message_id = sqlite3_column_int(stmt, 0);
        message.sender_id = sqlite3_column_int(stmt, 1);
        
        if (sqlite3_column_type(stmt, 2) == SQLITE_NULL) {
            message.receiver_id = -1;
        } else {
            message.receiver_id = sqlite3_column_int(stmt, 2);
        }
        
        message.group_id = sqlite3_column_int(stmt, 3);
        message.content = safe_sqlite3_text(stmt, 4);
        message.type = safe_sqlite3_text(stmt, 5);
        message.timestamp = safe_sqlite3_text(stmt, 6);
        
        messages.push_back(message);
    }
    
    sqlite3_finalize(stmt);
    
    // 反转顺序，使消息按时间正序排列
    std::reverse(messages.begin(), messages.end());
    
    return messages;
}

std::vector<Message> Database::get_group_messages_before(int group_id, int before_id, int limit) {
    return get_group_messages(group_id, limit, before_id);
}

bool Database::create_group(const Group& group) {
    std::string sql = "INSERT INTO groups (group_name, description, creator_id, created_time) VALUES (?, ?, ?, ?);";
    
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }
    
    sqlite3_bind_text(stmt, 1, group.group_name.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, group.description.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 3, group.creator_id);
    sqlite3_bind_text(stmt, 4, group.created_time.c_str(), -1, SQLITE_STATIC);
    
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    if (rc == SQLITE_DONE) {
        // 自动让创建者加入群组
        int group_id = sqlite3_last_insert_rowid(db);
        return join_group(group.creator_id, group_id);
    }
    
    return false;
}

std::vector<Group> Database::get_all_groups() {
    std::vector<Group> groups;
    std::string sql = "SELECT group_id, group_name, description, creator_id, created_time FROM groups ORDER BY created_time;";
    
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        return groups;
    }
    
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        Group group;
        group.group_id = sqlite3_column_int(stmt, 0);
        group.group_name = safe_sqlite3_text(stmt, 1);
        group.description = safe_sqlite3_text(stmt, 2);
        group.creator_id = sqlite3_column_int(stmt, 3);
        group.created_time = safe_sqlite3_text(stmt, 4);
        
        groups.push_back(group);
    }
    
    sqlite3_finalize(stmt);
    return groups;
}

std::vector<Group> Database::get_user_groups(int user_id) {
    std::vector<Group> groups;
    std::string sql = R"(
        SELECT g.group_id, g.group_name, g.description, g.creator_id, g.created_time 
        FROM groups g 
        INNER JOIN group_members gm ON g.group_id = gm.group_id 
        WHERE gm.user_id = ? 
        ORDER BY g.created_time;
    )";
    
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        return groups;
    }
    
    sqlite3_bind_int(stmt, 1, user_id);
    
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        Group group;
        group.group_id = sqlite3_column_int(stmt, 0);
        group.group_name = safe_sqlite3_text(stmt, 1);
        group.description = safe_sqlite3_text(stmt, 2);
        group.creator_id = sqlite3_column_int(stmt, 3);
        group.created_time = safe_sqlite3_text(stmt, 4);
        
        groups.push_back(group);
    }
    
    sqlite3_finalize(stmt);
    return groups;
}

bool Database::join_group(int user_id, int group_id) {
    std::string sql = "INSERT INTO group_members (group_id, user_id, joined_time) VALUES (?, ?, datetime('now'));";
    
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }
    
    sqlite3_bind_int(stmt, 1, group_id);
    sqlite3_bind_int(stmt, 2, user_id);
    
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    return rc == SQLITE_DONE;
}

bool Database::leave_group(int user_id, int group_id) {
    std::string sql = "DELETE FROM group_members WHERE group_id = ? AND user_id = ?;";
    
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }
    
    sqlite3_bind_int(stmt, 1, group_id);
    sqlite3_bind_int(stmt, 2, user_id);
    
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    return rc == SQLITE_DONE;
}

bool Database::is_user_in_group(int user_id, int group_id) {
    std::string sql = "SELECT 1 FROM group_members WHERE group_id = ? AND user_id = ?;";
    
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }
    
    sqlite3_bind_int(stmt, 1, group_id);
    sqlite3_bind_int(stmt, 2, user_id);
    
    bool exists = (sqlite3_step(stmt) == SQLITE_ROW);
    sqlite3_finalize(stmt);
    
    return exists;
}

// 新增：通过用户ID获取用户名
std::string Database::get_username_by_id(int user_id) {
    std::string sql = "SELECT username FROM users WHERE user_id = ?;";
    
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        return "";
    }
    
    sqlite3_bind_int(stmt, 1, user_id);
    
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        std::string username = safe_sqlite3_text(stmt, 0);
        sqlite3_finalize(stmt);
        return username;
    }
    
    sqlite3_finalize(stmt);
    return "";
}

// 新增：通过帖子ID获取帖子详情
Post Database::get_post_by_id(int post_id) {
    Post post;
    std::string sql = "SELECT p.post_id, p.user_id, u.username, p.title, p.content, p.timestamp "
                     "FROM posts p JOIN users u ON p.user_id = u.user_id WHERE p.post_id = ?;";
    
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        return post;
    }
    
    sqlite3_bind_int(stmt, 1, post_id);
    
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        post.post_id = sqlite3_column_int(stmt, 0);
        post.user_id = sqlite3_column_int(stmt, 1);
        post.username = safe_sqlite3_text(stmt, 2);
        post.title = safe_sqlite3_text(stmt, 3);
        post.content = safe_sqlite3_text(stmt, 4);
        post.timestamp = safe_sqlite3_text(stmt, 5);
    }
    
    sqlite3_finalize(stmt);
    return post;
}
