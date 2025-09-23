/**
 * @file Logger.h
 * @brief 日志管理类
 * @author YG Software
 * @version 1.0.1
 * @date 2025-09-17
 */

#pragma once

#include "Common.h"
#include <fstream>
#include <mutex>
#include <sstream>

namespace YG {
    
    /**
     * @brief 日志级别枚举
     */
    enum class LogLevel : int {
        Debug = 0,    ///< 调试信息
        Info = 1,     ///< 一般信息
        Warning = 2,  ///< 警告信息
        Error = 3,    ///< 错误信息
        Fatal = 4     ///< 致命错误
    };
    
    /**
     * @brief 日志管理类
     * 
     * 提供线程安全的日志记录功能
     * 支持多种日志级别和输出目标
     */
    class Logger {
    public:
        /**
         * @brief 获取日志管理器实例
         * @return Logger& 日志管理器引用
         */
        static Logger& GetInstance();
        
        /**
         * @brief 初始化日志系统
         * @param logFilePath 日志文件路径
         * @param level 最低日志级别
         * @param maxFileSize 最大文件大小(字节)
         * @param maxBackupFiles 最大备份文件数
         * @return ErrorCode 操作结果
         */
        ErrorCode Initialize(const String& logFilePath = L"", 
                           LogLevel level = LogLevel::Info,
                           size_t maxFileSize = 10 * 1024 * 1024,  // 10MB
                           int maxBackupFiles = 5);
        
        /**
         * @brief 关闭日志系统
         */
        void Shutdown();
        
        /**
         * @brief 设置日志级别
         * @param level 日志级别
         */
        void SetLevel(LogLevel level);
        
        /**
         * @brief 获取当前日志级别
         * @return LogLevel 当前日志级别
         */
        LogLevel GetLevel() const;
        
        /**
         * @brief 启用/禁用控制台输出
         * @param enable 是否启用
         */
        void EnableConsoleOutput(bool enable);
        
        /**
         * @brief 启用/禁用文件输出
         * @param enable 是否启用
         */
        void EnableFileOutput(bool enable);
        
        /**
         * @brief 记录调试信息
         * @param message 日志消息
         * @param file 源文件名
         * @param line 行号
         * @param function 函数名
         */
        void Debug(const String& message, const char* file = nullptr, 
                  int line = 0, const char* function = nullptr);
        
        /**
         * @brief 记录一般信息
         * @param message 日志消息
         * @param file 源文件名
         * @param line 行号
         * @param function 函数名
         */
        void Info(const String& message, const char* file = nullptr, 
                 int line = 0, const char* function = nullptr);
        
        /**
         * @brief 记录警告信息
         * @param message 日志消息
         * @param file 源文件名
         * @param line 行号
         * @param function 函数名
         */
        void Warning(const String& message, const char* file = nullptr, 
                    int line = 0, const char* function = nullptr);
        
        /**
         * @brief 记录错误信息
         * @param message 日志消息
         * @param file 源文件名
         * @param line 行号
         * @param function 函数名
         */
        void Error(const String& message, const char* file = nullptr, 
                  int line = 0, const char* function = nullptr);
        
        /**
         * @brief 记录致命错误
         * @param message 日志消息
         * @param file 源文件名
         * @param line 行号
         * @param function 函数名
         */
        void Fatal(const String& message, const char* file = nullptr, 
                  int line = 0, const char* function = nullptr);
        
        /**
         * @brief 强制刷新日志缓冲区
         */
        void Flush();
        
        /**
         * @brief 获取日志文件路径
         * @return String 日志文件路径
         */
        String GetLogFilePath() const;
        
        /**
         * @brief 清空日志文件
         * @return ErrorCode 操作结果
         */
        ErrorCode ClearLogFile();
        
    private:
        // 私有构造函数，实现单例模式
        Logger();
        ~Logger();
        
        YG_DISABLE_COPY_AND_ASSIGN(Logger);
        
        /**
         * @brief 写入日志消息
         * @param level 日志级别
         * @param message 日志消息
         * @param file 源文件名
         * @param line 行号
         * @param function 函数名
         */
        void WriteLog(LogLevel level, const String& message, 
                     const char* file, int line, const char* function);
        
        /**
         * @brief 格式化日志消息
         * @param level 日志级别
         * @param message 日志消息
         * @param file 源文件名
         * @param line 行号
         * @param function 函数名
         * @return String 格式化后的消息
         */
        String FormatMessage(LogLevel level, const String& message, 
                           const char* file, int line, const char* function) const;
        
        /**
         * @brief 获取日志级别字符串
         * @param level 日志级别
         * @return String 级别字符串
         */
        String GetLevelString(LogLevel level) const;
        
        /**
         * @brief 获取当前时间字符串
         * @return String 时间字符串
         */
        String GetCurrentTimeString() const;
        
        /**
         * @brief 检查并轮转日志文件
         */
        void RotateLogFile();
        
        /**
         * @brief 创建日志目录
         * @param filePath 文件路径
         * @return bool 是否成功
         */
        bool CreateLogDirectory(const String& filePath);
        
    private:
        LogLevel m_level;              ///< 当前日志级别
        String m_logFilePath;          ///< 日志文件路径
        std::ofstream m_fileStream;    ///< 文件输出流（使用UTF-8）
        mutable std::mutex m_mutex;    ///< 线程安全锁
        bool m_consoleOutput;          ///< 是否输出到控制台
        bool m_fileOutput;             ///< 是否输出到文件
        bool m_initialized;            ///< 是否已初始化
        size_t m_maxFileSize;          ///< 最大文件大小
        int m_maxBackupFiles;          ///< 最大备份文件数
        size_t m_currentFileSize;      ///< 当前文件大小
    };
    
} // namespace YG

// 便捷宏定义
#define YG_LOG_DEBUG(msg) YG::Logger::GetInstance().Debug(msg, __FILE__, __LINE__, __FUNCTION__)
#define YG_LOG_INFO(msg) YG::Logger::GetInstance().Info(msg, __FILE__, __LINE__, __FUNCTION__)
#define YG_LOG_WARNING(msg) YG::Logger::GetInstance().Warning(msg, __FILE__, __LINE__, __FUNCTION__)
#define YG_LOG_ERROR(msg) YG::Logger::GetInstance().Error(msg, __FILE__, __LINE__, __FUNCTION__)
#define YG_LOG_FATAL(msg) YG::Logger::GetInstance().Fatal(msg, __FILE__, __LINE__, __FUNCTION__)
