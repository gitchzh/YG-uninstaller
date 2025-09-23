/**
 * @file ProgramDetector.h
 * @brief 程序检测和扫描服务
 * @author YG Software
 * @version 1.0.1
 * @date 2025-09-17
 */

#pragma once

#include "core/Common.h"
#include "core/Logger.h"
#include "services/ProgramCache.h"
#include <vector>
#include <memory>
#include <functional>
#include <thread>
#include <atomic>
#include <condition_variable>
#include <mutex>

namespace YG {
    
    // 注册表路径结构体
    struct RegistryPath {
        HKEY rootKey;
        const wchar_t* path;
        const wchar_t* description;
    };
    
    /**
     * @brief 程序检测器类
     * 
     * 负责扫描系统中已安装的程序，支持多种检测方式
     */
    class ProgramDetector {
    public:
        // 扫描进度回调函数类型
        using ScanProgressCallback = std::function<void(int percentage, const String& currentItem)>;
        using ScanCompletedCallback = std::function<void(const std::vector<ProgramInfo>& programs, ErrorCode result)>;
        
        /**
         * @brief 构造函数
         */
        ProgramDetector();
        
        /**
         * @brief 析构函数
         */
        ~ProgramDetector();
        
        YG_DISABLE_COPY_AND_ASSIGN(ProgramDetector);
        
        /**
         * @brief 开始扫描已安装的程序
         * @param includeSystemComponents 是否包含系统组件
         * @param progressCallback 进度回调函数
         * @param completedCallback 完成回调函数
         * @return ErrorCode 操作结果
         */
        ErrorCode StartScan(bool includeSystemComponents = false,
                          const ScanProgressCallback& progressCallback = nullptr,
                          const ScanCompletedCallback& completedCallback = nullptr);
        
        /**
         * @brief 同步扫描已安装的程序
         * @param includeSystemComponents 是否包含系统组件
         * @param programs 输出程序列表
         * @return ErrorCode 操作结果
         */
        ErrorCode ScanSync(bool includeSystemComponents, std::vector<ProgramInfo>& programs);
        
        /**
         * @brief 停止当前扫描
         */
        void StopScan();
        
        /**
         * @brief 检查是否正在扫描
         * @return bool 是否正在扫描
         */
        bool IsScanning() const;
        
        /**
         * @brief 刷新程序列表
         * @param includeSystemComponents 是否包含系统组件
         * @return ErrorCode 操作结果
         */
        ErrorCode RefreshProgramList(bool includeSystemComponents = false);
        
        /**
         * @brief 获取程序详细信息
         * @param programName 程序名称或显示名称
         * @param programInfo 输出程序信息
         * @return ErrorCode 操作结果
         */
        ErrorCode GetProgramInfo(const String& programName, ProgramInfo& programInfo);
        
        /**
         * @brief 搜索程序
         * @param keyword 搜索关键词
         * @param results 搜索结果
         * @return ErrorCode 操作结果
         */
        ErrorCode SearchPrograms(const String& keyword, std::vector<ProgramInfo>& results);
        
        /**
         * @brief 验证程序是否仍然安装
         * @param programInfo 程序信息
         * @return bool 是否仍然安装
         */
        bool ValidateProgram(const ProgramInfo& programInfo);
        
        /**
         * @brief 获取程序图标
         * @param programInfo 程序信息
         * @param iconPath 输出图标路径
         * @return ErrorCode 操作结果
         */
        ErrorCode GetProgramIcon(const ProgramInfo& programInfo, String& iconPath);
        
        /**
         * @brief 计算目录大小
         * @param directoryPath 目录路径
         * @return DWORD64 目录大小（字节）
         */
        DWORD64 CalculateDirectorySize(const String& directoryPath);
        
        /**
         * @brief 获取可执行文件大小
         * @param uninstallString 卸载字符串
         * @return DWORD64 文件大小（字节）
         */
        DWORD64 GetExecutableSize(const String& uninstallString);
        
        /**
         * @brief 估算程序大小
         * @param programInfo 程序信息
         * @return DWORD64 程序大小(字节)
         */
        DWORD64 EstimateProgramSize(const ProgramInfo& programInfo);
        
        /**
         * @brief 从图标路径估算程序大小
         * @param iconPath 图标路径
         * @return DWORD64 估算的程序大小（字节）
         */
        DWORD64 EstimateFromIconPath(const String& iconPath);
        
        /**
         * @brief 从路径中提取发布者信息
         * @param path 程序路径或卸载字符串
         * @return String 提取的发布者名称
         */
        String ExtractPublisherFromPath(const String& path);
        
        /**
         * @brief 基于程序名称和发布者估算程序大小
         * @param programInfo 程序信息
         * @return DWORD64 估算的程序大小（字节）
         */
        DWORD64 EstimateProgramSizeByName(const ProgramInfo& programInfo);
        
        /**
         * @brief 从图标路径获取文件创建日期
         * @param iconPath 图标路径
         * @return String 日期字符串 (YYYYMMDD格式)
         */
        String GetDateFromIconPath(const String& iconPath);
        
        /**
         * @brief 基于程序名称和发布者估算安装日期
         * @param programInfo 程序信息
         * @return String 估算的日期字符串 (YYYYMMDD格式)
         */
        String EstimateInstallDateByName(const ProgramInfo& programInfo);
        
        /**
         * @brief 设置扫描超时时间
         * @param timeoutMs 超时时间(毫秒)
         */
        void SetScanTimeout(DWORD timeoutMs);
        
        /**
         * @brief 启用/禁用深度扫描
         * @param enable 是否启用
         */
        void EnableDeepScan(bool enable);
        
        /**
         * @brief 获取扫描统计信息
         * @param totalFound 找到的程序总数
         * @param scanTimeMs 扫描耗时(毫秒)
         * @param lastScanTime 最后扫描时间
         */
        void GetScanStatistics(int& totalFound, DWORD& scanTimeMs, String& lastScanTime);
        
    private:
        /**
         * @brief 扫描注册表卸载信息
         * @param programs 程序列表
         * @param includeSystemComponents 是否包含系统组件
         * @return ErrorCode 操作结果
         */
        ErrorCode ScanRegistryUninstall(std::vector<ProgramInfo>& programs, bool includeSystemComponents);
        
        /**
         * @brief 扫描Windows应用商店应用
         * @param programs 程序列表
         * @return ErrorCode 操作结果
         */
        ErrorCode ScanWindowsStoreApps(std::vector<ProgramInfo>& programs);
        
        /**
         * @brief 扫描便携式程序
         * @param programs 程序列表
         * @return ErrorCode 操作结果
         */
        ErrorCode ScanPortablePrograms(std::vector<ProgramInfo>& programs);
        
        /**
         * @brief 从注册表键获取程序信息
         * @param hKey 注册表键句柄
         * @param subKeyName 子键名称
         * @param programInfo 输出程序信息
         * @return ErrorCode 操作结果
         */
        ErrorCode GetProgramInfoFromRegistry(HKEY hKey, const String& subKeyName, ProgramInfo& programInfo, const RegistryPath& registryPath);
        
        /**
         * @brief 解析卸载字符串
         * @param uninstallString 卸载字符串
         * @param executablePath 输出可执行文件路径
         * @param parameters 输出参数
         * @return bool 是否解析成功
         */
        bool ParseUninstallString(const String& uninstallString, String& executablePath, String& parameters);
        
        /**
         * @brief 获取文件版本信息
         * @param filePath 文件路径
         * @param version 输出版本号
         * @param description 输出描述
         * @return bool 是否成功
         */
        bool GetFileVersionInfo(const String& filePath, String& version, String& description);
        
        /**
         * @brief 检查是否为系统组件
         * @param programInfo 程序信息
         * @return bool 是否为系统组件
         */
        bool IsSystemComponent(const ProgramInfo& programInfo);
        
        /**
         * @brief 扫描工作线程函数
         */
        void ScanWorkerThread();
        
        /**
         * @brief 更新扫描进度
         * @param percentage 进度百分比
         * @param currentItem 当前扫描项
         */
        void UpdateProgress(int percentage, const String& currentItem);
        
    private:
        std::vector<ProgramInfo> m_programs;        ///< 程序列表
        std::unique_ptr<std::thread> m_scanThread;  ///< 扫描线程
        std::atomic<bool> m_scanning;               ///< 是否正在扫描
        std::atomic<bool> m_stopRequested;          ///< 是否请求停止
        
        ScanProgressCallback m_progressCallback;   ///< 进度回调
        ScanCompletedCallback m_completedCallback; ///< 完成回调
        
        bool m_includeSystemComponents;             ///< 是否包含系统组件
        bool m_deepScanEnabled;                     ///< 是否启用深度扫描
        DWORD m_scanTimeout;                        ///< 扫描超时时间
        
        // 统计信息
        int m_totalFound;                           ///< 找到的程序总数
        DWORD m_lastScanTime;                       ///< 最后扫描耗时
        String m_lastScanTimeString;                ///< 最后扫描时间字符串
        
        // 线程同步
        mutable std::mutex m_mutex;                 ///< 线程安全锁
        std::condition_variable m_stopCondition;    ///< 停止条件变量
        mutable std::mutex m_stopMutex;             ///< 停止操作锁
        
        // 缓存管理
        std::unique_ptr<ProgramCache> m_cache;      ///< 程序缓存
        
        // 系统组件过滤列表
        static const StringVector s_systemComponentNames;
        static const StringVector s_systemPublishers;
    };
    
} // namespace YG
