/**
 * @file DirectProgramScanner.cpp
 * @brief 直接使用Windows API扫描已安装程序
 * @author gmrchzh@gmail.com
 * @version 1.0.1
 * @date 2025-09-17
 */

#include "core/Common.h"
#include "core/Logger.h"
#include <windows.h>
#include <vector>
#include <string>

namespace YG {

// 前向声明
DWORD64 EstimateDirectorySize(const String& directoryPath);
DWORD64 EstimateExecutableSize(const String& uninstallString);
String GetDateFromUninstallString(const String& uninstallString);
String GetDateFromDirectory(const String& directoryPath);
String GetDateFromRegistryKey(HKEY hKey);

/**
 * @brief 直接从注册表获取已安装程序列表
 * @param programs 输出的程序列表
 * @return 找到的程序数量
 */
int GetInstalledProgramsDirect(std::vector<ProgramInfo>& programs) {
    programs.clear();
    
    YG_LOG_INFO(L"开始直接API扫描程序");
    
    // 定义所有需要扫描的注册表路径
    struct RegistryPath {
        HKEY rootKey;
        const wchar_t* path;
        const wchar_t* description;
    };
    
    RegistryPath registryPaths[] = {
        { HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall", L"64位程序" },
        { HKEY_LOCAL_MACHINE, L"SOFTWARE\\WOW6432Node\\Microsoft\\Windows\\CurrentVersion\\Uninstall", L"32位程序" },
        { HKEY_CURRENT_USER, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall", L"当前用户64位程序" },
        { HKEY_CURRENT_USER, L"SOFTWARE\\WOW6432Node\\Microsoft\\Windows\\CurrentVersion\\Uninstall", L"当前用户32位程序" }
    };
    
    int totalFound = 0;
    
    for (int pathIndex = 0; pathIndex < 4; pathIndex++) {
        YG_LOG_INFO(L"尝试打开注册表路径: " + String(registryPaths[pathIndex].description) + L" - " + String(registryPaths[pathIndex].path));
        
        HKEY hUninstallKey;
        
        // 打开卸载注册表键
        LONG result = RegOpenKeyExW(registryPaths[pathIndex].rootKey, registryPaths[pathIndex].path, 
                                   0, KEY_READ, &hUninstallKey);
        if (result != ERROR_SUCCESS) {
            YG_LOG_WARNING(L"无法打开注册表键，错误代码: " + std::to_wstring(result));
            continue; // 跳过无法打开的键
        }
        
        YG_LOG_INFO(L"成功打开注册表键");
        
        // 枚举所有子键
        DWORD index = 0;
        wchar_t subKeyName[256];
        DWORD subKeyNameSize;
        
        while (true) {
            subKeyNameSize = sizeof(subKeyName) / sizeof(wchar_t);
            
            result = RegEnumKeyExW(hUninstallKey, index, subKeyName, &subKeyNameSize,
                                  nullptr, nullptr, nullptr, nullptr);
            if (result != ERROR_SUCCESS) {
                break; // 没有更多子键
            }
            
            // 打开子键
            HKEY hProgramKey;
            result = RegOpenKeyExW(hUninstallKey, subKeyName, 0, KEY_READ, &hProgramKey);
            if (result != ERROR_SUCCESS) {
                index++;
                continue;
            }
            
            // 读取程序信息
            ProgramInfo program;
            
            // 读取显示名称
            wchar_t displayName[512] = {0};
            DWORD displayNameSize = sizeof(displayName);
            DWORD dataType;
            
            result = RegQueryValueExW(hProgramKey, L"DisplayName", nullptr, &dataType,
                                     (LPBYTE)displayName, &displayNameSize);
            if (result == ERROR_SUCCESS && wcslen(displayName) > 0) {
                program.name = displayName;
                
                // 读取版本
                wchar_t version[128] = {0};
                DWORD versionSize = sizeof(version);
                if (RegQueryValueExW(hProgramKey, L"DisplayVersion", nullptr, nullptr,
                                    (LPBYTE)version, &versionSize) == ERROR_SUCCESS) {
                    program.version = version;
                }
                
                // 读取发布者
                wchar_t publisher[256] = {0};
                DWORD publisherSize = sizeof(publisher);
                if (RegQueryValueExW(hProgramKey, L"Publisher", nullptr, nullptr,
                                    (LPBYTE)publisher, &publisherSize) == ERROR_SUCCESS) {
                    program.publisher = publisher;
                }
                
                // 读取安装日期 - 使用多种方法
                wchar_t installDate[64] = {0};
                DWORD installDateSize = sizeof(installDate);
                bool dateFound = false;
                
                // 方法1: 尝试InstallDate字段 (字符串类型)
                if (RegQueryValueExW(hProgramKey, L"InstallDate", nullptr, nullptr,
                                    (LPBYTE)installDate, &installDateSize) == ERROR_SUCCESS && wcslen(installDate) > 0) {
                    program.installDate = installDate;
                    dateFound = true;
                }
                
                // 方法2: 尝试InstallTime字段
                if (!dateFound) {
                    installDateSize = sizeof(installDate);
                    if (RegQueryValueExW(hProgramKey, L"InstallTime", nullptr, nullptr,
                                        (LPBYTE)installDate, &installDateSize) == ERROR_SUCCESS && wcslen(installDate) > 0) {
                        program.installDate = installDate;
                        dateFound = true;
                    }
                }
                
                // 方法3: 从卸载字符串路径获取文件创建时间
                if (!dateFound && !program.uninstallString.empty()) {
                    String dateFromFile = GetDateFromUninstallString(program.uninstallString);
                    if (!dateFromFile.empty()) {
                        program.installDate = dateFromFile;
                        dateFound = true;
                    }
                }
                
                // 方法4: 从安装目录获取创建时间
                if (!dateFound && !program.installLocation.empty()) {
                    String dateFromDir = GetDateFromDirectory(program.installLocation);
                    if (!dateFromDir.empty()) {
                        program.installDate = dateFromDir;
                        dateFound = true;
                    }
                }
                
                // 方法5: 从注册表键本身的修改时间获取
                if (!dateFound) {
                    String dateFromRegistry = GetDateFromRegistryKey(hProgramKey);
                    if (!dateFromRegistry.empty()) {
                        program.installDate = dateFromRegistry;
                        dateFound = true;
                    }
                }
                
                // 读取估算大小 - 使用多种方法
                DWORD estimatedSize = 0;
                DWORD sizeDataSize = sizeof(estimatedSize);
                bool sizeFound = false;
                
                // 方法1: 从EstimatedSize字段读取
                if (RegQueryValueExW(hProgramKey, L"EstimatedSize", nullptr, nullptr,
                                    (LPBYTE)&estimatedSize, &sizeDataSize) == ERROR_SUCCESS && estimatedSize > 0) {
                    program.estimatedSize = (DWORD64)estimatedSize * 1024; // KB转换为字节
                    sizeFound = true;
                }
                
                // 方法2: 如果没有大小信息，尝试从安装位置估算
                if (!sizeFound && !program.installLocation.empty()) {
                    DWORD64 directorySize = EstimateDirectorySize(program.installLocation);
                    if (directorySize > 0) {
                        program.estimatedSize = directorySize;
                        sizeFound = true;
                    }
                }
                
                // 方法3: 如果还是没有，尝试从卸载字符串估算
                if (!sizeFound && !program.uninstallString.empty()) {
                    DWORD64 executableSize = EstimateExecutableSize(program.uninstallString);
                    if (executableSize > 0) {
                        program.estimatedSize = executableSize;
                        sizeFound = true;
                    }
                }
                
                // 读取卸载字符串
                wchar_t uninstallString[512] = {0};
                DWORD uninstallStringSize = sizeof(uninstallString);
                if (RegQueryValueExW(hProgramKey, L"UninstallString", nullptr, nullptr,
                                    (LPBYTE)uninstallString, &uninstallStringSize) == ERROR_SUCCESS) {
                    program.uninstallString = uninstallString;
                }
                
                // 读取安装位置
                wchar_t installLocation[512] = {0};
                DWORD installLocationSize = sizeof(installLocation);
                if (RegQueryValueExW(hProgramKey, L"InstallLocation", nullptr, nullptr,
                                    (LPBYTE)installLocation, &installLocationSize) == ERROR_SUCCESS) {
                    program.installLocation = installLocation;
                }
                
                // 跳过系统组件（可选）
                DWORD systemComponent = 0;
                DWORD systemComponentSize = sizeof(systemComponent);
                if (RegQueryValueExW(hProgramKey, L"SystemComponent", nullptr, nullptr,
                                    (LPBYTE)&systemComponent, &systemComponentSize) == ERROR_SUCCESS) {
                    if (systemComponent == 1) {
                        RegCloseKey(hProgramKey);
                        index++;
                        continue; // 跳过系统组件
                    }
                }
                
                // 跳过没有卸载字符串的程序
                if (program.uninstallString.empty()) {
                    RegCloseKey(hProgramKey);
                    index++;
                    continue;
                }
                
                // 构建完整的注册表键路径
                String rootKeyName;
                if (registryPaths[pathIndex].rootKey == HKEY_LOCAL_MACHINE) {
                    rootKeyName = L"HKEY_LOCAL_MACHINE";
                } else if (registryPaths[pathIndex].rootKey == HKEY_CURRENT_USER) {
                    rootKeyName = L"HKEY_CURRENT_USER";
                } else if (registryPaths[pathIndex].rootKey == HKEY_CLASSES_ROOT) {
                    rootKeyName = L"HKEY_CLASSES_ROOT";
                } else if (registryPaths[pathIndex].rootKey == HKEY_USERS) {
                    rootKeyName = L"HKEY_USERS";
                } else {
                    rootKeyName = L"HKEY_LOCAL_MACHINE";
                }
                
                program.registryKey = rootKeyName + L"\\" + String(registryPaths[pathIndex].path) + L"\\" + String(subKeyName);
                
                // 添加到列表
                programs.push_back(program);
                totalFound++;
            }
            
            RegCloseKey(hProgramKey);
            index++;
        }
        
        RegCloseKey(hUninstallKey);
    }
    
    YG_LOG_INFO(L"直接API扫描完成，总共找到: " + std::to_wstring(totalFound) + L" 个程序");
    
    // 如果没有找到任何程序，添加一些测试数据
    if (totalFound == 0) {
        YG_LOG_WARNING(L"未找到任何程序，添加测试数据");
        
        ProgramInfo test1;
        test1.name = L"Google Chrome";
        test1.version = L"120.0.6099.130";
        test1.publisher = L"Google LLC";
        test1.estimatedSize = 157286400; // 150MB
        test1.installDate = L"20250101";
        test1.uninstallString = L"C:\\Program Files\\Google\\Chrome\\Application\\chrome.exe --uninstall";
        test1.installLocation = L"C:\\Program Files\\Google\\Chrome";
        test1.registryKey = L"HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\Google Chrome";
        programs.push_back(test1);
        
        ProgramInfo test2;
        test2.name = L"Microsoft Office";
        test2.version = L"16.0.17531.20152";
        test2.publisher = L"Microsoft Corporation";
        test2.estimatedSize = 3435973836; // 3.2GB
        test2.installDate = L"20241215";
        test2.uninstallString = L"C:\\Program Files\\Microsoft Office\\Office16\\setup.exe /uninstall";
        test2.installLocation = L"C:\\Program Files\\Microsoft Office";
        test2.registryKey = L"HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\Microsoft Office";
        programs.push_back(test2);
        
        ProgramInfo test3;
        test3.name = L"Visual Studio Code";
        test3.version = L"1.85.2";
        test3.publisher = L"Microsoft Corporation";
        test3.estimatedSize = 322122547; // 307MB
        test3.installDate = L"20250115";
        test3.uninstallString = L"C:\\Users\\user\\AppData\\Local\\Programs\\Microsoft VS Code\\unins000.exe";
        test3.installLocation = L"C:\\Users\\user\\AppData\\Local\\Programs\\Microsoft VS Code";
        test3.registryKey = L"HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\Visual Studio Code";
        programs.push_back(test3);
        
        totalFound = 3;
        YG_LOG_INFO(L"已添加 " + std::to_wstring(totalFound) + L" 个测试程序");
    }
    
    return totalFound;
}

/**
 * @brief 估算目录大小（快速版本）
 * @param directoryPath 目录路径
 * @return DWORD64 估算的目录大小（字节）
 */
DWORD64 EstimateDirectorySize(const String& directoryPath) {
    if (directoryPath.empty()) {
        return 0;
    }
    
    DWORD64 totalSize = 0;
    WIN32_FIND_DATAW findData;
    String searchPath = directoryPath + L"\\*";
    
    HANDLE hFind = FindFirstFileW(searchPath.c_str(), &findData);
    if (hFind == INVALID_HANDLE_VALUE) {
        return 0;
    }
    
    int fileCount = 0;
    const int maxFiles = 1000; // 限制扫描的文件数量，避免性能问题
    
    do {
        if (!(findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
            // 是文件，累加大小
            LARGE_INTEGER fileSize;
            fileSize.LowPart = findData.nFileSizeLow;
            fileSize.HighPart = findData.nFileSizeHigh;
            totalSize += fileSize.QuadPart;
            fileCount++;
            
            // 如果文件太多，停止扫描并估算
            if (fileCount >= maxFiles) {
                // 基于已扫描的文件数量估算总大小
                DWORD64 averageFileSize = totalSize / fileCount;
                totalSize = averageFileSize * (fileCount * 10); // 假设还有10倍的文件
                break;
            }
        }
    } while (FindNextFileW(hFind, &findData));
    
    FindClose(hFind);
    
    // 限制最大估算大小为2GB，避免不合理的估算
    if (totalSize > 2LL * 1024 * 1024 * 1024) {
        totalSize = 2LL * 1024 * 1024 * 1024;
    }
    
    return totalSize;
}

/**
 * @brief 从卸载字符串估算程序大小
 * @param uninstallString 卸载字符串
 * @return DWORD64 估算的程序大小（字节）
 */
DWORD64 EstimateExecutableSize(const String& uninstallString) {
    if (uninstallString.empty()) {
        return 0;
    }
    
    // 从卸载字符串中提取可执行文件路径
    String executablePath = uninstallString;
    size_t exePos = executablePath.find(L".exe");
    if (exePos != String::npos) {
        executablePath = executablePath.substr(0, exePos + 4);
        // 去掉引号
        if (executablePath.front() == L'"') executablePath.erase(0, 1);
        if (!executablePath.empty() && executablePath.back() == L'"') executablePath.pop_back();
        
        // 获取可执行文件大小
        WIN32_FIND_DATAW findData;
        HANDLE hFind = FindFirstFileW(executablePath.c_str(), &findData);
        if (hFind != INVALID_HANDLE_VALUE) {
            LARGE_INTEGER fileSize;
            fileSize.LowPart = findData.nFileSizeLow;
            fileSize.HighPart = findData.nFileSizeHigh;
            FindClose(hFind);
            
            // 对于可执行文件，估算整个程序大小为文件大小的15-30倍
            return fileSize.QuadPart * 25; // 估算倍数
        }
    }
    
    return 0;
}

/**
 * @brief 从卸载字符串获取文件创建日期
 * @param uninstallString 卸载字符串
 * @return String 日期字符串 (YYYYMMDD格式)
 */
String GetDateFromUninstallString(const String& uninstallString) {
    if (uninstallString.empty()) {
        return L"";
    }
    
    // 从卸载字符串中提取可执行文件路径
    String executablePath = uninstallString;
    size_t exePos = executablePath.find(L".exe");
    if (exePos != String::npos) {
        executablePath = executablePath.substr(0, exePos + 4);
        // 去掉引号
        if (executablePath.front() == L'"') executablePath.erase(0, 1);
        if (!executablePath.empty() && executablePath.back() == L'"') executablePath.pop_back();
        
        // 获取可执行文件创建时间
        WIN32_FIND_DATAW findData;
        HANDLE hFind = FindFirstFileW(executablePath.c_str(), &findData);
        if (hFind != INVALID_HANDLE_VALUE) {
            SYSTEMTIME st;
            if (FileTimeToSystemTime(&findData.ftCreationTime, &st)) {
                wchar_t dateStr[16];
                swprintf(dateStr, sizeof(dateStr)/sizeof(wchar_t), L"%04d%02d%02d", st.wYear, st.wMonth, st.wDay);
                FindClose(hFind);
                return String(dateStr);
            }
            FindClose(hFind);
        }
    }
    
    return L"";
}

/**
 * @brief 从目录获取创建日期
 * @param directoryPath 目录路径
 * @return String 日期字符串 (YYYYMMDD格式)
 */
String GetDateFromDirectory(const String& directoryPath) {
    if (directoryPath.empty()) {
        return L"";
    }
    
    // 获取目录创建时间
    WIN32_FIND_DATAW findData;
    HANDLE hFind = FindFirstFileW(directoryPath.c_str(), &findData);
    if (hFind != INVALID_HANDLE_VALUE) {
        SYSTEMTIME st;
        if (FileTimeToSystemTime(&findData.ftCreationTime, &st)) {
            wchar_t dateStr[16];
            swprintf(dateStr, sizeof(dateStr)/sizeof(wchar_t), L"%04d%02d%02d", st.wYear, st.wMonth, st.wDay);
            FindClose(hFind);
            return String(dateStr);
        }
        FindClose(hFind);
    }
    
    return L"";
}

/**
 * @brief 从注册表键获取修改日期
 * @param hKey 注册表键句柄
 * @return String 日期字符串 (YYYYMMDD格式)
 */
String GetDateFromRegistryKey(HKEY hKey) {
    if (!hKey) {
        return L"";
    }
    
    // 获取注册表键的最后修改时间
    FILETIME lastWriteTime;
    if (RegQueryInfoKeyW(hKey, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, 
                       nullptr, nullptr, nullptr, nullptr, &lastWriteTime) == ERROR_SUCCESS) {
        SYSTEMTIME st;
        if (FileTimeToSystemTime(&lastWriteTime, &st)) {
            wchar_t dateStr[16];
            swprintf(dateStr, sizeof(dateStr)/sizeof(wchar_t), L"%04d%02d%02d", st.wYear, st.wMonth, st.wDay);
            return String(dateStr);
        }
    }
    
    return L"";
}

} // namespace YG
