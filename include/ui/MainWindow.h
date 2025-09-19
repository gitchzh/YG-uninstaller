/**
 * @file MainWindow.h
 * @brief 主窗口类
 * @author YG Software
 * @version 1.0.0
 * @date 2025-09-17
 */

#pragma once

#include "core/Common.h"
#include "services/ProgramDetector.h"
#include "services/UninstallerService.h"
#include <windows.h>
#include <commctrl.h>
#include <vector>
#include <memory>

namespace YG {
    
    /**
     * @brief 主窗口类
     * 
     * 应用程序的主界面窗口，负责程序列表显示和用户交互
     */
    class MainWindow {
    public:
        /**
         * @brief 构造函数
         */
        MainWindow();
        
        /**
         * @brief 析构函数
         */
        ~MainWindow();
        
        YG_DISABLE_COPY_AND_ASSIGN(MainWindow);
        
        /**
         * @brief 创建主窗口
         * @param hInstance 应用程序实例句柄
         * @return ErrorCode 操作结果
         */
        ErrorCode Create(HINSTANCE hInstance);
        
        /**
         * @brief 显示窗口
         * @param nCmdShow 显示方式
         */
        void Show(int nCmdShow = SW_SHOW);
        
        /**
         * @brief 获取窗口句柄
         * @return HWND 窗口句柄
         */
        HWND GetHandle() const;
        
        /**
         * @brief 运行消息循环
         * @return int 退出代码
         */
        int RunMessageLoop();
        
        /**
         * @brief 刷新程序列表
         * @param includeSystemComponents 是否包含系统组件
         */
        void RefreshProgramList(bool includeSystemComponents = false);
        
        /**
         * @brief 搜索程序
         * @param keyword 搜索关键词
         */
        void SearchPrograms(const String& keyword);
        
        /**
         * @brief 卸载选中的程序
         * @param mode 卸载模式
         */
        void UninstallSelectedProgram(UninstallMode mode = UninstallMode::Standard);
        
        /**
         * @brief 批量卸载选中的程序
         * @param mode 卸载模式
         */
        void BatchUninstallSelectedPrograms(UninstallMode mode = UninstallMode::Standard);
        
        /**
         * @brief 获取所有选中的程序
         * @param programs 输出选中的程序列表
         * @return int 选中程序的数量
         */
        int GetSelectedPrograms(std::vector<ProgramInfo>& programs);
        
        /**
         * @brief 全选/取消全选程序
         * @param selectAll 是否全选
         */
        void SelectAllPrograms(bool selectAll);
        
        /**
         * @brief 显示程序详细信息
         * @param program 程序信息
         */
        void ShowProgramDetails(const ProgramInfo& program);
        
        /**
         * @brief 设置状态栏文本
         * @param text 状态文本
         */
        void SetStatusText(const String& text);
        
        /**
         * @brief 更新进度条
         * @param percentage 进度百分比
         * @param visible 是否显示进度条
         */
        void UpdateProgress(int percentage, bool visible = true);
        
    private:
        /**
         * @brief 窗口过程函数
         * @param hWnd 窗口句柄
         * @param uMsg 消息类型
         * @param wParam 消息参数1
         * @param lParam 消息参数2
         * @return LRESULT 消息处理结果
         */
        static LRESULT CALLBACK WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
        
        /**
         * @brief 实际的窗口过程处理函数
         * @param hWnd 窗口句柄
         * @param uMsg 消息类型
         * @param wParam 消息参数1
         * @param lParam 消息参数2
         * @return LRESULT 消息处理结果
         */
        LRESULT HandleMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
        
        /**
         * @brief 处理创建消息
         * @param hWnd 窗口句柄
         * @param lParam 创建参数
         * @return LRESULT 处理结果
         */
        LRESULT OnCreate(HWND hWnd, LPARAM lParam);
        
        /**
         * @brief 处理销毁消息
         * @return LRESULT 处理结果
         */
        LRESULT OnDestroy();
        
        /**
         * @brief 处理大小改变消息
         * @param width 新宽度
         * @param height 新高度
         * @return LRESULT 处理结果
         */
        LRESULT OnSize(int width, int height);
        
        /**
         * @brief 处理命令消息
         * @param commandId 命令ID
         * @param notificationCode 通知代码
         * @param controlHandle 控件句柄
         * @return LRESULT 处理结果
         */
        LRESULT OnCommand(WORD commandId, WORD notificationCode, HWND controlHandle);
        
        /**
         * @brief 处理通知消息
         * @param pNMHDR 通知消息头
         * @return LRESULT 处理结果
         */
        LRESULT OnNotify(LPNMHDR pNMHDR);
        
        /**
         * @brief 处理按键消息
         * @param key 按键代码
         * @param flags 标志
         * @return LRESULT 处理结果
         */
        LRESULT OnKeyDown(UINT key, UINT flags);
        
        /**
         * @brief 处理右键菜单消息
         * @param x X坐标
         * @param y Y坐标
         * @return LRESULT 处理结果
         */
        LRESULT OnContextMenu(int x, int y);
        
        /**
         * @brief 创建控件
         * @return ErrorCode 操作结果
         */
        ErrorCode CreateControls();
        
        /**
         * @brief 创建菜单
         * @return ErrorCode 操作结果
         */
        ErrorCode CreateMenus();
        
        /**
         * @brief 创建菜单栏搜索控件
         */
        void CreateMenuBarSearchControls();
        
        /**
         * @brief 创建系统托盘图标
         */
        void CreateSystemTray();
        
        /**
         * @brief 显示/隐藏系统托盘图标
         */
        void ShowSystemTray(bool show);
        
        /**
         * @brief 处理系统托盘消息
         */
        void OnTrayNotify(WPARAM wParam, LPARAM lParam);
        
        /**
         * @brief 最小化到托盘
         */
        void MinimizeToTray();
        
        /**
         * @brief 从托盘恢复窗口
         */
        void RestoreFromTray();
        
        /**
         * @brief 显示设置对话框
         */
        void ShowSettingsDialog();
        
        /**
         * @brief 创建工具栏
         * @return ErrorCode 操作结果
         */
        ErrorCode CreateToolbar();
        
        /**
         * @brief 创建状态栏
         * @return ErrorCode 操作结果
         */
        ErrorCode CreateStatusBar();
        
        /**
         * @brief 创建程序列表视图
         * @return ErrorCode 操作结果
         */
        ErrorCode CreateListView();
        
        /**
         * @brief 初始化列表视图列
         */
        void InitializeListViewColumns();
        
        /**
         * @brief 调整列表视图列宽以适应窗口宽度
         */
        void AdjustListViewColumns();
        
        /**
         * @brief 计算5列表格的精确宽度
         * @return int 5列表格的总宽度
         */
        int CalculateTableWidth();
        
        /**
         * @brief 填充程序列表
         * @param programs 程序列表
         */
        void PopulateProgramList(const std::vector<ProgramInfo>& programs);
        
        /**
         * @brief 获取选中的程序
         * @param program 输出程序信息
         * @return bool 是否有选中的程序
         */
        bool GetSelectedProgram(ProgramInfo& program);
        
        /**
         * @brief 更新UI状态
         */
        void UpdateUIState();
        
        /**
         * @brief 调整控件布局
         */
        void AdjustLayout();
        
        /**
         * @brief 加载窗口设置
         */
        void LoadWindowSettings();
        
        /**
         * @brief 保存窗口设置
         */
        void SaveWindowSettings();
        
        /**
         * @brief 扫描进度回调
         * @param percentage 进度百分比
         * @param currentItem 当前扫描项
         */
        void OnScanProgress(int percentage, const String& currentItem);
        
        /**
         * @brief 扫描完成回调
         * @param programs 程序列表
         * @param result 扫描结果
         */
        void OnScanCompleted(const std::vector<ProgramInfo>& programs, ErrorCode result);
        
        /**
         * @brief 卸载进度回调
         * @param percentage 进度百分比
         * @param message 进度消息
         */
        void OnUninstallProgress(int percentage, const String& message);
        
        /**
         * @brief 卸载完成回调
         * @param result 卸载结果
         */
        void OnUninstallCompleted(const UninstallResult& result);
        
        /**
         * @brief 显示错误对话框
         * @param title 标题
         * @param message 消息
         * @param errorCode 错误代码
         */
        void ShowError(const String& title, const String& message, ErrorCode errorCode = ErrorCode::GeneralError);
        
        /**
         * @brief 显示确认对话框
         * @param title 标题
         * @param message 消息
         * @return bool 用户是否确认
         */
        bool ShowConfirmation(const String& title, const String& message);
        
        /**
         * @brief 格式化文件大小
         * @param sizeInBytes 字节大小
         * @return String 格式化后的大小字符串
         */
        String FormatFileSize(DWORD64 sizeInBytes);
        
        /**
         * @brief 格式化安装日期
         * @param dateString 日期字符串 (YYYY-MM-DD 格式)
         * @return String 格式化后的日期字符串 (M/d/yyyy 格式)
         */
        String FormatInstallDate(const String& dateString);
        
        /**
         * @brief 添加测试程序数据
         */
        void AddTestPrograms();
        
        /**
         * @brief 添加示例程序数据
         */
        void AddSamplePrograms();
        
        /**
         * @brief 排序程序列表
         * @param column 排序列索引
         * @param ascending 是否升序
         */
        void SortProgramList(int column, bool ascending);
        
        /**
         * @brief 处理列标题点击事件
         * @param column 点击的列索引
         */
        void OnColumnHeaderClick(int column);
        
        /**
         * @brief 比较两个程序信息
         * @param program1 程序1
         * @param program2 程序2
         * @param column 比较列
         * @param ascending 是否升序
         * @return int 比较结果
         */
        static int CompareProgramInfo(const ProgramInfo& program1, const ProgramInfo& program2, int column, bool ascending);
        
        /**
         * @brief 创建图标列表
         * @return ErrorCode 操作结果
         */
        ErrorCode CreateImageList();
        
        /**
         * @brief 提取程序图标
         * @param program 程序信息
         * @return int 图标在ImageList中的索引，-1表示失败
         */
        int ExtractProgramIcon(const ProgramInfo& program);
        
        /**
         * @brief 获取默认程序图标
         * @return int 默认图标在ImageList中的索引
         */
        int GetDefaultProgramIcon();
        
        /**
         * @brief 去除程序列表中的重复项
         * @param programs 程序列表
         * @return std::vector<ProgramInfo> 去重后的程序列表
         */
        std::vector<ProgramInfo> RemoveDuplicatePrograms(const std::vector<ProgramInfo>& programs);
        
        /**
         * @brief 判断两个程序是否相同
         * @param program1 程序1
         * @param program2 程序2
         * @return bool 是否为同一程序
         */
        bool IsSameProgram(const ProgramInfo& program1, const ProgramInfo& program2);
        
    private:
        // 窗口相关
        HWND m_hWnd;                    ///< 主窗口句柄
        HINSTANCE m_hInstance;          ///< 应用程序实例
        HMENU m_hMenu;                  ///< 主菜单句柄
        HMENU m_hContextMenu;           ///< 右键菜单句柄
        
        // 控件句柄
        HWND m_hToolbar;                ///< 工具栏句柄
        HWND m_hStatusBar;              ///< 状态栏句柄
        HWND m_hListView;               ///< 列表视图句柄
        
        HWND m_hSearchEdit;             ///< 搜索框句柄
        HWND m_hProgressBar;            ///< 进度条句柄
        
        // 图标相关
        HIMAGELIST m_hImageList;        ///< 图标列表句柄
        
        // 系统托盘相关
        NOTIFYICONDATAW m_nid;          ///< 系统托盘图标数据
        bool m_isInTray;                ///< 是否在托盘中
        
        // 服务对象
        std::unique_ptr<ProgramDetector> m_programDetector;  ///< 程序检测器
        std::unique_ptr<UninstallerService> m_uninstallerService; ///< 卸载服务
        
        // 数据
        std::vector<ProgramInfo> m_programs;        ///< 程序列表
        std::vector<ProgramInfo> m_filteredPrograms; ///< 过滤后的程序列表
        String m_currentSearchKeyword;              ///< 当前搜索关键词
        bool m_includeSystemComponents;             ///< 是否包含系统组件
        
        // 状态
        bool m_isScanning;                          ///< 是否正在扫描
        bool m_isUninstalling;                      ///< 是否正在卸载
        bool m_isListViewMode;                      ///< 是否使用ListView表格模式
        String m_currentUninstallTask;              ///< 当前卸载任务ID
        
        // 排序相关
        int m_sortColumn;                           ///< 当前排序列
        bool m_sortAscending;                       ///< 是否升序排序
        
        // 窗口类名和标题
        static constexpr const wchar_t* WINDOW_CLASS_NAME = L"YGUninstallerMainWindow";
        static constexpr const wchar_t* WINDOW_TITLE = L"YG Uninstaller";
        
        // 控件ID
        enum ControlId {
            ID_TOOLBAR = 1000,
            ID_STATUSBAR,
            ID_LISTVIEW,
            ID_SEARCH_EDIT,
            ID_PROGRESS_BAR,
            
            // 文件菜单ID
            ID_FILE_EXIT = 2000,
            
            // 操作菜单ID
            ID_ACTION_REFRESH = 2100,
            ID_ACTION_UNINSTALL,
            ID_ACTION_FORCE_UNINSTALL,
            ID_ACTION_PROPERTIES,
            ID_ACTION_OPEN_LOCATION,
            
            // 查看菜单ID
            ID_VIEW_TOOLBAR = 2200,
            ID_VIEW_STATUSBAR,
            ID_VIEW_REFRESH,
            ID_VIEW_LARGE_ICONS,
            ID_VIEW_SMALL_ICONS,
            ID_VIEW_LIST,
            ID_VIEW_DETAILS,
            ID_VIEW_SHOW_SYSTEM,
            ID_VIEW_SHOW_UPDATES,
            
            // 帮助菜单ID
            ID_HELP_HELP = 2300,
            ID_HELP_WEBSITE,
            ID_HELP_CHECK_UPDATE,
            ID_HELP_ABOUT,
            
            // 兼容旧的ID
            ID_EDIT_SEARCH,
            ID_EDIT_REFRESH,
            ID_EDIT_SELECT_ALL,
            ID_VIEW_SYSTEM_COMPONENTS,
            ID_TOOLS_UNINSTALL,
            ID_TOOLS_FORCE_UNINSTALL,
            ID_TOOLS_DEEP_CLEAN,
            ID_TOOLS_OPTIONS,
            
            // 工具栏按钮ID
            ID_TB_REFRESH = 3000,
            ID_TB_UNINSTALL,
            ID_TB_FORCE_UNINSTALL,
            ID_TB_SEARCH,
            ID_TB_OPTIONS,
            
            // 右键菜单ID
            ID_CM_UNINSTALL = 4000,
            ID_CM_FORCE_UNINSTALL,
            ID_CM_PROPERTIES,
            ID_CM_OPEN_LOCATION
        };
    };
    
} // namespace YG
