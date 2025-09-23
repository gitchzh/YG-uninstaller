/**
 * @file ProgramCache.cpp
 * @brief 程序扫描缓存管理实现
 * @author YG Software
 * @version 1.0.1
 * @date 2025-01-21
 */

#include "services/ProgramCache.h"
#include "core/Logger.h"
#include <algorithm>
#include <sstream>
#include <iomanip>

namespace YG {
    
    ProgramCache::ProgramCache(int maxCacheAge, size_t maxCacheSize)
        : m_maxCacheAge(maxCacheAge), m_maxCacheSize(maxCacheSize),
          m_cacheHits(0), m_cacheMisses(0), m_cacheUpdates(0) {
        YG_LOG_INFO(L"程序缓存管理器初始化，最大缓存时间: " + std::to_wstring(maxCacheAge) + 
                   L"秒，最大缓存大小: " + std::to_wstring(maxCacheSize));
    }
    
    ProgramCache::~ProgramCache() {
        std::lock_guard<std::mutex> lock(m_mutex);
        YG_LOG_INFO(L"程序缓存管理器销毁，统计信息: 命中" + std::to_wstring(m_cacheHits) + 
                   L"次，未命中" + std::to_wstring(m_cacheMisses) + L"次");
    }
    
    bool ProgramCache::HasValidCache(bool includeSystemComponents) const {
        std::lock_guard<std::mutex> lock(m_mutex);
        
        String key = GenerateCacheKey(includeSystemComponents);
        auto it = m_cache.find(key);
        
        if (it == m_cache.end()) {
            return false;
        }
        
        return !IsCacheExpired(it->second);
    }
    
    ErrorContext ProgramCache::GetCachedPrograms(bool includeSystemComponents, 
                                               std::vector<ProgramInfo>& programs) const {
        std::lock_guard<std::mutex> lock(m_mutex);
        
        String key = GenerateCacheKey(includeSystemComponents);
        auto it = m_cache.find(key);
        
        if (it == m_cache.end()) {
            m_cacheMisses++;
            return YG_DETAILED_ERROR(DetailedErrorCode::DataNotFound, L"缓存中未找到对应数据");
        }
        
        if (IsCacheExpired(it->second)) {
            m_cacheMisses++;
            return YG_DETAILED_ERROR(DetailedErrorCode::DataNotFound, L"缓存已过期");
        }
        
        programs = it->second.programs;
        m_cacheHits++;
        
        YG_LOG_INFO(L"从缓存获取程序列表成功，程序数量: " + std::to_wstring(programs.size()) + 
                   L"，缓存时间: " + std::to_wstring(
                       std::chrono::duration_cast<std::chrono::seconds>(
                           std::chrono::system_clock::now() - it->second.lastUpdate).count()) + L"秒前");
        
        return ErrorContext(DetailedErrorCode::Success);
    }
    
    ErrorContext ProgramCache::UpdateCache(bool includeSystemComponents, 
                                         const std::vector<ProgramInfo>& programs,
                                         DWORD scanDuration) {
        std::lock_guard<std::mutex> lock(m_mutex);
        
        // 检查缓存大小限制
        if (m_cache.size() >= m_maxCacheSize) {
            CleanupOldestCache();
        }
        
        String key = GenerateCacheKey(includeSystemComponents);
        
        CacheItem item;
        item.programs = programs;
        item.lastUpdate = std::chrono::system_clock::now();
        item.includeSystemComponents = includeSystemComponents;
        item.programCount = programs.size();
        item.scanDuration = scanDuration;
        
        m_cache[key] = std::move(item);
        m_cacheUpdates++;
        
        YG_LOG_INFO(L"缓存已更新，键: " + key + L"，程序数量: " + std::to_wstring(programs.size()) + 
                   L"，扫描耗时: " + std::to_wstring(scanDuration) + L"毫秒");
        
        return ErrorContext(DetailedErrorCode::Success);
    }
    
    void ProgramCache::ClearCache() {
        std::lock_guard<std::mutex> lock(m_mutex);
        
        size_t oldSize = m_cache.size();
        m_cache.clear();
        
        YG_LOG_INFO(L"已清除所有缓存，清除项数: " + std::to_wstring(oldSize));
    }
    
    void ProgramCache::ClearExpiredCache() {
        std::lock_guard<std::mutex> lock(m_mutex);
        
        size_t removedCount = 0;
        auto it = m_cache.begin();
        while (it != m_cache.end()) {
            if (IsCacheExpired(it->second)) {
                it = m_cache.erase(it);
                removedCount++;
            } else {
                ++it;
            }
        }
        
        if (removedCount > 0) {
            YG_LOG_INFO(L"已清除过期缓存，清除项数: " + std::to_wstring(removedCount));
        }
    }
    
    String ProgramCache::GetCacheStats() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        
        std::wstringstream stats;
        stats << L"缓存统计信息:\n";
        stats << L"  缓存项数: " << m_cache.size() << L"/" << m_maxCacheSize << L"\n";
        stats << L"  最大缓存时间: " << m_maxCacheAge << L"秒\n";
        stats << L"  缓存命中: " << m_cacheHits << L"次\n";
        stats << L"  缓存未命中: " << m_cacheMisses << L"次\n";
        stats << L"  缓存更新: " << m_cacheUpdates << L"次\n";
        
        if (m_cacheHits + m_cacheMisses > 0) {
            double hitRate = (double)m_cacheHits / (m_cacheHits + m_cacheMisses) * 100.0;
            stats << L"  命中率: " << (int)hitRate << L"%\n";  // 简化格式化
        }
        
        stats << L"  缓存详情:\n";
        for (const auto& pair : m_cache) {
            auto age = std::chrono::duration_cast<std::chrono::seconds>(
                std::chrono::system_clock::now() - pair.second.lastUpdate).count();
            stats << L"    " << pair.first << L": " << pair.second.programCount 
                  << L"个程序, " << age << L"秒前, " 
                  << pair.second.scanDuration << L"毫秒\n";
        }
        
        return stats.str();
    }
    
    void ProgramCache::SetMaxCacheAge(int seconds) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_maxCacheAge = seconds;
        YG_LOG_INFO(L"缓存最大时间已设置为: " + std::to_wstring(seconds) + L"秒");
    }
    
    void ProgramCache::SetMaxCacheSize(size_t size) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_maxCacheSize = size;
        
        // 如果当前缓存超过新限制，清理多余项
        while (m_cache.size() > m_maxCacheSize) {
            CleanupOldestCache();
        }
        
        YG_LOG_INFO(L"缓存最大大小已设置为: " + std::to_wstring(size));
    }
    
    bool ProgramCache::ShouldRefreshCache(bool includeSystemComponents) const {
        std::lock_guard<std::mutex> lock(m_mutex);
        
        String key = GenerateCacheKey(includeSystemComponents);
        auto it = m_cache.find(key);
        
        if (it == m_cache.end()) {
            return true; // 无缓存，需要刷新
        }
        
        // 检查是否接近过期（剩余时间少于总时间的20%）
        auto age = std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now() - it->second.lastUpdate).count();
        
        return age > (m_maxCacheAge * 0.8);
    }
    
    void ProgramCache::WarmupCache() {
        YG_LOG_INFO(L"开始预热缓存（功能占位，实际实现需要ProgramDetector配合）");
        // 这里可以在后台预先扫描常用配置
        // 实际实现需要与ProgramDetector协调
    }
    
    String ProgramCache::GenerateCacheKey(bool includeSystemComponents) const {
        return includeSystemComponents ? L"with_system" : L"without_system";
    }
    
    void ProgramCache::CleanupOldestCache() {
        if (m_cache.empty()) {
            return;
        }
        
        // 找到最旧的缓存项
        auto oldestIt = m_cache.begin();
        for (auto it = m_cache.begin(); it != m_cache.end(); ++it) {
            if (it->second.lastUpdate < oldestIt->second.lastUpdate) {
                oldestIt = it;
            }
        }
        
        YG_LOG_INFO(L"清理最旧的缓存项: " + oldestIt->first);
        m_cache.erase(oldestIt);
    }
    
    bool ProgramCache::IsCacheExpired(const CacheItem& item) const {
        auto age = std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now() - item.lastUpdate).count();
        return age > m_maxCacheAge;
    }
    
} // namespace YG
