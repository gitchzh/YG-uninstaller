/**
 * @file UninstallerService.cpp
 * @brief 程序卸载服务实现
 * @author gmrchzh@gmail.com
 * @version 1.0.1
 * @date 2025-09-17
 */

#include "services/UninstallerService.h"
#include "services/ResidualScanner.h"
#include "core/Logger.h"
#include <windows.h>
#include <shellapi.h>
#include <shlobj.h>
#include <cstdio>

namespace YG {
    
    UninstallerService::UninstallerService() {
        YG_LOG_INFO(L"卸载服务已创建");
    }
    
    UninstallerService::~UninstallerService() {
        YG_LOG_INFO(L"卸载服务已销毁");
    }
    
    void UninstallerService::SetUninstallCompleteCallback(UninstallCompleteCallback callback) {
        m_completeCallback = callback;
    }
    
    void UninstallerService::SetResidualScanner(std::shared_ptr<ResidualScanner> scanner) {
        m_scanner = scanner;
    }
    
    ErrorCode UninstallerService::UninstallProgram(const ProgramInfo& program, UninstallMode mode) {
        // 验证程序信息
        if (program.name.empty() || program.uninstallString.empty()) {
            YG_LOG_ERROR(L"程序信息不完整，无法卸载: " + program.name);
            return ErrorCode::InvalidParameter;
        }
        
        YG_LOG_INFO(L"开始卸载程序: " + program.name);
        YG_LOG_INFO(L"卸载命令: " + program.uninstallString);
        
        // 根据卸载模式执行不同的卸载策略
        ErrorCode result = ErrorCode::GeneralError;
        
        switch (mode) {
            case UninstallMode::Standard:
                result = ExecuteStandardUninstall(program);
                break;
            case UninstallMode::Silent:
                result = ExecuteSilentUninstall(program);
                break;
            case UninstallMode::Force:
                result = ExecuteForceUninstall(program);
                break;
            case UninstallMode::Deep:
                result = ExecuteDeepUninstall(program);
                break;
            default:
                result = ErrorCode::InvalidParameter;
                break;
        }
        
        // 触发卸载完成回调
        bool success = (result == ErrorCode::Success);
        if (m_completeCallback) {
            YG_LOG_INFO(L"触发卸载完成回调，成功: " + std::to_wstring(success));
            m_completeCallback(program, success);
        }
        
        return result;
    }
    
    ErrorCode UninstallerService::ExecuteStandardUninstall(const ProgramInfo& program) {
        YG_LOG_INFO(L"执行标准卸载: " + program.name);
        
        // 运行卸载程序
        STARTUPINFOW si = {};
        PROCESS_INFORMATION pi = {};
        si.cb = sizeof(si);
        
        // 创建进程
        std::wstring cmdLine = program.uninstallString;
        BOOL created = CreateProcessW(
            nullptr,
            const_cast<wchar_t*>(cmdLine.c_str()),
            nullptr,
            nullptr,
            FALSE,
            0, // 显示窗口
            nullptr,
            nullptr,
            &si,
            &pi
        );
        
        if (!created) {
            DWORD error = GetLastError();
            YG_LOG_ERROR(L"启动卸载程序失败，错误代码: " + std::to_wstring(error));
            return ErrorCode::GeneralError;
        }
        
        YG_LOG_INFO(L"卸载程序已启动，等待完成...");
        
        // 等待进程完成（最多5分钟）
        DWORD waitResult = WaitForSingleObject(pi.hProcess, 300000);
        
        DWORD exitCode = 0;
        GetExitCodeProcess(pi.hProcess, &exitCode);
        
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        
        if (waitResult == WAIT_TIMEOUT) {
            YG_LOG_ERROR(L"卸载程序超时");
            return ErrorCode::GeneralError;
        } else if (waitResult == WAIT_OBJECT_0) {
            YG_LOG_INFO(L"卸载程序完成，退出代码: " + std::to_wstring(exitCode));
            return (exitCode == 0) ? ErrorCode::Success : ErrorCode::GeneralError;
        }
        
        return ErrorCode::GeneralError;
    }
    
    ErrorCode UninstallerService::ExecuteSilentUninstall(const ProgramInfo& program) {
        YG_LOG_INFO(L"执行静默卸载: " + program.name);
        
        // 添加静默参数
        std::wstring silentCmd = program.uninstallString;
        if (silentCmd.find(L"/S") == String::npos && 
            silentCmd.find(L"/SILENT") == String::npos &&
            silentCmd.find(L"/VERYSILENT") == String::npos) {
            silentCmd += L" /S";
        }
        
        // 创建静默进程
        STARTUPINFOW si = {};
        PROCESS_INFORMATION pi = {};
        si.cb = sizeof(si);
        si.dwFlags = STARTF_USESHOWWINDOW;
        si.wShowWindow = SW_HIDE;
        
        BOOL created = CreateProcessW(
            nullptr,
            const_cast<wchar_t*>(silentCmd.c_str()),
            nullptr,
            nullptr,
            FALSE,
            CREATE_NO_WINDOW,
            nullptr,
            nullptr,
            &si,
            &pi
        );
        
        if (!created) {
            YG_LOG_ERROR(L"启动静默卸载失败");
            return ErrorCode::GeneralError;
        }
        
        // 等待完成（3分钟超时）
        DWORD waitResult = WaitForSingleObject(pi.hProcess, 180000);
        
        DWORD exitCode = 0;
        GetExitCodeProcess(pi.hProcess, &exitCode);
        
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        
        YG_LOG_INFO(L"静默卸载完成，退出代码: " + std::to_wstring(exitCode));
        return (waitResult == WAIT_OBJECT_0 && exitCode == 0) ? ErrorCode::Success : ErrorCode::GeneralError;
    }
    
    ErrorCode UninstallerService::ExecuteForceUninstall(const ProgramInfo& program) {
        YG_LOG_INFO(L"执行强制卸载: " + program.name);
        
        // 先尝试标准卸载
        ErrorCode standardResult = ExecuteStandardUninstall(program);
        if (standardResult == ErrorCode::Success) {
            YG_LOG_INFO(L"标准卸载成功");
            return ErrorCode::Success;
        }
        
        YG_LOG_WARNING(L"标准卸载失败，开始强制清理");
        
        // 强制删除安装目录
        if (!program.installLocation.empty()) {
            YG_LOG_INFO(L"删除安装目录: " + program.installLocation);
            
            SHFILEOPSTRUCTW fileOp = {};
            fileOp.wFunc = FO_DELETE;
            fileOp.fFlags = FOF_ALLOWUNDO | FOF_NOCONFIRMATION | FOF_SILENT;
            
            std::wstring pathToDelete = program.installLocation + L"\0";
            fileOp.pFrom = pathToDelete.c_str();
            
            int result = SHFileOperationW(&fileOp);
            if (result == 0) {
                YG_LOG_INFO(L"安装目录删除成功");
            } else {
                YG_LOG_WARNING(L"安装目录删除失败，错误代码: " + std::to_wstring(result));
            }
        }
        
        YG_LOG_INFO(L"强制卸载完成");
        return ErrorCode::Success;
    }
    
    ErrorCode UninstallerService::ExecuteDeepUninstall(const ProgramInfo& program) {
        YG_LOG_INFO(L"执行深度卸载: " + program.name);
        
        // 深度卸载 = 强制卸载 + 注册表清理
        ErrorCode result = ExecuteForceUninstall(program);
        
        if (result == ErrorCode::Success) {
            YG_LOG_INFO(L"开始深度清理...");
            
            // 清理注册表项
            CleanupRegistryEntries(program);
            
            // 清理快捷方式
            CleanupShortcuts(program);
            
            YG_LOG_INFO(L"深度卸载完成");
        }
        
        return result;
    }
    
    ErrorCode UninstallerService::CleanupRegistryEntries(const ProgramInfo& program) {
        YG_LOG_INFO(L"清理注册表项: " + program.name);
        
        // 清理卸载注册表项
        const wchar_t* uninstallKeys[] = {
            L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall",
            L"SOFTWARE\\WOW6432Node\\Microsoft\\Windows\\CurrentVersion\\Uninstall"
        };
        
        for (const auto& keyPath : uninstallKeys) {
            HKEY hKey;
            if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, keyPath, 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
                
                DWORD index = 0;
                wchar_t subKeyName[256];
                DWORD subKeyNameSize = sizeof(subKeyName) / sizeof(wchar_t);
                
                while (RegEnumKeyExW(hKey, index++, subKeyName, &subKeyNameSize, 
                                   nullptr, nullptr, nullptr, nullptr) == ERROR_SUCCESS) {
                    
                    HKEY hSubKey;
                    if (RegOpenKeyExW(hKey, subKeyName, 0, KEY_READ, &hSubKey) == ERROR_SUCCESS) {
                        
                        wchar_t displayName[512] = {0};
                        DWORD displayNameSize = sizeof(displayName);
                        if (RegQueryValueExW(hSubKey, L"DisplayName", nullptr, nullptr, 
                                           (LPBYTE)displayName, &displayNameSize) == ERROR_SUCCESS) {
                            
                            if (wcscmp(displayName, program.name.c_str()) == 0) {
                                RegCloseKey(hSubKey);
                                
                                // 删除注册表项
                                if (RegDeleteKeyW(hKey, subKeyName) == ERROR_SUCCESS) {
                                    YG_LOG_INFO(L"删除注册表项成功: " + String(subKeyName));
                                } else {
                                    YG_LOG_WARNING(L"删除注册表项失败: " + String(subKeyName));
                                }
                                break;
                            }
                        }
                        
                        RegCloseKey(hSubKey);
                    }
                    
                    subKeyNameSize = sizeof(subKeyName) / sizeof(wchar_t);
                }
                
                RegCloseKey(hKey);
            }
        }
        
        return ErrorCode::Success;
    }
    
    ErrorCode UninstallerService::CleanupShortcuts(const ProgramInfo& program) {
        YG_LOG_INFO(L"清理快捷方式: " + program.name);
        
        // 清理开始菜单快捷方式
        wchar_t startMenuPath[MAX_PATH];
        if (SHGetFolderPathW(nullptr, CSIDL_COMMON_STARTMENU, nullptr, SHGFP_TYPE_CURRENT, startMenuPath) == S_OK) {
            String searchPath = String(startMenuPath) + L"\\Programs";
            
            WIN32_FIND_DATAW findData;
            String pattern = searchPath + L"\\*" + program.name + L"*";
            HANDLE hFind = FindFirstFileW(pattern.c_str(), &findData);
            
            if (hFind != INVALID_HANDLE_VALUE) {
                do {
                    String shortcutPath = searchPath + L"\\" + findData.cFileName;
                    if (DeleteFileW(shortcutPath.c_str())) {
                        YG_LOG_INFO(L"删除开始菜单快捷方式: " + shortcutPath);
                    }
                } while (FindNextFileW(hFind, &findData));
                
                FindClose(hFind);
            }
        }
        
        // 清理桌面快捷方式
        wchar_t desktopPath[MAX_PATH];
        if (SHGetFolderPathW(nullptr, CSIDL_DESKTOP, nullptr, SHGFP_TYPE_CURRENT, desktopPath) == S_OK) {
            
            WIN32_FIND_DATAW findData;
            String pattern = String(desktopPath) + L"\\*" + program.name + L"*";
            HANDLE hFind = FindFirstFileW(pattern.c_str(), &findData);
            
            if (hFind != INVALID_HANDLE_VALUE) {
                do {
                    String shortcutPath = String(desktopPath) + L"\\" + findData.cFileName;
                    if (DeleteFileW(shortcutPath.c_str())) {
                        YG_LOG_INFO(L"删除桌面快捷方式: " + shortcutPath);
                    }
                } while (FindNextFileW(hFind, &findData));
                
                FindClose(hFind);
            }
        }
        
        return ErrorCode::Success;
    }
    
    // PathExists 函数已经在 Common.cpp 中定义
    
} // namespace YG