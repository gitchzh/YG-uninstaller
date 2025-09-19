/**
 * @file UninstallerService.h
 * @brief 程序卸载服务头文件
 * @author gmrchzh@gmail.com
 * @version 1.0.0
 * @date 2025-09-17
 */

#pragma once

#include "core/Common.h"

namespace YG {

    /**
     * @brief 卸载模式
     */
    enum class UninstallMode {
        Standard,   ///< 标准卸载
        Silent,     ///< 静默卸载
        Force,      ///< 强制卸载
        Deep        ///< 深度卸载
    };

    /**
     * @brief 程序卸载服务
     * 
     * 提供程序卸载的核心功能，支持多种卸载模式
     */
    class UninstallerService {
    public:
        /**
         * @brief 构造函数
         */
        UninstallerService();

        /**
         * @brief 析构函数
         */
        ~UninstallerService();

        /**
         * @brief 卸载指定程序
         * @param program 程序信息
         * @param mode 卸载模式
         * @return ErrorCode 操作结果
         */
        ErrorCode UninstallProgram(const ProgramInfo& program, UninstallMode mode);

    private:
        /**
         * @brief 执行标准卸载
         * @param program 程序信息
         * @return ErrorCode 操作结果
         */
        ErrorCode ExecuteStandardUninstall(const ProgramInfo& program);

        /**
         * @brief 执行静默卸载
         * @param program 程序信息
         * @return ErrorCode 操作结果
         */
        ErrorCode ExecuteSilentUninstall(const ProgramInfo& program);

        /**
         * @brief 执行强制卸载
         * @param program 程序信息
         * @return ErrorCode 操作结果
         */
        ErrorCode ExecuteForceUninstall(const ProgramInfo& program);

        /**
         * @brief 执行深度卸载
         * @param program 程序信息
         * @return ErrorCode 操作结果
         */
        ErrorCode ExecuteDeepUninstall(const ProgramInfo& program);

        /**
         * @brief 清理注册表项
         * @param program 程序信息
         * @return ErrorCode 操作结果
         */
        ErrorCode CleanupRegistryEntries(const ProgramInfo& program);

        /**
         * @brief 清理快捷方式
         * @param program 程序信息
         * @return ErrorCode 操作结果
         */
        ErrorCode CleanupShortcuts(const ProgramInfo& program);
    };

    // 辅助函数
    bool PathExists(const String& path);

} // namespace YG