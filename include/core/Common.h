/**
 * @file Common.h
 * @brief 通用定义和常量
 * @author YG Software
 * @version 1.0.0
 * @date 2025-09-17
 */

#pragma once

#include <windows.h>
#include <string>
#include <vector>
#include <memory>
#include <exception>
#include <functional>

// 版本信息
#define YG_VERSION_MAJOR 1
#define YG_VERSION_MINOR 0
#define YG_VERSION_PATCH 0
#define YG_VERSION_STRING L"1.0.0"

// 应用程序信息
#define YG_APP_NAME L"YG Uninstaller"
#define YG_APP_DESCRIPTION L"高效的Windows程序卸载工具"
#define YG_COMPANY_NAME L"YG Software"

// 通用类型定义
namespace YG {
    using String = std::wstring;
    using StringA = std::string;
    using StringVector = std::vector<String>;
    
    // 错误代码枚举
    enum class ErrorCode : int {
        Success = 0,
        GeneralError = 1,
        InvalidParameter = 2,
        AccessDenied = 3,
        FileNotFound = 4,
        RegistryError = 5,
        NetworkError = 6,
        UnknownError = 99
    };
    
    // 程序信息结构
    struct ProgramInfo {
        String name;              // 程序名称
        String displayName;       // 显示名称
        String version;           // 版本号
        String publisher;         // 发布者
        String installDate;       // 安装日期
        String installLocation;   // 安装路径
        String uninstallString;   // 卸载命令
        String iconPath;          // 图标路径
        DWORD64 estimatedSize;    // 估计大小(KB)
        bool isSystemComponent;   // 是否为系统组件
        
        ProgramInfo() : estimatedSize(0), isSystemComponent(false) {}
    };
    
    // 卸载结果结构
    struct UninstallResult {
        ErrorCode errorCode;
        String message;
        bool success;
        StringVector remainingFiles;
        StringVector remainingRegistries;
        
        UninstallResult() : errorCode(ErrorCode::Success), success(false) {}
    };
    
    // 智能指针别名
    template<typename T>
    using UniquePtr = std::unique_ptr<T>;
    
    template<typename T>
    using SharedPtr = std::shared_ptr<T>;
    
    template<typename T, typename... Args>
    UniquePtr<T> MakeUnique(Args&&... args) {
        return std::make_unique<T>(std::forward<Args>(args)...);
    }
    
    template<typename T, typename... Args>
    SharedPtr<T> MakeShared(Args&&... args) {
        return std::make_shared<T>(std::forward<Args>(args)...);
    }
    
    // 回调函数类型
    using ProgressCallback = std::function<void(int percentage, const String& message)>;
    using CompletionCallback = std::function<void(const UninstallResult& result)>;
    
    // 常用宏定义
    #define YG_SAFE_DELETE(p) { if(p) { delete (p); (p) = nullptr; } }
    #define YG_SAFE_DELETE_ARRAY(p) { if(p) { delete[] (p); (p) = nullptr; } }
    #define YG_SAFE_RELEASE(p) { if(p) { (p)->Release(); (p) = nullptr; } }
    
    // 禁用拷贝和赋值的宏
    #define YG_DISABLE_COPY_AND_ASSIGN(TypeName) \
        TypeName(const TypeName&) = delete; \
        TypeName& operator=(const TypeName&) = delete
    
    // 字符串转换工具
    String StringToWString(const StringA& str);
    StringA WStringToString(const String& wstr);
    
    // 路径处理工具
    String GetApplicationPath();
    String GetTempPath();
    bool IsValidPath(const String& path);
    bool PathExists(const String& path);
    
    // 系统工具
    bool IsRunningAsAdmin();
    bool IsWindows64Bit();
    String GetWindowsVersion();
    
} // namespace YG
