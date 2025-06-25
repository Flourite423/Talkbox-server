#include "common.h"
#include <ctime>
#include <sstream>
#include <iostream>

// 获取当前时间戳
std::string get_current_timestamp() {
    time_t now = time(nullptr);
    char buffer[80];
    struct tm* timeinfo = localtime(&now);
    strftime(buffer, sizeof(buffer), "%a %b %d %H:%M:%S %Y", timeinfo);
    return std::string(buffer);
}

// 从JSON字符串中解析值
std::string parse_json_value(const std::string& json, const std::string& key) {
    std::string search_key = "\"" + key + "\"";
    size_t pos = json.find(search_key);
    if (pos == std::string::npos) {
        return "";
    }
    
    pos = json.find(":", pos);
    if (pos == std::string::npos) {
        return "";
    }
    
    // 跳过冒号和空白字符
    pos++;
    while (pos < json.length() && (json[pos] == ' ' || json[pos] == '\t')) {
        pos++;
    }
    
    if (pos >= json.length()) {
        return "";
    }
    
    // 处理字符串类型的值
    if (json[pos] == '\"') {
        size_t start = pos + 1;
        size_t end = json.find("\"", start);
        while (end != std::string::npos && json[end-1] == '\\') {
            end = json.find("\"", end + 1);
        }
        
        if (end == std::string::npos) {
            return "";
        }
        
        return json.substr(start, end - start);
    }
    
    // 处理数字或其他非字符串类型的值
    size_t end = json.find_first_of(",}\n", pos);
    if (end == std::string::npos) {
        return json.substr(pos);
    }
    
    return json.substr(pos, end - pos);
}

// 创建JSON响应
std::string create_json_response(const std::string& status, const std::string& data) {
    std::ostringstream oss;
    oss << "HTTP/1.1 200 OK\r\n";
    oss << "Content-Type: application/json\r\n";
    oss << "Access-Control-Allow-Origin: *\r\n";  // 允许跨域
    oss << "\r\n";
    
    if (data.empty()) {
        oss << "{\"status\":\"" << status << "\",\"data\":\"\"}";
    } else if (data[0] == '{' || data[0] == '[') {
        oss << "{\"status\":\"" << status << "\",\"data\":" << data << "}";
    } else {
        oss << "{\"status\":\"" << status << "\",\"data\":\"" << data << "\"}";
    }
    
    return oss.str();
}
