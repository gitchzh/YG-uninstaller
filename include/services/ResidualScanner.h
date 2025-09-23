/**
 * @file ResidualScanner.h
 * @brief 程序残留文件扫描服务
 * @author YG Software
 * @version 1.0.1
 * @date 2025-09-22
 */

#pragma once

#include "core/Common.h"
#include "core/ResidualItem.h"
#include <vector>
#include <memory>
#include <atomic>
#include <thread>
#include <mutex>

namespace YG {
    
    /**
     * @brief 残留文件扫描服务类
     */
    class ResidualScanner {
    private:
        std::atomic<bool> m_isScanning;         ///< 是否正在扫描
        std::atomic<bool> m_shouldStop;         ///< 是否应该停止扫描
        std::unique_ptr<std::thread> m_scanThread; ///< 扫描线程
        mutable std::mutex m_resultsMutex;      ///< 结果访问互斥锁
        
        std::vector<ResidualGroup> m_scanResults;   ///< 扫描结果
        ScanProgressCallback m_progressCallback;   ///< 进度回调
        
        // 扫描配置
        bool m_scanFiles;               ///< 是否扫描文件
        bool m_scanRegistry;            ///< 是否扫描注册表
        bool m_scanShortcuts;           ///< 是否扫描快捷方式
        bool m_scanServices;            ///< 是否扫描服务
        bool m_deepScan;                ///< 是否深度扫描
        
    public:
        /**
         * @brief 构造函数
         */
        ResidualScanner();
        
        /**
         * @brief 析构函数
         */
        ~ResidualScanner();
        
        /**
         * @brief 开始扫描程序残留
         * @param programInfo 已卸载的程序信息
         * @param progressCallback 进度回调函数
         * @return ErrorCode 操作结果
         */
        ErrorCode StartScan(const ProgramInfo& programInfo, ScanProgressCallback progressCallback);
        
        /**
         * @brief 停止扫描
         */
        void StopScan();
        
        /**
         * @brief 检查是否正在扫描
         * @return bool 是否正在扫描
         */
        bool IsScanning() const;
        
        /**
         * @brief 获取扫描结果
         * @return std::vector<ResidualGroup> 扫描结果分组
         */
        std::vector<ResidualGroup> GetScanResults() const;
        
        /**
         * @brief 设置扫描选项
         * @param scanFiles 是否扫描文件
         * @param scanRegistry 是否扫描注册表
         * @param scanShortcuts 是否扫描快捷方式
         * @param scanServices 是否扫描服务
         * @param deepScan 是否深度扫描
         */
        void SetScanOptions(bool scanFiles, bool scanRegistry, bool scanShortcuts, 
                           bool scanServices, bool deepScan);
        
        /**
         * @brief 删除指定的残留项
         * @param items 要删除的残留项列表
         * @param deleteCallback 删除进度回调
         * @return ErrorCode 操作结果
         */
        ErrorCode DeleteResidualItems(const std::vector<ResidualItem>& items, 
                                     DeleteProgressCallback deleteCallback);
        
    private:
        /**
         * @brief 扫描工作线程
         * @param programInfo 程序信息
         */
        void ScanWorkerThread(const ProgramInfo& programInfo);
        
        /**
         * @brief 扫描文件系统残留
         * @param programInfo 程序信息
         * @param results 扫描结果
         */
        void ScanFileSystemResiduals(const ProgramInfo& programInfo, std::vector<ResidualGroup>& results);
        
        /**
         * @brief 扫描注册表残留
         * @param programInfo 程序信息
         * @param results 扫描结果
         */
        void ScanRegistryResiduals(const ProgramInfo& programInfo, std::vector<ResidualGroup>& results);
        
        /**
         * @brief 扫描快捷方式残留
         * @param programInfo 程序信息
         * @param results 扫描结果
         */
        void ScanShortcutResiduals(const ProgramInfo& programInfo, std::vector<ResidualGroup>& results);
        
        /**
         * @brief 扫描服务残留
         * @param programInfo 程序信息
         * @param results 扫描结果
         */
        void ScanServiceResiduals(const ProgramInfo& programInfo, std::vector<ResidualGroup>& results);
        
        /**
         * @brief 扫描指定目录中的相关文件
         * @param directory 目录路径
         * @param programName 程序名称
         * @param searchPatterns 搜索模式
         * @param results 结果列表
         */
        void ScanDirectoryForResiduals(const String& directory, const String& programName,
                                     const std::vector<String>& searchPatterns,
                                     std::vector<ResidualItem>& results);
        
        /**
         * @brief 扫描注册表键
         * @param rootKey 根键
         * @param keyPath 键路径
         * @param programName 程序名称
         * @param results 结果列表
         */
        void ScanRegistryKey(HKEY rootKey, const String& keyPath, const String& programName,
                           std::vector<ResidualItem>& results);
        
        /**
         * @brief 生成搜索模式
         * @param programInfo 程序信息
         * @return std::vector<String> 搜索模式列表
         */
        std::vector<String> GenerateSearchPatterns(const ProgramInfo& programInfo);
        
        /**
         * @brief 评估残留项风险级别
         * @param item 残留项
         * @return RiskLevel 风险级别
         */
        RiskLevel EvaluateRiskLevel(const ResidualItem& item);
        
        /**
         * @brief 更新扫描进度
         * @param percentage 进度百分比
         * @param currentPath 当前扫描路径
         * @param foundCount 已找到数量
         */
        void UpdateProgress(int percentage, const String& currentPath, int foundCount);
    };
    
} // namespace YG
