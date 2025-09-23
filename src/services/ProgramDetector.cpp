/**
 * @file ProgramDetector.cpp
 * @brief 程序检测和扫描服务实现
 * @author gmrchzh@gmail.com
 * @version 1.0.1
 * @date 2025-09-17
 */

#include "services/ProgramDetector.h"
#include "utils/RegistryHelper.h"
#include <windows.h>
#include <shlobj.h>
#include <vector>
#include <algorithm>
#include <cstdio>  // 为swprintf_s提供支持
#include <future>
#include <chrono>
#include <condition_variable>

namespace YG {
    
    ProgramDetector::ProgramDetector() 
        : m_scanning(false), m_stopRequested(false), 
          m_includeSystemComponents(false), m_deepScanEnabled(false),
          m_scanTimeout(30000), m_totalFound(0), m_lastScanTime(0) {
        
        // 初始化程序缓存
        m_cache = YG::MakeUnique<ProgramCache>(300, 5); // 5分钟缓存，最多5个缓存项
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
        
        // 首先尝试从缓存获取
        if (m_cache && m_cache->HasValidCache(includeSystemComponents)) {
            YG_LOG_INFO(L"从缓存获取程序列表");
            auto cacheResult = m_cache->GetCachedPrograms(includeSystemComponents, programs);
            if (cacheResult.code == DetailedErrorCode::Success) {
                m_programs = programs;
                m_totalFound = static_cast<int>(programs.size());
                return ErrorCode::Success;
            }
        }
        
        // 缓存无效，执行实际扫描
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
        
        // 更新缓存
        if (m_cache) {
            m_cache->UpdateCache(includeSystemComponents, m_programs, m_lastScanTime);
        }
        
        programs = m_programs;
        return ErrorCode::Success;
    }
    
    void ProgramDetector::StopScan() {
        YG_LOG_INFO(L"开始停止程序扫描...");
        
        // 设置停止标志并通知条件变量
        {
            std::lock_guard<std::mutex> lock(m_stopMutex);
            m_stopRequested = true;
        }
        m_stopCondition.notify_all();
        
        // 等待线程结束
        if (m_scanThread && m_scanThread->joinable()) {
            try {
                YG_LOG_INFO(L"等待扫描线程结束...");
                
                // 使用条件变量等待，最多等待3秒
                std::unique_lock<std::mutex> lock(m_stopMutex);
                auto timeout = std::chrono::steady_clock::now() + std::chrono::seconds(3);
                
                // 等待扫描完成或超时
                if (m_stopCondition.wait_until(lock, timeout, [this] { return !m_scanning.load(); })) {
                    YG_LOG_INFO(L"扫描线程正常结束");
                    lock.unlock(); // 解锁后再join
                    m_scanThread->join();
                } else {
                    YG_LOG_WARNING(L"扫描线程停止超时，强制分离线程");
                    lock.unlock();
                    m_scanThread->detach();
                }
                
            } catch (const std::exception& e) {
                YG_LOG_ERROR(L"停止扫描线程时发生异常: " + StringToWString(e.what()));
                try {
                    if (m_scanThread->joinable()) {
                        m_scanThread->detach();
                    }
                } catch (...) {
                    YG_LOG_ERROR(L"分离扫描线程时发生异常");
                }
            } catch (...) {
                YG_LOG_ERROR(L"停止扫描线程时发生未知异常");
                try {
                    if (m_scanThread->joinable()) {
                        m_scanThread->detach();
                    }
                } catch (...) {
                    YG_LOG_ERROR(L"分离扫描线程时发生异常");
                }
            }
        }
        
        // 强制标记为非扫描状态
        m_scanning = false;
        
        // 释放线程对象
        try {
            m_scanThread.reset();
            YG_LOG_INFO(L"扫描线程对象已释放");
        } catch (...) {
            YG_LOG_ERROR(L"释放扫描线程对象时发生异常");
        }
        
        // 额外等待确保资源完全释放
        Sleep(100);
        
        YG_LOG_INFO(L"程序扫描停止完成");
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
        
        // 定义所有需要扫描的注册表路径
        RegistryPath uninstallKeys[] = {
            { HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall", L"64位程序" },
            { HKEY_LOCAL_MACHINE, L"SOFTWARE\\WOW6432Node\\Microsoft\\Windows\\CurrentVersion\\Uninstall", L"32位程序" },
            { HKEY_CURRENT_USER, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall", L"当前用户64位程序" },
            { HKEY_CURRENT_USER, L"SOFTWARE\\WOW6432Node\\Microsoft\\Windows\\CurrentVersion\\Uninstall", L"当前用户32位程序" }
        };
        
        for (int keyIndex = 0; keyIndex < 4; keyIndex++) {
            YG_LOG_INFO(L"尝试打开注册表键: " + String(uninstallKeys[keyIndex].description) + L" - " + String(uninstallKeys[keyIndex].path));
            
            HKEY hKey;
            LONG result = RegOpenKeyExW(uninstallKeys[keyIndex].rootKey, uninstallKeys[keyIndex].path, 0, KEY_READ, &hKey);
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
                ErrorCode result = GetProgramInfoFromRegistry(hKey, subKeyName, programInfo, uninstallKeys[keyIndex]);
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
    
    ErrorCode ProgramDetector::GetProgramInfoFromRegistry(HKEY hParentKey, const String& subKeyName, ProgramInfo& programInfo, const RegistryPath& registryPath) {
        HKEY hSubKey;
        LONG result = RegOpenKeyExW(hParentKey, subKeyName.c_str(), 0, KEY_READ, &hSubKey);
        if (result != ERROR_SUCCESS) {
            return ErrorCode::RegistryError;
        }
        
        // 构建完整的注册表键路径
        String rootKeyName;
        if (registryPath.rootKey == HKEY_LOCAL_MACHINE) {
            rootKeyName = L"HKEY_LOCAL_MACHINE";
        } else if (registryPath.rootKey == HKEY_CURRENT_USER) {
            rootKeyName = L"HKEY_CURRENT_USER";
        } else if (registryPath.rootKey == HKEY_CLASSES_ROOT) {
            rootKeyName = L"HKEY_CLASSES_ROOT";
        } else if (registryPath.rootKey == HKEY_USERS) {
            rootKeyName = L"HKEY_USERS";
        } else {
            rootKeyName = L"HKEY_LOCAL_MACHINE";
        }
        
        programInfo.registryKey = rootKeyName + L"\\" + String(registryPath.path) + L"\\" + subKeyName;
        
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
        
        // 读取版本 - 多种方法尝试
        wchar_t version[128] = {0};
        DWORD versionSize = sizeof(version);
        bool versionFound = false;
        
        // 方法1: DisplayVersion字段
        if (RegQueryValueExW(hSubKey, L"DisplayVersion", nullptr, nullptr, 
                           (LPBYTE)version, &versionSize) == ERROR_SUCCESS && wcslen(version) > 0) {
            programInfo.version = version;
            versionFound = true;
        }
        
        // 方法2: Version字段
        if (!versionFound) {
            versionSize = sizeof(version);
            if (RegQueryValueExW(hSubKey, L"Version", nullptr, nullptr, 
                               (LPBYTE)version, &versionSize) == ERROR_SUCCESS && wcslen(version) > 0) {
                programInfo.version = version;
                versionFound = true;
            }
        }
        
        // 方法3: VersionMajor + VersionMinor
        if (!versionFound) {
            DWORD majorVersion = 0, minorVersion = 0;
            DWORD dwSize = sizeof(DWORD);
            
            if (RegQueryValueExW(hSubKey, L"VersionMajor", nullptr, nullptr, 
                               (LPBYTE)&majorVersion, &dwSize) == ERROR_SUCCESS) {
                dwSize = sizeof(DWORD);
                RegQueryValueExW(hSubKey, L"VersionMinor", nullptr, nullptr, 
                               (LPBYTE)&minorVersion, &dwSize);
                
                if (majorVersion > 0) {
                    wchar_t versionStr[32];
                    swprintf(versionStr, sizeof(versionStr)/sizeof(wchar_t), L"%d.%d", majorVersion, minorVersion);
                    programInfo.version = versionStr;
                    versionFound = true;
                }
            }
        }
        
        // 方法4: 从可执行文件获取版本信息
        if (!versionFound && !programInfo.uninstallString.empty()) {
            String executablePath;
            String parameters;
            if (ParseUninstallString(programInfo.uninstallString, executablePath, parameters)) {
                String fileVersion, fileDescription;
                if (GetFileVersionInfo(executablePath, fileVersion, fileDescription)) {
                    if (!fileVersion.empty()) {
                        programInfo.version = fileVersion;
                        versionFound = true;
                    }
                }
            }
        }
        
        // 读取发布者 - 多种方法尝试
        wchar_t publisher[256] = {0};
        DWORD publisherSize = sizeof(publisher);
        bool publisherFound = false;
        
        // 方法1: Publisher字段
        if (RegQueryValueExW(hSubKey, L"Publisher", nullptr, nullptr, 
                           (LPBYTE)publisher, &publisherSize) == ERROR_SUCCESS && wcslen(publisher) > 0) {
            programInfo.publisher = publisher;
            publisherFound = true;
        }
        
        // 方法2: Manufacturer字段（某些程序使用此字段）
        if (!publisherFound) {
            publisherSize = sizeof(publisher);
            if (RegQueryValueExW(hSubKey, L"Manufacturer", nullptr, nullptr, 
                               (LPBYTE)publisher, &publisherSize) == ERROR_SUCCESS && wcslen(publisher) > 0) {
                programInfo.publisher = publisher;
                publisherFound = true;
            }
        }
        
        // 方法3: Contact字段
        if (!publisherFound) {
            publisherSize = sizeof(publisher);
            if (RegQueryValueExW(hSubKey, L"Contact", nullptr, nullptr, 
                               (LPBYTE)publisher, &publisherSize) == ERROR_SUCCESS && wcslen(publisher) > 0) {
                programInfo.publisher = publisher;
                publisherFound = true;
            }
        }
        
        // 方法4: 从安装路径中提取发布者信息
        if (!publisherFound && !programInfo.installLocation.empty()) {
            String extractedPublisher = ExtractPublisherFromPath(programInfo.installLocation);
            if (!extractedPublisher.empty()) {
                programInfo.publisher = extractedPublisher;
                publisherFound = true;
            }
        }
        
        // 方法5: 从卸载字符串中提取发布者信息
        if (!publisherFound && !programInfo.uninstallString.empty()) {
            String extractedPublisher = ExtractPublisherFromPath(programInfo.uninstallString);
            if (!extractedPublisher.empty()) {
                programInfo.publisher = extractedPublisher;
                publisherFound = true;
            }
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
        
        // 读取安装日期 - 尝试多种字段名和数据类型
        wchar_t installDate[64] = {0};
        DWORD installDateSize = sizeof(installDate);
        bool dateFound = false;
        
        // 方法1: 尝试InstallDate字段 (字符串类型)
        if (RegQueryValueExW(hSubKey, L"InstallDate", nullptr, nullptr, 
                           (LPBYTE)installDate, &installDateSize) == ERROR_SUCCESS && wcslen(installDate) > 0) {
            programInfo.installDate = installDate;
            dateFound = true;
        }
        
        // 方法2: 尝试InstallTime字段
        if (!dateFound) {
            installDateSize = sizeof(installDate);
            if (RegQueryValueExW(hSubKey, L"InstallTime", nullptr, nullptr, 
                               (LPBYTE)installDate, &installDateSize) == ERROR_SUCCESS && wcslen(installDate) > 0) {
                programInfo.installDate = installDate;
                dateFound = true;
            }
        }
        
        // 方法3: 尝试HelpLink字段中的日期信息
        if (!dateFound) {
            wchar_t helpLink[512] = {0};
            DWORD helpLinkSize = sizeof(helpLink);
            if (RegQueryValueExW(hSubKey, L"HelpLink", nullptr, nullptr, 
                               (LPBYTE)helpLink, &helpLinkSize) == ERROR_SUCCESS) {
                // 从HelpLink中提取日期信息（某些程序会在URL中包含版本和日期）
                String helpStr = helpLink;
                // 查找类似 2024, 2025 这样的年份
                for (int year = 2020; year <= 2030; year++) {
                    String yearStr = std::to_wstring(year);
                    if (helpStr.find(yearStr) != String::npos) {
                        // 假设为当年1月1日
                        swprintf(installDate, 64, L"%04d0101", year);
                        programInfo.installDate = installDate;
                        dateFound = true;
                        break;
                    }
                }
            }
        }
        
        // 方法4: 从卸载字符串路径获取文件创建时间
        if (!dateFound && !programInfo.uninstallString.empty()) {
            // 提取卸载程序的路径
            String uninstallPath = programInfo.uninstallString;
            size_t exePos = uninstallPath.find(L".exe");
            if (exePos != String::npos) {
                uninstallPath = uninstallPath.substr(0, exePos + 4);
                // 去掉引号
                if (uninstallPath.front() == L'"') uninstallPath.erase(0, 1);
                if (!uninstallPath.empty() && uninstallPath.back() == L'"') uninstallPath.pop_back();
                
                WIN32_FIND_DATAW findData;
                HANDLE hFind = FindFirstFileW(uninstallPath.c_str(), &findData);
                if (hFind != INVALID_HANDLE_VALUE) {
                    SYSTEMTIME st;
                    if (FileTimeToSystemTime(&findData.ftCreationTime, &st)) {
                        swprintf(installDate, 64, L"%04d%02d%02d", st.wYear, st.wMonth, st.wDay);
                        programInfo.installDate = installDate;
                        dateFound = true;
                    }
                    FindClose(hFind);
                }
            }
        }
        
        // 方法5: 从安装目录获取创建时间
        if (!dateFound && !programInfo.installLocation.empty()) {
            WIN32_FIND_DATAW findData;
            HANDLE hFind = FindFirstFileW(programInfo.installLocation.c_str(), &findData);
            if (hFind != INVALID_HANDLE_VALUE) {
                SYSTEMTIME st;
                if (FileTimeToSystemTime(&findData.ftCreationTime, &st)) {
                    swprintf(installDate, 64, L"%04d%02d%02d", st.wYear, st.wMonth, st.wDay);
                    programInfo.installDate = installDate;
                    dateFound = true;
                }
                FindClose(hFind);
            }
        }
        
        // 方法6: 从注册表键本身的修改时间获取
        if (!dateFound) {
            FILETIME lastWriteTime;
            if (RegQueryInfoKeyW(hSubKey, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, 
                               nullptr, nullptr, nullptr, nullptr, &lastWriteTime) == ERROR_SUCCESS) {
                SYSTEMTIME st;
                if (FileTimeToSystemTime(&lastWriteTime, &st)) {
                    swprintf(installDate, 64, L"%04d%02d%02d", st.wYear, st.wMonth, st.wDay);
                    programInfo.installDate = installDate;
                    dateFound = true;
                }
            }
        }
        
        // 方法7: 从DisplayIcon路径获取文件创建时间
        if (!dateFound && !programInfo.iconPath.empty()) {
            String dateFromIcon = GetDateFromIconPath(programInfo.iconPath);
            if (!dateFromIcon.empty()) {
                programInfo.installDate = dateFromIcon;
                dateFound = true;
            }
        }
        
        // 方法8: 基于程序版本和发布者的智能估算
        if (!dateFound) {
            String estimatedDate = EstimateInstallDateByName(programInfo);
            if (!estimatedDate.empty()) {
                programInfo.installDate = estimatedDate;
                dateFound = true;
            }
        }
        
        // 读取程序大小 - 尝试多种方法获取
        DWORD size = 0;
        DWORD sizeDataSize = sizeof(size);
        bool sizeFound = false;
        
        // 方法1: 从EstimatedSize字段读取
        if (RegQueryValueExW(hSubKey, L"EstimatedSize", nullptr, nullptr, 
                           (LPBYTE)&size, &sizeDataSize) == ERROR_SUCCESS && size > 0) {
            programInfo.estimatedSize = static_cast<DWORD64>(size) * 1024; // KB转换为字节
            sizeFound = true;
        }
        
        // 方法2: 如果EstimatedSize不存在或为0，尝试计算安装目录大小
        if (!sizeFound && !programInfo.installLocation.empty()) {
            DWORD64 directorySize = CalculateDirectorySize(programInfo.installLocation);
            if (directorySize > 0) {
                programInfo.estimatedSize = directorySize;
                sizeFound = true;
            }
        }
        
        // 方法3: 如果还是没有，尝试从卸载字符串中提取可执行文件大小
        if (!sizeFound && !programInfo.uninstallString.empty()) {
            DWORD64 executableSize = GetExecutableSize(programInfo.uninstallString);
            if (executableSize > 0) {
                programInfo.estimatedSize = executableSize;
                sizeFound = true;
            }
        }
        
        // 方法4: 如果还是没有，尝试从DisplayIcon路径估算
        if (!sizeFound && !programInfo.iconPath.empty()) {
            DWORD64 iconSize = EstimateFromIconPath(programInfo.iconPath);
            if (iconSize > 0) {
                programInfo.estimatedSize = iconSize;
                sizeFound = true;
            }
        }
        
        // 方法5: 最后的备用方案 - 基于程序名称和发布者的经验估算
        if (!sizeFound) {
            DWORD64 estimatedSize = EstimateProgramSizeByName(programInfo);
            if (estimatedSize > 0) {
                programInfo.estimatedSize = estimatedSize;
                sizeFound = true;
            }
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
        YG_LOG_INFO(L"扫描工作线程开始");
        m_scanning = true;
        DWORD startTime = GetTickCount();
        ErrorCode result = ErrorCode::Success;
        
        try {
            m_programs.clear();
            
            // 检查是否已被请求停止
            if (m_stopRequested.load()) {
                YG_LOG_INFO(L"扫描线程启动时发现停止请求，立即退出");
                result = ErrorCode::OperationCancelled;
                goto cleanup;
            }
            
            UpdateProgress(10, L"开始扫描注册表...");
            
            // 扫描注册表
            result = ScanRegistryUninstall(m_programs, m_includeSystemComponents);
            
            if (result == ErrorCode::Success && !m_stopRequested.load()) {
                UpdateProgress(80, L"扫描Windows Store应用...");
                
                if (m_includeSystemComponents) {
                    ScanWindowsStoreApps(m_programs);
                }
                
                if (!m_stopRequested.load()) {
                    UpdateProgress(100, L"扫描完成");
                } else {
                    YG_LOG_INFO(L"扫描被用户取消");
                    result = ErrorCode::OperationCancelled;
                }
            }
            
            m_lastScanTime = GetTickCount() - startTime;
            m_totalFound = static_cast<int>(m_programs.size());
            
        } catch (const std::exception& e) {
            YG_LOG_ERROR(L"扫描工作线程发生异常: " + StringToWString(e.what()));
            result = ErrorCode::UnknownError;
        } catch (...) {
            YG_LOG_ERROR(L"扫描工作线程发生未知异常");
            result = ErrorCode::UnknownError;
        }
        
    cleanup:
        // 清理和回调
        try {
            // 调用完成回调
            if (m_completedCallback && !m_stopRequested.load()) {
                m_completedCallback(m_programs, result);
            }
        } catch (...) {
            YG_LOG_ERROR(L"回调函数执行时发生异常");
        }
        
        // 确保状态正确设置并通知等待的线程
        m_scanning = false;
        
        // 通知StopScan函数扫描已完成
        {
            std::lock_guard<std::mutex> lock(m_stopMutex);
        }
        m_stopCondition.notify_all();
        
        YG_LOG_INFO(L"扫描工作线程结束");
    }
    
    void ProgramDetector::UpdateProgress(int percentage, const String& currentItem) {
        if (m_progressCallback) {
            m_progressCallback(percentage, currentItem);
        }
    }
    
    DWORD64 ProgramDetector::CalculateDirectorySize(const String& directoryPath) {
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
        const int maxFiles = 500; // 限制扫描的文件数量，避免性能问题
        const int maxDirectories = 10; // 限制扫描的目录数量
        int directoryCount = 0;
        
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
                    totalSize = averageFileSize * (fileCount * 5); // 假设还有5倍的文件
                    break;
                }
            } else if (wcscmp(findData.cFileName, L".") != 0 && wcscmp(findData.cFileName, L"..") != 0) {
                // 是子目录，限制递归深度避免性能问题
                if (directoryCount < maxDirectories) {
                    String subDir = directoryPath + L"\\" + findData.cFileName;
                    totalSize += CalculateDirectorySize(subDir);
                    directoryCount++;
                }
            }
            
            // 如果总大小已经很大，停止计算
            if (totalSize > 3LL * 1024 * 1024 * 1024) { // 3GB限制
                break;
            }
        } while (FindNextFileW(hFind, &findData));
        
        FindClose(hFind);
        
        // 限制最大估算大小为3GB，避免不合理的估算
        if (totalSize > 3LL * 1024 * 1024 * 1024) {
            totalSize = 3LL * 1024 * 1024 * 1024;
        }
        
        return totalSize;
    }
    
    DWORD64 ProgramDetector::GetExecutableSize(const String& uninstallString) {
        if (uninstallString.empty()) {
            return 0;
        }
        
        // 从卸载字符串中提取可执行文件路径
        String executablePath;
        String parameters;
        if (!ParseUninstallString(uninstallString, executablePath, parameters)) {
            return 0;
        }
        
        // 获取可执行文件大小
        WIN32_FIND_DATAW findData;
        HANDLE hFind = FindFirstFileW(executablePath.c_str(), &findData);
        if (hFind != INVALID_HANDLE_VALUE) {
            LARGE_INTEGER fileSize;
            fileSize.LowPart = findData.nFileSizeLow;
            fileSize.HighPart = findData.nFileSizeHigh;
            FindClose(hFind);
            
            // 对于可执行文件，估算整个程序大小为文件大小的10-50倍
            return fileSize.QuadPart * 20; // 估算倍数
        }
        
        return 0;
    }
    
    /**
     * @brief 从图标路径估算程序大小
     * @param iconPath 图标路径
     * @return DWORD64 估算的程序大小（字节）
     */
    DWORD64 ProgramDetector::EstimateFromIconPath(const String& iconPath) {
        if (iconPath.empty()) {
            return 0;
        }
        
        // 从图标路径中提取可执行文件路径（去掉图标索引）
        String executablePath = iconPath;
        size_t commaPos = executablePath.find(L',');
        if (commaPos != String::npos) {
            executablePath = executablePath.substr(0, commaPos);
        }
        
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
            
            // 对于可执行文件，估算整个程序大小为文件大小的20-40倍
            return fileSize.QuadPart * 30; // 估算倍数
        }
        
        return 0;
    }
    
    /**
     * @brief 基于程序名称和发布者估算程序大小
     * @param programInfo 程序信息
     * @return DWORD64 估算的程序大小（字节）
     */
    DWORD64 ProgramDetector::EstimateProgramSizeByName(const ProgramInfo& programInfo) {
        String name = !programInfo.displayName.empty() ? programInfo.displayName : programInfo.name;
        String publisher = programInfo.publisher;
        
        // 转换为小写进行比较
        std::transform(name.begin(), name.end(), name.begin(), ::towlower);
        std::transform(publisher.begin(), publisher.end(), publisher.begin(), ::towlower);
        
        // 基于程序名称的常见模式进行估算
        if (name.find(L"microsoft") != String::npos) {
            if (name.find(L"office") != String::npos) {
                return 2LL * 1024 * 1024 * 1024; // 2GB - Office套件
            } else if (name.find(L"visual studio") != String::npos) {
                return 5LL * 1024 * 1024 * 1024; // 5GB - Visual Studio
            } else if (name.find(L"sql server") != String::npos) {
                return 3LL * 1024 * 1024 * 1024; // 3GB - SQL Server
            } else if (name.find(L".net") != String::npos) {
                return 500LL * 1024 * 1024; // 500MB - .NET Framework
            } else {
                return 1LL * 1024 * 1024 * 1024; // 1GB - 其他Microsoft程序
            }
        } else if (name.find(L"google") != String::npos) {
            if (name.find(L"chrome") != String::npos) {
                return 500LL * 1024 * 1024; // 500MB - Chrome浏览器
            } else {
                return 200LL * 1024 * 1024; // 200MB - 其他Google程序
            }
        } else if (name.find(L"adobe") != String::npos) {
            if (name.find(L"photoshop") != String::npos) {
                return 3LL * 1024 * 1024 * 1024; // 3GB - Photoshop
            } else if (name.find(L"acrobat") != String::npos) {
                return 1LL * 1024 * 1024 * 1024; // 1GB - Acrobat
            } else {
                return 2LL * 1024 * 1024 * 1024; // 2GB - 其他Adobe程序
            }
        } else if (name.find(L"游戏") != String::npos || name.find(L"game") != String::npos) {
            return 5LL * 1024 * 1024 * 1024; // 5GB - 游戏
        } else if (name.find(L"开发") != String::npos || name.find(L"development") != String::npos) {
            return 2LL * 1024 * 1024 * 1024; // 2GB - 开发工具
        } else if (name.find(L"安全") != String::npos || name.find(L"security") != String::npos || 
                   name.find(L"杀毒") != String::npos || name.find(L"antivirus") != String::npos) {
            return 1LL * 1024 * 1024 * 1024; // 1GB - 安全软件
        } else {
            // 默认估算：基于发布者
            if (publisher.find(L"microsoft") != String::npos) {
                return 500LL * 1024 * 1024; // 500MB - Microsoft程序
            } else if (publisher.find(L"adobe") != String::npos) {
                return 1LL * 1024 * 1024 * 1024; // 1GB - Adobe程序
            } else if (publisher.find(L"google") != String::npos) {
                return 300LL * 1024 * 1024; // 300MB - Google程序
            } else {
                return 200LL * 1024 * 1024; // 200MB - 其他程序
            }
        }
    }
    
    /**
     * @brief 从图标路径获取文件创建日期
     * @param iconPath 图标路径
     * @return String 日期字符串 (YYYYMMDD格式)
     */
    String ProgramDetector::GetDateFromIconPath(const String& iconPath) {
        if (iconPath.empty()) {
            return L"";
        }
        
        // 从图标路径中提取可执行文件路径（去掉图标索引）
        String executablePath = iconPath;
        size_t commaPos = executablePath.find(L',');
        if (commaPos != String::npos) {
            executablePath = executablePath.substr(0, commaPos);
        }
        
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
        
        return L"";
    }
    
    /**
     * @brief 基于程序名称和发布者估算安装日期
     * @param programInfo 程序信息
     * @return String 估算的日期字符串 (YYYYMMDD格式)
     */
    String ProgramDetector::EstimateInstallDateByName(const ProgramInfo& programInfo) {
        String name = !programInfo.displayName.empty() ? programInfo.displayName : programInfo.name;
        String version = programInfo.version;
        
        // 转换为小写进行比较
        std::transform(name.begin(), name.end(), name.begin(), ::towlower);
        
        // 基于程序名称的常见模式进行估算
        if (name.find(L"microsoft") != String::npos) {
            if (name.find(L"office") != String::npos) {
                // Office通常是最近几年安装的
                return L"20240101"; // 2024年1月1日
            } else if (name.find(L"visual studio") != String::npos) {
                // Visual Studio通常是开发工具，安装时间相对较新
                return L"20240101"; // 2024年1月1日
            } else if (name.find(L".net") != String::npos) {
                // .NET Framework通常是系统组件，安装时间较早
                return L"20220101"; // 2022年1月1日
            } else {
                return L"20230101"; // 2023年1月1日
            }
        } else if (name.find(L"google") != String::npos) {
            if (name.find(L"chrome") != String::npos) {
                // Chrome浏览器更新频繁，通常是最近安装的
                return L"20240101"; // 2024年1月1日
            } else {
                return L"20230101"; // 2023年1月1日
            }
        } else if (name.find(L"adobe") != String::npos) {
            // Adobe程序通常是专业软件，安装时间相对稳定
            return L"20230101"; // 2023年1月1日
        } else if (name.find(L"游戏") != String::npos || name.find(L"game") != String::npos) {
            // 游戏程序通常是最近安装的
            return L"20240101"; // 2024年1月1日
        } else if (name.find(L"开发") != String::npos || name.find(L"development") != String::npos) {
            // 开发工具通常是最近安装的
            return L"20240101"; // 2024年1月1日
        } else if (name.find(L"安全") != String::npos || name.find(L"security") != String::npos || 
                   name.find(L"杀毒") != String::npos || name.find(L"antivirus") != String::npos) {
            // 安全软件通常是系统基础软件，安装时间较早
            return L"20220101"; // 2022年1月1日
        } else {
            // 默认估算：基于版本信息
            if (!version.empty()) {
                // 尝试从版本号中提取年份信息
                if (version.find(L"2024") != String::npos) {
                    return L"20240101";
                } else if (version.find(L"2023") != String::npos) {
                    return L"20230101";
                } else if (version.find(L"2022") != String::npos) {
                    return L"20220101";
                } else if (version.find(L"2021") != String::npos) {
                    return L"20210101";
                }
            }
            // 默认估算为2023年
            return L"20230101";
        }
    }
    
    String ProgramDetector::ExtractPublisherFromPath(const String& path) {
        if (path.empty()) {
            return L"";
        }
        
        // 常见的发布者路径模式
        std::vector<std::pair<String, String>> publisherPatterns = {
            {L"\\Microsoft\\", L"Microsoft Corporation"},
            {L"\\Google\\", L"Google LLC"},
            {L"\\Adobe\\", L"Adobe Inc."},
            {L"\\Mozilla\\", L"Mozilla Foundation"},
            {L"\\Oracle\\", L"Oracle Corporation"},
            {L"\\Apple\\", L"Apple Inc."},
            {L"\\Autodesk\\", L"Autodesk, Inc."},
            {L"\\Tencent\\", L"Tencent Technology"},
            {L"\\Alibaba\\", L"Alibaba Group"},
            {L"\\Baidu\\", L"Baidu, Inc."},
            {L"\\360\\", L"Qihoo 360"},
            {L"\\JetBrains\\", L"JetBrains s.r.o."},
            {L"\\Steam\\", L"Valve Corporation"},
            {L"\\NVIDIA\\", L"NVIDIA Corporation"},
            {L"\\Intel\\", L"Intel Corporation"},
            {L"\\AMD\\", L"Advanced Micro Devices"},
        };
        
        String upperPath = path;
        std::transform(upperPath.begin(), upperPath.end(), upperPath.begin(), ::towupper);
        
        for (const auto& pattern : publisherPatterns) {
            String upperPattern = pattern.first;
            std::transform(upperPattern.begin(), upperPattern.end(), upperPattern.begin(), ::towupper);
            
            if (upperPath.find(upperPattern) != String::npos) {
                return pattern.second;
            }
        }
        
        // 尝试从路径中提取可能的公司名称
        // 查找 Program Files\CompanyName\ 模式
        size_t programFilesPos = upperPath.find(L"PROGRAM FILES");
        if (programFilesPos != String::npos) {
            size_t startPos = programFilesPos + 13; // "PROGRAM FILES"长度
            if (startPos < path.length() && path[startPos] == L'\\') {
                startPos++;
                size_t endPos = path.find(L'\\', startPos);
                if (endPos != String::npos && endPos > startPos) {
                    String companyName = path.substr(startPos, endPos - startPos);
                    if (companyName.length() > 2 && companyName.length() < 50) {
                        return companyName;
                    }
                }
            }
        }
        
        return L"";
    }
    
} // namespace YG
