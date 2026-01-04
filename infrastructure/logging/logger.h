#ifndef LOGGER_H
#define LOGGER_H

#include <string>
#include <fstream>
#include <mutex>

namespace infrastructure::logging {

enum class LogLevel {
    Debug,
    Info,
    Warning,
    Error
};

class Logger {
public:
    static Logger& getInstance();

    void setLogFile(const std::string& filePath);
    void log(LogLevel level, const std::string& message);

private:
    Logger() = default;
    ~Logger();

    std::string getLevelString(LogLevel level) const;

    std::ofstream logFile;
    std::mutex logMutex;
};

#define LOG_DEBUG(msg) Logger::getInstance().log(infrastructure::logging::LogLevel::Debug, msg)
#define LOG_INFO(msg) Logger::getInstance().log(infrastructure::logging::LogLevel::Info, msg)
#define LOG_WARNING(msg) Logger::getInstance().log(infrastructure::logging::LogLevel::Warning, msg)
#define LOG_ERROR(msg) Logger::getInstance().log(infrastructure::logging::LogLevel::Error, msg)

} // namespace infrastructure::logging


#endif // LOGGER_H
