/**
 * @file ResidualItem.h
 * @brief 程序残留项数据结构定义
 * @author YG Software
 * @version 1.0.1
 * @date 2025-09-22
 */

#pragma once

#include "Common.h"
#include <vector>

namespace YG {
    
    /**
     * @brief 残留项类型枚举
     */
    enum class ResidualType {
        File,           ///< 文件
        Directory,      ///< 目录
        RegistryKey,    ///< 注册表键
        RegistryValue,  ///< 注册表值
        Shortcut,       ///< 快捷方式
        Service,        ///< 系统服务
        StartupItem,    ///< 启动项
        Cache,          ///< 缓存文件
        Log,            ///< 日志文件
        Temp,           ///< 临时文件
        Config          ///< 配置文件
    };
    
    /**
     * @brief 残留项风险级别
     */
    enum class RiskLevel {
        Safe,           ///< 安全删除
        Low,            ///< 低风险
        Medium,         ///< 中等风险
        High,           ///< 高风险
        Critical        ///< 危险（不建议删除）
    };
    
    /**
     * @brief 程序残留项信息结构
     */
    struct ResidualItem {
        String path;                ///< 文件/注册表路径
        String name;                ///< 显示名称
        String description;         ///< 描述信息
        ResidualType type;          ///< 残留类型
        RiskLevel riskLevel;        ///< 风险级别
        DWORD64 size;              ///< 文件大小（字节，注册表项为0）
        String lastModified;        ///< 最后修改时间
        bool isSelected;            ///< 是否被选中删除
        String category;            ///< 分类名称（用于分组显示）
        
        /**
         * @brief 默认构造函数
         */
        ResidualItem() 
            : type(ResidualType::File), riskLevel(RiskLevel::Safe), 
              size(0), isSelected(true) {}
        
        /**
         * @brief 构造函数
         */
        ResidualItem(const String& itemPath, const String& itemName, 
                    ResidualType itemType, RiskLevel risk = RiskLevel::Safe)
            : path(itemPath), name(itemName), type(itemType), 
              riskLevel(risk), size(0), isSelected(true) {}
    };
    
    /**
     * @brief 残留项分组结构
     */
    struct ResidualGroup {
        String groupName;                   ///< 分组名称
        String groupDescription;            ///< 分组描述
        ResidualType groupType;            ///< 分组类型
        std::vector<ResidualItem> items;   ///< 分组中的残留项
        bool isExpanded;                   ///< 是否展开显示
        int selectedCount;                 ///< 选中项数量
        DWORD64 totalSize;                ///< 总大小
        
        /**
         * @brief 构造函数
         */
        ResidualGroup(const String& name, const String& desc, ResidualType type)
            : groupName(name), groupDescription(desc), groupType(type),
              isExpanded(true), selectedCount(0), totalSize(0) {}
    };
    
    /**
     * @brief 扫描进度回调函数类型
     */
    using ScanProgressCallback = std::function<void(int percentage, const String& currentPath, int foundCount)>;
    
    /**
     * @brief 删除进度回调函数类型  
     */
    using DeleteProgressCallback = std::function<void(int percentage, const String& currentItem, bool success)>;
    
} // namespace YG
