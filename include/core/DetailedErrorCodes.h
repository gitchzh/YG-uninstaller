/**
 * @file DetailedErrorCodes.h
 * @brief 详细错误代码定义
 * @author YG Software
 * @version 1.0.1
 * @date 2025-01-21
 */

#pragma once

#include "Common.h"
#include <unordered_map>

namespace YG {
    
    /**
     * @brief 详细错误代码枚举
     * 
     * 提供比基本ErrorCode更详细的错误分类
     */
    enum class DetailedErrorCode : int {
        // 成功
        Success = 0,
        
        // 文件系统错误 (1000-1999)
        FileNotFound = 1001,
        FileAccessDenied = 1002,
        FileInUse = 1003,
        FileCorrupted = 1004,
        DirectoryNotFound = 1005,
        DirectoryAccessDenied = 1006,
        DiskSpaceInsufficient = 1007,
        PathTooLong = 1008,
        InvalidFileName = 1009,
        FileReadError = 1010,
        FileWriteError = 1011,
        
        // 注册表错误 (2000-2999)
        RegistryKeyNotFound = 2001,
        RegistryValueNotFound = 2002,
        RegistryAccessDenied = 2003,
        RegistryKeyCorrupted = 2004,
        RegistryWriteProtected = 2005,
        RegistryInvalidDataType = 2006,
        RegistryBufferOverflow = 2007,
        RegistryConnectionFailed = 2008,
        
        // 进程和线程错误 (3000-3999)
        ProcessCreationFailed = 3001,
        ProcessExecutionFailed = 3002,
        ProcessAccessDenied = 3003,
        ProcessNotFound = 3004,
        ProcessAlreadyRunning = 3005,
        ThreadCreationFailed = 3006,
        ThreadSynchronizationFailed = 3007,
        ThreadTimeoutExpired = 3008,
        ThreadTerminationFailed = 3009,
        
        // 系统权限错误 (4000-4999)
        InsufficientPrivileges = 4001,
        AdminRightsRequired = 4002,
        UserCancelled = 4003,
        AccessTokenInvalid = 4004,
        SecurityDescriptorInvalid = 4005,
        
        // 网络错误 (5000-5999)
        NetworkConnectionFailed = 5001,
        NetworkTimeoutExpired = 5002,
        NetworkResourceUnavailable = 5003,
        NetworkAuthenticationFailed = 5004,
        NetworkProtocolError = 5005,
        
        // 程序卸载相关错误 (6000-6999)
        UninstallStringNotFound = 6001,
        UninstallStringInvalid = 6002,
        UninstallerNotFound = 6003,
        UninstallerExecutionFailed = 6004,
        UninstallProcessTimeout = 6005,
        UninstallUserCancelled = 6006,
        UninstallIncomplete = 6007,
        UninstallRollbackFailed = 6008,
        ProgramNotInstalled = 6009,
        ProgramInUse = 6010,
        
        // 扫描和检测错误 (7000-7999)
        ScanOperationFailed = 7001,
        ScanTimeout = 7002,
        ScanInterrupted = 7003,
        ScanDataCorrupted = 7004,
        ScanInsufficientMemory = 7005,
        ScanPermissionDenied = 7006,
        DataNotFound = 7007,
        
        // 配置和设置错误 (8000-8999)
        ConfigFileNotFound = 8001,
        ConfigFileCorrupted = 8002,
        ConfigValueInvalid = 8003,
        ConfigAccessDenied = 8004,
        ConfigVersionMismatch = 8005,
        
        // UI和界面错误 (9000-9999)
        WindowCreationFailed = 9001,
        ControlCreationFailed = 9002,
        ResourceLoadFailed = 9003,
        IconLoadFailed = 9004,
        MenuCreationFailed = 9005,
        DialogCreationFailed = 9006,
        
        // 内存和资源错误 (10000-10999)
        OutOfMemory = 10001,
        ResourceLeakDetected = 10002,
        InvalidPointer = 10003,
        BufferOverflow = 10004,
        ResourceAllocationFailed = 10005,
        
        // 输入验证错误 (11000-11999)
        InvalidParameter = 11001,
        ParameterOutOfRange = 11002,
        ParameterFormatInvalid = 11003,
        RequiredParameterMissing = 11004,
        ParameterTooLong = 11005,
        ParameterTooShort = 11006,
        
        // 通用错误 (99000+)
        UnknownError = 99001,
        NotImplemented = 99002,
        OperationCancelled = 99003,
        OperationTimeout = 99004,
        InternalError = 99005
    };
    
    /**
     * @brief 错误上下文信息结构
     */
    struct ErrorContext {
        DetailedErrorCode code;             ///< 详细错误码
        String message;                     ///< 错误消息
        String context;                     ///< 错误上下文
        String suggestion;                  ///< 解决建议
        String technicalDetails;            ///< 技术细节
        DWORD systemErrorCode;              ///< 系统错误码（如果有）
        String fileName;                    ///< 相关文件名
        int lineNumber;                     ///< 错误发生行号
        String functionName;                ///< 错误发生函数
        
        ErrorContext() : code(DetailedErrorCode::Success), systemErrorCode(0), lineNumber(0) {}
        
        ErrorContext(DetailedErrorCode errorCode, const String& msg = L"") 
            : code(errorCode), message(msg), systemErrorCode(0), lineNumber(0) {}
    };
    
    /**
     * @brief 详细错误处理器
     */
    class DetailedErrorHandler {
    public:
        /**
         * @brief 获取详细错误处理器实例
         */
        static DetailedErrorHandler& GetInstance();
        
        /**
         * @brief 将DetailedErrorCode转换为字符串
         * @param code 详细错误码
         * @return String 错误码字符串描述
         */
        static String ErrorCodeToString(DetailedErrorCode code);
        
        /**
         * @brief 获取错误建议
         * @param code 详细错误码
         * @return String 解决建议
         */
        static String GetErrorSuggestion(DetailedErrorCode code);
        
        /**
         * @brief 创建错误上下文
         * @param code 详细错误码
         * @param message 错误消息
         * @param context 上下文信息
         * @param fileName 文件名
         * @param lineNumber 行号
         * @param functionName 函数名
         * @return ErrorContext 错误上下文
         */
        static ErrorContext CreateErrorContext(
            DetailedErrorCode code,
            const String& message = L"",
            const String& context = L"",
            const char* fileName = nullptr,
            int lineNumber = 0,
            const char* functionName = nullptr
        );
        
        /**
         * @brief 格式化错误上下文为用户友好的消息
         * @param context 错误上下文
         * @return String 格式化的错误消息
         */
        static String FormatErrorMessage(const ErrorContext& context);
        
        /**
         * @brief 将系统错误码转换为详细错误码
         * @param systemError 系统错误码
         * @return DetailedErrorCode 对应的详细错误码
         */
        static DetailedErrorCode SystemErrorToDetailedError(DWORD systemError);
        
        /**
         * @brief 将基本错误码转换为详细错误码
         * @param basicError 基本错误码
         * @return DetailedErrorCode 对应的详细错误码
         */
        static DetailedErrorCode BasicErrorToDetailedError(ErrorCode basicError);
        
    private:
        DetailedErrorHandler() = default;
        ~DetailedErrorHandler() = default;
        
        YG_DISABLE_COPY_AND_ASSIGN(DetailedErrorHandler);
        
        /// 错误码到描述的映射表
        static const std::unordered_map<DetailedErrorCode, String> s_errorDescriptions;
        
        /// 错误码到建议的映射表
        static const std::unordered_map<DetailedErrorCode, String> s_errorSuggestions;
    };
    
} // namespace YG

// 便捷宏定义
#define YG_CREATE_ERROR_CONTEXT(code, message, context) \
    YG::DetailedErrorHandler::CreateErrorContext(code, message, context, __FILE__, __LINE__, __FUNCTION__)

#define YG_DETAILED_ERROR(code, message) \
    YG_CREATE_ERROR_CONTEXT(code, message, L"")

#define YG_SYSTEM_ERROR(message) \
    YG_CREATE_ERROR_CONTEXT(YG::DetailedErrorHandler::SystemErrorToDetailedError(GetLastError()), message, L"")
