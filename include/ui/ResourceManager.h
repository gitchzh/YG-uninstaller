/**
 * @file ResourceManager.h
 * @brief 资源管理器类 - 使用RAII管理UI资源
 * @author YG Software
 * @version 1.0.1
 * @date 2025-01-21
 */

#pragma once

#include "core/Common.h"
#include <windows.h>
#include <commctrl.h>
#include <mutex>
#include <functional>

namespace YG {
    
    /**
     * @brief UI资源管理器
     * 
     * 使用RAII原则管理所有UI资源，确保异常安全的资源释放
     */
    class ResourceManager {
    public:
        /**
         * @brief 构造函数
         */
        ResourceManager();
        
        /**
         * @brief 析构函数 - 自动清理所有资源
         */
        ~ResourceManager();
        
        // 禁用拷贝和赋值
        YG_DISABLE_COPY_AND_ASSIGN(ResourceManager);
        
        /**
         * @brief 设置主窗口句柄
         * @param hWnd 窗口句柄
         */
        void SetMainWindow(HWND hWnd);
        
        /**
         * @brief 设置菜单资源
         * @param hMenu 主菜单句柄
         * @param hContextMenu 上下文菜单句柄
         */
        void SetMenus(HMENU hMenu, HMENU hContextMenu);
        
        /**
         * @brief 设置控件句柄
         * @param hListView ListView句柄
         * @param originalProc 原始窗口过程
         */
        void SetControls(HWND hListView, WNDPROC originalProc);
        
        /**
         * @brief 设置图像列表
         * @param hImageList 图像列表句柄
         */
        void SetImageList(HIMAGELIST hImageList);
        
        /**
         * @brief 停止所有定时器
         */
        void StopTimers();
        
        /**
         * @brief 清理系统托盘
         * @param cleanupFunc 清理函数
         */
        void SetTrayCleanup(std::function<void()> cleanupFunc);
        
        /**
         * @brief 设置服务清理函数
         * @param programDetectorCleanup 程序检测器清理函数
         * @param uninstallerCleanup 卸载服务清理函数
         */
        void SetServiceCleanup(std::function<void()> programDetectorCleanup,
                              std::function<void()> uninstallerCleanup);
        
        /**
         * @brief 手动触发清理（用于紧急情况）
         */
        void ForceCleanup();
        
        /**
         * @brief 检查资源状态
         * @return String 资源状态报告
         */
        String GetResourceStatus() const;
        
    private:
        /**
         * @brief 清理窗口资源
         */
        void CleanupWindow();
        
        /**
         * @brief 清理菜单资源
         */
        void CleanupMenus();
        
        /**
         * @brief 清理控件资源
         */
        void CleanupControls();
        
        /**
         * @brief 清理图像资源
         */
        void CleanupImages();
        
        /**
         * @brief 清理定时器
         */
        void CleanupTimers();
        
        /**
         * @brief 清理服务
         */
        void CleanupServices();
        
        /**
         * @brief 清理系统托盘
         */
        void CleanupTray();
        
    private:
        // 窗口资源
        HWND m_hWnd;
        
        // 菜单资源
        HMENU m_hMenu;
        HMENU m_hContextMenu;
        
        // 控件资源
        HWND m_hListView;
        WNDPROC m_originalListViewProc;
        
        // 图像资源
        HIMAGELIST m_hImageList;
        
        // 清理函数
        std::function<void()> m_trayCleanupFunc;
        std::function<void()> m_programDetectorCleanupFunc;
        std::function<void()> m_uninstallerCleanupFunc;
        
        // 状态标志
        bool m_timersActive;
        bool m_resourcesActive;
        
        mutable std::mutex m_mutex; ///< 线程安全锁
    };
    
} // namespace YG
