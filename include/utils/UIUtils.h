/**
 * @file UIUtils.h
 * @brief UI工具类
 * @author YG Software
 * @version 1.0.1
 * @date 2025-01-21
 */

#pragma once

#include "core/Common.h"
#include <windows.h>
#include <commctrl.h>

namespace YG {
    
    /**
     * @brief UI工具类
     * 
     * 提供常用的UI操作功能
     */
    class UIUtils {
    public:
        /**
         * @brief 居中显示窗口
         * @param hWnd 窗口句柄
         * @param hParent 父窗口句柄，nullptr表示相对于屏幕居中
         */
        static void CenterWindow(HWND hWnd, HWND hParent = nullptr);
        
        /**
         * @brief 设置窗口图标
         * @param hWnd 窗口句柄
         * @param iconId 图标资源ID
         * @param hInstance 实例句柄
         * @return bool 是否成功
         */
        static bool SetWindowIcon(HWND hWnd, int iconId, HINSTANCE hInstance);
        
        /**
         * @brief 创建工具提示
         * @param hParent 父窗口句柄
         * @param hControl 控件句柄
         * @param text 提示文本
         * @return HWND 工具提示句柄
         */
        static HWND CreateToolTip(HWND hParent, HWND hControl, const String& text);
        
        /**
         * @brief 设置ListView列
         * @param hListView ListView句柄
         * @param columnIndex 列索引
         * @param text 列标题
         * @param width 列宽度
         * @param format 对齐方式
         * @return bool 是否成功
         */
        static bool SetListViewColumn(HWND hListView, int columnIndex, const String& text, 
                                    int width, int format = LVCFMT_LEFT);
        
        /**
         * @brief 添加ListView项
         * @param hListView ListView句柄
         * @param itemIndex 项索引
         * @param subItemIndex 子项索引
         * @param text 文本
         * @param imageIndex 图像索引
         * @return bool 是否成功
         */
        static bool SetListViewItem(HWND hListView, int itemIndex, int subItemIndex, 
                                  const String& text, int imageIndex = -1);
        
        /**
         * @brief 获取ListView选中项
         * @param hListView ListView句柄
         * @return std::vector<int> 选中项索引列表
         */
        static std::vector<int> GetListViewSelectedItems(HWND hListView);
        
        /**
         * @brief 设置状态栏文本
         * @param hStatusBar 状态栏句柄
         * @param partIndex 部分索引
         * @param text 文本
         * @return bool 是否成功
         */
        static bool SetStatusBarText(HWND hStatusBar, int partIndex, const String& text);
        
        /**
         * @brief 设置进度条值
         * @param hProgressBar 进度条句柄
         * @param value 值
         * @param min 最小值
         * @param max 最大值
         * @return bool 是否成功
         */
        static bool SetProgressBarValue(HWND hProgressBar, int value, int min = 0, int max = 100);
        
        /**
         * @brief 显示消息框
         * @param hParent 父窗口句柄
         * @param title 标题
         * @param message 消息
         * @param type 类型
         * @return int 用户选择结果
         */
        static int ShowMessageBox(HWND hParent, const String& title, const String& message, UINT type);
        
        /**
         * @brief 显示确认对话框
         * @param hParent 父窗口句柄
         * @param title 标题
         * @param message 消息
         * @return bool 用户是否确认
         */
        static bool ShowConfirmDialog(HWND hParent, const String& title, const String& message);
        
        /**
         * @brief 显示错误对话框
         * @param hParent 父窗口句柄
         * @param title 标题
         * @param message 消息
         */
        static void ShowErrorDialog(HWND hParent, const String& title, const String& message);
        
        /**
         * @brief 显示信息对话框
         * @param hParent 父窗口句柄
         * @param title 标题
         * @param message 消息
         */
        static void ShowInfoDialog(HWND hParent, const String& title, const String& message);
        
        /**
         * @brief 获取控件文本
         * @param hControl 控件句柄
         * @return String 控件文本
         */
        static String GetControlText(HWND hControl);
        
        /**
         * @brief 设置控件文本
         * @param hControl 控件句柄
         * @param text 文本
         * @return bool 是否成功
         */
        static bool SetControlText(HWND hControl, const String& text);
        
        /**
         * @brief 启用/禁用控件
         * @param hControl 控件句柄
         * @param enable 是否启用
         * @return bool 是否成功
         */
        static bool EnableControl(HWND hControl, bool enable);
        
        /**
         * @brief 显示/隐藏控件
         * @param hControl 控件句柄
         * @param show 是否显示
         * @return bool 是否成功
         */
        static bool ShowControl(HWND hControl, bool show);
        
        /**
         * @brief 获取窗口矩形
         * @param hWnd 窗口句柄
         * @param isClientArea 是否为客户区
         * @return RECT 窗口矩形
         */
        static RECT GetWindowRect(HWND hWnd, bool isClientArea = false);
        
        /**
         * @brief 调整窗口大小
         * @param hWnd 窗口句柄
         * @param width 宽度
         * @param height 高度
         * @param repaint 是否重绘
         * @return bool 是否成功
         */
        static bool ResizeWindow(HWND hWnd, int width, int height, bool repaint = true);
        
        /**
         * @brief 移动窗口
         * @param hWnd 窗口句柄
         * @param x X坐标
         * @param y Y坐标
         * @param repaint 是否重绘
         * @return bool 是否成功
         */
        static bool MoveWindow(HWND hWnd, int x, int y, bool repaint = true);
        
        /**
         * @brief 获取DPI缩放比例
         * @param hWnd 窗口句柄
         * @return double DPI缩放比例
         */
        static double GetDPIScale(HWND hWnd);
        
        /**
         * @brief 根据DPI调整大小
         * @param size 原始大小
         * @param hWnd 窗口句柄
         * @return int 调整后的大小
         */
        static int ScaleForDPI(int size, HWND hWnd);
        
        /**
         * @brief 加载并缩放图标
         * @param hInstance 实例句柄
         * @param iconId 图标ID
         * @param width 宽度
         * @param height 高度
         * @return HICON 图标句柄
         */
        static HICON LoadScaledIcon(HINSTANCE hInstance, int iconId, int width, int height);
        
        /**
         * @brief 安全销毁窗口
         * @param hWnd 窗口句柄
         * @return bool 是否成功
         */
        static bool SafeDestroyWindow(HWND& hWnd);
        
        /**
         * @brief 安全销毁菜单
         * @param hMenu 菜单句柄
         * @return bool 是否成功
         */
        static bool SafeDestroyMenu(HMENU& hMenu);
        
        /**
         * @brief 安全销毁图像列表
         * @param hImageList 图像列表句柄
         * @return bool 是否成功
         */
        static bool SafeDestroyImageList(HIMAGELIST& hImageList);
        
    private:
        /**
         * @brief 获取系统DPI
         * @return int DPI值
         */
        static int GetSystemDPI();
    };
    
} // namespace YG
