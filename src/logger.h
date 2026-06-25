#ifndef LOGGER_H
#define LOGGER_H

#include <string>
#include <fstream>
#include <mutex>
#include <iostream>
#include <ctime>

enum class LogLevel {
    DEBUG,
    INFO,
    WARNING,
    ERROR
};

class Logger {
public:
    static Logger& getInstance();
    
    void setLogLevel(LogLevel level);
    void setLogFile(const std::string& filename);
    
    void debug(const std::string& message);
    void info(const std::string& message);
    void warning(const std::string& message);
    void error(const std::string& message);
    
private:
    Logger();
    ~Logger();
    
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;
    
    void log(LogLevel level, const std::string& message);
    std::string getCurrentTimestamp();
    std::string logLevelToString(LogLevel level);
    
    LogLevel current_level;
    std::ofstream log_file;
    std::mutex log_mutex;
    bool console_output;
};

// 便捷宏
#define LOG_DEBUG(msg) Logger::getInstance().debug(msg)
#define LOG_INFO(msg) Logger::getInstance().info(msg)
#define LOG_WARNING(msg) Logger::getInstance().warning(msg)
#define LOG_ERROR(msg) Logger::getInstance().error(msg)

#endif // LOGGER_H
