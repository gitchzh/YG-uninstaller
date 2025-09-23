/**
 * @file CleanupDialog.cpp
 * @brief 程序残留清理对话框实现
 * @author YG Software
 * @version 1.0.1
 * @date 2025-09-22
 */

#include "ui/CleanupDialog.h"
#include "services/ResidualScanner.h"
#include "core/Logger.h"
#include "core/ErrorHandler.h"
#include "utils/StringUtils.h"
#include "utils/UIUtils.h"
#include <commctrl.h>
#include <shellapi.h>

namespace YG {
    
    CleanupDialog::CleanupDialog(HWND hParent, const ProgramInfo& programInfo, 
                               std::shared_ptr<ResidualScanner> scanner)
        : m_hDialog(nullptr), m_hParent(hParent), m_hTreeView(nullptr), m_hListView(nullptr),
          m_hProgressBar(nullptr), m_hStatusLabel(nullptr),
          m_hSelectAllButton(nullptr), m_hSelectNoneButton(nullptr),
          m_hDeleteSelectedButton(nullptr), m_hDeleteAllButton(nullptr), m_hCancelButton(nullptr),
          m_scanner(scanner), m_programInfo(programInfo), m_result(CleanupResult::None),
          m_isDeleting(false), m_dialogClosed(false) {
        
        YG_LOG_INFO(L"清理对话框已创建: " + programInfo.name);
    }
    
    CleanupDialog::~CleanupDialog() {
        YG_LOG_INFO(L"清理对话框已销毁");
    }
    
    CleanupResult CleanupDialog::ShowDialog() {
        YG_LOG_INFO(L"显示清理对话框");
        
        // 注册窗口类
        static bool classRegistered = false;
        if (!classRegistered) {
            WNDCLASSEX wc = {};
            wc.cbSize = sizeof(WNDCLASSEX);
            wc.style = CS_HREDRAW | CS_VREDRAW;
            wc.lpfnWndProc = DialogProc;
            wc.hInstance = GetModuleHandle(nullptr);
            wc.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
            wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
            wc.hbrBackground = CreateSolidBrush(BG_COLOR);
            wc.lpszClassName = L"YGCleanupDialog";
            
            RegisterClassEx(&wc);
            classRegistered = true;
        }
        
        // 创建窗口标题
        String windowTitle = L"程序残留清理 - " + 
                           (!m_programInfo.displayName.empty() ? m_programInfo.displayName : m_programInfo.name);
        
        // 创建对话框
        m_hDialog = CreateWindowEx(
            WS_EX_DLGMODALFRAME,
            L"YGCleanupDialog",
            windowTitle.c_str(),
            WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
            CW_USEDEFAULT, CW_USEDEFAULT,
            DIALOG_WIDTH, DIALOG_HEIGHT,
            m_hParent,
            nullptr,
            GetModuleHandle(nullptr),
            this
        );
        
        if (!m_hDialog) {
            YG_LOG_ERROR(L"清理对话框创建失败");
            return CleanupResult::None;
        }
        
        // 居中显示
        CenterWindow();
        
        // 创建控件
        CreateControls();
        
        // 显示对话框
        ShowWindow(m_hDialog, SW_SHOW);
        UpdateWindow(m_hDialog);
        
        // 设置为模态
        EnableWindow(m_hParent, FALSE);
        
        // 消息循环
        MSG msg;
        while (!m_dialogClosed && IsWindow(m_hDialog)) {
            BOOL bRet = GetMessage(&msg, nullptr, 0, 0);
            
            if (bRet == 0) {
                // WM_QUIT消息
                break;
            } else if (bRet == -1) {
                // 错误
                YG_LOG_ERROR(L"GetMessage发生错误");
                break;
            } else {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        }
        
        // 恢复父窗口
        if (IsWindow(m_hParent)) {
            EnableWindow(m_hParent, TRUE);
            SetForegroundWindow(m_hParent);
        }
        
        return m_result;
    }
    
    void CleanupDialog::SetResidualData(const std::vector<ResidualGroup>& groups) {
        m_residualGroups = groups;
        
        // 如果对话框已创建，更新显示
        if (m_hDialog && IsWindow(m_hDialog)) {
            PopulateListView(0); // 显示所有项目
            UpdateSelectionStats();
        }
    }
    
    void CleanupDialog::CreateControls() {
        // 创建信息标签（顶部）
        CreateInfoLabel();
        
        // 创建主列表控件（显示所有残留项）
        CreateMainListView();
        
        // 创建操作按钮组
        CreateButtons();
        
        // 创建状态标签和进度条
        CreateStatusControls();
        
        YG_LOG_INFO(L"清理对话框控件创建完成");
    }
    
    void CleanupDialog::CreateInfoLabel() {
        // 创建信息标签，类似日志管理窗口的顶部标签
        String infoText = L"程序残留清理 - " + 
                         (!m_programInfo.displayName.empty() ? m_programInfo.displayName : m_programInfo.name);
        
        HWND hInfoLabel = CreateWindow(L"STATIC", infoText.c_str(),
                                     WS_CHILD | WS_VISIBLE | SS_LEFT,
                                     MARGIN, MARGIN, DIALOG_WIDTH - 2 * MARGIN, 12,
                                     m_hDialog, nullptr, GetModuleHandle(nullptr), nullptr);
        
        if (hInfoLabel) {
            YG_LOG_INFO(L"信息标签创建成功");
        }
    }
    
    void CleanupDialog::CreateMainListView() {
        // 创建主列表控件，类似日志管理窗口的列表
        m_hListView = CreateWindowEx(
            WS_EX_CLIENTEDGE,
            WC_LISTVIEWW,
            nullptr,
            WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SHOWSELALWAYS | LVS_SINGLESEL,
            MARGIN, MARGIN + 20,
            DIALOG_WIDTH - 2 * MARGIN, 110, // 主要显示区域
            m_hDialog,
            (HMENU)2002,
            GetModuleHandle(nullptr),
            nullptr
        );
        
        if (m_hListView) {
            // 设置ListView扩展样式，与日志管理窗口一致
            ListView_SetExtendedListViewStyle(m_hListView, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_CHECKBOXES);
            
            // 添加列
            LVCOLUMNW lvc = {0};
            lvc.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
            
            // 类型列
            lvc.cx = 60;
            lvc.pszText = const_cast<LPWSTR>(L"类型");
            ListView_InsertColumn(m_hListView, 0, &lvc);
            
            // 路径列
            lvc.cx = 180;
            lvc.pszText = const_cast<LPWSTR>(L"路径");
            ListView_InsertColumn(m_hListView, 1, &lvc);
            
            // 大小列
            lvc.cx = 60;
            lvc.pszText = const_cast<LPWSTR>(L"大小");
            ListView_InsertColumn(m_hListView, 2, &lvc);
            
            YG_LOG_INFO(L"主列表控件创建成功");
        } else {
            YG_LOG_ERROR(L"主列表控件创建失败");
        }
    }
    
    void CleanupDialog::CreateStatusControls() {
        // 创建状态标签
        m_hStatusLabel = CreateWindow(L"STATIC", L"准备清理...",
                                    WS_CHILD | WS_VISIBLE | SS_LEFT,
                                    MARGIN, DIALOG_HEIGHT - 55, DIALOG_WIDTH - 2 * MARGIN, 12,
                                    m_hDialog, nullptr, GetModuleHandle(nullptr), nullptr);
        
        // 创建进度条（较小）
        m_hProgressBar = CreateWindow(PROGRESS_CLASSW, nullptr,
                                    WS_CHILD | WS_VISIBLE,
                                    MARGIN, DIALOG_HEIGHT - 40, DIALOG_WIDTH - 2 * MARGIN, 12,
                                    m_hDialog, nullptr, GetModuleHandle(nullptr), nullptr);
    }
    
    void CleanupDialog::CreateTreeView() {
        m_hTreeView = CreateWindowEx(
            WS_EX_CLIENTEDGE,
            WC_TREEVIEWW,
            nullptr,
            WS_CHILD | WS_VISIBLE | TVS_HASLINES | TVS_HASBUTTONS | TVS_LINESATROOT | TVS_CHECKBOXES,
            MARGIN, MARGIN + 30,
            250, DIALOG_HEIGHT - 150,
            m_hDialog,
            (HMENU)2001,
            GetModuleHandle(nullptr),
            nullptr
        );
        
        if (m_hTreeView) {
            YG_LOG_INFO(L"树形控件创建成功");
        }
    }
    
    void CleanupDialog::CreateListView() {
        m_hListView = CreateWindowEx(
            WS_EX_CLIENTEDGE,
            WC_LISTVIEWW,
            nullptr,
            WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SHOWSELALWAYS | LVS_SINGLESEL,
            MARGIN + 260, MARGIN + 30,
            DIALOG_WIDTH - MARGIN - 270, DIALOG_HEIGHT - 150,
            m_hDialog,
            (HMENU)2002,
            GetModuleHandle(nullptr),
            nullptr
        );
        
        if (m_hListView) {
            // 设置扩展样式
            ListView_SetExtendedListViewStyle(m_hListView, 
                LVS_EX_FULLROWSELECT | LVS_EX_CHECKBOXES | LVS_EX_GRIDLINES);
            
            // 添加列
            LVCOLUMNW col = {};
            col.mask = LVCF_TEXT | LVCF_WIDTH;
            
            col.pszText = const_cast<wchar_t*>(L"名称");
            col.cx = 200;
            ListView_InsertColumn(m_hListView, 0, &col);
            
            col.pszText = const_cast<wchar_t*>(L"路径");
            col.cx = 250;
            ListView_InsertColumn(m_hListView, 1, &col);
            
            col.pszText = const_cast<wchar_t*>(L"大小");
            col.cx = 80;
            ListView_InsertColumn(m_hListView, 2, &col);
            
            col.pszText = const_cast<wchar_t*>(L"风险");
            col.cx = 60;
            ListView_InsertColumn(m_hListView, 3, &col);
            
            YG_LOG_INFO(L"列表控件创建成功");
        }
    }
    
    void CleanupDialog::CreateButtons() {
        // 创建操作按钮组，类似日志管理窗口的风格
        int groupBoxY = DIALOG_HEIGHT - 80;
        int buttonY = groupBoxY + 15;
        int buttonSpacing = BUTTON_WIDTH + 8;
        int startX = MARGIN + 10;
        
        // 创建操作分组框
        CreateWindow(L"BUTTON", L"操作",
                   WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
                   MARGIN, groupBoxY, DIALOG_WIDTH - 2 * MARGIN, 40,
                   m_hDialog, nullptr, GetModuleHandle(nullptr), nullptr);
        
        // 查看详情按钮
        CreateWindow(L"BUTTON", L"查看详情",
                   WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                   startX, buttonY, BUTTON_WIDTH, BUTTON_HEIGHT,
                   m_hDialog, (HMENU)3001, GetModuleHandle(nullptr), nullptr);
        startX += buttonSpacing;
        
        // 删除选中按钮
        m_hDeleteSelectedButton = CreateWindow(L"BUTTON", L"删除选中",
                                             WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                                             startX, buttonY, BUTTON_WIDTH, BUTTON_HEIGHT,
                                             m_hDialog, (HMENU)3003, GetModuleHandle(nullptr), nullptr);
        startX += buttonSpacing;
        
        // 清空所有按钮
        m_hDeleteAllButton = CreateWindow(L"BUTTON", L"清空所有",
                                        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                                        startX, buttonY, BUTTON_WIDTH, BUTTON_HEIGHT,
                                        m_hDialog, (HMENU)3004, GetModuleHandle(nullptr), nullptr);
        startX += buttonSpacing;
        
        // 刷新按钮
        CreateWindow(L"BUTTON", L"刷新",
                   WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                   startX, buttonY, 50, BUTTON_HEIGHT,
                   m_hDialog, (HMENU)3006, GetModuleHandle(nullptr), nullptr);
        
        // 底部确定/取消按钮，与日志管理窗口一致
        int bottomButtonY = DIALOG_HEIGHT - 25;
        CreateWindow(L"BUTTON", L"确定",
                   WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON,
                   DIALOG_WIDTH - 120, bottomButtonY, 50, 14,
                   m_hDialog, (HMENU)IDOK, GetModuleHandle(nullptr), nullptr);
        
        m_hCancelButton = CreateWindow(L"BUTTON", L"取消",
                                     WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                                     DIALOG_WIDTH - 65, bottomButtonY, 50, 14,
                                     m_hDialog, (HMENU)IDCANCEL, GetModuleHandle(nullptr), nullptr);
        
        YG_LOG_INFO(L"清理对话框按钮创建完成");
    }
    
    void CleanupDialog::PopulateTreeView() {
        if (!m_hTreeView) return;
        
        // 清空树形控件
        TreeView_DeleteAllItems(m_hTreeView);
        
        // 添加分组节点
        for (size_t i = 0; i < m_residualGroups.size(); i++) {
            const auto& group = m_residualGroups[i];
            
            String nodeText = group.groupName + L" (" + std::to_wstring(group.items.size()) + L"项)";
            
            TVINSERTSTRUCTW tvis = {};
            tvis.hParent = TVI_ROOT;
            tvis.hInsertAfter = TVI_LAST;
            tvis.item.mask = TVIF_TEXT | TVIF_PARAM;
            tvis.item.pszText = const_cast<wchar_t*>(nodeText.c_str());
            tvis.item.lParam = static_cast<LPARAM>(i);
            
            TreeView_InsertItem(m_hTreeView, &tvis);
        }
        
        YG_LOG_INFO(L"树形控件数据填充完成");
    }
    
    void CleanupDialog::PopulateListView(int groupIndex) {
        if (!m_hListView) return;
        
        // 清空列表
        ListView_DeleteAllItems(m_hListView);
        
        // 显示所有残留项，类似日志管理窗口的风格
        int itemIndex = 0;
        for (size_t groupIdx = 0; groupIdx < m_residualGroups.size(); groupIdx++) {
            const auto& group = m_residualGroups[groupIdx];
            
            for (size_t i = 0; i < group.items.size(); i++) {
                const auto& item = group.items[i];
                
                LVITEMW lvItem = {};
                lvItem.mask = LVIF_TEXT | LVIF_PARAM;
                lvItem.iItem = itemIndex;
                lvItem.iSubItem = 0;
                
                // 类型列 - 显示分组名称
                String typeText = GetResidualTypeText(item.type);
                lvItem.pszText = const_cast<wchar_t*>(typeText.c_str());
                lvItem.lParam = static_cast<LPARAM>((groupIdx << 16) | i); // 编码组和项索引
                
                int insertedIndex = ListView_InsertItem(m_hListView, &lvItem);
                
                if (insertedIndex >= 0) {
                    // 设置选中状态
                    ListView_SetCheckState(m_hListView, insertedIndex, item.isSelected);
                    
                    // 路径列
                    ListView_SetItemText(m_hListView, insertedIndex, 1, const_cast<wchar_t*>(item.path.c_str()));
                    
                    // 大小列
                    String sizeText = item.size > 0 ? StringUtils::FormatFileSize(item.size) : L"-";
                    ListView_SetItemText(m_hListView, insertedIndex, 2, const_cast<wchar_t*>(sizeText.c_str()));
                }
                
                itemIndex++;
            }
        }
        
        YG_LOG_INFO(L"列表控件数据填充完成，总计: " + std::to_wstring(itemIndex) + L" 项");
    }
    
    void CleanupDialog::UpdateSelectionStats() {
        int totalItems = 0;
        int selectedItems = 0;
        DWORD64 totalSize = 0;
        DWORD64 selectedSize = 0;
        
        for (const auto& group : m_residualGroups) {
            totalItems += static_cast<int>(group.items.size());
            for (const auto& item : group.items) {
                totalSize += item.size;
                if (item.isSelected) {
                    selectedItems++;
                    selectedSize += item.size;
                }
            }
        }
        
        String statusText = L"总计 " + std::to_wstring(totalItems) + L" 项，已选中 " + 
                          std::to_wstring(selectedItems) + L" 项";
        
        if (selectedSize > 0) {
            statusText += L"，将释放 " + StringUtils::FormatFileSize(selectedSize) + L" 空间";
        }
        
        if (m_hStatusLabel) {
            SetWindowTextW(m_hStatusLabel, statusText.c_str());
        }
    }
    
    void CleanupDialog::SelectAll() {
        for (auto& group : m_residualGroups) {
            for (auto& item : group.items) {
                item.isSelected = true;
            }
        }
        
        // 更新当前显示的列表
        HTREEITEM selectedItem = TreeView_GetSelection(m_hTreeView);
        if (selectedItem) {
            TVITEMW tvItem = {};
            tvItem.mask = TVIF_PARAM;
            tvItem.hItem = selectedItem;
            TreeView_GetItem(m_hTreeView, &tvItem);
            
            int groupIndex = static_cast<int>(tvItem.lParam);
            PopulateListView(groupIndex);
        }
        
        UpdateSelectionStats();
        YG_LOG_INFO(L"已全选所有残留项");
    }
    
    void CleanupDialog::SelectNone() {
        for (auto& group : m_residualGroups) {
            for (auto& item : group.items) {
                item.isSelected = false;
            }
        }
        
        // 更新当前显示的列表
        HTREEITEM selectedItem = TreeView_GetSelection(m_hTreeView);
        if (selectedItem) {
            TVITEMW tvItem = {};
            tvItem.mask = TVIF_PARAM;
            tvItem.hItem = selectedItem;
            TreeView_GetItem(m_hTreeView, &tvItem);
            
            int groupIndex = static_cast<int>(tvItem.lParam);
            PopulateListView(groupIndex);
        }
        
        UpdateSelectionStats();
        YG_LOG_INFO(L"已取消选择所有残留项");
    }
    
    void CleanupDialog::DeleteSelected() {
        // 收集选中的项
        std::vector<ResidualItem> selectedItems;
        for (const auto& group : m_residualGroups) {
            for (const auto& item : group.items) {
                if (item.isSelected) {
                    selectedItems.push_back(item);
                }
            }
        }
        
        if (selectedItems.empty()) {
            MessageBox(m_hDialog, L"请先选择要删除的项目。", L"提示", MB_OK | MB_ICONINFORMATION);
            return;
        }
        
        // 确认删除
        String confirmMsg = L"确定要删除选中的 " + std::to_wstring(selectedItems.size()) + L" 个残留项吗？\n\n";
        confirmMsg += L"此操作无法撤销！";
        
        if (MessageBox(m_hDialog, confirmMsg.c_str(), L"确认删除", MB_YESNO | MB_ICONQUESTION) == IDYES) {
            PerformDelete(selectedItems);
        }
    }
    
    void CleanupDialog::DeleteAll() {
        // 收集所有项
        std::vector<ResidualItem> allItems;
        for (const auto& group : m_residualGroups) {
            for (const auto& item : group.items) {
                allItems.push_back(item);
            }
        }
        
        if (allItems.empty()) {
            MessageBox(m_hDialog, L"没有可删除的项目。", L"提示", MB_OK | MB_ICONINFORMATION);
            return;
        }
        
        // 确认删除
        String confirmMsg = L"确定要删除全部 " + std::to_wstring(allItems.size()) + L" 个残留项吗？\n\n";
        confirmMsg += L"此操作无法撤销！";
        
        if (MessageBox(m_hDialog, confirmMsg.c_str(), L"确认删除全部", MB_YESNO | MB_ICONWARNING) == IDYES) {
            PerformDelete(allItems);
        }
    }
    
    void CleanupDialog::PerformDelete(const std::vector<ResidualItem>& items) {
        m_isDeleting = true;
        
        // 禁用按钮
        EnableWindow(m_hSelectAllButton, FALSE);
        EnableWindow(m_hSelectNoneButton, FALSE);
        EnableWindow(m_hDeleteSelectedButton, FALSE);
        EnableWindow(m_hDeleteAllButton, FALSE);
        
        // 显示进度条
        ShowWindow(m_hProgressBar, SW_SHOW);
        SendMessage(m_hProgressBar, PBM_SETRANGE, 0, MAKELPARAM(0, 100));
        
        YG_LOG_INFO(L"开始删除操作，项目数: " + std::to_wstring(items.size()));
        
        // 使用扫描器执行删除
        if (m_scanner) {
            m_scanner->DeleteResidualItems(items, 
                [this](int percentage, const String& currentItem, bool success) {
                    OnDeleteProgress(percentage, currentItem, success);
                });
        }
        
        // 删除完成
        m_isDeleting = false;
        m_result = CleanupResult::Completed;
        
        // 重新启用按钮
        EnableWindow(m_hSelectAllButton, TRUE);
        EnableWindow(m_hSelectNoneButton, TRUE);
        EnableWindow(m_hDeleteSelectedButton, TRUE);
        EnableWindow(m_hDeleteAllButton, TRUE);
        
        // 隐藏进度条
        ShowWindow(m_hProgressBar, SW_HIDE);
        
        MessageBox(m_hDialog, L"清理操作完成！", L"完成", MB_OK | MB_ICONINFORMATION);
        
        YG_LOG_INFO(L"删除操作完成");
    }
    
    void CleanupDialog::OnDeleteProgress(int percentage, const String& currentItem, bool success) {
        if (m_hProgressBar) {
            SendMessage(m_hProgressBar, PBM_SETPOS, percentage, 0);
        }
        
        if (m_hStatusLabel) {
            String statusText = L"正在删除: " + currentItem;
            if (!success) {
                statusText += L" (失败)";
            }
            SetWindowTextW(m_hStatusLabel, statusText.c_str());
        }
    }
    
    void CleanupDialog::CenterWindow() {
        RECT parentRect, dialogRect;
        GetWindowRect(m_hParent, &parentRect);
        GetWindowRect(m_hDialog, &dialogRect);
        
        int parentWidth = parentRect.right - parentRect.left;
        int parentHeight = parentRect.bottom - parentRect.top;
        int dialogWidth = dialogRect.right - dialogRect.left;
        int dialogHeight = dialogRect.bottom - dialogRect.top;
        
        int x = parentRect.left + (parentWidth - dialogWidth) / 2;
        int y = parentRect.top + (parentHeight - dialogHeight) / 2;
        
        SetWindowPos(m_hDialog, nullptr, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
    }
    
    String CleanupDialog::GetRiskLevelDescription(RiskLevel riskLevel) {
        switch (riskLevel) {
            case RiskLevel::Safe: return L"安全";
            case RiskLevel::Low: return L"低风险";
            case RiskLevel::Medium: return L"中风险";
            case RiskLevel::High: return L"高风险";
            case RiskLevel::Critical: return L"危险";
            default: return L"未知";
        }
    }
    
    String CleanupDialog::GetResidualTypeText(ResidualType type) {
        switch (type) {
            case ResidualType::File: return L"📄 文件";
            case ResidualType::Directory: return L"📁 目录";
            case ResidualType::RegistryKey: return L"🔧 注册表键";
            case ResidualType::RegistryValue: return L"🔧 注册表值";
            case ResidualType::Shortcut: return L"🔗 快捷方式";
            case ResidualType::Service: return L"⚙️ 服务";
            case ResidualType::StartupItem: return L"🚀 启动项";
            case ResidualType::Cache: return L"💾 缓存";
            case ResidualType::Log: return L"📋 日志";
            case ResidualType::Temp: return L"🗑️ 临时文件";
            case ResidualType::Config: return L"⚙️ 配置";
            default: return L"❓ 未知";
        }
    }
    
    String CleanupDialog::GetTypeIcon(ResidualType type) {
        switch (type) {
            case ResidualType::File: return L"📄";
            case ResidualType::Directory: return L"📁";
            case ResidualType::RegistryKey: return L"📝";
            case ResidualType::RegistryValue: return L"🔑";
            case ResidualType::Shortcut: return L"🔗";
            case ResidualType::Service: return L"⚙️";
            case ResidualType::Cache: return L"💾";
            case ResidualType::Log: return L"📋";
            case ResidualType::Temp: return L"🗂️";
            case ResidualType::Config: return L"⚙️";
            default: return L"❓";
        }
    }
    
    LRESULT CALLBACK CleanupDialog::DialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
        CleanupDialog* dialog = nullptr;
        
        if (message == WM_NCCREATE) {
            CREATESTRUCT* pCreate = (CREATESTRUCT*)lParam;
            dialog = (CleanupDialog*)pCreate->lpCreateParams;
            SetWindowLongPtr(hDlg, GWLP_USERDATA, (LONG_PTR)dialog);
            dialog->m_hDialog = hDlg;
        } else {
            dialog = (CleanupDialog*)GetWindowLongPtr(hDlg, GWLP_USERDATA);
        }
        
        if (dialog) {
            return dialog->HandleMessage(message, wParam, lParam);
        }
        
        return DefWindowProc(hDlg, message, wParam, lParam);
    }
    
    LRESULT CleanupDialog::HandleMessage(UINT message, WPARAM wParam, LPARAM lParam) {
        switch (message) {
            case WM_COMMAND:
                switch (LOWORD(wParam)) {
                    case 3001: // 全选
                        SelectAll();
                        return 0;
                    case 3002: // 全不选
                        SelectNone();
                        return 0;
                    case 3003: // 删除选中
                        DeleteSelected();
                        return 0;
                    case 3004: // 删除全部
                        DeleteAll();
                        return 0;
                    case IDCANCEL: // 取消
                        m_result = CleanupResult::Cancelled;
                        m_dialogClosed = true;
                        DestroyWindow(m_hDialog);
                        return 0;
                }
                break;
                
            case WM_CLOSE:
                m_result = CleanupResult::Cancelled;
                m_dialogClosed = true;
                DestroyWindow(m_hDialog);
                return 0;
                
            case WM_DESTROY:
                m_dialogClosed = true;
                return 0;
                
            case WM_NOTIFY:
                {
                    LPNMHDR pNMHDR = (LPNMHDR)lParam;
                    if (pNMHDR->hwndFrom == m_hTreeView) {
                        // 处理树形控件通知
                        if (pNMHDR->code == TVN_SELCHANGED) {
                            LPNMTREEVIEW pNMTV = (LPNMTREEVIEW)lParam;
                            OnTreeSelectionChanged(pNMTV->itemNew.hItem);
                        }
                    } else if (pNMHDR->hwndFrom == m_hListView) {
                        // 处理列表控件通知
                        if (pNMHDR->code == LVN_ITEMCHANGED) {
                            LPNMLISTVIEW pNMLV = (LPNMLISTVIEW)lParam;
                            if (pNMLV->uChanged & LVIF_STATE) {
                                // 选择状态变化
                                bool isSelected = ListView_GetCheckState(m_hListView, pNMLV->iItem) != 0;
                                OnListItemSelectionChanged(pNMLV->iItem, isSelected);
                            }
                        }
                    }
                }
                return 0;
        }
        
        return DefWindowProc(m_hDialog, message, wParam, lParam);
    }
    
    void CleanupDialog::OnTreeSelectionChanged(HTREEITEM hItem) {
        if (!hItem) return;
        
        TVITEMW tvItem = {};
        tvItem.mask = TVIF_PARAM;
        tvItem.hItem = hItem;
        TreeView_GetItem(m_hTreeView, &tvItem);
        
        int groupIndex = static_cast<int>(tvItem.lParam);
        PopulateListView(groupIndex);
    }
    
    void CleanupDialog::OnListItemSelectionChanged(int itemIndex, bool selected) {
        // 更新对应的残留项选择状态
        HTREEITEM selectedTreeItem = TreeView_GetSelection(m_hTreeView);
        if (!selectedTreeItem) return;
        
        TVITEMW tvItem = {};
        tvItem.mask = TVIF_PARAM;
        tvItem.hItem = selectedTreeItem;
        TreeView_GetItem(m_hTreeView, &tvItem);
        
        int groupIndex = static_cast<int>(tvItem.lParam);
        if (groupIndex >= 0 && groupIndex < static_cast<int>(m_residualGroups.size()) &&
            itemIndex >= 0 && itemIndex < static_cast<int>(m_residualGroups[groupIndex].items.size())) {
            
            m_residualGroups[groupIndex].items[itemIndex].isSelected = selected;
            UpdateSelectionStats();
        }
    }
    
} // namespace YG
