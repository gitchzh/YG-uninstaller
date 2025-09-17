/**
 * @file Logger.cpp
 * @brief 日志系统实现
 * @author YG Software
 * @version 1.0.0
 * @date 2025-09-17
 */

#include "core/Logger.h"
#include <iostream>
#include <ctime>

namespace YG {
    
    Logger& Logger::GetInstance() {
        static Logger instance;
        return instance;
    }
    
    Logger::Logger() : m_level(LogLevel::Info), m_consoleOutput(true), 
                      m_fileOutput(false), m_initialized(false) {
    }
    
    Logger::~Logger() {
        Shutdown();
    }
    
    ErrorCode Logger::Initialize(const String& logFilePath, LogLevel level,
                               size_t maxFileSize, int maxBackupFiles) {
        std::lock_guard<std::mutex> lock(m_mutex);
        
        m_logFilePath = logFilePath;
        m_level = level;
        m_maxFileSize = maxFileSize;
        m_maxBackupFiles = maxBackupFiles;
        m_initialized = true;
        
        return ErrorCode::Success;
    }
    
    void Logger::Shutdown() {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_fileStream.is_open()) {
            m_fileStream.close();
        }
        m_initialized = false;
    }
    
    void Logger::SetLevel(LogLevel level) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_level = level;
    }
    
    LogLevel Logger::GetLevel() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_level;
    }
    
    void Logger::EnableConsoleOutput(bool enable) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_consoleOutput = enable;
    }
    
    void Logger::EnableFileOutput(bool enable) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_fileOutput = enable;
    }
    
    void Logger::Debug(const String& message, const char* file, 
                      int line, const char* function) {
        WriteLog(LogLevel::Debug, message, file, line, function);
    }
    
    void Logger::Info(const String& message, const char* file, 
                     int line, const char* function) {
        WriteLog(LogLevel::Info, message, file, line, function);
    }
    
    void Logger::Warning(const String& message, const char* file, 
                        int line, const char* function) {
        WriteLog(LogLevel::Warning, message, file, line, function);
    }
    
    void Logger::Error(const String& message, const char* file, 
                      int line, const char* function) {
        WriteLog(LogLevel::Error, message, file, line, function);
    }
    
    void Logger::Fatal(const String& message, const char* file, 
                      int line, const char* function) {
        WriteLog(LogLevel::Fatal, message, file, line, function);
    }
    
    void Logger::Flush() {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_fileStream.is_open()) {
            m_fileStream.flush();
        }
    }
    
    String Logger::GetLogFilePath() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_logFilePath;
    }
    
    ErrorCode Logger::ClearLogFile() {
        // 简单实现
        return ErrorCode::Success;
    }
    
    void Logger::WriteLog(LogLevel level, const String& message, 
                         const char* file, int line, const char* function) {
        if (level < m_level) {
            return;
        }
        
        std::lock_guard<std::mutex> lock(m_mutex);
        
        String formattedMessage = FormatMessage(level, message, file, line, function);
        
        // 控制台输出
        if (m_consoleOutput) {
            std::wcout << formattedMessage << std::endl;
        }
        
        // 文件输出（简化实现）
        if (m_fileOutput && !m_logFilePath.empty()) {
            // TODO: 实现文件输出
        }
    }
    
    String Logger::FormatMessage(LogLevel level, const String& message, 
                               const char* file, int line, const char* function) const {
        String timeStr = GetCurrentTimeString();
        String levelStr = GetLevelString(level);
        
        // 简单格式化
        return L"[" + timeStr + L"] [" + levelStr + L"] " + message;
    }
    
    String Logger::GetLevelString(LogLevel level) const {
        switch (level) {
            case LogLevel::Debug:   return L"DEBUG";
            case LogLevel::Info:    return L"INFO";
            case LogLevel::Warning: return L"WARN";
            case LogLevel::Error:   return L"ERROR";
            case LogLevel::Fatal:   return L"FATAL";
            default:                return L"UNKNOWN";
        }
    }
    
    String Logger::GetCurrentTimeString() const {
        time_t now = time(0);
        wchar_t buffer[64];
        wcsftime(buffer, sizeof(buffer)/sizeof(wchar_t), L"%H:%M:%S", localtime(&now));
        return String(buffer);
    }
    
    void Logger::RotateLogFile() {
        // TODO: 实现日志轮转
    }
    
    bool Logger::CreateLogDirectory(const String& filePath) {
        // TODO: 实现目录创建
        return true;
    }
    
} // namespace YG
