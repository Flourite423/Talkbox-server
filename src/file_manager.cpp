#include "file_manager.h"
#include "user_manager.h"
#include "common.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <sys/stat.h>

FileManager::FileManager(const std::string& upload_dir, UserManager* user_manager)
    : upload_dir(upload_dir), user_manager(user_manager) {
    ensure_upload_dir_exists();
}

FileManager::~FileManager() {
}

std::string FileManager::upload_file(const std::string& body) {
    std::string username = parse_json_value(body, "username");
    if (username.empty()) {
        return create_json_response("error", "用户名不能为空");
    }
    
    int user_id = user_manager->get_user_id_by_username(username);
    if (user_id == -1) {
        return create_json_response("error", "无效的用户名");
    }
    
    std::string filename = parse_json_value(body, "filename");
    std::string data = parse_json_value(body, "data");
    
    if (filename.empty() || data.empty()) {
        return create_json_response("error", "文件名和数据不能为空");
    }
    
    if (!ensure_upload_dir_exists()) {
        return create_json_response("error", "创建上传目录失败");
    }
    
    std::string file_path = upload_dir + "/" + filename;
    std::ofstream file(file_path, std::ios::binary);
    
    if (!file) {
        return create_json_response("error", "无法创建文件");
    }
    
    file << data;
    file.close();
    
    return create_json_response("success", "文件上传成功");
}

std::string FileManager::download_file(const std::string& query_string) {
    // 解析查询参数中的filename
    std::string filename = "";
    if (!query_string.empty()) {
        std::string search_key = "filename=";
        size_t pos = query_string.find(search_key);
        if (pos != std::string::npos) {
            pos += search_key.length();
            size_t end = query_string.find('&', pos);
            if (end == std::string::npos) {
                end = query_string.length();
            }
            filename = query_string.substr(pos, end - pos);
        }
    }
    
    if (filename.empty()) {
        return create_json_response("error", "文件名不能为空");
    }
    
    std::string file_path = upload_dir + "/" + filename;
    std::ifstream file(file_path, std::ios::binary);
    
    if (!file) {
        return create_json_response("error", "文件不存在");
    }
    
    std::ostringstream ss;
    ss << file.rdbuf();
    std::string data = ss.str();
    
    return create_json_response("success", data);
}

bool FileManager::ensure_upload_dir_exists() {
    struct stat st;
    
    if (stat(upload_dir.c_str(), &st) == 0) {
        return S_ISDIR(st.st_mode);
    }
    
    // 创建目录
    return mkdir(upload_dir.c_str(), 0755) == 0;
}
