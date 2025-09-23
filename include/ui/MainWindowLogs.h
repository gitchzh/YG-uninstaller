/**
 * @file MainWindowLogs.h
 * @brief 主窗口日志管理功能头文件
 * @author gmrchzh@gmail.com
 * @version 1.0.1
 * @date 2025-09-21
 */

#pragma once

#include "core/Common.h"
#include <windows.h>

namespace YG {

    // 前向声明
    class MainWindow;

    /**
     * @brief 主窗口日志管理功能类
     * 
     * 负责处理所有与日志管理相关的功能，包括：
     * - 日志管理对话框
     * - 日志文件操作（删除、清空）
     * - 日志列表显示和刷新
     */
    class MainWindowLogs {
    public:
        /**
         * @brief 构造函数
         * @param mainWindow 主窗口指针
         */
        explicit MainWindowLogs(MainWindow* mainWindow);
        
        /**
         * @brief 析构函数
         */
        ~MainWindowLogs() = default;
        
        /**
         * @brief 显示日志管理对话框
         */
        void ShowLogManagerDialog();
        
        /**
         * @brief 清理日志文件
         */
        void ClearLogFiles();
        
        /**
         * @brief 检查文件是否被占用
         * @param filePath 文件路径
         * @return bool 是否被占用
         */
        static bool IsFileInUse(const String& filePath);
        
        /**
         * @brief 刷新日志列表
         * @param hDlg 对话框句柄
         */
        static void RefreshLogList(HWND hDlg);
        
        /**
         * @brief 格式化文件大小
         * @param fileSize 文件大小（字节）
         * @return String 格式化后的大小字符串
         */
        static String FormatFileSize(DWORD fileSize);
        
        /**
         * @brief 日志管理对话框过程
         * @param hDlg 对话框句柄
         * @param message 消息类型
         * @param wParam 消息参数1
         * @param lParam 消息参数2
         * @return INT_PTR 消息处理结果
         */
        static INT_PTR CALLBACK LogManagerDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
        
    private:
        MainWindow* m_mainWindow;  ///< 主窗口指针
        
        // 禁用拷贝构造和赋值操作
        MainWindowLogs(const MainWindowLogs&) = delete;
        MainWindowLogs& operator=(const MainWindowLogs&) = delete;
    };

} // namespace YG
