/**
 * @file ProgramCache.h
 * @brief 程序扫描缓存管理类
 * @author YG Software
 * @version 1.0.1
 * @date 2025-01-21
 */

#pragma once

#include "core/Common.h"
#include "core/DetailedErrorCodes.h"
#include <unordered_map>
#include <vector>
#include <mutex>
#include <chrono>

namespace YG {
    
    /**
     * @brief 缓存项结构
     */
    struct CacheItem {
        std::vector<ProgramInfo> programs;      ///< 程序列表
        std::chrono::system_clock::time_point lastUpdate; ///< 最后更新时间
        bool includeSystemComponents;           ///< 是否包含系统组件
        size_t programCount;                    ///< 程序数量
        DWORD scanDuration;                     ///< 扫描耗时(毫秒)
        
        CacheItem() : includeSystemComponents(false), programCount(0), scanDuration(0) {
            lastUpdate = std::chrono::system_clock::now();
        }
    };
    
    /**
     * @brief 程序扫描缓存管理器
     * 
     * 提供智能缓存机制，避免重复扫描，提升性能
     */
    class ProgramCache {
    public:
        /**
         * @brief 构造函数
         * @param maxCacheAge 最大缓存时间(秒)，默认5分钟
         * @param maxCacheSize 最大缓存项数量，默认10
         */
        ProgramCache(int maxCacheAge = 300, size_t maxCacheSize = 10);
        
        /**
         * @brief 析构函数
         */
        ~ProgramCache();
        
        YG_DISABLE_COPY_AND_ASSIGN(ProgramCache);
        
        /**
         * @brief 检查缓存是否有效
         * @param includeSystemComponents 是否包含系统组件
         * @return bool 是否有有效缓存
         */
        bool HasValidCache(bool includeSystemComponents) const;
        
        /**
         * @brief 获取缓存的程序列表
         * @param includeSystemComponents 是否包含系统组件
         * @param programs 输出程序列表
         * @return ErrorContext 操作结果
         */
        ErrorContext GetCachedPrograms(bool includeSystemComponents, 
                                     std::vector<ProgramInfo>& programs) const;
        
        /**
         * @brief 更新缓存
         * @param includeSystemComponents 是否包含系统组件
         * @param programs 程序列表
         * @param scanDuration 扫描耗时
         * @return ErrorContext 操作结果
         */
        ErrorContext UpdateCache(bool includeSystemComponents, 
                               const std::vector<ProgramInfo>& programs,
                               DWORD scanDuration);
        
        /**
         * @brief 清除所有缓存
         */
        void ClearCache();
        
        /**
         * @brief 清除过期缓存
         */
        void ClearExpiredCache();
        
        /**
         * @brief 获取缓存统计信息
         * @return String 缓存统计信息
         */
        String GetCacheStats() const;
        
        /**
         * @brief 设置缓存最大时间
         * @param seconds 秒数
         */
        void SetMaxCacheAge(int seconds);
        
        /**
         * @brief 设置最大缓存大小
         * @param size 缓存项数量
         */
        void SetMaxCacheSize(size_t size);
        
        /**
         * @brief 检查是否需要刷新缓存
         * @param includeSystemComponents 是否包含系统组件
         * @return bool 是否需要刷新
         */
        bool ShouldRefreshCache(bool includeSystemComponents) const;
        
        /**
         * @brief 预热缓存（异步扫描常用配置）
         */
        void WarmupCache();
        
    private:
        /**
         * @brief 生成缓存键
         * @param includeSystemComponents 是否包含系统组件
         * @return String 缓存键
         */
        String GenerateCacheKey(bool includeSystemComponents) const;
        
        /**
         * @brief 清理最旧的缓存项
         */
        void CleanupOldestCache();
        
        /**
         * @brief 检查缓存项是否过期
         * @param item 缓存项
         * @return bool 是否过期
         */
        bool IsCacheExpired(const CacheItem& item) const;
        
    private:
        mutable std::mutex m_mutex;                           ///< 线程安全锁
        std::unordered_map<String, CacheItem> m_cache;        ///< 缓存映射表
        int m_maxCacheAge;                                    ///< 最大缓存时间(秒)
        size_t m_maxCacheSize;                                ///< 最大缓存大小
        
        // 统计信息
        mutable size_t m_cacheHits;                           ///< 缓存命中次数
        mutable size_t m_cacheMisses;                         ///< 缓存未命中次数
        mutable size_t m_cacheUpdates;                        ///< 缓存更新次数
    };
    
} // namespace YG
