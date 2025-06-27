#ifndef FILE_MANAGER_H
#define FILE_MANAGER_H

#include "common.h"
#include <string>
#include <memory>

// 前向声明
class UserManager;

class FileManager {
public:
    FileManager(const std::string& upload_dir, UserManager* user_manager);
    ~FileManager();
    
    // 文件API
    std::string upload_file(const std::string& body);
    std::string download_file(const std::string& query_string);
    
private:
    std::string upload_dir;
    UserManager* user_manager;
    
    // 工具函数
    bool ensure_upload_dir_exists();
};

#endif // FILE_MANAGER_H
