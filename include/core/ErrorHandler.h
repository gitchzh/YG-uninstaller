/**
 * @file ErrorHandler.h
 * @brief 错误处理和异常管理类
 * @author YG Software
 * @version 1.0.1
 * @date 2025-09-17
 */

#pragma once

#include "Common.h"
#include <stdexcept>
#include <functional>
#include <mutex>

namespace YG {
    
    /**
     * @brief YG异常基类
     */
    class YGException : public std::exception {
    public:
        /**
         * @brief 构造函数
         * @param errorCode 错误代码
         * @param message 错误消息
         * @param file 源文件名
         * @param line 行号
         */
        YGException(ErrorCode errorCode, const String& message, 
                   const char* file = nullptr, int line = 0);
        
        /**
         * @brief 获取错误消息
         * @return const char* 错误消息
         */
        const char* what() const noexcept override;
        
        /**
         * @brief 获取错误代码
         * @return ErrorCode 错误代码
         */
        ErrorCode GetErrorCode() const noexcept;
        
        /**
         * @brief 获取宽字符错误消息
         * @return String 宽字符错误消息
         */
        String GetWhatW() const noexcept;
        
        /**
         * @brief 获取源文件名
         * @return const char* 源文件名
         */
        const char* GetFile() const noexcept;
        
        /**
         * @brief 获取行号
         * @return int 行号
         */
        int GetLine() const noexcept;
        
    private:
        ErrorCode m_errorCode;    ///< 错误代码
        String m_messageW;        ///< 宽字符错误消息
        StringA m_messageA;       ///< ANSI错误消息
        StringA m_file;           ///< 源文件名
        int m_line;               ///< 行号
    };
    
    /**
     * @brief 系统错误异常类
     */
    class SystemException : public YGException {
    public:
        SystemException(DWORD systemErrorCode, const String& message, 
                       const char* file = nullptr, int line = 0);
        
        DWORD GetSystemErrorCode() const noexcept;
        String GetSystemErrorMessage() const noexcept;
        
    private:
        DWORD m_systemErrorCode;
    };
    
    /**
     * @brief 注册表错误异常类
     */
    class RegistryException : public YGException {
    public:
        RegistryException(LONG regResult, const String& keyPath, 
                         const char* file = nullptr, int line = 0);
        
        LONG GetRegistryResult() const noexcept;
        String GetKeyPath() const noexcept;
        
    private:
        LONG m_regResult;
        String m_keyPath;
    };
    
    /**
     * @brief 文件操作错误异常类
     */
    class FileException : public YGException {
    public:
        FileException(const String& filePath, const String& operation, 
                     const char* file = nullptr, int line = 0);
        
        String GetFilePath() const noexcept;
        String GetOperation() const noexcept;
        
    private:
        String m_filePath;
        String m_operation;
    };
    
    /**
     * @brief 错误处理器类
     * 
     * 提供统一的错误处理和报告机制
     */
    class ErrorHandler {
    public:
        // 错误处理回调函数类型
        using ErrorCallback = std::function<void(const YGException& ex)>;
        using CriticalErrorCallback = std::function<void(const YGException& ex)>;
        
        /**
         * @brief 获取错误处理器实例
         * @return ErrorHandler& 错误处理器引用
         */
        static ErrorHandler& GetInstance();
        
        /**
         * @brief 设置错误回调函数
         * @param callback 错误回调函数
         */
        void SetErrorCallback(const ErrorCallback& callback);
        
        /**
         * @brief 设置致命错误回调函数
         * @param callback 致命错误回调函数
         */
        void SetCriticalErrorCallback(const CriticalErrorCallback& callback);
        
        /**
         * @brief 处理异常
         * @param ex 异常对象
         * @param isCritical 是否为致命错误
         */
        void HandleException(const YGException& ex, bool isCritical = false);
        
        /**
         * @brief 处理标准异常
         * @param ex 标准异常对象
         * @param isCritical 是否为致命错误
         */
        void HandleStdException(const std::exception& ex, bool isCritical = false);
        
        /**
         * @brief 处理未知异常
         * @param isCritical 是否为致命错误
         */
        void HandleUnknownException(bool isCritical = false);
        
        /**
         * @brief 获取最后一个Windows错误的字符串描述
         * @param errorCode 错误代码，默认为GetLastError()的返回值
         * @return String 错误描述
         */
        static String GetLastErrorString(DWORD errorCode = 0);
        
        /**
         * @brief 获取注册表错误的字符串描述
         * @param regResult 注册表操作结果
         * @return String 错误描述
         */
        static String GetRegistryErrorString(LONG regResult);
        
        /**
         * @brief 将ErrorCode转换为字符串
         * @param errorCode 错误代码
         * @return String 错误描述
         */
        static String ErrorCodeToString(ErrorCode errorCode);
        
        /**
         * @brief 显示错误对话框
         * @param title 对话框标题
         * @param message 错误消息
         * @param errorCode 错误代码
         * @return int 用户选择的按钮
         */
        static int ShowErrorDialog(const String& title, const String& message, 
                                 ErrorCode errorCode = ErrorCode::GeneralError);
        
        /**
         * @brief 记录错误到日志
         * @param ex 异常对象
         */
        void LogError(const YGException& ex);
        
        /**
         * @brief 启用/禁用自动日志记录
         * @param enable 是否启用
         */
        void EnableAutoLogging(bool enable);
        
        /**
         * @brief 启用/禁用错误对话框
         * @param enable 是否启用
         */
        void EnableErrorDialog(bool enable);
        
    private:
        // 私有构造函数，实现单例模式
        ErrorHandler();
        ~ErrorHandler();
        
        YG_DISABLE_COPY_AND_ASSIGN(ErrorHandler);
        
    private:
        ErrorCallback m_errorCallback;              ///< 错误回调函数
        CriticalErrorCallback m_criticalCallback;  ///< 致命错误回调函数
        bool m_autoLogging;                         ///< 自动日志记录
        bool m_showErrorDialog;                     ///< 显示错误对话框
        mutable std::mutex m_mutex;                 ///< 线程安全锁
    };
    
} // namespace YG

// 便捷宏定义
#define YG_THROW(errorCode, message) \
    throw YG::YGException(errorCode, message, __FILE__, __LINE__)

#define YG_THROW_SYSTEM(message) \
    throw YG::SystemException(GetLastError(), message, __FILE__, __LINE__)

#define YG_THROW_REGISTRY(regResult, keyPath) \
    throw YG::RegistryException(regResult, keyPath, __FILE__, __LINE__)

#define YG_THROW_FILE(filePath, operation) \
    throw YG::FileException(filePath, operation, __FILE__, __LINE__)

#define YG_TRY_CATCH_LOG(code) \
    try { \
        code \
    } catch (const YG::YGException& ex) { \
        YG::ErrorHandler::GetInstance().HandleException(ex); \
    } catch (const std::exception& ex) { \
        YG::ErrorHandler::GetInstance().HandleStdException(ex); \
    } catch (...) { \
        YG::ErrorHandler::GetInstance().HandleUnknownException(); \
    }
