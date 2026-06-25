#include "logger.h"
#include <iomanip>
#include <sstream>

Logger& Logger::getInstance() {
    static Logger instance;
    return instance;
}

Logger::Logger() : current_level(LogLevel::INFO), console_output(true) {
}

Logger::~Logger() {
    if (log_file.is_open()) {
        log_file.close();
    }
}

void Logger::setLogLevel(LogLevel level) {
    current_level = level;
}

void Logger::setLogFile(const std::string& filename) {
    std::lock_guard<std::mutex> lock(log_mutex);
    if (log_file.is_open()) {
        log_file.close();
    }
    log_file.open(filename, std::ios::app);
}

void Logger::debug(const std::string& message) {
    log(LogLevel::DEBUG, message);
}

void Logger::info(const std::string& message) {
    log(LogLevel::INFO, message);
}

void Logger::warning(const std::string& message) {
    log(LogLevel::WARNING, message);
}

void Logger::error(const std::string& message) {
    log(LogLevel::ERROR, message);
}

void Logger::log(LogLevel level, const std::string& message) {
    if (level < current_level) {
        return;
    }
    
    std::lock_guard<std::mutex> lock(log_mutex);
    
    std::string timestamp = getCurrentTimestamp();
    std::string level_str = logLevelToString(level);
    std::string log_message = "[" + timestamp + "] [" + level_str + "] " + message;
    
    if (console_output) {
        if (level >= LogLevel::WARNING) {
            std::cerr << log_message << std::endl;
        } else {
            std::cout << log_message << std::endl;
        }
    }
    
    if (log_file.is_open()) {
        log_file << log_message << std::endl;
    }
}

std::string Logger::getCurrentTimestamp() {
    auto now = std::time(nullptr);
    auto* local_time = std::localtime(&now);
    
    std::ostringstream oss;
    oss << std::put_time(local_time, "%Y-%m-%d %H:%M:%S");
    return oss.str();
}

std::string Logger::logLevelToString(LogLevel level) {
    switch (level) {
        case LogLevel::DEBUG:   return "DEBUG";
        case LogLevel::INFO:    return "INFO";
        case LogLevel::WARNING: return "WARNING";
        case LogLevel::ERROR:   return "ERROR";
        default:                return "UNKNOWN";
    }
}
