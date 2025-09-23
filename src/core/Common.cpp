/**
 * @file Common.cpp
 * @brief 通用功能实现
 * @author gmrchzh@gmail.com
 * @version 1.0.1
 * @date 2025-09-17
 */

#include "core/Common.h"
#include <shlobj.h>

namespace YG {
    
    // 字符串转换工具
    String StringToWString(const StringA& str) {
        if (str.empty()) {
            return String();
        }
        
        int sizeNeeded = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, nullptr, 0);
        if (sizeNeeded <= 0) {
            return String();
        }
        
        String wstr(sizeNeeded - 1, L'\0');
        MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, &wstr[0], sizeNeeded);
        return wstr;
    }
    
    // C++17优化的字符串转换工具
    String StringToWStringView(StringViewA str) {
        if (str.empty()) {
            return String();
        }
        
        int sizeNeeded = MultiByteToWideChar(CP_UTF8, 0, str.data(), static_cast<int>(str.length()), nullptr, 0);
        if (sizeNeeded <= 0) {
            return String();
        }
        
        String wstr(sizeNeeded, L'\0');
        MultiByteToWideChar(CP_UTF8, 0, str.data(), static_cast<int>(str.length()), &wstr[0], sizeNeeded);
        return wstr;
    }
    
    StringA WStringToString(const String& wstr) {
        if (wstr.empty()) {
            return StringA();
        }
        
        int sizeNeeded = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, nullptr, 0, nullptr, nullptr);
        if (sizeNeeded <= 0) {
            return StringA();
        }
        
        StringA str(sizeNeeded - 1, '\0');
        WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, &str[0], sizeNeeded, nullptr, nullptr);
        return str;
    }
    
    // C++17优化的字符串转换工具
    StringA WStringToStringView(StringView wstr) {
        if (wstr.empty()) {
            return StringA();
        }
        
        int sizeNeeded = WideCharToMultiByte(CP_UTF8, 0, wstr.data(), static_cast<int>(wstr.length()), nullptr, 0, nullptr, nullptr);
        if (sizeNeeded <= 0) {
            return StringA();
        }
        
        StringA str(sizeNeeded, '\0');
        WideCharToMultiByte(CP_UTF8, 0, wstr.data(), static_cast<int>(wstr.length()), &str[0], sizeNeeded, nullptr, nullptr);
        return str;
    }
    
    // 路径处理工具
    String GetApplicationPath() {
        wchar_t path[MAX_PATH];
        if (GetModuleFileNameW(nullptr, path, MAX_PATH) == 0) {
            return String();
        }
        
        String fullPath(path);
        size_t lastSlash = fullPath.find_last_of(L"\\/");
        if (lastSlash != String::npos) {
            return fullPath.substr(0, lastSlash);
        }
        
        return fullPath;
    }
    
    // C++17安全的路径获取工具
    std::optional<String> GetApplicationPathSafe() {
        wchar_t path[MAX_PATH];
        if (GetModuleFileNameW(nullptr, path, MAX_PATH) == 0) {
            return std::nullopt;
        }
        
        String fullPath(path);
        size_t lastSlash = fullPath.find_last_of(L"\\/");
        if (lastSlash != String::npos) {
            return fullPath.substr(0, lastSlash);
        }
        
        return fullPath;
    }
    
    String GetTempPath() {
        wchar_t tempPath[MAX_PATH];
        DWORD result = ::GetTempPathW(MAX_PATH, tempPath);
        if (result == 0 || result > MAX_PATH) {
            return L"C:\\Temp";
        }
        return String(tempPath);
    }
    
    // C++17安全的临时路径获取工具
    std::optional<String> GetTempPathSafe() {
        wchar_t tempPath[MAX_PATH];
        DWORD result = ::GetTempPathW(MAX_PATH, tempPath);
        if (result == 0 || result > MAX_PATH) {
            return std::nullopt;
        }
        return String(tempPath);
    }
    
    bool IsValidPath(const String& path) {
        if (path.empty() || path.length() > MAX_PATH) {
            return false;
        }
        
        // 检查非法字符
        const wchar_t* illegalChars = L"<>:\"|?*";
        if (path.find_first_of(illegalChars) != String::npos) {
            return false;
        }
        
        // 检查路径格式
        if (path.length() >= 2) {
            // Windows路径格式检查
            if (path[1] == L':' && ((path[0] >= L'A' && path[0] <= L'Z') || 
                                   (path[0] >= L'a' && path[0] <= L'z'))) {
                return true;
            }
            
            // UNC路径检查
            if (path[0] == L'\\' && path[1] == L'\\') {
                return true;
            }
        }
        
        // 相对路径
        if (path[0] != L'\\' && path[1] != L':') {
            return true;
        }
        
        return false;
    }
    
    // C++17优化的路径验证工具
    bool IsValidPath(StringView path) {
        if (path.empty() || path.length() > MAX_PATH) {
            return false;
        }
        
        // 检查非法字符
        const wchar_t* illegalChars = L"<>:\"|?*";
        if (path.find_first_of(illegalChars) != StringView::npos) {
            return false;
        }
        
        // 检查路径格式
        if (path.length() >= 2) {
            // Windows路径格式检查
            if (path[1] == L':' && ((path[0] >= L'A' && path[0] <= L'Z') || 
                                   (path[0] >= L'a' && path[0] <= L'z'))) {
                return true;
            }
            
            // UNC路径检查
            if (path[0] == L'\\' && path[1] == L'\\') {
                return true;
            }
        }
        
        // 相对路径
        if (path[0] != L'\\' && path[1] != L':') {
            return true;
        }
        
        return false;
    }
    
    bool PathExists(const String& path) {
        if (path.empty()) {
            return false;
        }
        
        DWORD attributes = GetFileAttributesW(path.c_str());
        return (attributes != INVALID_FILE_ATTRIBUTES);
    }
    
    // C++17优化的路径存在检查
    bool PathExists(StringView path) {
        if (path.empty()) {
            return false;
        }
        
        // 创建临时字符串用于API调用
        String tempPath(path);
        DWORD attributes = GetFileAttributesW(tempPath.c_str());
        return (attributes != INVALID_FILE_ATTRIBUTES);
    }
    
    // 系统工具
    bool IsRunningAsAdmin() {
        BOOL isAdmin = FALSE;
        PSID administratorsGroup = nullptr;
        
        // 创建管理员组的SID
        SID_IDENTIFIER_AUTHORITY ntAuthority = SECURITY_NT_AUTHORITY;
        if (AllocateAndInitializeSid(&ntAuthority, 2, 
                                   SECURITY_BUILTIN_DOMAIN_RID, 
                                   DOMAIN_ALIAS_RID_ADMINS,
                                   0, 0, 0, 0, 0, 0, 
                                   &administratorsGroup)) {
            // 检查当前进程是否在管理员组中
            if (!CheckTokenMembership(nullptr, administratorsGroup, &isAdmin)) {
                isAdmin = FALSE;
            }
            FreeSid(administratorsGroup);
        }
        
        return (isAdmin == TRUE);
    }
    
    bool IsWindows64Bit() {
        SYSTEM_INFO sysInfo;
        GetNativeSystemInfo(&sysInfo);
        return (sysInfo.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64 ||
                sysInfo.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_IA64);
    }
    
    String GetWindowsVersion() {
        OSVERSIONINFOEXW osvi;
        ZeroMemory(&osvi, sizeof(OSVERSIONINFOEXW));
        osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEXW);
        
        // 使用RtlGetVersion获取真实版本信息
        typedef NTSTATUS(WINAPI* RtlGetVersionPtr)(PRTL_OSVERSIONINFOW);
        HMODULE hMod = GetModuleHandleW(L"ntdll.dll");
        if (hMod) {
            RtlGetVersionPtr RtlGetVersion = (RtlGetVersionPtr)GetProcAddress(hMod, "RtlGetVersion");
            if (RtlGetVersion) {
                RTL_OSVERSIONINFOW rovi;
                ZeroMemory(&rovi, sizeof(RTL_OSVERSIONINFOW));
                rovi.dwOSVersionInfoSize = sizeof(RTL_OSVERSIONINFOW);
                
                if (RtlGetVersion(&rovi) == 0) {
                    osvi.dwMajorVersion = rovi.dwMajorVersion;
                    osvi.dwMinorVersion = rovi.dwMinorVersion;
                    osvi.dwBuildNumber = rovi.dwBuildNumber;
                }
            }
        }
        
        // 如果RtlGetVersion失败，使用GetVersionEx
        if (osvi.dwMajorVersion == 0) {
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4996) // 忽略GetVersionEx的弃用警告
#endif
            if (!GetVersionExW((OSVERSIONINFOW*)&osvi)) {
                return L"Unknown";
            }
#ifdef _MSC_VER
#pragma warning(pop)
#endif
        }
        
        // 构建版本字符串
        String version;
        if (osvi.dwMajorVersion == 10 && osvi.dwMinorVersion == 0) {
            if (osvi.dwBuildNumber >= 22000) {
                version = L"Windows 11";
            } else {
                version = L"Windows 10";
            }
        } else if (osvi.dwMajorVersion == 6) {
            switch (osvi.dwMinorVersion) {
                case 3:
                    version = L"Windows 8.1";
                    break;
                case 2:
                    version = L"Windows 8";
                    break;
                case 1:
                    version = L"Windows 7";
                    break;
                case 0:
                    version = L"Windows Vista";
                    break;
                default:
                    version = L"Windows NT 6.x";
                    break;
            }
        } else if (osvi.dwMajorVersion == 5) {
            if (osvi.dwMinorVersion == 2) {
                version = L"Windows Server 2003";
            } else if (osvi.dwMinorVersion == 1) {
                version = L"Windows XP";
            } else {
                version = L"Windows 2000";
            }
        } else {
            version = L"Windows NT " + std::to_wstring(osvi.dwMajorVersion) + 
                     L"." + std::to_wstring(osvi.dwMinorVersion);
        }
        
        // 添加构建号
        version += L" (Build " + std::to_wstring(osvi.dwBuildNumber) + L")";
        
        // 添加架构信息
        if (IsWindows64Bit()) {
            version += L" x64";
        } else {
            version += L" x86";
        }
        
        return version;
    }
    
} // namespace YG
