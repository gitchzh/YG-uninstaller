/**
 * @file StringUtils.h
 * @brief 字符串处理工具类
 * @author YG Software
 * @version 1.0.1
 * @date 2025-01-21
 */

#pragma once

#include "core/Common.h"
#include <vector>
#include <algorithm>
#include <locale>

namespace YG {
    
    /**
     * @brief 字符串处理工具类
     * 
     * 提供常用的字符串操作功能
     */
    class StringUtils {
    public:
        /**
         * @brief 去除字符串首尾空白字符
         * @param str 输入字符串
         * @return String 处理后的字符串
         */
        static String Trim(const String& str);
        
        /**
         * @brief 去除字符串左侧空白字符
         * @param str 输入字符串
         * @return String 处理后的字符串
         */
        static String TrimLeft(const String& str);
        
        /**
         * @brief 去除字符串右侧空白字符
         * @param str 输入字符串
         * @return String 处理后的字符串
         */
        static String TrimRight(const String& str);
        
        /**
         * @brief 转换为大写
         * @param str 输入字符串
         * @return String 大写字符串
         */
        static String ToUpper(const String& str);
        
        /**
         * @brief 转换为小写
         * @param str 输入字符串
         * @return String 小写字符串
         */
        static String ToLower(const String& str);
        
        /**
         * @brief 分割字符串
         * @param str 输入字符串
         * @param delimiter 分隔符
         * @param maxSplits 最大分割次数，0表示无限制
         * @return StringVector 分割结果
         */
        static StringVector Split(const String& str, const String& delimiter, size_t maxSplits = 0);
        
        /**
         * @brief 连接字符串数组
         * @param strings 字符串数组
         * @param separator 分隔符
         * @return String 连接结果
         */
        static String Join(const StringVector& strings, const String& separator);
        
        /**
         * @brief 替换字符串中的所有匹配项
         * @param str 输入字符串
         * @param from 要替换的子串
         * @param to 替换为的子串
         * @return String 替换后的字符串
         */
        static String ReplaceAll(const String& str, const String& from, const String& to);
        
        /**
         * @brief 检查字符串是否以指定前缀开始
         * @param str 输入字符串
         * @param prefix 前缀
         * @param ignoreCase 是否忽略大小写
         * @return bool 是否以前缀开始
         */
        static bool StartsWith(const String& str, const String& prefix, bool ignoreCase = false);
        
        /**
         * @brief 检查字符串是否以指定后缀结束
         * @param str 输入字符串
         * @param suffix 后缀
         * @param ignoreCase 是否忽略大小写
         * @return bool 是否以后缀结束
         */
        static bool EndsWith(const String& str, const String& suffix, bool ignoreCase = false);
        
        /**
         * @brief 检查字符串是否包含子串
         * @param str 输入字符串
         * @param substring 子串
         * @param ignoreCase 是否忽略大小写
         * @return bool 是否包含
         */
        static bool Contains(const String& str, const String& substring, bool ignoreCase = false);
        
        /**
         * @brief 格式化文件大小
         * @param sizeInBytes 字节大小
         * @param precision 小数精度
         * @return String 格式化的大小字符串
         */
        static String FormatFileSize(DWORD64 sizeInBytes, int precision = 1);
        
        /**
         * @brief 格式化时间间隔
         * @param milliseconds 毫秒数
         * @return String 格式化的时间字符串
         */
        static String FormatDuration(DWORD milliseconds);
        
        /**
         * @brief 格式化日期时间
         * @param systemTime 系统时间
         * @param format 格式字符串
         * @return String 格式化的日期时间
         */
        static String FormatDateTime(const SYSTEMTIME& systemTime, const String& format = L"yyyy-MM-dd HH:mm:ss");
        
        /**
         * @brief 截断字符串到指定长度
         * @param str 输入字符串
         * @param maxLength 最大长度
         * @param ellipsis 省略号
         * @return String 截断后的字符串
         */
        static String Truncate(const String& str, size_t maxLength, const String& ellipsis = L"...");
        
        /**
         * @brief 填充字符串到指定长度
         * @param str 输入字符串
         * @param length 目标长度
         * @param fillChar 填充字符
         * @param leftAlign 是否左对齐
         * @return String 填充后的字符串
         */
        static String Pad(const String& str, size_t length, wchar_t fillChar = L' ', bool leftAlign = true);
        
        /**
         * @brief 比较两个字符串（忽略大小写）
         * @param str1 字符串1
         * @param str2 字符串2
         * @return int 比较结果：<0, 0, >0
         */
        static int CompareIgnoreCase(const String& str1, const String& str2);
        
        /**
         * @brief 检查字符串是否为数字
         * @param str 输入字符串
         * @return bool 是否为数字
         */
        static bool IsNumeric(const String& str);
        
        /**
         * @brief 检查字符串是否为有效的版本号
         * @param version 版本字符串
         * @return bool 是否为有效版本号
         */
        static bool IsValidVersion(const String& version);
        
        /**
         * @brief 比较版本号
         * @param version1 版本1
         * @param version2 版本2
         * @return int 比较结果：<0表示version1<version2，0表示相等，>0表示version1>version2
         */
        static int CompareVersions(const String& version1, const String& version2);
        
        /**
         * @brief 生成随机字符串
         * @param length 长度
         * @param useAlphaNumeric 是否仅使用字母数字
         * @return String 随机字符串
         */
        static String GenerateRandomString(size_t length, bool useAlphaNumeric = true);
        
        /**
         * @brief 计算字符串的哈希值
         * @param str 输入字符串
         * @return size_t 哈希值
         */
        static size_t CalculateHash(const String& str);
        
    private:
        /**
         * @brief 解析版本号为数字数组
         * @param version 版本字符串
         * @return std::vector<int> 版本号数组
         */
        static std::vector<int> ParseVersion(const String& version);
    };
    
} // namespace YG
