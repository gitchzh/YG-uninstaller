/**
 * @file InputValidator.h
 * @brief 输入验证工具类
 * @author YG Software
 * @version 1.0.1
 * @date 2025-01-21
 */

#pragma once

#include "core/Common.h"
#include "core/DetailedErrorCodes.h"
#include <regex>
#include <vector>

namespace YG {
    
    /**
     * @brief 输入验证工具类
     * 
     * 提供各种输入验证功能，确保数据安全性
     */
    class InputValidator {
    public:
        /**
         * @brief 验证路径是否安全
         * @param path 要验证的路径
         * @param allowRelative 是否允许相对路径
         * @return ErrorContext 验证结果
         */
        static ErrorContext ValidatePath(const String& path, bool allowRelative = true);
        
        /**
         * @brief 验证文件名是否有效
         * @param fileName 要验证的文件名
         * @return ErrorContext 验证结果
         */
        static ErrorContext ValidateFileName(const String& fileName);
        
        /**
         * @brief 验证注册表键路径
         * @param keyPath 注册表键路径
         * @return ErrorContext 验证结果
         */
        static ErrorContext ValidateRegistryKeyPath(const String& keyPath);
        
        /**
         * @brief 验证程序卸载字符串
         * @param uninstallString 卸载字符串
         * @return ErrorContext 验证结果
         */
        static ErrorContext ValidateUninstallString(const String& uninstallString);
        
        /**
         * @brief 验证配置值
         * @param key 配置键
         * @param value 配置值
         * @return ErrorContext 验证结果
         */
        static ErrorContext ValidateConfigValue(const String& key, const String& value);
        
        /**
         * @brief 验证整数范围
         * @param value 要验证的值
         * @param minValue 最小值
         * @param maxValue 最大值
         * @param paramName 参数名称
         * @return ErrorContext 验证结果
         */
        static ErrorContext ValidateIntRange(int value, int minValue, int maxValue, 
                                           const String& paramName = L"参数");
        
        /**
         * @brief 验证字符串长度
         * @param str 要验证的字符串
         * @param minLength 最小长度
         * @param maxLength 最大长度
         * @param paramName 参数名称
         * @return ErrorContext 验证结果
         */
        static ErrorContext ValidateStringLength(const String& str, size_t minLength, 
                                                size_t maxLength, const String& paramName = L"字符串");
        
        /**
         * @brief 验证字符串是否为空或仅包含空白字符
         * @param str 要验证的字符串
         * @param paramName 参数名称
         * @return ErrorContext 验证结果
         */
        static ErrorContext ValidateNotEmpty(const String& str, const String& paramName = L"参数");
        
        /**
         * @brief 验证字符串是否包含危险字符
         * @param str 要验证的字符串
         * @param paramName 参数名称
         * @return ErrorContext 验证结果
         */
        static ErrorContext ValidateSafeString(const String& str, const String& paramName = L"字符串");
        
        /**
         * @brief 验证版本号格式
         * @param version 版本号字符串
         * @return ErrorContext 验证结果
         */
        static ErrorContext ValidateVersionString(const String& version);
        
        /**
         * @brief 验证URL格式
         * @param url URL字符串
         * @return ErrorContext 验证结果
         */
        static ErrorContext ValidateURL(const String& url);
        
        /**
         * @brief 验证邮箱地址格式
         * @param email 邮箱地址
         * @return ErrorContext 验证结果
         */
        static ErrorContext ValidateEmail(const String& email);
        
        /**
         * @brief 清理和转义字符串中的特殊字符
         * @param input 输入字符串
         * @return String 清理后的字符串
         */
        static String SanitizeString(const String& input);
        
        /**
         * @brief 转义SQL查询中的特殊字符（虽然项目不用SQL，但作为安全最佳实践）
         * @param input 输入字符串
         * @return String 转义后的字符串
         */
        static String EscapeSQLString(const String& input);
        
        /**
         * @brief 转义正则表达式中的特殊字符
         * @param input 输入字符串
         * @return String 转义后的字符串
         */
        static String EscapeRegexString(const String& input);
        
        /**
         * @brief 验证程序信息结构的完整性
         * @param programInfo 程序信息
         * @return ErrorContext 验证结果
         */
        static ErrorContext ValidateProgramInfo(const ProgramInfo& programInfo);
        
    private:
        /**
         * @brief 检查路径是否包含危险模式
         * @param path 路径字符串
         * @return bool 是否包含危险模式
         */
        static bool ContainsDangerousPathPatterns(const String& path);
        
        /**
         * @brief 检查字符串是否包含控制字符
         * @param str 字符串
         * @return bool 是否包含控制字符
         */
        static bool ContainsControlCharacters(const String& str);
        
        /**
         * @brief 检查是否为有效的Windows路径字符
         * @param ch 字符
         * @return bool 是否为有效字符
         */
        static bool IsValidPathCharacter(wchar_t ch);
        
        /**
         * @brief 检查是否为有效的文件名字符
         * @param ch 字符
         * @return bool 是否为有效字符
         */
        static bool IsValidFileNameCharacter(wchar_t ch);
        
        // 危险路径模式列表
        static const std::vector<String> s_dangerousPathPatterns;
        
        // 危险字符列表
        static const std::vector<wchar_t> s_dangerousCharacters;
        
        // 保留的Windows文件名
        static const std::vector<String> s_reservedFileNames;
    };
    
} // namespace YG
