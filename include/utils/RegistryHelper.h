/**
 * @file RegistryHelper.h
 * @brief Windows注册表操作工具类
 * @author YG Software
 * @version 1.0.1
 * @date 2025-09-17
 */

#pragma once

#include "core/Common.h"
#include <vector>
#include <functional>

namespace YG {
    
    /**
     * @brief 注册表值类型枚举
     */
    enum class RegistryValueType {
        String = REG_SZ,
        ExpandString = REG_EXPAND_SZ,
        Binary = REG_BINARY,
        DWord = REG_DWORD,
        QWord = REG_QWORD,
        MultiString = REG_MULTI_SZ
    };
    
    /**
     * @brief 注册表值信息结构
     */
    struct RegistryValueInfo {
        String name;                    ///< 值名称
        RegistryValueType type;         ///< 值类型
        std::vector<BYTE> data;         ///< 值数据
        DWORD dataSize;                 ///< 数据大小
        
        RegistryValueInfo() : type(RegistryValueType::String), dataSize(0) {}
    };
    
    /**
     * @brief 注册表键信息结构
     */
    struct RegistryKeyInfo {
        String name;                    ///< 键名称
        String fullPath;                ///< 完整路径
        DWORD subKeyCount;              ///< 子键数量
        DWORD valueCount;               ///< 值数量
        FILETIME lastWriteTime;         ///< 最后写入时间
        
        RegistryKeyInfo() : subKeyCount(0), valueCount(0) {
            lastWriteTime.dwLowDateTime = 0;
            lastWriteTime.dwHighDateTime = 0;
        }
    };
    
    /**
     * @brief 注册表操作工具类
     * 
     * 提供安全、便捷的注册表操作接口
     */
    class RegistryHelper {
    public:
        /**
         * @brief 默认构造函数
         */
        RegistryHelper() = default;
        
        /**
         * @brief 析构函数
         */
        ~RegistryHelper() = default;
        
        // 键操作
        
        /**
         * @brief 打开注册表键
         * @param hKeyParent 父键句柄
         * @param subKey 子键路径
         * @param samDesired 访问权限
         * @param hKey 输出键句柄
         * @return ErrorCode 操作结果
         */
        static ErrorCode OpenKey(HKEY hKeyParent, const String& subKey, 
                               REGSAM samDesired, HKEY& hKey);
        
        /**
         * @brief 创建注册表键
         * @param hKeyParent 父键句柄
         * @param subKey 子键路径
         * @param hKey 输出键句柄
         * @param created 是否新创建
         * @return ErrorCode 操作结果
         */
        static ErrorCode CreateKey(HKEY hKeyParent, const String& subKey, 
                                 HKEY& hKey, bool* created = nullptr);
        
        /**
         * @brief 删除注册表键
         * @param hKeyParent 父键句柄
         * @param subKey 子键路径
         * @param recursive 是否递归删除
         * @return ErrorCode 操作结果
         */
        static ErrorCode DeleteKey(HKEY hKeyParent, const String& subKey, bool recursive = false);
        
        /**
         * @brief 检查注册表键是否存在
         * @param hKeyParent 父键句柄
         * @param subKey 子键路径
         * @return bool 是否存在
         */
        static bool KeyExists(HKEY hKeyParent, const String& subKey);
        
        /**
         * @brief 枚举子键
         * @param hKey 键句柄
         * @param subKeys 输出子键列表
         * @return ErrorCode 操作结果
         */
        static ErrorCode EnumerateSubKeys(HKEY hKey, StringVector& subKeys);
        
        /**
         * @brief 枚举子键详细信息
         * @param hKey 键句柄
         * @param keyInfos 输出键信息列表
         * @return ErrorCode 操作结果
         */
        static ErrorCode EnumerateSubKeysInfo(HKEY hKey, std::vector<RegistryKeyInfo>& keyInfos);
        
        /**
         * @brief 获取键信息
         * @param hKey 键句柄
         * @param keyInfo 输出键信息
         * @return ErrorCode 操作结果
         */
        static ErrorCode GetKeyInfo(HKEY hKey, RegistryKeyInfo& keyInfo);
        
        // 值操作
        
        /**
         * @brief 读取字符串值
         * @param hKey 键句柄
         * @param valueName 值名称
         * @param value 输出字符串值
         * @return ErrorCode 操作结果
         */
        static ErrorCode ReadString(HKEY hKey, const String& valueName, String& value);
        
        /**
         * @brief 读取DWORD值
         * @param hKey 键句柄
         * @param valueName 值名称
         * @param value 输出DWORD值
         * @return ErrorCode 操作结果
         */
        static ErrorCode ReadDWord(HKEY hKey, const String& valueName, DWORD& value);
        
        /**
         * @brief 读取QWORD值
         * @param hKey 键句柄
         * @param valueName 值名称
         * @param value 输出QWORD值
         * @return ErrorCode 操作结果
         */
        static ErrorCode ReadQWord(HKEY hKey, const String& valueName, DWORD64& value);
        
        /**
         * @brief 读取二进制值
         * @param hKey 键句柄
         * @param valueName 值名称
         * @param data 输出二进制数据
         * @return ErrorCode 操作结果
         */
        static ErrorCode ReadBinary(HKEY hKey, const String& valueName, std::vector<BYTE>& data);
        
        /**
         * @brief 写入字符串值
         * @param hKey 键句柄
         * @param valueName 值名称
         * @param value 字符串值
         * @return ErrorCode 操作结果
         */
        static ErrorCode WriteString(HKEY hKey, const String& valueName, const String& value);
        
        /**
         * @brief 写入DWORD值
         * @param hKey 键句柄
         * @param valueName 值名称
         * @param value DWORD值
         * @return ErrorCode 操作结果
         */
        static ErrorCode WriteDWord(HKEY hKey, const String& valueName, DWORD value);
        
        /**
         * @brief 写入QWORD值
         * @param hKey 键句柄
         * @param valueName 值名称
         * @param value QWORD值
         * @return ErrorCode 操作结果
         */
        static ErrorCode WriteQWord(HKEY hKey, const String& valueName, DWORD64 value);
        
        /**
         * @brief 写入二进制值
         * @param hKey 键句柄
         * @param valueName 值名称
         * @param data 二进制数据
         * @return ErrorCode 操作结果
         */
        static ErrorCode WriteBinary(HKEY hKey, const String& valueName, const std::vector<BYTE>& data);
        
        /**
         * @brief 删除值
         * @param hKey 键句柄
         * @param valueName 值名称
         * @return ErrorCode 操作结果
         */
        static ErrorCode DeleteValue(HKEY hKey, const String& valueName);
        
        /**
         * @brief 检查值是否存在
         * @param hKey 键句柄
         * @param valueName 值名称
         * @return bool 是否存在
         */
        static bool ValueExists(HKEY hKey, const String& valueName);
        
        /**
         * @brief 枚举值
         * @param hKey 键句柄
         * @param valueNames 输出值名称列表
         * @return ErrorCode 操作结果
         */
        static ErrorCode EnumerateValues(HKEY hKey, StringVector& valueNames);
        
        /**
         * @brief 枚举值详细信息
         * @param hKey 键句柄
         * @param valueInfos 输出值信息列表
         * @return ErrorCode 操作结果
         */
        static ErrorCode EnumerateValuesInfo(HKEY hKey, std::vector<RegistryValueInfo>& valueInfos);
        
        // 高级操作
        
        /**
         * @brief 导出注册表键到文件
         * @param hKey 键句柄
         * @param filePath 输出文件路径
         * @return ErrorCode 操作结果
         */
        static ErrorCode ExportKey(HKEY hKey, const String& filePath);
        
        /**
         * @brief 从文件导入注册表
         * @param filePath 注册表文件路径
         * @return ErrorCode 操作结果
         */
        static ErrorCode ImportFromFile(const String& filePath);
        
        /**
         * @brief 备份注册表键
         * @param hKeyParent 父键句柄
         * @param subKey 子键路径
         * @param backupPath 备份文件路径
         * @return ErrorCode 操作结果
         */
        static ErrorCode BackupKey(HKEY hKeyParent, const String& subKey, const String& backupPath);
        
        /**
         * @brief 搜索注册表键
         * @param hKeyRoot 根键句柄
         * @param searchPattern 搜索模式
         * @param foundKeys 输出找到的键路径
         * @param maxResults 最大结果数
         * @return ErrorCode 操作结果
         */
        static ErrorCode SearchKeys(HKEY hKeyRoot, const String& searchPattern, 
                                   StringVector& foundKeys, int maxResults = 1000);
        
        /**
         * @brief 搜索注册表值
         * @param hKeyRoot 根键句柄
         * @param searchPattern 搜索模式
         * @param foundValues 输出找到的值路径
         * @param maxResults 最大结果数
         * @return ErrorCode 操作结果
         */
        static ErrorCode SearchValues(HKEY hKeyRoot, const String& searchPattern, 
                                     StringVector& foundValues, int maxResults = 1000);
        
        /**
         * @brief 递归删除键及其所有子键
         * @param hKeyParent 父键句柄
         * @param subKey 子键路径
         * @return ErrorCode 操作结果
         */
        static ErrorCode RecursiveDeleteKey(HKEY hKeyParent, const String& subKey);
        
        /**
         * @brief 复制注册表键
         * @param hKeySrc 源键句柄
         * @param hKeyDest 目标键句柄
         * @param recursive 是否递归复制
         * @return ErrorCode 操作结果
         */
        static ErrorCode CopyKey(HKEY hKeySrc, HKEY hKeyDest, bool recursive = true);
        
        // 工具函数
        
        /**
         * @brief 获取预定义键的字符串表示
         * @param hKey 预定义键句柄
         * @return String 键的字符串表示
         */
        static String GetPredefinedKeyName(HKEY hKey);
        
        /**
         * @brief 解析完整的注册表路径
         * @param fullPath 完整路径
         * @param rootKey 输出根键
         * @param subKey 输出子键路径
         * @return bool 是否解析成功
         */
        static bool ParseRegistryPath(const String& fullPath, HKEY& rootKey, String& subKey);
        
        /**
         * @brief 格式化注册表路径
         * @param hKeyRoot 根键句柄
         * @param subKey 子键路径
         * @return String 格式化的路径
         */
        static String FormatRegistryPath(HKEY hKeyRoot, const String& subKey);
        
        /**
         * @brief 检查是否有注册表访问权限
         * @param hKeyParent 父键句柄
         * @param subKey 子键路径
         * @param samDesired 所需权限
         * @return bool 是否有权限
         */
        static bool HasRegistryAccess(HKEY hKeyParent, const String& subKey, REGSAM samDesired);
        
        /**
         * @brief 获取注册表值的字符串表示
         * @param valueInfo 值信息
         * @return String 值的字符串表示
         */
        static String ValueToString(const RegistryValueInfo& valueInfo);
        
        /**
         * @brief 关闭注册表键句柄
         * @param hKey 键句柄
         */
        static void CloseKey(HKEY hKey);
        
    private:
        /**
         * @brief 递归搜索键
         * @param hKey 当前键句柄
         * @param currentPath 当前路径
         * @param searchPattern 搜索模式
         * @param foundKeys 找到的键列表
         * @param maxResults 最大结果数
         * @param currentCount 当前计数
         */
        static void RecursiveSearchKeys(HKEY hKey, const String& currentPath, 
                                       const String& searchPattern, StringVector& foundKeys, 
                                       int maxResults, int& currentCount);
        
        /**
         * @brief 递归搜索值
         * @param hKey 当前键句柄
         * @param currentPath 当前路径
         * @param searchPattern 搜索模式
         * @param foundValues 找到的值列表
         * @param maxResults 最大结果数
         * @param currentCount 当前计数
         */
        static void RecursiveSearchValues(HKEY hKey, const String& currentPath, 
                                         const String& searchPattern, StringVector& foundValues, 
                                         int maxResults, int& currentCount);
        
        /**
         * @brief 模式匹配
         * @param text 文本
         * @param pattern 模式
         * @return bool 是否匹配
         */
        static bool PatternMatch(const String& text, const String& pattern);
    };
    
} // namespace YG
