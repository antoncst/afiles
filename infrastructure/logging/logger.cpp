#include "logger.h"
#include <iostream>
#include <iomanip>
#include <chrono>

namespace infrastructure::logging {

Logger& Logger::getInstance() {
    static Logger instance;
    return instance;
}

void Logger::setLogFile(const std::string& filePath) {
    std::lock_guard<std::mutex> lock(logMutex);
    if (logFile.is_open()) {
        logFile.close();
    }
    logFile.open(filePath, std::ios::app);
}

void Logger::log(LogLevel level, const std::string& message) {
    std::lock_guard<std::mutex> lock(logMutex);

    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);

    std::stringstream ss;
    ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S")
       << " [" << getLevelString(level) << "] " << message;

    //std::cout << ss.str() << std::endl;

    if (logFile.is_open()) {
        logFile << ss.str() << std::endl;
    }
}

Logger::~Logger() {
    if (logFile.is_open()) {
        logFile.close();
    }
}

std::string Logger::getLevelString(LogLevel level) const {
    switch (level) {

    case LogLevel::Debug: return "DEBUG";
    case LogLevel::Info: return "INFO";
    case LogLevel::Warning: return "WARNING";
    case LogLevel::Error: return "ERROR";
    default: return "UNKNOWN";
    }
}

} // namespace infrastructure::logging

