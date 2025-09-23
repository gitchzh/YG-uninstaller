/**
 * @file MainWindowTray.h
 * @brief 主窗口系统托盘功能头文件
 * @author gmrchzh@gmail.com
 * @version 1.0.1
 * @date 2025-09-21
 */

#pragma once

#include "core/Common.h"
#include <windows.h>
#include <shellapi.h>

namespace YG {

    // 前向声明
    class MainWindow;

    /**
     * @brief 主窗口系统托盘功能类
     * 
     * 负责处理所有与系统托盘相关的功能，包括：
     * - 系统托盘图标的创建和管理
     * - 托盘消息处理
     * - 最小化到托盘和恢复
     * - 托盘右键菜单
     */
    class MainWindowTray {
    public:
        /**
         * @brief 构造函数
         * @param mainWindow 主窗口指针
         */
        explicit MainWindowTray(MainWindow* mainWindow);
        
        /**
         * @brief 析构函数
         */
        ~MainWindowTray();
        
        /**
         * @brief 创建系统托盘图标
         */
        void CreateSystemTray();
        
        /**
         * @brief 显示或隐藏系统托盘图标
         * @param show 是否显示
         */
        void ShowSystemTray(bool show);
        
        /**
         * @brief 处理托盘通知消息
         * @param wParam 消息参数1
         * @param lParam 消息参数2
         */
        void OnTrayNotify(WPARAM wParam, LPARAM lParam);
        
        /**
         * @brief 最小化到系统托盘
         */
        void MinimizeToTray();
        
        /**
         * @brief 从系统托盘恢复窗口
         */
        void RestoreFromTray();
        
        /**
         * @brief 检查是否在托盘中
         * @return bool 是否在托盘中
         */
        bool IsInTray() const { return m_isInTray; }
        
        /**
         * @brief 清理系统托盘
         */
        void Cleanup();
        
    private:
        MainWindow* m_mainWindow;           ///< 主窗口指针
        NOTIFYICONDATAW m_nid;             ///< 托盘图标数据
        bool m_isInTray;                   ///< 是否在托盘中
        
        // 禁用拷贝构造和赋值操作
        MainWindowTray(const MainWindowTray&) = delete;
        MainWindowTray& operator=(const MainWindowTray&) = delete;
    };

} // namespace YG
