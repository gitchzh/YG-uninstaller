/**
 * @file ProgramDetector.cpp
 * @brief 程序检测和扫描服务实现
 * @author gmrchzh@gmail.com
 * @version 1.0.0
 * @date 2025-09-17
 */

#include "services/ProgramDetector.h"
#include "utils/RegistryHelper.h"
#include <windows.h>
#include <shlobj.h>
#include <vector>
#include <algorithm>
#include <cstdio>  // 为swprintf_s提供支持

namespace YG {
    
    ProgramDetector::ProgramDetector() 
        : m_scanning(false), m_stopRequested(false), 
          m_includeSystemComponents(false), m_deepScanEnabled(false),
          m_scanTimeout(30000), m_totalFound(0), m_lastScanTime(0) {
    }
    
    ProgramDetector::~ProgramDetector() {
        StopScan();
    }
    
    ErrorCode ProgramDetector::StartScan(bool includeSystemComponents,
                                       const ScanProgressCallback& progressCallback,
                                       const ScanCompletedCallback& completedCallback) {
        if (m_scanning.load()) {
            return ErrorCode::OperationInProgress;
        }
        
        m_includeSystemComponents = includeSystemComponents;
        m_progressCallback = progressCallback;
        m_completedCallback = completedCallback;
        m_stopRequested = false;
        
        // 启动扫描线程
        m_scanThread = YG::MakeUnique<std::thread>(&ProgramDetector::ScanWorkerThread, this);
        
        return ErrorCode::Success;
    }
    
    ErrorCode ProgramDetector::ScanSync(bool includeSystemComponents, std::vector<ProgramInfo>& programs) {
        if (m_scanning.load()) {
            return ErrorCode::OperationInProgress;
        }
        
        m_includeSystemComponents = includeSystemComponents;
        m_programs.clear();
        
        DWORD startTime = GetTickCount();
        
        // 扫描注册表卸载信息
        ErrorCode result = ScanRegistryUninstall(m_programs, includeSystemComponents);
        if (result != ErrorCode::Success) {
            return result;
        }
        
        // 扫描Windows Store应用
        if (includeSystemComponents) {
            ScanWindowsStoreApps(m_programs);
        }
        
        m_lastScanTime = GetTickCount() - startTime;
        m_totalFound = static_cast<int>(m_programs.size());
        
        programs = m_programs;
        return ErrorCode::Success;
    }
    
    void ProgramDetector::StopScan() {
        m_stopRequested = true;
        if (m_scanThread && m_scanThread->joinable()) {
            m_scanThread->join();
        }
        m_scanning = false;
    }
    
    bool ProgramDetector::IsScanning() const {
        return m_scanning.load();
    }
    
    void ProgramDetector::SetScanTimeout(DWORD timeoutMs) {
        m_scanTimeout = timeoutMs;
    }
    
    void ProgramDetector::EnableDeepScan(bool enable) {
        m_deepScanEnabled = enable;
    }
    
    void ProgramDetector::GetScanStatistics(int& totalFound, DWORD& scanTimeMs, String& lastScanTime) {
        totalFound = m_totalFound;
        scanTimeMs = m_lastScanTime;
        
        // 获取当前时间作为最后扫描时间
        SYSTEMTIME st;
        GetLocalTime(&st);
        wchar_t timeStr[64];
#ifdef _MSC_VER
        swprintf_s(timeStr, L"%04d-%02d-%02d %02d:%02d:%02d", 
                  st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
#else
        swprintf(timeStr, sizeof(timeStr)/sizeof(wchar_t), L"%04d-%02d-%02d %02d:%02d:%02d", 
                st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
#endif
        lastScanTime = timeStr;
    }
    
    ErrorCode ProgramDetector::ScanRegistryUninstall(std::vector<ProgramInfo>& programs, bool includeSystemComponents) {
        YG_LOG_INFO(L"开始扫描注册表卸载信息");
        
        const wchar_t* uninstallKeys[] = {
            L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall",
            L"SOFTWARE\\WOW6432Node\\Microsoft\\Windows\\CurrentVersion\\Uninstall"
        };
        
        for (int keyIndex = 0; keyIndex < 2; keyIndex++) {
            YG_LOG_INFO(L"尝试打开注册表键: " + String(uninstallKeys[keyIndex]));
            
            HKEY hKey;
            LONG result = RegOpenKeyExW(HKEY_LOCAL_MACHINE, uninstallKeys[keyIndex], 0, KEY_READ, &hKey);
            if (result != ERROR_SUCCESS) {
                YG_LOG_WARNING(L"无法打开注册表键，错误代码: " + std::to_wstring(result));
                continue;
            }
            
            YG_LOG_INFO(L"成功打开注册表键");
            
            DWORD index = 0;
            wchar_t subKeyName[256];
            DWORD subKeyNameSize = sizeof(subKeyName) / sizeof(wchar_t);
            int foundCount = 0;
            
            while (RegEnumKeyExW(hKey, index++, subKeyName, &subKeyNameSize, nullptr, nullptr, nullptr, nullptr) == ERROR_SUCCESS) {
                if (m_stopRequested) {
                    RegCloseKey(hKey);
                    return ErrorCode::OperationCancelled;
                }
                
                ProgramInfo programInfo;
                ErrorCode result = GetProgramInfoFromRegistry(hKey, subKeyName, programInfo);
                if (result == ErrorCode::Success) {
                    foundCount++;
                    YG_LOG_INFO(L"找到程序: " + programInfo.name);
                    // 检查是否应该包含系统组件
                    if (!includeSystemComponents && IsSystemComponent(programInfo)) {
                        subKeyNameSize = sizeof(subKeyName) / sizeof(wchar_t);
                        continue;
                    }
                    
                    programs.push_back(programInfo);
                    
                    // 调试信息
                    YG_LOG_INFO(L"找到程序: " + programInfo.name);
                } else {
                    // 调试信息
                    YG_LOG_DEBUG(L"跳过无效程序项: " + String(subKeyName));
                }
                
                subKeyNameSize = sizeof(subKeyName) / sizeof(wchar_t);
                
                // 更新进度
                if (m_progressCallback) {
                    int progress = (index * 100) / 200; // 估算进度
                    UpdateProgress(progress, programInfo.name);
                }
            }
            
            YG_LOG_INFO(L"注册表键扫描完成，找到 " + std::to_wstring(foundCount) + L" 个程序");
            RegCloseKey(hKey);
        }
        
        YG_LOG_INFO(L"注册表扫描完成，总计找到 " + std::to_wstring(programs.size()) + L" 个程序");
        
        return ErrorCode::Success;
    }
    
    ErrorCode ProgramDetector::ScanWindowsStoreApps(std::vector<ProgramInfo>& programs) {
        YG_LOG_INFO(L"开始扫描Windows Store应用");
        
        // 扫描当前用户的UWP应用包
        const wchar_t* uwpKeyPath = L"SOFTWARE\\Classes\\Local Settings\\Software\\Microsoft\\Windows\\CurrentVersion\\AppModel\\Repository\\Packages";
        
        HKEY hKey;
        LONG result = RegOpenKeyExW(HKEY_CURRENT_USER, uwpKeyPath, 0, KEY_READ, &hKey);
        if (result != ERROR_SUCCESS) {
            YG_LOG_WARNING(L"无法打开UWP应用注册表键，错误代码: " + std::to_wstring(result));
            return ErrorCode::RegistryError;
        }
        
        DWORD index = 0;
        wchar_t subKeyName[256];
        DWORD subKeyNameSize = sizeof(subKeyName) / sizeof(wchar_t);
        int foundCount = 0;
        
        while (RegEnumKeyExW(hKey, index++, subKeyName, &subKeyNameSize, 
                           nullptr, nullptr, nullptr, nullptr) == ERROR_SUCCESS) {
            
            if (m_stopRequested) {
                RegCloseKey(hKey);
                return ErrorCode::OperationCancelled;
            }
            
            // 解析UWP应用包名
            String packageName(subKeyName);
            
            // 跳过系统应用和框架
            if (packageName.find(L"Microsoft.") == 0 ||
                packageName.find(L"Windows.") == 0 ||
                packageName.find(L"Microsoft.VCLibs") != String::npos ||
                packageName.find(L"Microsoft.NET") != String::npos) {
                subKeyNameSize = sizeof(subKeyName) / sizeof(wchar_t);
                continue;
            }
            
            // 提取应用名称（包名的第一部分）
            size_t underscorePos = packageName.find(L'_');
            if (underscorePos != String::npos) {
                String appName = packageName.substr(0, underscorePos);
                
                // 创建UWP应用信息
                ProgramInfo uwpApp;
                uwpApp.name = appName;
                uwpApp.displayName = appName;
                uwpApp.publisher = L"Microsoft Store";
                uwpApp.version = L"Store App";
                uwpApp.installLocation = L"Windows Apps";
                uwpApp.uninstallString = L"powershell -Command \"Get-AppxPackage " + packageName + L" | Remove-AppxPackage\"";
                uwpApp.isSystemComponent = false;
                
                programs.push_back(uwpApp);
                foundCount++;
            }
            
            subKeyNameSize = sizeof(subKeyName) / sizeof(wchar_t);
        }
        
        RegCloseKey(hKey);
        
        YG_LOG_INFO(L"UWP应用扫描完成，找到 " + std::to_wstring(foundCount) + L" 个应用");
        return ErrorCode::Success;
    }
    
    ErrorCode ProgramDetector::ScanPortablePrograms(std::vector<ProgramInfo>& programs) {
        // 便携程序扫描的简化实现
        // TODO: 实现便携程序检测
        return ErrorCode::Success;
    }
    
    ErrorCode ProgramDetector::GetProgramInfoFromRegistry(HKEY hParentKey, const String& subKeyName, ProgramInfo& programInfo) {
        HKEY hSubKey;
        LONG result = RegOpenKeyExW(hParentKey, subKeyName.c_str(), 0, KEY_READ, &hSubKey);
        if (result != ERROR_SUCCESS) {
            return ErrorCode::RegistryError;
        }
        
        // 读取程序名称
        wchar_t displayName[512] = {0};
        DWORD displayNameSize = sizeof(displayName);
        if (RegQueryValueExW(hSubKey, L"DisplayName", nullptr, nullptr, 
                           (LPBYTE)displayName, &displayNameSize) == ERROR_SUCCESS) {
            programInfo.name = displayName;
            programInfo.displayName = displayName;  // 同时设置displayName字段
        } else {
            // 如果没有DisplayName，跳过此项
            RegCloseKey(hSubKey);
            return ErrorCode::DataNotFound;
        }
        
        // 读取版本
        wchar_t version[128] = {0};
        DWORD versionSize = sizeof(version);
        if (RegQueryValueExW(hSubKey, L"DisplayVersion", nullptr, nullptr, 
                           (LPBYTE)version, &versionSize) == ERROR_SUCCESS) {
            programInfo.version = version;
        }
        
        // 读取发布者
        wchar_t publisher[256] = {0};
        DWORD publisherSize = sizeof(publisher);
        if (RegQueryValueExW(hSubKey, L"Publisher", nullptr, nullptr, 
                           (LPBYTE)publisher, &publisherSize) == ERROR_SUCCESS) {
            programInfo.publisher = publisher;
        }
        
        // 读取安装路径
        wchar_t installLocation[MAX_PATH] = {0};
        DWORD installLocationSize = sizeof(installLocation);
        if (RegQueryValueExW(hSubKey, L"InstallLocation", nullptr, nullptr, 
                           (LPBYTE)installLocation, &installLocationSize) == ERROR_SUCCESS) {
            programInfo.installLocation = installLocation;
        }
        
        // 读取卸载字符串
        wchar_t uninstallString[512] = {0};
        DWORD uninstallStringSize = sizeof(uninstallString);
        if (RegQueryValueExW(hSubKey, L"UninstallString", nullptr, nullptr, 
                           (LPBYTE)uninstallString, &uninstallStringSize) == ERROR_SUCCESS) {
            programInfo.uninstallString = uninstallString;
        }
        
        // 读取安装日期
        wchar_t installDate[32] = {0};
        DWORD installDateSize = sizeof(installDate);
        if (RegQueryValueExW(hSubKey, L"InstallDate", nullptr, nullptr, 
                           (LPBYTE)installDate, &installDateSize) == ERROR_SUCCESS) {
            programInfo.installDate = installDate;
        }
        
        // 读取程序大小（注册表中以KB为单位，需要转换为字节）
        DWORD size = 0;
        DWORD sizeDataSize = sizeof(size);
        if (RegQueryValueExW(hSubKey, L"EstimatedSize", nullptr, nullptr, 
                           (LPBYTE)&size, &sizeDataSize) == ERROR_SUCCESS) {
            programInfo.estimatedSize = static_cast<DWORD64>(size) * 1024; // KB转换为字节
        }
        
        // 读取图标路径
        wchar_t iconPath[MAX_PATH] = {0};
        DWORD iconPathSize = sizeof(iconPath);
        if (RegQueryValueExW(hSubKey, L"DisplayIcon", nullptr, nullptr, 
                           (LPBYTE)iconPath, &iconPathSize) == ERROR_SUCCESS) {
            programInfo.iconPath = iconPath;
        }
        
        // 检查是否为系统组件（读取SystemComponent字段）
        DWORD systemComponent = 0;
        DWORD systemComponentSize = sizeof(systemComponent);
        if (RegQueryValueExW(hSubKey, L"SystemComponent", nullptr, nullptr, 
                           (LPBYTE)&systemComponent, &systemComponentSize) == ERROR_SUCCESS) {
            programInfo.isSystemComponent = (systemComponent == 1);
        } else {
            programInfo.isSystemComponent = false;
        }
        
        // 验证必要字段
        if (programInfo.name.empty() || programInfo.uninstallString.empty()) {
            RegCloseKey(hSubKey);
            return ErrorCode::DataNotFound; // 缺少必要信息，跳过
        }
        
        RegCloseKey(hSubKey);
        return ErrorCode::Success;
    }
    
    bool ProgramDetector::ParseUninstallString(const String& uninstallString, String& executablePath, String& parameters) {
        if (uninstallString.empty()) {
            return false;
        }
        
        // 简单的解析实现
        size_t pos = uninstallString.find(L".exe");
        if (pos != String::npos) {
            executablePath = uninstallString.substr(0, pos + 4);
            if (pos + 4 < uninstallString.length()) {
                parameters = uninstallString.substr(pos + 4);
            }
            return true;
        }
        
        executablePath = uninstallString;
        return true;
    }
    
    bool ProgramDetector::GetFileVersionInfo(const String& filePath, String& version, String& description) {
        DWORD handle = 0;
        DWORD size = ::GetFileVersionInfoSizeW(filePath.c_str(), &handle);
        if (size == 0) {
            return false;
        }
        
        std::vector<BYTE> versionData(size);
        if (!::GetFileVersionInfoW(filePath.c_str(), handle, size, versionData.data())) {
            return false;
        }
        
        // 获取版本信息
        VS_FIXEDFILEINFO* fileInfo;
        UINT fileInfoSize;
        if (VerQueryValueW(versionData.data(), L"\\", (LPVOID*)&fileInfo, &fileInfoSize)) {
            wchar_t versionStr[64];
#ifdef _MSC_VER
            swprintf_s(versionStr, L"%d.%d.%d.%d",
                      HIWORD(fileInfo->dwFileVersionMS),
                      LOWORD(fileInfo->dwFileVersionMS),
                      HIWORD(fileInfo->dwFileVersionLS),
                      LOWORD(fileInfo->dwFileVersionLS));
#else
            swprintf(versionStr, sizeof(versionStr)/sizeof(wchar_t), L"%d.%d.%d.%d",
                    HIWORD(fileInfo->dwFileVersionMS),
                    LOWORD(fileInfo->dwFileVersionMS),
                    HIWORD(fileInfo->dwFileVersionLS),
                    LOWORD(fileInfo->dwFileVersionLS));
#endif
            version = versionStr;
        }
        
        // 获取文件描述
        LPWSTR desc;
        UINT descSize;
        if (VerQueryValueW(versionData.data(), L"\\StringFileInfo\\040904b0\\FileDescription", 
                          (LPVOID*)&desc, &descSize)) {
            description = desc;
        }
        
        return true;
    }
    
    DWORD64 ProgramDetector::CalculateDirectorySize(const String& directoryPath) {
        DWORD64 totalSize = 0;
        WIN32_FIND_DATAW findData;
        
        String searchPath = directoryPath + L"\\*";
        HANDLE hFind = FindFirstFileW(searchPath.c_str(), &findData);
        
        if (hFind == INVALID_HANDLE_VALUE) {
            return 0;
        }
        
        do {
            if (wcscmp(findData.cFileName, L".") == 0 || wcscmp(findData.cFileName, L"..") == 0) {
                continue;
            }
            
            LARGE_INTEGER fileSize;
            fileSize.LowPart = findData.nFileSizeLow;
            fileSize.HighPart = findData.nFileSizeHigh;
            totalSize += fileSize.QuadPart;
            
            // 如果是目录，递归计算
            if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                String subDir = directoryPath + L"\\" + findData.cFileName;
                totalSize += CalculateDirectorySize(subDir);
            }
            
        } while (FindNextFileW(hFind, &findData));
        
        FindClose(hFind);
        return totalSize;
    }
    
    bool ProgramDetector::IsSystemComponent(const ProgramInfo& programInfo) {
        // 1. 如果注册表中明确标记为系统组件
        if (programInfo.isSystemComponent) {
            return true;
        }
        
        // 2. 检查是否为Windows更新或安全更新
        String name = programInfo.displayName.empty() ? programInfo.name : programInfo.displayName;
        std::transform(name.begin(), name.end(), name.begin(), ::towlower);
        
        if (name.find(L"security update") != String::npos ||
            name.find(L"hotfix") != String::npos ||
            name.find(L"update for") != String::npos ||
            name.find(L"kb") == 0 ||  // 以KB开头的更新
            name.find(L"microsoft .net") != String::npos ||
            name.find(L"microsoft visual c++") != String::npos) {
            return true;
        }
        
        // 3. 检查系统发布者
        String publisher = programInfo.publisher;
        std::transform(publisher.begin(), publisher.end(), publisher.begin(), ::towlower);
        
        const wchar_t* systemPublishers[] = {
            L"microsoft corporation",
            L"microsoft",
            L"windows",
            L"intel corporation", 
            L"intel",
            L"nvidia corporation",
            L"nvidia",
            L"amd",
            L"advanced micro devices"
        };
        
        for (const auto& sysPublisher : systemPublishers) {
            if (publisher.find(sysPublisher) != String::npos) {
                // Microsoft的程序需要进一步检查，不是所有Microsoft程序都是系统组件
                if (wcsstr(sysPublisher, L"microsoft") != nullptr) {
                    // 检查是否为用户程序
                    if (name.find(L"office") != String::npos ||
                        name.find(L"visual studio") != String::npos ||
                        name.find(L"teams") != String::npos ||
                        name.find(L"edge") != String::npos ||
                        name.find(L"onedrive") != String::npos) {
                        return false; // 这些是用户程序，不是系统组件
                    }
                }
                return true;
            }
        }
        
        // 4. 检查安装路径是否在系统目录
        String installPath = programInfo.installLocation;
        std::transform(installPath.begin(), installPath.end(), installPath.begin(), ::towlower);
        
        if (installPath.find(L"\\windows\\") != String::npos ||
            installPath.find(L"\\program files\\windows") != String::npos ||
            installPath.find(L"\\program files (x86)\\windows") != String::npos) {
            return true;
        }
        
        return false;
    }
    
    void ProgramDetector::ScanWorkerThread() {
        m_scanning = true;
        DWORD startTime = GetTickCount();
        
        try {
            m_programs.clear();
            
            UpdateProgress(10, L"开始扫描注册表...");
            
            // 扫描注册表
            ErrorCode result = ScanRegistryUninstall(m_programs, m_includeSystemComponents);
            
            if (result == ErrorCode::Success && !m_stopRequested) {
                UpdateProgress(80, L"扫描Windows Store应用...");
                
                if (m_includeSystemComponents) {
                    ScanWindowsStoreApps(m_programs);
                }
                
                UpdateProgress(100, L"扫描完成");
            }
            
            m_lastScanTime = GetTickCount() - startTime;
            m_totalFound = static_cast<int>(m_programs.size());
            
            // 调用完成回调
            if (m_completedCallback && !m_stopRequested) {
                m_completedCallback(m_programs, result);
            }
            
        } catch (...) {
            if (m_completedCallback) {
                m_completedCallback({}, ErrorCode::GeneralError);
            }
        }
        
        m_scanning = false;
    }
    
    void ProgramDetector::UpdateProgress(int percentage, const String& currentItem) {
        if (m_progressCallback) {
            m_progressCallback(percentage, currentItem);
        }
    }
    
} // namespace YG
