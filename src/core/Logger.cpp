/**
 * @file Logger.cpp
 * @brief 日志系统实现
 * @author gmrchzh@gmail.com
 * @version 1.0.1
 * @date 2025-09-17
 */

#include "core/Logger.h"
#include <iostream>
#include <ctime>
#include <shlwapi.h>
#include <io.h>
#include <fcntl.h>
#include <algorithm>

// 只在MSVC中使用pragma comment
#ifdef _MSC_VER
#pragma comment(lib, "shlwapi.lib")
#endif

namespace YG {
    
    Logger& Logger::GetInstance() {
        static Logger instance;
        return instance;
    }
    
    Logger::Logger() : m_level(LogLevel::Info), m_consoleOutput(true), 
                      m_fileOutput(false), m_initialized(false),
                      m_maxFileSize(10 * 1024 * 1024), m_maxBackupFiles(5),
                      m_currentFileSize(0) {
    }
    
    Logger::~Logger() {
        Shutdown();
    }
    
    ErrorCode Logger::Initialize(const String& logFilePath, LogLevel level,
                               size_t maxFileSize, int maxBackupFiles) {
        std::lock_guard<std::mutex> lock(m_mutex);
        
        if (m_initialized) {
            Shutdown();
        }
        
        m_logFilePath = logFilePath;
        m_level = level;
        m_maxFileSize = maxFileSize;
        m_maxBackupFiles = maxBackupFiles;
        m_currentFileSize = 0;
        
        // 如果指定了日志文件路径，启用文件输出
        if (!logFilePath.empty()) {
            m_fileOutput = true;
            
            // 创建日志目录
            if (!CreateLogDirectory(logFilePath)) {
                return ErrorCode::GeneralError;
            }
            
            // 打开日志文件（追加模式）
            // 先转换为窄字符编码
            std::string logFilePathA = WStringToString(logFilePath);
            m_fileStream.open(logFilePathA, std::ios::app | std::ios::out);
            if (!m_fileStream.is_open()) {
                return ErrorCode::FileNotFound;
            }
            
            // 设置UTF-8编码的支持在上层处理
            
            // 获取当前文件大小
            m_fileStream.seekp(0, std::ios::end);
            m_currentFileSize = static_cast<size_t>(m_fileStream.tellp());
            m_fileStream.seekp(0, std::ios::beg);
        }
        
        m_initialized = true;
        return ErrorCode::Success;
    }
    
    void Logger::Shutdown() {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_fileStream.is_open()) {
            // 强制刷新并关闭文件流
            m_fileStream.flush();
            m_fileStream.close();
        }
        m_initialized = false;
        m_fileOutput = false;
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
        std::lock_guard<std::mutex> lock(m_mutex);
        
        if (!m_initialized || m_logFilePath.empty()) {
            return ErrorCode::GeneralError;
        }
        
        // 关闭当前文件流
        if (m_fileStream.is_open()) {
            m_fileStream.close();
        }
        
        // 重新打开文件（截断模式）
        std::string logFilePathA = WStringToString(m_logFilePath);
        m_fileStream.open(logFilePathA, std::ios::out | std::ios::trunc);
        if (!m_fileStream.is_open()) {
            return ErrorCode::FileNotFound;
        }
        
        m_fileStream.imbue(std::locale(".UTF8"));
        m_currentFileSize = 0;
        
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
        
        // 文件输出
        if (m_fileOutput && !m_logFilePath.empty() && m_fileStream.is_open()) {
            // 检查是否需要轮转日志文件
            if (m_currentFileSize >= m_maxFileSize) {
                RotateLogFile();
            }
            
            // 输出UTF-8编码的日志
            std::string formattedMessageA = WStringToString(formattedMessage);
            m_fileStream << formattedMessageA << std::endl;
            // 减少频繁的flush调用，只在错误和致命错误时才立即刷新
            if (level >= LogLevel::Error) {
                m_fileStream.flush();
            }
            
            // 更新文件大小
            m_currentFileSize += formattedMessage.length() * sizeof(wchar_t) + 2; // +2 for \r\n
        }
    }
    
    String Logger::FormatMessage(LogLevel level, const String& message, 
                               const char* file, int line, const char* function) const {
        String timeStr = GetCurrentTimeString();
        String levelStr = GetLevelString(level);
        
        std::wstringstream ss;
        ss << L"[" << timeStr << L"] [" << levelStr << L"] ";
        
        // 如果提供了文件信息，添加到日志中
        if (file && line > 0) {
            // 只显示文件名，不显示完整路径
            const char* fileName = strrchr(file, '\\');
            if (!fileName) fileName = strrchr(file, '/');
            if (!fileName) fileName = file;
            else fileName++; // 跳过路径分隔符
            
            String fileNameW = StringToWString(StringA(fileName));
            ss << L"[" << fileNameW << L":" << line << L"] ";
        }
        
        if (function) {
            String functionW = StringToWString(StringA(function));
            ss << L"[" << functionW << L"] ";
        }
        
        ss << message;
        return ss.str();
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
        SYSTEMTIME st;
        GetLocalTime(&st);
        
        wchar_t buffer[64];
        swprintf_s(buffer, sizeof(buffer)/sizeof(wchar_t), 
                  L"%04d-%02d-%02d %02d:%02d:%02d.%03d",
                  st.wYear, st.wMonth, st.wDay,
                  st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
        
        return String(buffer);
    }
    
    void Logger::RotateLogFile() {
        if (m_logFilePath.empty() || !m_fileStream.is_open()) {
            return;
        }
        
        // 关闭当前文件
        m_fileStream.close();
        
        // 删除最老的备份文件
        if (m_maxBackupFiles > 0) {
            String oldestBackup = m_logFilePath + L"." + std::to_wstring(m_maxBackupFiles);
            DeleteFileW(oldestBackup.c_str());
            
            // 重命名现有备份文件
            for (int i = m_maxBackupFiles - 1; i >= 1; i--) {
                String oldName = m_logFilePath + L"." + std::to_wstring(i);
                String newName = m_logFilePath + L"." + std::to_wstring(i + 1);
                MoveFileW(oldName.c_str(), newName.c_str());
            }
            
            // 重命名当前日志文件为.1
            String backupName = m_logFilePath + L".1";
            MoveFileW(m_logFilePath.c_str(), backupName.c_str());
        }
        
        // 重新打开新的日志文件
        std::string logFilePathA = WStringToString(m_logFilePath);
        m_fileStream.open(logFilePathA, std::ios::out | std::ios::trunc);
        if (m_fileStream.is_open()) {
        // 不需要设置编码
            m_currentFileSize = 0;
        }
    }
    
    bool Logger::CreateLogDirectory(const String& filePath) {
        // 提取目录路径
        size_t lastSlash = filePath.find_last_of(L"\\/");
        if (lastSlash == String::npos) {
            return true; // 当前目录，无需创建
        }
        
        String dirPath = filePath.substr(0, lastSlash);
        
        // 检查目录是否已存在
        DWORD attributes = GetFileAttributesW(dirPath.c_str());
        if (attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_DIRECTORY)) {
            return true; // 目录已存在
        }
        
        // 递归创建目录
        return CreateDirectoryW(dirPath.c_str(), nullptr) == TRUE || GetLastError() == ERROR_ALREADY_EXISTS;
    }
    
} // namespace YG
