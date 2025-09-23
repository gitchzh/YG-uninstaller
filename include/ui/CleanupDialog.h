/**
 * @file CleanupDialog.h
 * @brief 程序残留清理对话框
 * @author YG Software
 * @version 1.0.1
 * @date 2025-09-22
 */

#pragma once

#include "core/Common.h"
#include "core/ResidualItem.h"
#include <vector>
#include <memory>
#include <commctrl.h>

namespace YG {
    
    // 前向声明
    class ResidualScanner;
    
    /**
     * @brief 清理对话框结果枚举
     */
    enum class CleanupResult {
        None,           ///< 无操作
        Cancelled,      ///< 用户取消
        DeleteSelected, ///< 删除选中项
        DeleteAll,      ///< 删除全部
        Completed       ///< 清理完成
    };
    
    /**
     * @brief 程序残留清理对话框类
     */
    class CleanupDialog {
    private:
        HWND m_hDialog;                     ///< 对话框句柄
        HWND m_hParent;                     ///< 父窗口句柄
        HWND m_hTreeView;                   ///< 树形控件句柄
        HWND m_hListView;                   ///< 列表控件句柄
        HWND m_hProgressBar;                ///< 进度条句柄
        HWND m_hStatusLabel;                ///< 状态标签句柄
        
        // 按钮控件
        HWND m_hSelectAllButton;            ///< 全选按钮
        HWND m_hSelectNoneButton;           ///< 全不选按钮
        HWND m_hDeleteSelectedButton;       ///< 删除选中按钮
        HWND m_hDeleteAllButton;            ///< 删除全部按钮
        HWND m_hCancelButton;               ///< 取消按钮
        
        std::vector<ResidualGroup> m_residualGroups;    ///< 残留项分组
        std::shared_ptr<ResidualScanner> m_scanner;     ///< 扫描器
        ProgramInfo m_programInfo;                      ///< 程序信息
        CleanupResult m_result;                         ///< 对话框结果
        bool m_isDeleting;                              ///< 是否正在删除
        bool m_dialogClosed;                            ///< 对话框是否已关闭
        
        // 界面常量 - 与日志管理窗口保持一致
        static constexpr int DIALOG_WIDTH = 333;
        static constexpr int DIALOG_HEIGHT = 250;
        static constexpr int MARGIN = 14;
        static constexpr int BUTTON_HEIGHT = 16;
        static constexpr int BUTTON_WIDTH = 60;
        
        // 现代化配色
        static constexpr COLORREF BG_COLOR = RGB(248, 249, 250);
        static constexpr COLORREF TEXT_COLOR = RGB(33, 37, 41);
        static constexpr COLORREF BUTTON_COLOR = RGB(13, 110, 253);
        static constexpr COLORREF DANGER_COLOR = RGB(220, 53, 69);
        static constexpr COLORREF SUCCESS_COLOR = RGB(25, 135, 84);
        
    public:
        /**
         * @brief 构造函数
         * @param hParent 父窗口句柄
         * @param programInfo 程序信息
         * @param scanner 残留扫描器
         */
        CleanupDialog(HWND hParent, const ProgramInfo& programInfo, 
                     std::shared_ptr<ResidualScanner> scanner);
        
        /**
         * @brief 析构函数
         */
        ~CleanupDialog();
        
        /**
         * @brief 显示清理对话框
         * @return CleanupResult 用户操作结果
         */
        CleanupResult ShowDialog();
        
        /**
         * @brief 设置残留项数据
         * @param groups 残留项分组
         */
        void SetResidualData(const std::vector<ResidualGroup>& groups);
        
    private:
        /**
         * @brief 创建对话框控件
         */
        void CreateControls();
        
        /**
         * @brief 创建信息标签
         */
        void CreateInfoLabel();
        
        /**
         * @brief 创建主列表控件
         */
        void CreateMainListView();
        
        /**
         * @brief 创建按钮控件
         */
        void CreateButtons();
        
        /**
         * @brief 创建状态控件
         */
        void CreateStatusControls();
        
        /**
         * @brief 创建树形控件 (旧版本)
         */
        void CreateTreeView();
        
        /**
         * @brief 创建列表控件 (旧版本)
         */
        void CreateListView();
        
        /**
         * @brief 填充树形控件数据
         */
        void PopulateTreeView();
        
        /**
         * @brief 填充列表控件数据
         * @param groupIndex 分组索引
         */
        void PopulateListView(int groupIndex = -1);
        
        /**
         * @brief 更新选择统计
         */
        void UpdateSelectionStats();
        
        /**
         * @brief 处理树形控件选择变化
         * @param hItem 选中项句柄
         */
        void OnTreeSelectionChanged(HTREEITEM hItem);
        
        /**
         * @brief 处理列表项选择变化
         * @param itemIndex 项索引
         * @param selected 是否选中
         */
        void OnListItemSelectionChanged(int itemIndex, bool selected);
        
        /**
         * @brief 全选操作
         */
        void SelectAll();
        
        /**
         * @brief 全不选操作
         */
        void SelectNone();
        
        /**
         * @brief 删除选中项
         */
        void DeleteSelected();
        
        /**
         * @brief 删除全部项
         */
        void DeleteAll();
        
        /**
         * @brief 执行删除操作
         * @param items 要删除的项
         */
        void PerformDelete(const std::vector<ResidualItem>& items);
        
        /**
         * @brief 删除进度回调
         * @param percentage 进度百分比
         * @param currentItem 当前删除项
         * @param success 是否成功
         */
        void OnDeleteProgress(int percentage, const String& currentItem, bool success);
        
        /**
         * @brief 居中显示对话框
         */
        void CenterWindow();
        
        /**
         * @brief 调整控件布局
         */
        void AdjustLayout();
        
        /**
         * @brief 获取风险级别颜色
         * @param riskLevel 风险级别
         * @return COLORREF 对应颜色
         */
        COLORREF GetRiskLevelColor(RiskLevel riskLevel);
        
        /**
         * @brief 获取风险级别描述
         * @param riskLevel 风险级别
         * @return String 风险描述
         */
        String GetRiskLevelDescription(RiskLevel riskLevel);
        
        /**
         * @brief 获取残留类型文本
         * @param type 残留类型
         * @return String 类型文本
         */
        String GetResidualTypeText(ResidualType type);
        
        /**
         * @brief 获取残留类型图标
         * @param type 残留类型
         * @return String 图标字符
         */
        String GetTypeIcon(ResidualType type);
        
        // 静态窗口过程
        static LRESULT CALLBACK DialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
        
        /**
         * @brief 处理窗口消息
         * @param message 消息类型
         * @param wParam 消息参数
         * @param lParam 消息参数
         * @return LRESULT 消息处理结果
         */
        LRESULT HandleMessage(UINT message, WPARAM wParam, LPARAM lParam);
    };
    
} // namespace YG
