/**
 * @file ResourceManager.cpp
 * @brief 资源管理器实现
 * @author YG Software
 * @version 1.0.1
 * @date 2025-01-21
 */

#include "ui/ResourceManager.h"
#include "core/Logger.h"
#include <algorithm>

namespace YG {
    
    ResourceManager::ResourceManager() 
        : m_hWnd(nullptr), m_hMenu(nullptr), m_hContextMenu(nullptr),
          m_hListView(nullptr), m_originalListViewProc(nullptr),
          m_hImageList(nullptr), m_timersActive(false), m_resourcesActive(true) {
        YG_LOG_INFO(L"资源管理器已创建");
    }
    
    ResourceManager::~ResourceManager() {
        YG_LOG_INFO(L"开始资源管理器析构");
        
        try {
            ForceCleanup();
        } catch (...) {
            // 析构函数中不应该抛出异常
            YG_LOG_ERROR(L"资源管理器析构过程中发生异常");
        }
        
        YG_LOG_INFO(L"资源管理器析构完成");
    }
    
    void ResourceManager::SetMainWindow(HWND hWnd) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_hWnd = hWnd;
        m_timersActive = (hWnd != nullptr);
        YG_LOG_INFO(L"设置主窗口句柄: " + std::to_wstring(reinterpret_cast<uintptr_t>(hWnd)));
    }
    
    void ResourceManager::SetMenus(HMENU hMenu, HMENU hContextMenu) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_hMenu = hMenu;
        m_hContextMenu = hContextMenu;
        YG_LOG_INFO(L"设置菜单资源");
    }
    
    void ResourceManager::SetControls(HWND hListView, WNDPROC originalProc) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_hListView = hListView;
        m_originalListViewProc = originalProc;
        YG_LOG_INFO(L"设置控件资源");
    }
    
    void ResourceManager::SetImageList(HIMAGELIST hImageList) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_hImageList = hImageList;
        YG_LOG_INFO(L"设置图像列表资源");
    }
    
    void ResourceManager::StopTimers() {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_hWnd && m_timersActive) {
            CleanupTimers();
        }
    }
    
    void ResourceManager::SetTrayCleanup(std::function<void()> cleanupFunc) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_trayCleanupFunc = cleanupFunc;
    }
    
    void ResourceManager::SetServiceCleanup(std::function<void()> programDetectorCleanup,
                                           std::function<void()> uninstallerCleanup) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_programDetectorCleanupFunc = programDetectorCleanup;
        m_uninstallerCleanupFunc = uninstallerCleanup;
    }
    
    void ResourceManager::ForceCleanup() {
        std::lock_guard<std::mutex> lock(m_mutex);
        
        if (!m_resourcesActive) {
            YG_LOG_INFO(L"资源已经清理过，跳过重复清理");
            return;
        }
        
        YG_LOG_INFO(L"开始强制清理所有资源");
        
        // 按照安全的顺序清理资源
        try {
            CleanupTimers();
        } catch (...) {
            YG_LOG_ERROR(L"清理定时器时发生异常");
        }
        
        try {
            CleanupServices();
        } catch (...) {
            YG_LOG_ERROR(L"清理服务时发生异常");
        }
        
        try {
            CleanupTray();
        } catch (...) {
            YG_LOG_ERROR(L"清理系统托盘时发生异常");
        }
        
        try {
            CleanupControls();
        } catch (...) {
            YG_LOG_ERROR(L"清理控件时发生异常");
        }
        
        try {
            CleanupImages();
        } catch (...) {
            YG_LOG_ERROR(L"清理图像时发生异常");
        }
        
        try {
            CleanupMenus();
        } catch (...) {
            YG_LOG_ERROR(L"清理菜单时发生异常");
        }
        
        try {
            CleanupWindow();
        } catch (...) {
            YG_LOG_ERROR(L"清理窗口时发生异常");
        }
        
        m_resourcesActive = false;
        YG_LOG_INFO(L"所有资源清理完成");
    }
    
    String ResourceManager::GetResourceStatus() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        
        String status = L"资源管理器状态:\n";
        status += L"  活动状态: " + String(m_resourcesActive ? L"是" : L"否") + L"\n";
        status += L"  主窗口: " + String(m_hWnd ? L"有效" : L"无效") + L"\n";
        status += L"  主菜单: " + String(m_hMenu ? L"有效" : L"无效") + L"\n";
        status += L"  上下文菜单: " + String(m_hContextMenu ? L"有效" : L"无效") + L"\n";
        status += L"  ListView: " + String(m_hListView ? L"有效" : L"无效") + L"\n";
        status += L"  图像列表: " + String(m_hImageList ? L"有效" : L"无效") + L"\n";
        status += L"  定时器: " + String(m_timersActive ? L"活动" : L"停止") + L"\n";
        
        return status;
    }
    
    void ResourceManager::CleanupWindow() {
        if (m_hWnd) {
            YG_LOG_INFO(L"清理主窗口");
            if (IsWindow(m_hWnd)) {
                DestroyWindow(m_hWnd);
            }
            m_hWnd = nullptr;
        }
    }
    
    void ResourceManager::CleanupMenus() {
        if (m_hMenu) {
            YG_LOG_INFO(L"清理主菜单");
            DestroyMenu(m_hMenu);
            m_hMenu = nullptr;
        }
        
        if (m_hContextMenu) {
            YG_LOG_INFO(L"清理上下文菜单");
            DestroyMenu(m_hContextMenu);
            m_hContextMenu = nullptr;
        }
    }
    
    void ResourceManager::CleanupControls() {
        if (m_hListView && m_originalListViewProc) {
            YG_LOG_INFO(L"恢复ListView窗口过程");
            if (IsWindow(m_hListView)) {
                SetWindowLongPtr(m_hListView, GWLP_WNDPROC, 
                               reinterpret_cast<LONG_PTR>(m_originalListViewProc));
            }
            m_originalListViewProc = nullptr;
        }
        m_hListView = nullptr;
    }
    
    void ResourceManager::CleanupImages() {
        if (m_hImageList) {
            YG_LOG_INFO(L"清理图像列表");
            ImageList_Destroy(m_hImageList);
            m_hImageList = nullptr;
        }
    }
    
    void ResourceManager::CleanupTimers() {
        if (m_hWnd && m_timersActive) {
            YG_LOG_INFO(L"停止所有定时器");
            KillTimer(m_hWnd, 1);     // 扫描定时器
            KillTimer(m_hWnd, 2);     // 滚动条监控定时器
            KillTimer(m_hWnd, 9999);  // 强制退出定时器
            m_timersActive = false;
        }
    }
    
    void ResourceManager::CleanupServices() {
        if (m_programDetectorCleanupFunc) {
            YG_LOG_INFO(L"清理程序检测器");
            try {
                m_programDetectorCleanupFunc();
            } catch (...) {
                YG_LOG_ERROR(L"程序检测器清理函数异常");
            }
            m_programDetectorCleanupFunc = nullptr;
        }
        
        if (m_uninstallerCleanupFunc) {
            YG_LOG_INFO(L"清理卸载服务");
            try {
                m_uninstallerCleanupFunc();
            } catch (...) {
                YG_LOG_ERROR(L"卸载服务清理函数异常");
            }
            m_uninstallerCleanupFunc = nullptr;
        }
    }
    
    void ResourceManager::CleanupTray() {
        if (m_trayCleanupFunc) {
            YG_LOG_INFO(L"清理系统托盘");
            try {
                m_trayCleanupFunc();
            } catch (...) {
                YG_LOG_ERROR(L"系统托盘清理函数异常");
            }
            m_trayCleanupFunc = nullptr;
        }
    }
    
} // namespace YG
