/**
 * @file Config.cpp
 * @brief 配置管理类实现
 * @author gmrchzh@gmail.com
 * @version 1.0.1
 * @date 2025-09-17
 */

#include "core/Config.h"
#include "core/Logger.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <shlobj.h>

namespace YG {
    
    Config& Config::GetInstance() {
        static Config instance;
        return instance;
    }
    
    Config::Config() : m_modified(false) {
        // 获取配置文件路径
        wchar_t appDataPath[MAX_PATH];
        if (SUCCEEDED(SHGetFolderPathW(nullptr, CSIDL_APPDATA, nullptr, 0, appDataPath))) {
            m_configFilePath = String(appDataPath) + L"\\YGUninstaller\\config.ini";
        } else {
            m_configFilePath = GetApplicationPath() + L"\\config.ini";
        }
        
        LoadDefaults();
    }
    
    Config::~Config() {
        if (m_modified) {
            Save();
        }
    }
    
    ErrorCode Config::Load() {
        std::lock_guard<std::mutex> lock(m_mutex);
        
        try {
            std::wifstream file(WStringToString(m_configFilePath).c_str());
            if (!file.is_open()) {
                // 配置文件不存在，使用默认配置
                YG_LOG_INFO(L"配置文件不存在，使用默认配置: " + m_configFilePath);
                return ErrorCode::Success;
            }
            
            // 设置UTF-8编码（简化版本，兼容老版本GCC）
            
            String content;
            String line;
            while (std::getline(file, line)) {
                content += line + L"\n";
            }
            file.close();
            
            ErrorCode result = ParseContent(content);
            if (result == ErrorCode::Success) {
                m_modified = false;
                YG_LOG_INFO(L"成功加载配置文件: " + m_configFilePath);
            }
            
            return result;
        }
        catch (const std::exception& ex) {
            YG_LOG_ERROR(L"加载配置文件失败: " + StringToWString(ex.what()));
            return ErrorCode::FileNotFound;
        }
    }
    
    ErrorCode Config::Save() {
        std::lock_guard<std::mutex> lock(m_mutex);
        
        try {
            // 确保目录存在
            size_t lastSlash = m_configFilePath.find_last_of(L"\\/");
            if (lastSlash != String::npos) {
                String directory = m_configFilePath.substr(0, lastSlash);
                CreateDirectoryW(directory.c_str(), nullptr);
            }
            
            std::wofstream file(WStringToString(m_configFilePath).c_str());
            if (!file.is_open()) {
                YG_LOG_ERROR(L"无法打开配置文件进行写入: " + m_configFilePath);
                return ErrorCode::AccessDenied;
            }
            
            // 设置UTF-8编码（简化版本，兼容老版本GCC）
            
            String content = GenerateContent();
            file << content;
            file.close();
            
            m_modified = false;
            YG_LOG_INFO(L"成功保存配置文件: " + m_configFilePath);
            return ErrorCode::Success;
        }
        catch (const std::exception& ex) {
            YG_LOG_ERROR(L"保存配置文件失败: " + StringToWString(ex.what()));
            return ErrorCode::GeneralError;
        }
    }
    
    String Config::GetString(const String& key, const String& defaultValue) const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return GetStringUnsafe(key, defaultValue);
    }
    
    String Config::GetStringUnsafe(const String& key, const String& defaultValue) const {
        auto it = m_values.find(key);
        if (it != m_values.end()) {
            return it->second;
        }
        return defaultValue;
    }
    
    // C++17结构化绑定优化的配置获取
    std::optional<String> Config::GetStringOptional(const String& key) const {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_values.find(key);
        if (it != m_values.end()) {
            return it->second;
        }
        return std::nullopt;
    }
    
    int Config::GetInt(const String& key, int defaultValue) const {
        String value = GetString(key);
        if (value.empty()) {
            return defaultValue;
        }
        
        try {
            return std::stoi(value);
        }
        catch (...) {
            return defaultValue;
        }
    }
    
    bool Config::GetBool(const String& key, bool defaultValue) const {
        String value = GetString(key);
        if (value.empty()) {
            return defaultValue;
        }
        
        // 转换为小写进行比较
        String lowerValue = value;
        std::transform(lowerValue.begin(), lowerValue.end(), lowerValue.begin(), ::towlower);
        
        return (lowerValue == L"true" || lowerValue == L"1" || lowerValue == L"yes" || lowerValue == L"on");
    }
    
    double Config::GetDouble(const String& key, double defaultValue) const {
        String value = GetString(key);
        if (value.empty()) {
            return defaultValue;
        }
        
        try {
            return std::stod(value);
        }
        catch (...) {
            return defaultValue;
        }
    }
    
    void Config::SetString(const String& key, const String& value) {
        std::lock_guard<std::mutex> lock(m_mutex);
        
        auto it = m_values.find(key);
        if (it == m_values.end() || it->second != value) {
            m_values[key] = value;
            m_modified = true;
        }
    }
    
    void Config::SetInt(const String& key, int value) {
        SetString(key, std::to_wstring(value));
    }
    
    void Config::SetBool(const String& key, bool value) {
        SetString(key, value ? L"true" : L"false");
    }
    
    void Config::SetDouble(const String& key, double value) {
        SetString(key, std::to_wstring(value));
    }
    
    bool Config::HasKey(const String& key) const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_values.find(key) != m_values.end();
    }
    
    bool Config::RemoveKey(const String& key) {
        std::lock_guard<std::mutex> lock(m_mutex);
        
        auto it = m_values.find(key);
        if (it != m_values.end()) {
            m_values.erase(it);
            m_modified = true;
            return true;
        }
        return false;
    }
    
    void Config::Clear() {
        std::lock_guard<std::mutex> lock(m_mutex);
        
        if (!m_values.empty()) {
            m_values.clear();
            m_modified = true;
        }
    }
    
    void Config::ResetToDefaults() {
        std::lock_guard<std::mutex> lock(m_mutex);
        
        m_values.clear();
        LoadDefaults();
        m_modified = true;
    }
    
    String Config::GetConfigFilePath() const {
        return m_configFilePath;
    }
    
    void Config::LoadDefaults() {
        // 窗口设置
        m_values[KEY_WINDOW_WIDTH] = L"800";
        m_values[KEY_WINDOW_HEIGHT] = L"600";
        m_values[KEY_WINDOW_X] = L"-1";  // 居中
        m_values[KEY_WINDOW_Y] = L"-1";  // 居中
        m_values[KEY_WINDOW_MAXIMIZED] = L"false";
        
        // 功能设置
        m_values[KEY_AUTO_REFRESH] = L"true";
        m_values[KEY_SHOW_SYSTEM_COMPONENTS] = L"false";
        m_values[KEY_CONFIRM_UNINSTALL] = L"true";
        m_values[KEY_DEEP_SCAN] = L"true";
        
        // 日志设置
        m_values[KEY_LOG_LEVEL] = L"1";  // Info
        
        // 语言设置
        m_values[KEY_LANGUAGE] = L"zh-CN";
    }
    
    ErrorCode Config::ParseContent(const String& content) {
        std::wistringstream stream(content);
        String line;
        int lineNumber = 0;
        
        while (std::getline(stream, line)) {
            lineNumber++;
            
            // 去掉首尾空格
            line = Trim(line);
            
            // 跳过空行和注释行
            if (line.empty() || line[0] == L'#' || line[0] == L';') {
                continue;
            }
            
            // 查找等号分隔符
            size_t equalPos = line.find(L'=');
            if (equalPos == String::npos) {
                YG_LOG_WARNING(L"配置文件第" + std::to_wstring(lineNumber) + L"行格式错误: " + line);
                continue;
            }
            
            // 使用C++17结构化绑定优化键值对处理
            auto [key, value] = std::make_pair(
                Trim(line.substr(0, equalPos)),
                Trim(line.substr(equalPos + 1))
            );
            
            if (!key.empty()) {
                m_values[key] = value;
            }
        }
        
        return ErrorCode::Success;
    }
    
    String Config::GenerateContent() const {
        std::wostringstream stream;
        
        // 添加文件头注释
        stream << L"# YG Uninstaller Configuration File\n";
        stream << L"# Generated automatically, do not edit manually\n";
        stream << L"\n";
        
        // 窗口设置
        stream << L"# Window Settings\n";
        stream << KEY_WINDOW_WIDTH << L"=" << GetStringUnsafe(KEY_WINDOW_WIDTH) << L"\n";
        stream << KEY_WINDOW_HEIGHT << L"=" << GetStringUnsafe(KEY_WINDOW_HEIGHT) << L"\n";
        stream << KEY_WINDOW_X << L"=" << GetStringUnsafe(KEY_WINDOW_X) << L"\n";
        stream << KEY_WINDOW_Y << L"=" << GetStringUnsafe(KEY_WINDOW_Y) << L"\n";
        stream << KEY_WINDOW_MAXIMIZED << L"=" << GetStringUnsafe(KEY_WINDOW_MAXIMIZED) << L"\n";
        stream << L"\n";
        
        // 功能设置
        stream << L"# Feature Settings\n";
        stream << KEY_AUTO_REFRESH << L"=" << GetStringUnsafe(KEY_AUTO_REFRESH) << L"\n";
        stream << KEY_SHOW_SYSTEM_COMPONENTS << L"=" << GetStringUnsafe(KEY_SHOW_SYSTEM_COMPONENTS) << L"\n";
        stream << KEY_CONFIRM_UNINSTALL << L"=" << GetStringUnsafe(KEY_CONFIRM_UNINSTALL) << L"\n";
        stream << KEY_DEEP_SCAN << L"=" << GetStringUnsafe(KEY_DEEP_SCAN) << L"\n";
        stream << L"\n";
        
        // 日志设置
        stream << L"# Log Settings\n";
        stream << KEY_LOG_LEVEL << L"=" << GetStringUnsafe(KEY_LOG_LEVEL) << L"\n";
        stream << L"\n";
        
        // 语言设置
        stream << L"# Language Settings\n";
        stream << KEY_LANGUAGE << L"=" << GetStringUnsafe(KEY_LANGUAGE) << L"\n";
        stream << L"\n";
        
        // 添加其他自定义配置 - 使用C++17结构化绑定
        stream << L"# Custom Settings\n";
        for (const auto& [key, value] : m_values) {
            // 跳过已经写入的标准配置
            if (key == KEY_WINDOW_WIDTH || key == KEY_WINDOW_HEIGHT ||
                key == KEY_WINDOW_X || key == KEY_WINDOW_Y ||
                key == KEY_WINDOW_MAXIMIZED || key == KEY_AUTO_REFRESH ||
                key == KEY_SHOW_SYSTEM_COMPONENTS || key == KEY_CONFIRM_UNINSTALL ||
                key == KEY_DEEP_SCAN || key == KEY_LOG_LEVEL ||
                key == KEY_LANGUAGE) {
                continue;
            }
            
            stream << key << L"=" << value << L"\n";
        }
        
        return stream.str();
    }
    
    String Config::Trim(const String& str) const {
        if (str.empty()) {
            return str;
        }
        
        size_t start = str.find_first_not_of(L" \t\r\n");
        if (start == String::npos) {
            return String();
        }
        
        size_t end = str.find_last_not_of(L" \t\r\n");
        return str.substr(start, end - start + 1);
    }
    
} // namespace YG
