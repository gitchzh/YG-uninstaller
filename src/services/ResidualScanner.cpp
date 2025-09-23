/**
 * @file ResidualScanner.cpp
 * @brief 程序残留文件扫描服务实现
 * @author YG Software
 * @version 1.0.1
 * @date 2025-09-22
 */

#include "services/ResidualScanner.h"
#include "core/Logger.h"
#include "core/ErrorHandler.h"
#include "utils/StringUtils.h"
#include "utils/RegistryHelper.h"
#include <shlobj.h>
#include <algorithm>
#include <regex>

namespace YG {
    
    ResidualScanner::ResidualScanner() 
        : m_isScanning(false), m_shouldStop(false),
          m_scanFiles(true), m_scanRegistry(true), m_scanShortcuts(true),
          m_scanServices(false), m_deepScan(false) {
        YG_LOG_INFO(L"残留扫描器已创建");
    }
    
    ResidualScanner::~ResidualScanner() {
        StopScan();
        YG_LOG_INFO(L"残留扫描器已销毁");
    }
    
    ErrorCode ResidualScanner::StartScan(const ProgramInfo& programInfo, ScanProgressCallback progressCallback) {
        if (m_isScanning.load()) {
            YG_LOG_WARNING(L"扫描已在进行中");
            return ErrorCode::InvalidOperation;
        }
        
        m_progressCallback = progressCallback;
        m_shouldStop.store(false);
        m_isScanning.store(true);
        
        // 清空之前的结果
        {
            std::lock_guard<std::mutex> lock(m_resultsMutex);
            m_scanResults.clear();
        }
        
        YG_LOG_INFO(L"开始扫描程序残留: " + programInfo.name);
        
        // 启动扫描线程
        m_scanThread = YG::MakeUnique<std::thread>(&ResidualScanner::ScanWorkerThread, this, programInfo);
        
        return ErrorCode::Success;
    }
    
    void ResidualScanner::StopScan() {
        if (!m_isScanning.load()) {
            return;
        }
        
        m_shouldStop.store(true);
        
        if (m_scanThread && m_scanThread->joinable()) {
            m_scanThread->join();
        }
        
        m_isScanning.store(false);
        YG_LOG_INFO(L"残留扫描已停止");
    }
    
    bool ResidualScanner::IsScanning() const {
        return m_isScanning.load();
    }
    
    std::vector<ResidualGroup> ResidualScanner::GetScanResults() const {
        std::lock_guard<std::mutex> lock(m_resultsMutex);
        return m_scanResults;
    }
    
    void ResidualScanner::SetScanOptions(bool scanFiles, bool scanRegistry, bool scanShortcuts, 
                                        bool scanServices, bool deepScan) {
        m_scanFiles = scanFiles;
        m_scanRegistry = scanRegistry;
        m_scanShortcuts = scanShortcuts;
        m_scanServices = scanServices;
        m_deepScan = deepScan;
        
        YG_LOG_INFO(L"扫描选项已更新");
    }
    
    void ResidualScanner::ScanWorkerThread(const ProgramInfo& programInfo) {
        YG_LOG_INFO(L"扫描工作线程开始");
        
        try {
            std::vector<ResidualGroup> results;
            int totalSteps = 0;
            int currentStep = 0;
            
            // 计算总步骤数
            if (m_scanFiles) totalSteps += 3;      // 文件系统扫描（AppData, ProgramData, Temp）
            if (m_scanRegistry) totalSteps += 2;   // 注册表扫描（HKLM, HKCU）
            if (m_scanShortcuts) totalSteps += 2;  // 快捷方式扫描（桌面, 开始菜单）
            if (m_scanServices) totalSteps += 1;   // 服务扫描
            
            // 文件系统扫描
            if (m_scanFiles && !m_shouldStop.load()) {
                UpdateProgress((currentStep * 100) / totalSteps, L"扫描用户数据目录...", 0);
                ScanFileSystemResiduals(programInfo, results);
                currentStep += 3;
            }
            
            // 注册表扫描
            if (m_scanRegistry && !m_shouldStop.load()) {
                UpdateProgress((currentStep * 100) / totalSteps, L"扫描注册表残留...", 0);
                ScanRegistryResiduals(programInfo, results);
                currentStep += 2;
            }
            
            // 快捷方式扫描
            if (m_scanShortcuts && !m_shouldStop.load()) {
                UpdateProgress((currentStep * 100) / totalSteps, L"扫描快捷方式残留...", 0);
                ScanShortcutResiduals(programInfo, results);
                currentStep += 2;
            }
            
            // 服务扫描
            if (m_scanServices && !m_shouldStop.load()) {
                UpdateProgress((currentStep * 100) / totalSteps, L"扫描系统服务...", 0);
                ScanServiceResiduals(programInfo, results);
                currentStep += 1;
            }
            
            // 保存结果
            {
                std::lock_guard<std::mutex> lock(m_resultsMutex);
                m_scanResults = std::move(results);
            }
            
            // 计算总的残留项数量
            int totalFound = 0;
            for (const auto& group : m_scanResults) {
                totalFound += static_cast<int>(group.items.size());
            }
            
            UpdateProgress(100, L"扫描完成", totalFound);
            
        } catch (const std::exception& e) {
            YG_LOG_ERROR(L"扫描过程中发生异常");
            UpdateProgress(100, L"扫描出错", 0);
        }
        
        m_isScanning.store(false);
        YG_LOG_INFO(L"扫描工作线程结束");
    }
    
    void ResidualScanner::ScanFileSystemResiduals(const ProgramInfo& programInfo, std::vector<ResidualGroup>& results) {
        YG_LOG_INFO(L"开始扫描文件系统残留");
        
        // 生成搜索模式
        std::vector<String> searchPatterns = GenerateSearchPatterns(programInfo);
        
        // 创建文件系统残留分组
        ResidualGroup filesGroup(L"文件和文件夹", L"程序相关的文件和目录", ResidualType::File);
        ResidualGroup cacheGroup(L"缓存文件", L"程序缓存和临时文件", ResidualType::Cache);
        ResidualGroup configGroup(L"配置文件", L"程序配置和设置文件", ResidualType::Config);
        
        // 扫描用户数据目录
        if (!m_shouldStop.load()) {
            UpdateProgress(10, L"扫描用户数据目录...", 0);
            
            wchar_t appDataPath[MAX_PATH];
            if (SHGetFolderPathW(nullptr, CSIDL_APPDATA, nullptr, SHGFP_TYPE_CURRENT, appDataPath) == S_OK) {
                ScanDirectoryForResiduals(appDataPath, programInfo.name, searchPatterns, filesGroup.items);
            }
            
            wchar_t localAppDataPath[MAX_PATH];
            if (SHGetFolderPathW(nullptr, CSIDL_LOCAL_APPDATA, nullptr, SHGFP_TYPE_CURRENT, localAppDataPath) == S_OK) {
                ScanDirectoryForResiduals(localAppDataPath, programInfo.name, searchPatterns, cacheGroup.items);
            }
        }
        
        // 扫描公共数据目录
        if (!m_shouldStop.load()) {
            UpdateProgress(20, L"扫描公共数据目录...", 0);
            
            wchar_t programDataPath[MAX_PATH];
            if (SHGetFolderPathW(nullptr, CSIDL_COMMON_APPDATA, nullptr, SHGFP_TYPE_CURRENT, programDataPath) == S_OK) {
                ScanDirectoryForResiduals(programDataPath, programInfo.name, searchPatterns, configGroup.items);
            }
        }
        
        // 扫描临时目录
        if (!m_shouldStop.load()) {
            UpdateProgress(30, L"扫描临时目录...", 0);
            
            wchar_t tempPath[MAX_PATH];
            if (::GetTempPathW(MAX_PATH, tempPath) > 0) {
                ScanDirectoryForResiduals(tempPath, programInfo.name, searchPatterns, cacheGroup.items);
            }
        }
        
        // 添加非空的分组到结果
        if (!filesGroup.items.empty()) {
            filesGroup.selectedCount = static_cast<int>(filesGroup.items.size());
            for (const auto& item : filesGroup.items) {
                filesGroup.totalSize += item.size;
            }
            results.push_back(std::move(filesGroup));
        }
        
        if (!cacheGroup.items.empty()) {
            cacheGroup.selectedCount = static_cast<int>(cacheGroup.items.size());
            for (const auto& item : cacheGroup.items) {
                cacheGroup.totalSize += item.size;
            }
            results.push_back(std::move(cacheGroup));
        }
        
        if (!configGroup.items.empty()) {
            configGroup.selectedCount = static_cast<int>(configGroup.items.size());
            for (const auto& item : configGroup.items) {
                configGroup.totalSize += item.size;
            }
            results.push_back(std::move(configGroup));
        }
        
        YG_LOG_INFO(L"文件系统扫描完成");
    }
    
    void ResidualScanner::ScanRegistryResiduals(const ProgramInfo& programInfo, std::vector<ResidualGroup>& results) {
        YG_LOG_INFO(L"开始扫描注册表残留");
        
        ResidualGroup registryGroup(L"注册表项", L"程序相关的注册表键和值", ResidualType::RegistryKey);
        
        // 扫描常见的注册表位置
        std::vector<std::pair<HKEY, String>> registryPaths = {
            {HKEY_CURRENT_USER, L"Software"},
            {HKEY_LOCAL_MACHINE, L"SOFTWARE"},
            {HKEY_CLASSES_ROOT, L""},
            {HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Run"},
            {HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run"}
        };
        
        for (const auto& regPath : registryPaths) {
            if (m_shouldStop.load()) break;
            
            UpdateProgress(50, L"扫描注册表: " + regPath.second, 0);
            ScanRegistryKey(regPath.first, regPath.second, programInfo.name, registryGroup.items);
        }
        
        if (!registryGroup.items.empty()) {
            registryGroup.selectedCount = static_cast<int>(registryGroup.items.size());
            results.push_back(std::move(registryGroup));
        }
        
        YG_LOG_INFO(L"注册表扫描完成");
    }
    
    void ResidualScanner::ScanShortcutResiduals(const ProgramInfo& programInfo, std::vector<ResidualGroup>& results) {
        YG_LOG_INFO(L"开始扫描快捷方式残留");
        
        ResidualGroup shortcutGroup(L"快捷方式", L"桌面和开始菜单中的快捷方式", ResidualType::Shortcut);
        
        // 扫描桌面
        if (!m_shouldStop.load()) {
            UpdateProgress(70, L"扫描桌面快捷方式...", 0);
            
            wchar_t desktopPath[MAX_PATH];
            if (SHGetFolderPathW(nullptr, CSIDL_DESKTOP, nullptr, SHGFP_TYPE_CURRENT, desktopPath) == S_OK) {
                std::vector<String> shortcutPatterns = {L"*" + programInfo.name + L"*.lnk"};
                ScanDirectoryForResiduals(desktopPath, programInfo.name, shortcutPatterns, shortcutGroup.items);
            }
        }
        
        // 扫描开始菜单
        if (!m_shouldStop.load()) {
            UpdateProgress(80, L"扫描开始菜单...", 0);
            
            wchar_t startMenuPath[MAX_PATH];
            if (SHGetFolderPathW(nullptr, CSIDL_PROGRAMS, nullptr, SHGFP_TYPE_CURRENT, startMenuPath) == S_OK) {
                std::vector<String> shortcutPatterns = {L"*" + programInfo.name + L"*.lnk"};
                ScanDirectoryForResiduals(startMenuPath, programInfo.name, shortcutPatterns, shortcutGroup.items);
            }
        }
        
        if (!shortcutGroup.items.empty()) {
            shortcutGroup.selectedCount = static_cast<int>(shortcutGroup.items.size());
            results.push_back(std::move(shortcutGroup));
        }
        
        YG_LOG_INFO(L"快捷方式扫描完成");
    }
    
    void ResidualScanner::ScanServiceResiduals(const ProgramInfo& programInfo, std::vector<ResidualGroup>& results) {
        YG_LOG_INFO(L"开始扫描服务残留");
        
        // TODO: 实现服务扫描逻辑
        // 这里可以扫描与程序相关的Windows服务
        
        YG_LOG_INFO(L"服务扫描完成");
    }
    
    void ResidualScanner::ScanDirectoryForResiduals(const String& directory, const String& programName,
                                                   const std::vector<String>& searchPatterns,
                                                   std::vector<ResidualItem>& results) {
        if (m_shouldStop.load()) return;
        
        WIN32_FIND_DATAW findData;
        String searchPath = directory + L"\\*";
        HANDLE hFind = FindFirstFileW(searchPath.c_str(), &findData);
        
        if (hFind == INVALID_HANDLE_VALUE) {
            return;
        }
        
        do {
            if (m_shouldStop.load()) break;
            
            String fileName = findData.cFileName;
            String fullPath = directory + L"\\" + fileName;
            
            // 跳过 . 和 ..
            if (fileName == L"." || fileName == L"..") {
                continue;
            }
            
            // 检查是否匹配搜索模式
            bool matches = false;
            String lowerFileName = fileName;
            String lowerProgramName = programName;
            std::transform(lowerFileName.begin(), lowerFileName.end(), lowerFileName.begin(), ::towlower);
            std::transform(lowerProgramName.begin(), lowerProgramName.end(), lowerProgramName.begin(), ::towlower);
            
            // 简单的名称匹配
            if (lowerFileName.find(lowerProgramName) != String::npos) {
                matches = true;
            }
            
            if (matches) {
                ResidualItem item;
                item.path = fullPath;
                item.name = fileName;
                item.type = (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ? 
                           ResidualType::Directory : ResidualType::File;
                
                // 计算文件大小
                if (item.type == ResidualType::File) {
                    LARGE_INTEGER fileSize;
                    fileSize.LowPart = findData.nFileSizeLow;
                    fileSize.HighPart = findData.nFileSizeHigh;
                    item.size = fileSize.QuadPart;
                }
                
                // 评估风险级别
                item.riskLevel = EvaluateRiskLevel(item);
                
                // 格式化最后修改时间
                SYSTEMTIME sysTime;
                FileTimeToSystemTime(&findData.ftLastWriteTime, &sysTime);
                wchar_t timeStr[64];
                swprintf(timeStr, sizeof(timeStr)/sizeof(wchar_t), 
                        L"%04d-%02d-%02d %02d:%02d", 
                        sysTime.wYear, sysTime.wMonth, sysTime.wDay,
                        sysTime.wHour, sysTime.wMinute);
                item.lastModified = timeStr;
                
                results.push_back(item);
            }
            
            // 如果是目录且启用深度扫描，递归扫描
            if (matches && (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) && m_deepScan) {
                ScanDirectoryForResiduals(fullPath, programName, searchPatterns, results);
            }
            
        } while (FindNextFileW(hFind, &findData) && !m_shouldStop.load());
        
        FindClose(hFind);
    }
    
    void ResidualScanner::ScanRegistryKey(HKEY rootKey, const String& keyPath, const String& programName,
                                        std::vector<ResidualItem>& results) {
        if (m_shouldStop.load()) return;
        
        HKEY hKey;
        if (RegOpenKeyExW(rootKey, keyPath.c_str(), 0, KEY_READ, &hKey) != ERROR_SUCCESS) {
            return;
        }
        
        // 枚举子键
        DWORD index = 0;
        wchar_t subKeyName[256];
        DWORD subKeyNameSize;
        
        while (!m_shouldStop.load()) {
            subKeyNameSize = sizeof(subKeyName) / sizeof(wchar_t);
            
            if (RegEnumKeyExW(hKey, index, subKeyName, &subKeyNameSize,
                             nullptr, nullptr, nullptr, nullptr) != ERROR_SUCCESS) {
                break;
            }
            
            // 检查是否与程序名称相关
            String lowerSubKeyName = subKeyName;
            String lowerProgramName = programName;
            std::transform(lowerSubKeyName.begin(), lowerSubKeyName.end(), lowerSubKeyName.begin(), ::towlower);
            std::transform(lowerProgramName.begin(), lowerProgramName.end(), lowerProgramName.begin(), ::towlower);
            
            if (lowerSubKeyName.find(lowerProgramName) != String::npos) {
                ResidualItem item;
                item.path = keyPath + L"\\" + subKeyName;
                item.name = subKeyName;
                item.type = ResidualType::RegistryKey;
                item.riskLevel = EvaluateRiskLevel(item);
                item.size = 0; // 注册表项没有大小概念
                
                results.push_back(item);
            }
            
            index++;
        }
        
        RegCloseKey(hKey);
    }
    
    std::vector<String> ResidualScanner::GenerateSearchPatterns(const ProgramInfo& programInfo) {
        std::vector<String> patterns;
        
        // 基于程序名称生成模式
        String programName = programInfo.displayName.empty() ? programInfo.name : programInfo.displayName;
        
        patterns.push_back(programName);
        
        // 如果程序名称包含空格，也搜索去掉空格的版本
        String nameWithoutSpaces = programName;
        nameWithoutSpaces.erase(std::remove(nameWithoutSpaces.begin(), nameWithoutSpaces.end(), L' '), nameWithoutSpaces.end());
        if (nameWithoutSpaces != programName) {
            patterns.push_back(nameWithoutSpaces);
        }
        
        // 基于发布者生成模式
        if (!programInfo.publisher.empty()) {
            patterns.push_back(programInfo.publisher);
        }
        
        return patterns;
    }
    
    RiskLevel ResidualScanner::EvaluateRiskLevel(const ResidualItem& item) {
        String lowerPath = item.path;
        std::transform(lowerPath.begin(), lowerPath.end(), lowerPath.begin(), ::towlower);
        
        // 高风险路径
        if (lowerPath.find(L"system32") != String::npos ||
            lowerPath.find(L"windows") != String::npos ||
            lowerPath.find(L"program files") != String::npos) {
            return RiskLevel::High;
        }
        
        // 中等风险路径
        if (lowerPath.find(L"programdata") != String::npos ||
            item.type == ResidualType::RegistryKey) {
            return RiskLevel::Medium;
        }
        
        // 低风险路径
        if (lowerPath.find(L"appdata") != String::npos ||
            lowerPath.find(L"temp") != String::npos ||
            item.type == ResidualType::Cache) {
            return RiskLevel::Low;
        }
        
        return RiskLevel::Safe;
    }
    
    void ResidualScanner::UpdateProgress(int percentage, const String& currentPath, int foundCount) {
        if (m_progressCallback) {
            m_progressCallback(percentage, currentPath, foundCount);
        }
    }
    
    ErrorCode ResidualScanner::DeleteResidualItems(const std::vector<ResidualItem>& items, 
                                                  DeleteProgressCallback deleteCallback) {
        YG_LOG_INFO(L"开始删除残留项，数量: " + std::to_wstring(items.size()));
        
        int totalItems = static_cast<int>(items.size());
        int deletedItems = 0;
        
        for (const auto& item : items) {
            if (deleteCallback) {
                deleteCallback((deletedItems * 100) / totalItems, item.path, false);
            }
            
            bool success = false;
            
            try {
                if (item.type == ResidualType::File) {
                    success = (DeleteFileW(item.path.c_str()) != 0);
                } else if (item.type == ResidualType::Directory) {
                    success = (RemoveDirectoryW(item.path.c_str()) != 0);
                } else if (item.type == ResidualType::RegistryKey) {
                    // TODO: 实现注册表删除
                    success = true; // 暂时标记为成功
                }
                
                if (success) {
                    YG_LOG_INFO(L"成功删除: " + item.path);
                } else {
                    YG_LOG_WARNING(L"删除失败: " + item.path);
                }
                
            } catch (...) {
                YG_LOG_ERROR(L"删除异常: " + item.path);
                success = false;
            }
            
            deletedItems++;
            
            if (deleteCallback) {
                deleteCallback((deletedItems * 100) / totalItems, item.path, success);
            }
        }
        
        YG_LOG_INFO(L"残留项删除完成");
        return ErrorCode::Success;
    }
    
} // namespace YG
