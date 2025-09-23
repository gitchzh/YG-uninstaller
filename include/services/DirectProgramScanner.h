/**
 * @file DirectProgramScanner.h
 * @brief 直接使用Windows API扫描已安装程序
 * @author gmrchzh@gmail.com
 * @version 1.0.1
 * @date 2025-09-17
 */

#pragma once

#include "core/Common.h"
#include <vector>

namespace YG {

/**
 * @brief 直接从注册表获取已安装程序列表
 * @param programs 输出的程序列表
 * @return 找到的程序数量
 */
int GetInstalledProgramsDirect(std::vector<ProgramInfo>& programs);

/**
 * @brief 估算目录大小（快速版本）
 * @param directoryPath 目录路径
 * @return DWORD64 估算的目录大小（字节）
 */
DWORD64 EstimateDirectorySize(const String& directoryPath);

/**
 * @brief 从卸载字符串估算程序大小
 * @param uninstallString 卸载字符串
 * @return DWORD64 估算的程序大小（字节）
 */
DWORD64 EstimateExecutableSize(const String& uninstallString);

/**
 * @brief 从卸载字符串获取文件创建日期
 * @param uninstallString 卸载字符串
 * @return String 日期字符串 (YYYYMMDD格式)
 */
String GetDateFromUninstallString(const String& uninstallString);

/**
 * @brief 从目录获取创建日期
 * @param directoryPath 目录路径
 * @return String 日期字符串 (YYYYMMDD格式)
 */
String GetDateFromDirectory(const String& directoryPath);

/**
 * @brief 从注册表键获取修改日期
 * @param hKey 注册表键句柄
 * @return String 日期字符串 (YYYYMMDD格式)
 */
String GetDateFromRegistryKey(HKEY hKey);

} // namespace YG
