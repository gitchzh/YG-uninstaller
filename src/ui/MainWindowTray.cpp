/**
 * @file MainWindowTray.cpp
 * @brief 主窗口系统托盘功能实现
 * @author gmrchzh@gmail.com
 * @version 1.0.1
 * @date 2025-09-21
 */

#include "ui/MainWindowTray.h"
#include "ui/MainWindow.h"
#include "core/Logger.h"
#include "../resources/resource.h"
#include <windows.h>
#include <shellapi.h>

namespace YG {

    MainWindowTray::MainWindowTray(MainWindow* mainWindow) 
        : m_mainWindow(mainWindow), m_isInTray(false) {
        // 初始化托盘数据结构
        ZeroMemory(&m_nid, sizeof(NOTIFYICONDATAW));
    }

    MainWindowTray::~MainWindowTray() {
        Cleanup();
    }

    void MainWindowTray::CreateSystemTray() {
        YG_LOG_INFO(L"创建系统托盘图标");
        
        HWND hWnd = m_mainWindow->GetWindowHandle();
        HINSTANCE hInstance = GetModuleHandleW(nullptr);
        
        // 设置托盘数据
        m_nid.cbSize = sizeof(NOTIFYICONDATAW);
        m_nid.hWnd = hWnd;
        m_nid.uID = 1;
        m_nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
        m_nid.uCallbackMessage = WM_TRAYICON;
        
        // 设置图标
        m_nid.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_MAIN_ICON));
        if (!m_nid.hIcon) {
            m_nid.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
        }
        
        // 设置提示文本
        wcscpy_s(m_nid.szTip, L"YG Uninstaller - 程序卸载工具");
        
        YG_LOG_INFO(L"系统托盘图标创建完成");
    }

    void MainWindowTray::ShowSystemTray(bool show) {
        if (show) {
            // 确保托盘数据已初始化
            if (m_nid.cbSize == 0 || m_nid.hWnd == nullptr || m_nid.hIcon == nullptr) {
                CreateSystemTray();
            }
            if (!m_isInTray) {
                if (Shell_NotifyIconW(NIM_ADD, &m_nid)) {
                    m_isInTray = true;
                    YG_LOG_INFO(L"系统托盘图标已添加");
                } else {
                    YG_LOG_ERROR(L"添加系统托盘图标失败");
                }
            }
        } else {
            if (m_isInTray) {
                if (Shell_NotifyIconW(NIM_DELETE, &m_nid)) {
                    m_isInTray = false;
                    YG_LOG_INFO(L"系统托盘图标已移除");
                } else {
                    YG_LOG_ERROR(L"移除系统托盘图标失败");
                }
            }
        }
    }

    void MainWindowTray::OnTrayNotify(WPARAM wParam, LPARAM lParam) {
        if (wParam != 1) return; // 确保是我们的托盘图标
        
        HWND hWnd = m_mainWindow->GetWindowHandle();
        
        switch (lParam) {
            case WM_LBUTTONUP:
                // 左键点击 - 恢复窗口
                RestoreFromTray();
                break;
            case WM_LBUTTONDBLCLK:
                // 左键双击 - 恢复窗口
                RestoreFromTray();
                break;
            case WM_RBUTTONUP:
                // 右键点击 - 显示上下文菜单
                {
                    POINT pt;
                    GetCursorPos(&pt);
                    
                    HMENU hTrayMenu = CreatePopupMenu();
                    AppendMenuW(hTrayMenu, MF_STRING, ID_TRAY_RESTORE, L"显示主窗口(&S)");
                    AppendMenuW(hTrayMenu, MF_SEPARATOR, 0, nullptr);
                    AppendMenuW(hTrayMenu, MF_STRING, ID_TRAY_EXIT, L"退出(&X)");
                    
                    // 设置前台窗口以确保菜单正确显示
                    SetForegroundWindow(hWnd);
                    
                    TrackPopupMenu(hTrayMenu, TPM_RIGHTBUTTON, pt.x, pt.y, 0, hWnd, nullptr);
                    
                    // 清理菜单
                    DestroyMenu(hTrayMenu);
                    
                    // 发送一个空消息以确保菜单正确消失
                    PostMessage(hWnd, WM_NULL, 0, 0);
                }
                break;
        }
    }

    void MainWindowTray::MinimizeToTray() {
        // 初始化并显示托盘图标
        if (m_nid.cbSize == 0 || m_nid.hWnd == nullptr || m_nid.hIcon == nullptr) {
            CreateSystemTray();
        }
        ShowSystemTray(true);
        
        if (m_isInTray) {
            HWND hWnd = m_mainWindow->GetWindowHandle();
            ShowWindow(hWnd, SW_HIDE);
            YG_LOG_INFO(L"窗口已最小化到系统托盘");
        }
    }

    void MainWindowTray::RestoreFromTray() {
        HWND hWnd = m_mainWindow->GetWindowHandle();
        ShowWindow(hWnd, SW_RESTORE);
        SetForegroundWindow(hWnd);
        YG_LOG_INFO(L"窗口已从系统托盘恢复");
    }

    void MainWindowTray::Cleanup() {
        if (m_isInTray) {
            YG_LOG_INFO(L"清理系统托盘图标");
            Shell_NotifyIconW(NIM_DELETE, &m_nid);
            m_isInTray = false;
            YG_LOG_INFO(L"系统托盘图标已清理");
        }
    }

} // namespace YG
