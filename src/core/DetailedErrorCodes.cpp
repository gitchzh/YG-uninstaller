/**
 * @file DetailedErrorCodes.cpp
 * @brief 详细错误代码实现
 * @author YG Software
 * @version 1.0.1
 * @date 2025-01-21
 */

#include "core/DetailedErrorCodes.h"
#include "core/Logger.h"
#include <sstream>

namespace YG {
    
    // 错误描述映射表
    const std::unordered_map<DetailedErrorCode, String> DetailedErrorHandler::s_errorDescriptions = {
        // 成功
        {DetailedErrorCode::Success, L"Success"},
        
        // 文件系统错误
        {DetailedErrorCode::FileNotFound, L"File not found"},
        {DetailedErrorCode::FileAccessDenied, L"File access denied"},
        {DetailedErrorCode::FileInUse, L"File in use"},
        {DetailedErrorCode::FileCorrupted, L"File corrupted"},
        {DetailedErrorCode::DirectoryNotFound, L"Directory not found"},
        {DetailedErrorCode::DirectoryAccessDenied, L"Directory access denied"},
        {DetailedErrorCode::DiskSpaceInsufficient, L"Disk space insufficient"},
        {DetailedErrorCode::PathTooLong, L"Path too long"},
        {DetailedErrorCode::InvalidFileName, L"Invalid file name"},
        {DetailedErrorCode::FileReadError, L"File read error"},
        {DetailedErrorCode::FileWriteError, L"File write error"},
        
        // 注册表错误
        {DetailedErrorCode::RegistryKeyNotFound, L"Registry key not found"},
        {DetailedErrorCode::RegistryValueNotFound, L"Registry value not found"},
        {DetailedErrorCode::RegistryAccessDenied, L"Registry access denied"},
        {DetailedErrorCode::RegistryKeyCorrupted, L"Registry key corrupted"},
        {DetailedErrorCode::RegistryWriteProtected, L"Registry write protected"},
        {DetailedErrorCode::RegistryInvalidDataType, L"Registry invalid data type"},
        {DetailedErrorCode::RegistryBufferOverflow, L"Registry buffer overflow"},
        {DetailedErrorCode::RegistryConnectionFailed, L"Registry connection failed"},
        
        // 进程和线程错误
        {DetailedErrorCode::ProcessCreationFailed, L"Process creation failed"},
        {DetailedErrorCode::ProcessExecutionFailed, L"Process execution failed"},
        {DetailedErrorCode::ProcessAccessDenied, L"Process access denied"},
        {DetailedErrorCode::ProcessNotFound, L"Process not found"},
        {DetailedErrorCode::ProcessAlreadyRunning, L"Process already running"},
        {DetailedErrorCode::ThreadCreationFailed, L"Thread creation failed"},
        {DetailedErrorCode::ThreadSynchronizationFailed, L"Thread synchronization failed"},
        {DetailedErrorCode::ThreadTimeoutExpired, L"Thread timeout expired"},
        {DetailedErrorCode::ThreadTerminationFailed, L"Thread termination failed"},
        
        // 系统权限错误
        {DetailedErrorCode::InsufficientPrivileges, L"Insufficient privileges"},
        {DetailedErrorCode::AdminRightsRequired, L"Administrator rights required"},
        {DetailedErrorCode::UserCancelled, L"User cancelled"},
        {DetailedErrorCode::AccessTokenInvalid, L"Access token invalid"},
        {DetailedErrorCode::SecurityDescriptorInvalid, L"Security descriptor invalid"},
        
        // 网络错误
        {DetailedErrorCode::NetworkConnectionFailed, L"Network connection failed"},
        {DetailedErrorCode::NetworkTimeoutExpired, L"Network timeout expired"},
        {DetailedErrorCode::NetworkResourceUnavailable, L"Network resource unavailable"},
        {DetailedErrorCode::NetworkAuthenticationFailed, L"Network authentication failed"},
        {DetailedErrorCode::NetworkProtocolError, L"Network protocol error"},
        
        // 程序卸载相关错误
        {DetailedErrorCode::UninstallStringNotFound, L"Uninstall string not found"},
        {DetailedErrorCode::UninstallStringInvalid, L"Uninstall string invalid"},
        {DetailedErrorCode::UninstallerNotFound, L"Uninstaller not found"},
        {DetailedErrorCode::UninstallerExecutionFailed, L"Uninstaller execution failed"},
        {DetailedErrorCode::UninstallProcessTimeout, L"Uninstall process timeout"},
        {DetailedErrorCode::UninstallUserCancelled, L"Uninstall user cancelled"},
        {DetailedErrorCode::UninstallIncomplete, L"Uninstall incomplete"},
        {DetailedErrorCode::UninstallRollbackFailed, L"Uninstall rollback failed"},
        {DetailedErrorCode::ProgramNotInstalled, L"Program not installed"},
        {DetailedErrorCode::ProgramInUse, L"Program in use"},
        
        // 扫描和检测错误
        {DetailedErrorCode::ScanOperationFailed, L"Scan operation failed"},
        {DetailedErrorCode::ScanTimeout, L"Scan timeout"},
        {DetailedErrorCode::ScanInterrupted, L"Scan interrupted"},
        {DetailedErrorCode::ScanDataCorrupted, L"Scan data corrupted"},
        {DetailedErrorCode::ScanInsufficientMemory, L"Scan insufficient memory"},
        {DetailedErrorCode::ScanPermissionDenied, L"Scan permission denied"},
        {DetailedErrorCode::DataNotFound, L"Data not found"},
        
        // 配置和设置错误
        {DetailedErrorCode::ConfigFileNotFound, L"Config file not found"},
        {DetailedErrorCode::ConfigFileCorrupted, L"Config file corrupted"},
        {DetailedErrorCode::ConfigValueInvalid, L"Config value invalid"},
        {DetailedErrorCode::ConfigAccessDenied, L"Config access denied"},
        {DetailedErrorCode::ConfigVersionMismatch, L"Config version mismatch"},
        
        // UI和界面错误
        {DetailedErrorCode::WindowCreationFailed, L"Window creation failed"},
        {DetailedErrorCode::ControlCreationFailed, L"Control creation failed"},
        {DetailedErrorCode::ResourceLoadFailed, L"Resource load failed"},
        {DetailedErrorCode::IconLoadFailed, L"Icon load failed"},
        {DetailedErrorCode::MenuCreationFailed, L"Menu creation failed"},
        {DetailedErrorCode::DialogCreationFailed, L"Dialog creation failed"},
        
        // 内存和资源错误
        {DetailedErrorCode::OutOfMemory, L"Out of memory"},
        {DetailedErrorCode::ResourceLeakDetected, L"Resource leak detected"},
        {DetailedErrorCode::InvalidPointer, L"Invalid pointer"},
        {DetailedErrorCode::BufferOverflow, L"Buffer overflow"},
        {DetailedErrorCode::ResourceAllocationFailed, L"Resource allocation failed"},
        
        // 输入验证错误
        {DetailedErrorCode::InvalidParameter, L"Invalid parameter"},
        {DetailedErrorCode::ParameterOutOfRange, L"Parameter out of range"},
        {DetailedErrorCode::ParameterFormatInvalid, L"Parameter format invalid"},
        {DetailedErrorCode::RequiredParameterMissing, L"Required parameter missing"},
        {DetailedErrorCode::ParameterTooLong, L"Parameter too long"},
        {DetailedErrorCode::ParameterTooShort, L"Parameter too short"},
        
        // 通用错误
        {DetailedErrorCode::UnknownError, L"Unknown error"},
        {DetailedErrorCode::NotImplemented, L"Not implemented"},
        {DetailedErrorCode::OperationCancelled, L"Operation cancelled"},
        {DetailedErrorCode::OperationTimeout, L"Operation timeout"},
        {DetailedErrorCode::InternalError, L"Internal error"}
    };
    
    // 错误建议映射表
    const std::unordered_map<DetailedErrorCode, String> DetailedErrorHandler::s_errorSuggestions = {
        // 文件系统错误
        {DetailedErrorCode::FileNotFound, L"Check file path or if file was deleted"},
        {DetailedErrorCode::FileAccessDenied, L"Run as administrator or check file permissions"},
        {DetailedErrorCode::FileInUse, L"Close programs using the file and retry"},
        {DetailedErrorCode::FileCorrupted, L"Restore from backup or reinstall program"},
        {DetailedErrorCode::DiskSpaceInsufficient, L"Free disk space or choose different location"},
        {DetailedErrorCode::PathTooLong, L"Use shorter file path"},
        
        // 注册表错误
        {DetailedErrorCode::RegistryKeyNotFound, L"Check if program is correctly installed"},
        {DetailedErrorCode::RegistryAccessDenied, L"Run as administrator"},
        {DetailedErrorCode::RegistryKeyCorrupted, L"Run system file checker (sfc /scannow)"},
        
        // 进程和线程错误
        {DetailedErrorCode::ProcessCreationFailed, L"Check if program file exists and is not corrupted"},
        {DetailedErrorCode::ProcessAccessDenied, L"Run as administrator or check antivirus settings"},
        {DetailedErrorCode::ProcessAlreadyRunning, L"Close running program instance and retry"},
        {DetailedErrorCode::ThreadTimeoutExpired, L"Operation may take longer, retry later"},
        
        // 权限错误
        {DetailedErrorCode::InsufficientPrivileges, L"Run as administrator"},
        {DetailedErrorCode::AdminRightsRequired, L"Right-click and select 'Run as administrator'"},
        {DetailedErrorCode::UserCancelled, L"Retry operation if needed"},
        
        // 程序卸载错误
        {DetailedErrorCode::UninstallStringNotFound, L"Program may be manually deleted, try force cleanup"},
        {DetailedErrorCode::UninstallerNotFound, L"Uninstaller may be corrupted, try manual deletion"},
        {DetailedErrorCode::UninstallProcessTimeout, L"Uninstall may be slow, wait or restart and retry"},
        {DetailedErrorCode::ProgramInUse, L"Close all program instances and retry uninstall"},
        
        // 扫描错误
        {DetailedErrorCode::ScanTimeout, L"Check system performance or restart and retry"},
        {DetailedErrorCode::ScanPermissionDenied, L"Run as administrator for full scan permissions"},
        {DetailedErrorCode::ScanInsufficientMemory, L"Close other programs to free memory"},
        
        // 配置错误
        {DetailedErrorCode::ConfigFileCorrupted, L"Delete config file to regenerate defaults"},
        {DetailedErrorCode::ConfigAccessDenied, L"Check config folder write permissions"},
        
        // UI错误
        {DetailedErrorCode::WindowCreationFailed, L"Restart program or check system resources"},
        {DetailedErrorCode::ResourceLoadFailed, L"Reinstall program to fix corrupted resources"},
        
        // 内存错误
        {DetailedErrorCode::OutOfMemory, L"Close other programs or restart computer"},
        {DetailedErrorCode::InvalidPointer, L"Internal error, restart program or contact support"},
        
        // 输入验证错误
        {DetailedErrorCode::InvalidParameter, L"Check input parameters are correct"},
        {DetailedErrorCode::ParameterOutOfRange, L"Ensure input values are within valid range"},
        {DetailedErrorCode::RequiredParameterMissing, L"Provide all required parameters"},
        
        // 通用错误
        {DetailedErrorCode::UnknownError, L"Retry operation, contact support if problem persists"},
        {DetailedErrorCode::OperationTimeout, L"Check network connection or system performance"},
        {DetailedErrorCode::InternalError, L"Restart program or update to latest version"}
    };
    
    DetailedErrorHandler& DetailedErrorHandler::GetInstance() {
        static DetailedErrorHandler instance;
        return instance;
    }
    
    String DetailedErrorHandler::ErrorCodeToString(DetailedErrorCode code) {
        auto it = s_errorDescriptions.find(code);
        if (it != s_errorDescriptions.end()) {
            return it->second;
        }
        return L"未知错误 (" + std::to_wstring(static_cast<int>(code)) + L")";
    }
    
    String DetailedErrorHandler::GetErrorSuggestion(DetailedErrorCode code) {
        auto it = s_errorSuggestions.find(code);
        if (it != s_errorSuggestions.end()) {
            return it->second;
        }
        return L"请重试操作，如问题持续存在请联系技术支持。";
    }
    
    ErrorContext DetailedErrorHandler::CreateErrorContext(
        DetailedErrorCode code,
        const String& message,
        const String& context,
        const char* fileName,
        int lineNumber,
        const char* functionName) {
        
        ErrorContext errorContext;
        errorContext.code = code;
        errorContext.message = message.empty() ? ErrorCodeToString(code) : message;
        errorContext.context = context;
        errorContext.suggestion = GetErrorSuggestion(code);
        errorContext.systemErrorCode = GetLastError(); // 获取当前系统错误码
        errorContext.lineNumber = lineNumber;
        
        if (fileName) {
            errorContext.fileName = StringToWString(fileName);
        }
        if (functionName) {
            errorContext.functionName = StringToWString(functionName);
        }
        
        // 生成技术细节
        std::wstringstream technical;
        technical << L"错误码: " << static_cast<int>(code);
        if (errorContext.systemErrorCode != 0) {
            technical << L", 系统错误码: " << errorContext.systemErrorCode;
        }
        if (!errorContext.fileName.empty()) {
            technical << L", 文件: " << errorContext.fileName;
        }
        if (lineNumber > 0) {
            technical << L", 行号: " << lineNumber;
        }
        if (!errorContext.functionName.empty()) {
            technical << L", 函数: " << errorContext.functionName;
        }
        errorContext.technicalDetails = technical.str();
        
        return errorContext;
    }
    
    String DetailedErrorHandler::FormatErrorMessage(const ErrorContext& context) {
        std::wstringstream message;
        
        // 主要错误信息
        message << L"错误: " << context.message << L"\n";
        
        // 上下文信息
        if (!context.context.empty()) {
            message << L"详情: " << context.context << L"\n";
        }
        
        // 解决建议
        if (!context.suggestion.empty()) {
            message << L"建议: " << context.suggestion << L"\n";
        }
        
        // 技术细节
        if (!context.technicalDetails.empty()) {
            message << L"技术信息: " << context.technicalDetails;
        }
        
        return message.str();
    }
    
    DetailedErrorCode DetailedErrorHandler::SystemErrorToDetailedError(DWORD systemError) {
        switch (systemError) {
            case ERROR_FILE_NOT_FOUND:
            case ERROR_PATH_NOT_FOUND:
                return DetailedErrorCode::FileNotFound;
                
            case ERROR_ACCESS_DENIED:
                return DetailedErrorCode::FileAccessDenied;
                
            case ERROR_SHARING_VIOLATION:
            case ERROR_LOCK_VIOLATION:
                return DetailedErrorCode::FileInUse;
                
            case ERROR_DISK_FULL:
            case ERROR_HANDLE_DISK_FULL:
                return DetailedErrorCode::DiskSpaceInsufficient;
                
            case ERROR_FILENAME_EXCED_RANGE:
                return DetailedErrorCode::PathTooLong;
                
            case ERROR_INVALID_NAME:
                return DetailedErrorCode::InvalidFileName;
                
            case ERROR_OUTOFMEMORY:
            case ERROR_NOT_ENOUGH_MEMORY:
                return DetailedErrorCode::OutOfMemory;
                
            case ERROR_PRIVILEGE_NOT_HELD:
                return DetailedErrorCode::InsufficientPrivileges;
                
            case ERROR_CANCELLED:
                return DetailedErrorCode::UserCancelled;
                
            case ERROR_TIMEOUT:
                return DetailedErrorCode::OperationTimeout;
                
            default:
                return DetailedErrorCode::UnknownError;
        }
    }
    
    DetailedErrorCode DetailedErrorHandler::BasicErrorToDetailedError(ErrorCode basicError) {
        switch (basicError) {
            case ErrorCode::Success:
                return DetailedErrorCode::Success;
                
            case ErrorCode::InvalidParameter:
                return DetailedErrorCode::InvalidParameter;
                
            case ErrorCode::AccessDenied:
                return DetailedErrorCode::FileAccessDenied;
                
            case ErrorCode::FileNotFound:
                return DetailedErrorCode::FileNotFound;
                
            case ErrorCode::RegistryError:
                return DetailedErrorCode::RegistryKeyNotFound;
                
            case ErrorCode::NetworkError:
                return DetailedErrorCode::NetworkConnectionFailed;
                
            case ErrorCode::DataNotFound:
                return DetailedErrorCode::FileNotFound;
                
            case ErrorCode::OperationInProgress:
                return DetailedErrorCode::ProcessAlreadyRunning;
                
            case ErrorCode::OperationCancelled:
                return DetailedErrorCode::OperationCancelled;
                
            case ErrorCode::UnknownError:
            case ErrorCode::GeneralError:
            default:
                return DetailedErrorCode::UnknownError;
        }
    }
    
} // namespace YG
