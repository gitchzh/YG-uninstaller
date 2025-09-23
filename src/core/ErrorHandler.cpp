/**
 * @file ErrorHandler.cpp
 * @brief 错误处理实现
 * @author gmrchzh@gmail.com
 * @version 1.0.1
 * @date 2025-09-17
 */

#include "core/ErrorHandler.h"
#include "core/Logger.h"
#include <iostream>

namespace YG {
    
    // YGException 实现
    YGException::YGException(ErrorCode errorCode, const String& message, 
                           const char* file, int line) 
        : m_errorCode(errorCode), m_messageW(message), m_line(line) {
        
        if (file) {
            m_file = file;
        }
        
        // 转换为ANSI字符串
        m_messageA = WStringToString(message);
    }
    
    const char* YGException::what() const noexcept {
        return m_messageA.c_str();
    }
    
    ErrorCode YGException::GetErrorCode() const noexcept {
        return m_errorCode;
    }
    
    String YGException::GetWhatW() const noexcept {
        return m_messageW;
    }
    
    const char* YGException::GetFile() const noexcept {
        return m_file.c_str();
    }
    
    int YGException::GetLine() const noexcept {
        return m_line;
    }
    
    // SystemException 实现
    SystemException::SystemException(DWORD systemErrorCode, const String& message, 
                                   const char* file, int line)
        : YGException(ErrorCode::GeneralError, message, file, line)
        , m_systemErrorCode(systemErrorCode) {
    }
    
    DWORD SystemException::GetSystemErrorCode() const noexcept {
        return m_systemErrorCode;
    }
    
    String SystemException::GetSystemErrorMessage() const noexcept {
        return ErrorHandler::GetLastErrorString(m_systemErrorCode);
    }
    
    // RegistryException 实现
    RegistryException::RegistryException(LONG regResult, const String& keyPath, 
                                       const char* file, int line)
        : YGException(ErrorCode::RegistryError, L"注册表操作失败: " + keyPath, file, line)
        , m_regResult(regResult), m_keyPath(keyPath) {
    }
    
    LONG RegistryException::GetRegistryResult() const noexcept {
        return m_regResult;
    }
    
    String RegistryException::GetKeyPath() const noexcept {
        return m_keyPath;
    }
    
    // FileException 实现
    FileException::FileException(const String& filePath, const String& operation, 
                               const char* file, int line)
        : YGException(ErrorCode::FileNotFound, L"文件操作失败: " + operation + L" - " + filePath, file, line)
        , m_filePath(filePath), m_operation(operation) {
    }
    
    String FileException::GetFilePath() const noexcept {
        return m_filePath;
    }
    
    String FileException::GetOperation() const noexcept {
        return m_operation;
    }
    
    // ErrorHandler 实现
    ErrorHandler& ErrorHandler::GetInstance() {
        static ErrorHandler instance;
        return instance;
    }
    
    ErrorHandler::ErrorHandler() : m_autoLogging(true), m_showErrorDialog(true) {
    }
    
    ErrorHandler::~ErrorHandler() = default;
    
    void ErrorHandler::SetErrorCallback(const ErrorCallback& callback) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_errorCallback = callback;
    }
    
    void ErrorHandler::SetCriticalErrorCallback(const CriticalErrorCallback& callback) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_criticalCallback = callback;
    }
    
    void ErrorHandler::HandleException(const YGException& ex, bool isCritical) {
        if (m_autoLogging) {
            LogError(ex);
        }
        
        std::lock_guard<std::mutex> lock(m_mutex);
        
        if (isCritical && m_criticalCallback) {
            m_criticalCallback(ex);
        } else if (m_errorCallback) {
            m_errorCallback(ex);
        }
        
        if (m_showErrorDialog) {
            ShowErrorDialog(L"错误", ex.GetWhatW(), ex.GetErrorCode());
        }
    }
    
    void ErrorHandler::HandleStdException(const std::exception& ex, bool isCritical) {
        String message = StringToWString(ex.what());
        YGException ygEx(ErrorCode::GeneralError, message);
        HandleException(ygEx, isCritical);
    }
    
    void ErrorHandler::HandleUnknownException(bool isCritical) {
        YGException ygEx(ErrorCode::UnknownError, L"未知异常");
        HandleException(ygEx, isCritical);
    }
    
    String ErrorHandler::GetLastErrorString(DWORD errorCode) {
        if (errorCode == 0) {
            errorCode = GetLastError();
        }
        
        LPWSTR messageBuffer = nullptr;
        DWORD size = FormatMessageW(
            FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
            nullptr, errorCode, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            (LPWSTR)&messageBuffer, 0, nullptr);
        
        String message;
        if (size > 0 && messageBuffer) {
            message = String(messageBuffer);
            LocalFree(messageBuffer);
        } else {
            message = L"未知系统错误 (0x" + std::to_wstring(errorCode) + L")";
        }
        
        return message;
    }
    
    String ErrorHandler::GetRegistryErrorString(LONG regResult) {
        return L"注册表错误: " + std::to_wstring(regResult);
    }
    
    String ErrorHandler::ErrorCodeToString(ErrorCode errorCode) {
        switch (errorCode) {
            case ErrorCode::Success:          return L"成功";
            case ErrorCode::GeneralError:     return L"一般错误";
            case ErrorCode::InvalidParameter: return L"无效参数";
            case ErrorCode::AccessDenied:     return L"访问被拒绝";
            case ErrorCode::FileNotFound:     return L"文件未找到";
            case ErrorCode::RegistryError:    return L"注册表错误";
            case ErrorCode::NetworkError:     return L"网络错误";
            case ErrorCode::UnknownError:     return L"未知错误";
            default:                          return L"未定义错误";
        }
    }
    
    int ErrorHandler::ShowErrorDialog(const String& title, const String& message, ErrorCode errorCode) {
        String fullMessage = message + L"\n\n错误代码: " + ErrorCodeToString(errorCode);
        return MessageBoxW(nullptr, fullMessage.c_str(), title.c_str(), MB_OK | MB_ICONERROR);
    }
    
    void ErrorHandler::LogError(const YGException& ex) {
        String logMessage = L"异常: " + ex.GetWhatW();
        if (ex.GetFile() && strlen(ex.GetFile()) > 0) {
            logMessage += L" [" + StringToWString(ex.GetFile()) + L":" + std::to_wstring(ex.GetLine()) + L"]";
        }
        YG_LOG_ERROR(logMessage);
    }
    
    void ErrorHandler::EnableAutoLogging(bool enable) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_autoLogging = enable;
    }
    
    void ErrorHandler::EnableErrorDialog(bool enable) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_showErrorDialog = enable;
    }
    
} // namespace YG
