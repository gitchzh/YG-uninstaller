/**
 * @file Config.h
 * @brief 配置管理类
 * @author YG Software
 * @version 1.0.1
 * @date 2025-09-17
 */

#pragma once

#include "Common.h"
#include <unordered_map>
#include <mutex>

namespace YG {
    
    /**
     * @brief 配置管理类
     * 
     * 负责应用程序配置的读取、保存和管理
     * 使用单例模式确保全局唯一性
     */
    class Config {
    public:
        // 配置键常量
        static constexpr const wchar_t* KEY_WINDOW_WIDTH = L"WindowWidth";
        static constexpr const wchar_t* KEY_WINDOW_HEIGHT = L"WindowHeight";
        static constexpr const wchar_t* KEY_WINDOW_X = L"WindowX";
        static constexpr const wchar_t* KEY_WINDOW_Y = L"WindowY";
        static constexpr const wchar_t* KEY_WINDOW_MAXIMIZED = L"WindowMaximized";
        static constexpr const wchar_t* KEY_AUTO_REFRESH = L"AutoRefresh";
        static constexpr const wchar_t* KEY_SHOW_SYSTEM_COMPONENTS = L"ShowSystemComponents";
        static constexpr const wchar_t* KEY_CONFIRM_UNINSTALL = L"ConfirmUninstall";
        static constexpr const wchar_t* KEY_DEEP_SCAN = L"DeepScan";
        static constexpr const wchar_t* KEY_LOG_LEVEL = L"LogLevel";
        static constexpr const wchar_t* KEY_LANGUAGE = L"Language";
        
        /**
         * @brief 获取配置管理器实例
         * @return Config& 配置管理器引用
         */
        static Config& GetInstance();
        
        /**
         * @brief 加载配置文件
         * @return ErrorCode 操作结果
         */
        ErrorCode Load();
        
        /**
         * @brief 保存配置文件
         * @return ErrorCode 操作结果
         */
        ErrorCode Save();
        
        /**
         * @brief 获取字符串配置值
         * @param key 配置键
         * @param defaultValue 默认值
         * @return String 配置值
         */
        String GetString(const String& key, const String& defaultValue = L"") const;
        
        /**
         * @brief C++17优化的字符串配置值获取
         * @param key 配置键
         * @return std::optional<String> 配置值，如果不存在则返回nullopt
         */
        std::optional<String> GetStringOptional(const String& key) const;
        
        /**
         * @brief 获取整数配置值
         * @param key 配置键
         * @param defaultValue 默认值
         * @return int 配置值
         */
        int GetInt(const String& key, int defaultValue = 0) const;
        
        /**
         * @brief 获取布尔配置值
         * @param key 配置键
         * @param defaultValue 默认值
         * @return bool 配置值
         */
        bool GetBool(const String& key, bool defaultValue = false) const;
        
        /**
         * @brief 获取双精度浮点配置值
         * @param key 配置键
         * @param defaultValue 默认值
         * @return double 配置值
         */
        double GetDouble(const String& key, double defaultValue = 0.0) const;
        
        /**
         * @brief 设置字符串配置值
         * @param key 配置键
         * @param value 配置值
         */
        void SetString(const String& key, const String& value);
        
        /**
         * @brief 设置整数配置值
         * @param key 配置键
         * @param value 配置值
         */
        void SetInt(const String& key, int value);
        
        /**
         * @brief 设置布尔配置值
         * @param key 配置键
         * @param value 配置值
         */
        void SetBool(const String& key, bool value);
        
        /**
         * @brief 设置双精度浮点配置值
         * @param key 配置键
         * @param value 配置值
         */
        void SetDouble(const String& key, double value);
        
        /**
         * @brief 检查配置键是否存在
         * @param key 配置键
         * @return bool 是否存在
         */
        bool HasKey(const String& key) const;
        
        /**
         * @brief 删除配置键
         * @param key 配置键
         * @return bool 是否成功删除
         */
        bool RemoveKey(const String& key);
        
        /**
         * @brief 清空所有配置
         */
        void Clear();
        
        /**
         * @brief 重置为默认配置
         */
        void ResetToDefaults();
        
        /**
         * @brief 获取配置文件路径
         * @return String 配置文件路径
         */
        String GetConfigFilePath() const;
        
    private:
        // 私有构造函数，实现单例模式
        Config();
        ~Config();
        
        YG_DISABLE_COPY_AND_ASSIGN(Config);
        
        /**
         * @brief 加载默认配置
         */
        void LoadDefaults();
        
        /**
         * @brief 解析配置文件内容
         * @param content 文件内容
         * @return ErrorCode 操作结果
         */
        ErrorCode ParseContent(const String& content);
        
        /**
         * @brief 生成配置文件内容
         * @return String 配置文件内容
         */
        String GenerateContent() const;
        
        /**
         * @brief 修剪字符串首尾空格
         * @param str 要修剪的字符串
         * @return String 修剪后的字符串
         */
        String Trim(const String& str) const;
        
        /**
         * @brief 内部获取字符串配置值（不需要锁）
         * @param key 配置键
         * @param defaultValue 默认值
         * @return String 配置值
         */
        String GetStringUnsafe(const String& key, const String& defaultValue = L"") const;
        
    private:
        std::unordered_map<String, String> m_values;  ///< 配置值映射表
        String m_configFilePath;                      ///< 配置文件路径
        mutable std::mutex m_mutex;                   ///< 线程安全锁
        bool m_modified;                              ///< 是否已修改
    };
    
} // namespace YG
