#include "common.h"
#include <ctime>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <random>
#include <algorithm>
#include <openssl/evp.h>
#include <openssl/rand.h>
// 获取当前时间戳
std::string get_current_timestamp() {
    time_t now = time(nullptr);
    char buffer[80];
    struct tm timeinfo;
    localtime_r(&now, &timeinfo);
    strftime(buffer, sizeof(buffer), "%a %b %d %H:%M:%S %Y", &timeinfo);
    return std::string(buffer);
}

// 从JSON字符串中解析值
std::string parse_json_value(const std::string& json, const std::string& key) {
    // Debug: std::cout << "Parsing JSON: " << json << " for key: " << key << std::endl;
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
    std::string json_body;
    
    if (data.empty()) {
        json_body = "{\"status\":\"" + status + "\",\"data\":\"\"}";
    } else if (data[0] == '{' || data[0] == '[') {
        json_body = "{\"status\":\"" + status + "\",\"data\":" + data + "}";
    } else {
        json_body = "{\"status\":\"" + status + "\",\"data\":\"" + data + "\"}";
    }
    
    // 根据 status 确定 HTTP 状态码
    std::string http_status;
    if (status == "success") {
        http_status = "HTTP/1.1 200 OK";
    } else {
        http_status = "HTTP/1.1 400 Bad Request";
    }
    
    std::ostringstream oss;
    oss << http_status << "\r\n";
    oss << "Content-Type: application/json\r\n";
    oss << "Content-Length: " << json_body.length() << "\r\n";
    oss << "Connection: close\r\n";
    oss << "Access-Control-Allow-Origin: *\r\n";
    oss << "\r\n";
    oss << json_body;
    
    return oss.str();
}

// 安全的字符串转整数
int safe_stoi(const std::string& s, int default_val) {
    if (s.empty()) return default_val;
    try {
        return std::stoi(s);
    } catch (const std::exception&) {
        return default_val;
    }
}

// JSON 字符串转义
std::string escape_json_string(const std::string& s) {
    std::string result;
    result.reserve(s.length() + 10);
    for (char c : s) {
        switch (c) {
            case '"':  result += "\\\""; break;
            case '\\': result += "\\\\"; break;
            case '\b': result += "\\b"; break;
            case '\f': result += "\\f"; break;
            case '\n': result += "\\n"; break;
            case '\r': result += "\\r"; break;
            case '\t': result += "\\t"; break;
            default:
                if (static_cast<unsigned char>(c) < 0x20) {
                    char buf[8];
                    snprintf(buf, sizeof(buf), "\\u%04x", static_cast<unsigned char>(c));
                    result += buf;
                } else {
                    result += c;
                }
        }
    }
    return result;
}

// Base64 编码
static const char base64_chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

std::string base64_encode(const std::string& input) {
    std::string result;
    int i = 0;
    int j = 0;
    unsigned char char_array_3[3];
    unsigned char char_array_4[4];
    int in_len = input.length();
    const char* bytes_to_encode = input.c_str();
    
    while (in_len--) {
        char_array_3[i++] = *(bytes_to_encode++);
        if (i == 3) {
            char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
            char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
            char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
            char_array_4[3] = char_array_3[2] & 0x3f;
            
            for (i = 0; i < 4; i++)
                result += base64_chars[char_array_4[i]];
            i = 0;
        }
    }
    
    if (i) {
        for (j = i; j < 3; j++)
            char_array_3[j] = '\0';
        
        char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
        char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
        char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
        
        for (j = 0; j < i + 1; j++)
            result += base64_chars[char_array_4[j]];
        
        while (i++ < 3)
            result += '=';
    }
    
    return result;
}

// Base64 解码
std::string base64_decode(const std::string& encoded_string) {
    int in_len = encoded_string.length();
    int i = 0;
    int j = 0;
    int in_ = 0;
    unsigned char char_array_4[4], char_array_3[3];
    std::string ret;
    
    auto is_base64 = [](unsigned char c) -> bool {
        return (isalnum(c) || (c == '+') || (c == '/'));
    };
    
    while (in_len-- && (encoded_string[in_] != '=') && is_base64(encoded_string[in_])) {
        char_array_4[i++] = encoded_string[in_]; in_++;
        if (i == 4) {
            for (i = 0; i < 4; i++)
                char_array_4[i] = static_cast<unsigned char>(std::find(base64_chars, base64_chars + 64, char_array_4[i]) - base64_chars);
            
            char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
            char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
            char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];
            
            for (i = 0; i < 3; i++)
                ret += char_array_3[i];
            i = 0;
        }
    }
    
    if (i) {
        for (j = 0; j < i; j++)
            char_array_4[j] = static_cast<unsigned char>(std::find(base64_chars, base64_chars + 64, char_array_4[j]) - base64_chars);
        
        char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
        char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
        
        for (j = 0; j < i - 1; j++)
            ret += char_array_3[j];
    }
    
    return ret;
}

// 密码哈希函数 (PBKDF2-HMAC-SHA256)
// 存储格式: iterations:hex(salt):hex(hash)
std::string hash_password(const std::string& password) {
    const int ITERATIONS = 100000;
    const int SALT_LEN = 16;
    const int HASH_LEN = 32;
    
    // 生成随机盐
    unsigned char salt[SALT_LEN];
    if (RAND_bytes(salt, SALT_LEN) != 1) {
        std::cerr << "生成随机盐失败" << std::endl;
        return "";
    }
    
    // 计算 PBKDF2-HMAC-SHA256
    unsigned char hash[HASH_LEN];
    if (PKCS5_PBKDF2_HMAC(password.c_str(), password.length(),
                           salt, SALT_LEN,
                           ITERATIONS, EVP_sha256(),
                           HASH_LEN, hash) != 1) {
        std::cerr << "PBKDF2 计算失败" << std::endl;
        return "";
    }
    
    // 转换为十六进制字符串
    std::ostringstream oss;
    oss << ITERATIONS << ":";
    for (int i = 0; i < SALT_LEN; ++i) {
        oss << std::hex << std::setw(2) << std::setfill('0') << (int)salt[i];
    }
    oss << ":";
    for (int i = 0; i < HASH_LEN; ++i) {
        oss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
    }
    
    return oss.str();
}

bool verify_password(const std::string& password, const std::string& stored_hash) {
    // 解析存储的哈希: iterations:hex(salt):hex(hash)
    std::istringstream iss(stored_hash);
    std::string iterations_str, salt_hex, hash_hex;
    
    if (!std::getline(iss, iterations_str, ':') ||
        !std::getline(iss, salt_hex, ':') ||
        !std::getline(iss, hash_hex)) {
        return false;
    }
    
    int iterations;
    try {
        iterations = std::stoi(iterations_str);
    } catch (...) {
        return false;
    }
    
    // 解析盐
    const int SALT_LEN = 16;
    unsigned char salt[SALT_LEN];
    try {
        for (int i = 0; i < SALT_LEN; ++i) {
            salt[i] = (unsigned char)std::stoi(salt_hex.substr(i * 2, 2), nullptr, 16);
        }
    } catch (...) {
        return false;
    }
    
    // 解析原始哈希
    const int HASH_LEN = 32;
    unsigned char expected_hash[HASH_LEN];
    try {
        for (int i = 0; i < HASH_LEN; ++i) {
            expected_hash[i] = (unsigned char)std::stoi(hash_hex.substr(i * 2, 2), nullptr, 16);
        }
    } catch (...) {
        return false;
    }
    
    // 计算输入密码的哈希
    unsigned char computed_hash[HASH_LEN];
    if (PKCS5_PBKDF2_HMAC(password.c_str(), password.length(),
                           salt, SALT_LEN,
                           iterations, EVP_sha256(),
                           HASH_LEN, computed_hash) != 1) {
        return false;
    }
    
    // 比较哈希（常量时间比较防止时序攻击）
    int result = 0;
    for (int i = 0; i < HASH_LEN; ++i) {
        result |= expected_hash[i] ^ computed_hash[i];
    }
    
    return result == 0;
}
