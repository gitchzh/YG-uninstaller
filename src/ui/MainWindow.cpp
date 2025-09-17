/**
 * @file MainWindow.cpp
 * @brief 主窗口实现
 * @author YG Software
 * @version 1.0.0
 * @date 2025-09-17
 */

#include "ui/MainWindow.h"
#include "core/Logger.h"
#include "core/ErrorHandler.h"

namespace YG {
    
    MainWindow::MainWindow() : m_hWnd(nullptr), m_hInstance(nullptr),
                              m_isScanning(false), m_isUninstalling(false),
                              m_includeSystemComponents(false) {
    }
    
    MainWindow::~MainWindow() {
        if (m_hWnd) {
            DestroyWindow(m_hWnd);
        }
    }
    
    ErrorCode MainWindow::Create(HINSTANCE hInstance) {
        m_hInstance = hInstance;
        
        // 注册窗口类
        WNDCLASSEXW wcex;
        ZeroMemory(&wcex, sizeof(WNDCLASSEXW));
        wcex.cbSize = sizeof(WNDCLASSEXW);
        wcex.style = CS_HREDRAW | CS_VREDRAW;
        wcex.lpfnWndProc = WindowProc;
        wcex.cbClsExtra = 0;
        wcex.cbWndExtra = 0;
        wcex.hInstance = hInstance;
        wcex.hIcon = LoadIcon(hInstance, IDI_APPLICATION);
        wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
        wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
        wcex.lpszMenuName = nullptr;
        wcex.lpszClassName = WINDOW_CLASS_NAME;
        wcex.hIconSm = LoadIcon(wcex.hInstance, IDI_APPLICATION);
        
        if (!RegisterClassExW(&wcex)) {
            YG_LOG_ERROR(L"窗口类注册失败");
            return ErrorCode::GeneralError;
        }
        
        // 创建窗口
        m_hWnd = CreateWindowW(
            WINDOW_CLASS_NAME,
            WINDOW_TITLE,
            WS_OVERLAPPEDWINDOW,
            CW_USEDEFAULT, CW_USEDEFAULT,
            800, 600,
            nullptr,
            nullptr,
            hInstance,
            this
        );
        
        if (!m_hWnd) {
            YG_LOG_ERROR(L"窗口创建失败");
            return ErrorCode::GeneralError;
        }
        
        // 窗口创建成功后，立即创建菜单
        ErrorCode menuResult = CreateMenus();
        if (menuResult != ErrorCode::Success) {
            YG_LOG_ERROR(L"菜单创建失败");
        }
        
        YG_LOG_INFO(L"主窗口创建成功");
        return ErrorCode::Success;
    }
    
    void MainWindow::Show(int nCmdShow) {
        if (m_hWnd) {
            ShowWindow(m_hWnd, nCmdShow);
            UpdateWindow(m_hWnd);
        }
    }
    
    HWND MainWindow::GetHandle() const {
        return m_hWnd;
    }
    
    int MainWindow::RunMessageLoop() {
        MSG msg;
        while (GetMessage(&msg, nullptr, 0, 0)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        return (int)msg.wParam;
    }
    
    void MainWindow::RefreshProgramList(bool includeSystemComponents) {
        m_includeSystemComponents = includeSystemComponents;
        
        SetStatusText(L"正在扫描已安装的程序...");
        
        // 模拟扫描过程
        Sleep(500);  // 模拟扫描延迟
        
        // 重新加载示例程序
        AddSamplePrograms();
        
        YG_LOG_INFO(L"程序列表刷新完成");
    }
    
    void MainWindow::SearchPrograms(const String& keyword) {
        m_currentSearchKeyword = keyword;
        // TODO: 实现程序搜索
        YG_LOG_INFO(L"搜索程序: " + keyword);
    }
    
    void MainWindow::UninstallSelectedProgram(UninstallMode mode) {
        // TODO: 实现程序卸载
        YG_LOG_INFO(L"卸载选中的程序");
    }
    
    void MainWindow::ShowProgramDetails(const ProgramInfo& program) {
        // TODO: 显示程序详细信息
        YG_LOG_INFO(L"显示程序详细信息: " + program.name);
    }
    
    void MainWindow::SetStatusText(const String& text) {
        if (m_hStatusBar) {
            SendMessageW(m_hStatusBar, SB_SETTEXTW, 0, (LPARAM)text.c_str());
        }
    }
    
    void MainWindow::UpdateProgress(int percentage, bool visible) {
        // TODO: 更新进度条
    }
    
    LRESULT CALLBACK MainWindow::WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
        MainWindow* pThis = nullptr;
        
        if (uMsg == WM_NCCREATE) {
            CREATESTRUCT* pCreate = (CREATESTRUCT*)lParam;
            pThis = (MainWindow*)pCreate->lpCreateParams;
            SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)pThis);
        } else {
            pThis = (MainWindow*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
        }
        
        if (pThis) {
            return pThis->HandleMessage(hWnd, uMsg, wParam, lParam);
        } else {
            return DefWindowProc(hWnd, uMsg, wParam, lParam);
        }
    }
    
    LRESULT MainWindow::HandleMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
        switch (uMsg) {
            case WM_CREATE:
                return OnCreate(hWnd, lParam);
            case WM_DESTROY:
                return OnDestroy();
            case WM_SIZE:
                return OnSize(LOWORD(lParam), HIWORD(lParam));
            case WM_COMMAND:
                return OnCommand(LOWORD(wParam), HIWORD(wParam), (HWND)lParam);
            case WM_KEYDOWN:
                return OnKeyDown((UINT)wParam, (UINT)lParam);
            case WM_NOTIFY:
                return OnNotify((LPNMHDR)lParam);
            case WM_CONTEXTMENU:
                return OnContextMenu(LOWORD(lParam), HIWORD(lParam));
            default:
                return DefWindowProc(hWnd, uMsg, wParam, lParam);
        }
    }
    
    LRESULT MainWindow::OnCreate(HWND hWnd, LPARAM lParam) {
        // 创建控件
        CreateControls();
        YG_LOG_INFO(L"窗口创建完成");
        return 0;
    }
    
    LRESULT MainWindow::OnDestroy() {
        PostQuitMessage(0);
        return 0;
    }
    
    LRESULT MainWindow::OnSize(int width, int height) {
        // 调整控件布局
        AdjustLayout();
        return 0;
    }
    
    LRESULT MainWindow::OnCommand(WORD commandId, WORD notificationCode, HWND controlHandle) {
        switch (commandId) {
            // 文件菜单
            case ID_FILE_EXIT:
                PostMessage(m_hWnd, WM_CLOSE, 0, 0);
                break;
                
            // 操作菜单
            case ID_ACTION_REFRESH:
            case ID_EDIT_REFRESH:
            case ID_TB_REFRESH:
            case ID_VIEW_REFRESH:
                RefreshProgramList(m_includeSystemComponents);
                break;
            case ID_ACTION_UNINSTALL:
            case ID_TB_UNINSTALL:
                {
                    int selectedIndex = ListView_GetNextItem(m_hListView, -1, LVNI_SELECTED);
                    if (selectedIndex != -1) {
                        wchar_t programName[256];
                        ListView_GetItemText(m_hListView, selectedIndex, 0, programName, sizeof(programName)/sizeof(wchar_t));
                        
                        String message = L"确定要卸载程序 \"" + String(programName) + L"\" 吗？";
                        if (ShowConfirmation(L"确认卸载", message)) {
                            SetStatusText(L"正在卸载: " + String(programName));
                            MessageBoxW(m_hWnd, L"卸载功能演示\n\n实际项目中这里会调用真正的卸载逻辑。", L"演示", MB_OK | MB_ICONINFORMATION);
                            SetStatusText(L"卸载完成: " + String(programName));
                        }
                    } else {
                        MessageBoxW(m_hWnd, L"请先选择要卸载的程序。", L"提示", MB_OK | MB_ICONWARNING);
                    }
                }
                break;
            case ID_ACTION_FORCE_UNINSTALL:
            case ID_TB_FORCE_UNINSTALL:
                {
                    int selectedIndex = ListView_GetNextItem(m_hListView, -1, LVNI_SELECTED);
                    if (selectedIndex != -1) {
                        wchar_t programName[256];
                        ListView_GetItemText(m_hListView, selectedIndex, 0, programName, sizeof(programName)/sizeof(wchar_t));
                        
                        String message = L"确定要强制卸载程序 \"" + String(programName) + L"\" 吗？\n\n警告：强制卸载可能导致系统不稳定。";
                        if (ShowConfirmation(L"确认强制卸载", message)) {
                            SetStatusText(L"正在强制卸载: " + String(programName));
                            MessageBoxW(m_hWnd, L"强制卸载功能演示\n\n实际项目中这里会调用强制卸载逻辑。", L"演示", MB_OK | MB_ICONINFORMATION);
                            SetStatusText(L"强制卸载完成: " + String(programName));
                        }
                    } else {
                        MessageBoxW(m_hWnd, L"请先选择要强制卸载的程序。", L"提示", MB_OK | MB_ICONWARNING);
                    }
                }
                break;
                
            // 操作菜单 - 其他项目
            case ID_ACTION_PROPERTIES:
                {
                    int selectedIndex = ListView_GetNextItem(m_hListView, -1, LVNI_SELECTED);
                    if (selectedIndex != -1) {
                        wchar_t programName[256];
                        ListView_GetItemText(m_hListView, selectedIndex, 0, programName, sizeof(programName)/sizeof(wchar_t));
                        MessageBoxW(m_hWnd, (L"程序属性：\n\n程序名称：" + String(programName) + L"\n\n实际项目中这里会显示详细属性对话框。").c_str(), L"程序属性", MB_OK | MB_ICONINFORMATION);
                    } else {
                        MessageBoxW(m_hWnd, L"请先选择一个程序。", L"提示", MB_OK | MB_ICONWARNING);
                    }
                }
                break;
            case ID_ACTION_OPEN_LOCATION:
                {
                    int selectedIndex = ListView_GetNextItem(m_hListView, -1, LVNI_SELECTED);
                    if (selectedIndex != -1) {
                        wchar_t programName[256];
                        ListView_GetItemText(m_hListView, selectedIndex, 0, programName, sizeof(programName)/sizeof(wchar_t));
                        MessageBoxW(m_hWnd, (L"打开安装位置：\n\n程序：" + String(programName) + L"\n\n实际项目中这里会打开文件资源管理器。").c_str(), L"打开位置", MB_OK | MB_ICONINFORMATION);
                    } else {
                        MessageBoxW(m_hWnd, L"请先选择一个程序。", L"提示", MB_OK | MB_ICONWARNING);
                    }
                }
                break;
                
            // 查看菜单
            case ID_VIEW_TOOLBAR:
                {
                    HMENU hMenu = GetMenu(m_hWnd);
                    HMENU hViewMenu = GetSubMenu(hMenu, 2); // 查看菜单是第3个
                    UINT state = GetMenuState(hViewMenu, ID_VIEW_TOOLBAR, MF_BYCOMMAND);
                    bool isChecked = (state & MF_CHECKED) != 0;
                    
                    CheckMenuItem(hViewMenu, ID_VIEW_TOOLBAR, MF_BYCOMMAND | (isChecked ? MF_UNCHECKED : MF_CHECKED));
                    ShowWindow(m_hToolbar, isChecked ? SW_HIDE : SW_SHOW);
                    AdjustLayout();
                    SetStatusText(isChecked ? L"工具栏已隐藏" : L"工具栏已显示");
                }
                break;
            case ID_VIEW_STATUSBAR:
                {
                    HMENU hMenu = GetMenu(m_hWnd);
                    HMENU hViewMenu = GetSubMenu(hMenu, 2); // 查看菜单是第3个
                    UINT state = GetMenuState(hViewMenu, ID_VIEW_STATUSBAR, MF_BYCOMMAND);
                    bool isChecked = (state & MF_CHECKED) != 0;
                    
                    CheckMenuItem(hViewMenu, ID_VIEW_STATUSBAR, MF_BYCOMMAND | (isChecked ? MF_UNCHECKED : MF_CHECKED));
                    ShowWindow(m_hStatusBar, isChecked ? SW_HIDE : SW_SHOW);
                    AdjustLayout();
                    if (!isChecked) SetStatusText(L"状态栏已显示");
                }
                break;
            case ID_VIEW_SHOW_SYSTEM:
                {
                    HMENU hMenu = GetMenu(m_hWnd);
                    HMENU hViewMenu = GetSubMenu(hMenu, 2);
                    UINT state = GetMenuState(hViewMenu, ID_VIEW_SHOW_SYSTEM, MF_BYCOMMAND);
                    bool isChecked = (state & MF_CHECKED) != 0;
                    
                    CheckMenuItem(hViewMenu, ID_VIEW_SHOW_SYSTEM, MF_BYCOMMAND | (isChecked ? MF_UNCHECKED : MF_CHECKED));
                    m_includeSystemComponents = !isChecked;
                    RefreshProgramList(m_includeSystemComponents);
                    SetStatusText(m_includeSystemComponents ? L"显示系统组件" : L"隐藏系统组件");
                }
                break;
                
            // 帮助菜单
            case ID_HELP_HELP:
                MessageBoxW(m_hWnd, L"YG Uninstaller 帮助\n\n这是一个高效的程序卸载工具。\n\n功能:\n• 扫描已安装程序\n• 标准卸载\n• 强制卸载\n• 深度清理", L"帮助", MB_OK | MB_ICONINFORMATION);
                break;
            case ID_HELP_WEBSITE:
                MessageBoxW(m_hWnd, L"访问官网功能演示\n\n实际项目中这里会打开浏览器访问官网。", L"访问官网", MB_OK | MB_ICONINFORMATION);
                break;
            case ID_HELP_CHECK_UPDATE:
                MessageBoxW(m_hWnd, L"检查更新功能演示\n\n当前版本：1.0.0\n\n实际项目中这里会检查最新版本。", L"检查更新", MB_OK | MB_ICONINFORMATION);
                break;
            case ID_HELP_ABOUT:
                MessageBoxW(m_hWnd, L"关于 YG Uninstaller\n\n版本：1.0.0\n\n一个高效、轻量级的Windows程序卸载工具。\n\n© 2025 YG Software", L"关于", MB_OK | MB_ICONINFORMATION);
                break;
                
            default:
                return DefWindowProc(m_hWnd, WM_COMMAND, MAKEWPARAM(commandId, notificationCode), (LPARAM)controlHandle);
        }
        return 0;
    }
    
    LRESULT MainWindow::OnNotify(LPNMHDR pNMHDR) {
        // TODO: 处理通知消息
        return 0;
    }
    
    LRESULT MainWindow::OnKeyDown(UINT key, UINT flags) {
        switch (key) {
            case VK_F5:
                // F5 - 刷新
                RefreshProgramList(m_includeSystemComponents);
                break;
            case VK_DELETE:
                // Delete - 卸载程序
                if (GetKeyState(VK_SHIFT) & 0x8000) {
                    // Shift+Delete - 强制卸载
                    SendMessage(m_hWnd, WM_COMMAND, ID_ACTION_FORCE_UNINSTALL, 0);
                } else {
                    // Delete - 标准卸载
                    SendMessage(m_hWnd, WM_COMMAND, ID_ACTION_UNINSTALL, 0);
                }
                break;
            case VK_F1:
                // F1 - 帮助
                SendMessage(m_hWnd, WM_COMMAND, ID_HELP_HELP, 0);
                break;
            case VK_RETURN:
                // Alt+Enter - 属性
                if (GetKeyState(VK_MENU) & 0x8000) {
                    SendMessage(m_hWnd, WM_COMMAND, ID_ACTION_PROPERTIES, 0);
                }
                break;
            case 'O':
                // Ctrl+O - 打开位置
                if (GetKeyState(VK_CONTROL) & 0x8000) {
                    SendMessage(m_hWnd, WM_COMMAND, ID_ACTION_OPEN_LOCATION, 0);
                }
                break;
            default:
                return DefWindowProc(m_hWnd, WM_KEYDOWN, key, flags);
        }
        return 0;
    }
    
    LRESULT MainWindow::OnContextMenu(int x, int y) {
        // 创建右键菜单
        if (!m_hContextMenu) {
            m_hContextMenu = CreatePopupMenu();
            AppendMenuW(m_hContextMenu, MF_STRING, ID_ACTION_UNINSTALL, L"卸载程序");
            AppendMenuW(m_hContextMenu, MF_STRING, ID_ACTION_FORCE_UNINSTALL, L"强制卸载");
            AppendMenuW(m_hContextMenu, MF_SEPARATOR, 0, nullptr);
            AppendMenuW(m_hContextMenu, MF_STRING, ID_ACTION_PROPERTIES, L"属性");
            AppendMenuW(m_hContextMenu, MF_STRING, ID_ACTION_OPEN_LOCATION, L"打开安装位置");
        }
        
        // 检查是否有选中的项目
        int selectedIndex = ListView_GetNextItem(m_hListView, -1, LVNI_SELECTED);
        if (selectedIndex != -1) {
            // 显示右键菜单
            TrackPopupMenu(m_hContextMenu, TPM_RIGHTBUTTON, x, y, 0, m_hWnd, nullptr);
        }
        
        return 0;
    }
    
    ErrorCode MainWindow::CreateControls() {
        // 获取客户区大小
        RECT clientRect;
        GetClientRect(m_hWnd, &clientRect);
        int width = clientRect.right - clientRect.left;
        int height = clientRect.bottom - clientRect.top;
        
        // 创建工具栏
        m_hToolbar = CreateWindowW(
            TOOLBARCLASSNAMEW,
            nullptr,
            WS_CHILD | WS_VISIBLE | TBSTYLE_FLAT | TBSTYLE_TOOLTIPS,
            0, 0, 0, 0,
            m_hWnd,
            (HMENU)ID_TOOLBAR,
            m_hInstance,
            nullptr
        );
        
        if (m_hToolbar) {
            SendMessage(m_hToolbar, TB_BUTTONSTRUCTSIZE, sizeof(TBBUTTON), 0);
            
            // 添加工具栏按钮
            TBBUTTON buttons[] = {
                {0, ID_TB_REFRESH, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, (INT_PTR)L"刷新"},
                {0, 0, 0, BTNS_SEP, {0}, 0, 0},
                {1, ID_TB_UNINSTALL, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, (INT_PTR)L"卸载"},
                {2, ID_TB_FORCE_UNINSTALL, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, (INT_PTR)L"强制卸载"},
            };
            
            SendMessage(m_hToolbar, TB_ADDBUTTONSW, sizeof(buttons)/sizeof(TBBUTTON), (LPARAM)buttons);
            SendMessage(m_hToolbar, TB_AUTOSIZE, 0, 0);
        }
        
        // 获取工具栏高度
        RECT toolbarRect;
        GetWindowRect(m_hToolbar, &toolbarRect);
        int toolbarHeight = toolbarRect.bottom - toolbarRect.top;
        
        // 创建状态栏
        m_hStatusBar = CreateWindowW(
            STATUSCLASSNAMEW,
            nullptr,
            WS_CHILD | WS_VISIBLE | SBARS_SIZEGRIP,
            0, 0, 0, 0,
            m_hWnd,
            (HMENU)ID_STATUSBAR,
            m_hInstance,
            nullptr
        );
        
        // 获取状态栏高度
        RECT statusRect;
        GetWindowRect(m_hStatusBar, &statusRect);
        int statusHeight = statusRect.bottom - statusRect.top;
        
        // 创建搜索框
        CreateWindowW(
            L"STATIC",
            L"搜索:",
            WS_CHILD | WS_VISIBLE,
            10, toolbarHeight + 10, 40, 20,
            m_hWnd,
            nullptr,
            m_hInstance,
            nullptr
        );
        
        m_hSearchEdit = CreateWindowW(
            L"EDIT",
            L"",
            WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
            55, toolbarHeight + 8, 200, 24,
            m_hWnd,
            (HMENU)ID_SEARCH_EDIT,
            m_hInstance,
            nullptr
        );
        
        // 创建程序列表视图
        int listTop = toolbarHeight + 40;
        int listHeight = height - listTop - statusHeight - 10;
        
        m_hListView = CreateWindowW(
            WC_LISTVIEWW,
            nullptr,
            WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SINGLESEL | 
            LVS_SHOWSELALWAYS | WS_BORDER,
            10, listTop, width - 20, listHeight,
            m_hWnd,
            (HMENU)ID_LISTVIEW,
            m_hInstance,
            nullptr
        );
        
        if (!m_hListView) {
            YG_LOG_ERROR(L"列表视图创建失败");
            return ErrorCode::GeneralError;
        }
        
        // 设置列表视图扩展样式
        ListView_SetExtendedListViewStyle(m_hListView, 
            LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_DOUBLEBUFFER);
        
        // 初始化列表视图列
        InitializeListViewColumns();
        
        // 设置初始状态文本
        SetStatusText(L"就绪 - 点击刷新按钮扫描已安装的程序");
        
        // 添加一些示例数据
        AddSamplePrograms();
        
        YG_LOG_INFO(L"界面控件创建完成");
        return ErrorCode::Success;
    }
    
    ErrorCode MainWindow::CreateMenus() {
        // 创建主菜单
        m_hMenu = CreateMenu();
        if (!m_hMenu) {
            YG_LOG_ERROR(L"创建主菜单失败");
            return ErrorCode::GeneralError;
        }
        
        // 创建文件菜单
        HMENU hFileMenu = CreatePopupMenu();
        if (hFileMenu) {
            AppendMenuW(hFileMenu, MF_STRING, ID_FILE_EXIT, L"退出(&X)");
            AppendMenuW(m_hMenu, MF_POPUP, (UINT_PTR)hFileMenu, L"文件(&F)");
        }
        
        // 创建操作菜单
        HMENU hActionMenu = CreatePopupMenu();
        if (hActionMenu) {
            AppendMenuW(hActionMenu, MF_STRING, ID_ACTION_REFRESH, L"刷新程序列表(&R)\tF5");
            AppendMenuW(hActionMenu, MF_SEPARATOR, 0, nullptr);
            AppendMenuW(hActionMenu, MF_STRING, ID_ACTION_UNINSTALL, L"卸载程序(&U)\tDel");
            AppendMenuW(hActionMenu, MF_STRING, ID_ACTION_FORCE_UNINSTALL, L"强制卸载(&F)\tShift+Del");
            AppendMenuW(hActionMenu, MF_SEPARATOR, 0, nullptr);
            AppendMenuW(hActionMenu, MF_STRING, ID_ACTION_PROPERTIES, L"属性(&P)\tAlt+Enter");
            AppendMenuW(hActionMenu, MF_STRING, ID_ACTION_OPEN_LOCATION, L"打开安装位置(&O)\tCtrl+O");
            AppendMenuW(m_hMenu, MF_POPUP, (UINT_PTR)hActionMenu, L"操作(&A)");
        }
        
        // 创建查看菜单
        HMENU hViewMenu = CreatePopupMenu();
        if (hViewMenu) {
            AppendMenuW(hViewMenu, MF_STRING | MF_CHECKED, ID_VIEW_TOOLBAR, L"工具栏(&T)");
            AppendMenuW(hViewMenu, MF_STRING | MF_CHECKED, ID_VIEW_STATUSBAR, L"状态栏(&S)");
            AppendMenuW(hViewMenu, MF_SEPARATOR, 0, nullptr);
            AppendMenuW(hViewMenu, MF_STRING, ID_VIEW_REFRESH, L"刷新(&R)\tF5");
            AppendMenuW(hViewMenu, MF_SEPARATOR, 0, nullptr);
            AppendMenuW(hViewMenu, MF_STRING, ID_VIEW_LARGE_ICONS, L"大图标(&L)");
            AppendMenuW(hViewMenu, MF_STRING, ID_VIEW_SMALL_ICONS, L"小图标(&M)");
            AppendMenuW(hViewMenu, MF_STRING | MF_CHECKED, ID_VIEW_LIST, L"列表(&I)");
            AppendMenuW(hViewMenu, MF_STRING, ID_VIEW_DETAILS, L"详细信息(&D)");
            AppendMenuW(hViewMenu, MF_SEPARATOR, 0, nullptr);
            AppendMenuW(hViewMenu, MF_STRING, ID_VIEW_SHOW_SYSTEM, L"显示系统组件(&C)");
            AppendMenuW(hViewMenu, MF_STRING, ID_VIEW_SHOW_UPDATES, L"显示Windows更新(&W)");
            AppendMenuW(m_hMenu, MF_POPUP, (UINT_PTR)hViewMenu, L"查看(&V)");
        }
        
        // 创建帮助菜单
        HMENU hHelpMenu = CreatePopupMenu();
        if (hHelpMenu) {
            AppendMenuW(hHelpMenu, MF_STRING, ID_HELP_HELP, L"帮助主题(&H)\tF1");
            AppendMenuW(hHelpMenu, MF_SEPARATOR, 0, nullptr);
            AppendMenuW(hHelpMenu, MF_STRING, ID_HELP_WEBSITE, L"访问官网(&W)");
            AppendMenuW(hHelpMenu, MF_STRING, ID_HELP_CHECK_UPDATE, L"检查更新(&U)");
            AppendMenuW(hHelpMenu, MF_SEPARATOR, 0, nullptr);
            AppendMenuW(hHelpMenu, MF_STRING, ID_HELP_ABOUT, L"关于 YG Uninstaller(&A)...");
            AppendMenuW(m_hMenu, MF_POPUP, (UINT_PTR)hHelpMenu, L"帮助(&H)");
        }
        
        // 设置窗口菜单
        if (m_hWnd && m_hMenu) {
            SetMenu(m_hWnd, m_hMenu);
            DrawMenuBar(m_hWnd);  // 强制重绘菜单栏
        }
        
        YG_LOG_INFO(L"菜单创建完成");
        return ErrorCode::Success;
    }
    
    ErrorCode MainWindow::CreateToolbar() {
        // TODO: 创建工具栏
        return ErrorCode::Success;
    }
    
    ErrorCode MainWindow::CreateStatusBar() {
        // TODO: 创建状态栏
        return ErrorCode::Success;
    }
    
    ErrorCode MainWindow::CreateListView() {
        // TODO: 创建列表视图
        return ErrorCode::Success;
    }
    
    void MainWindow::InitializeListViewColumns() {
        // 定义列信息
        struct ColumnInfo {
            const wchar_t* title;
            int width;
            int format;
        };
        
        ColumnInfo columns[] = {
            {L"程序名称", 250, LVCFMT_LEFT},
            {L"版本", 100, LVCFMT_LEFT},
            {L"发布者", 150, LVCFMT_LEFT},
            {L"大小", 80, LVCFMT_RIGHT},
            {L"安装日期", 100, LVCFMT_LEFT}
        };
        
        // 添加列
        LVCOLUMNW column = {};
        column.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_FMT;
        
        for (int i = 0; i < sizeof(columns) / sizeof(columns[0]); i++) {
            column.pszText = (LPWSTR)columns[i].title;
            column.cx = columns[i].width;
            column.fmt = columns[i].format;
            
            if (ListView_InsertColumn(m_hListView, i, &column) == -1) {
                YG_LOG_ERROR(L"添加列失败: " + String(columns[i].title));
            }
        }
    }
    
    void MainWindow::PopulateProgramList(const std::vector<ProgramInfo>& programs) {
        // TODO: 填充程序列表
    }
    
    bool MainWindow::GetSelectedProgram(ProgramInfo& program) {
        // TODO: 获取选中的程序
        return false;
    }
    
    void MainWindow::UpdateUIState() {
        // TODO: 更新UI状态
    }
    
    void MainWindow::AdjustLayout() {
        if (!m_hWnd) return;
        
        RECT clientRect;
        GetClientRect(m_hWnd, &clientRect);
        int width = clientRect.right - clientRect.left;
        int height = clientRect.bottom - clientRect.top;
        
        // 调整工具栏
        if (m_hToolbar) {
            SendMessage(m_hToolbar, TB_AUTOSIZE, 0, 0);
        }
        
        // 调整状态栏
        if (m_hStatusBar) {
            SendMessage(m_hStatusBar, WM_SIZE, 0, 0);
        }
        
        // 获取工具栏和状态栏高度
        RECT toolbarRect = {0};
        RECT statusRect = {0};
        int toolbarHeight = 0;
        int statusHeight = 0;
        
        if (m_hToolbar) {
            GetWindowRect(m_hToolbar, &toolbarRect);
            toolbarHeight = toolbarRect.bottom - toolbarRect.top;
        }
        
        if (m_hStatusBar) {
            GetWindowRect(m_hStatusBar, &statusRect);
            statusHeight = statusRect.bottom - statusRect.top;
        }
        
        // 调整搜索框
        if (m_hSearchEdit) {
            SetWindowPos(m_hSearchEdit, nullptr, 55, toolbarHeight + 8, 200, 24, 
                        SWP_NOZORDER | SWP_NOACTIVATE);
        }
        
        // 调整列表视图
        if (m_hListView) {
            int listTop = toolbarHeight + 40;
            int listHeight = height - listTop - statusHeight - 10;
            SetWindowPos(m_hListView, nullptr, 10, listTop, width - 20, listHeight,
                        SWP_NOZORDER | SWP_NOACTIVATE);
        }
    }
    
    void MainWindow::LoadWindowSettings() {
        // TODO: 加载窗口设置
    }
    
    void MainWindow::SaveWindowSettings() {
        // TODO: 保存窗口设置
    }
    
    void MainWindow::OnScanProgress(int percentage, const String& currentItem) {
        // TODO: 处理扫描进度
    }
    
    void MainWindow::OnScanCompleted(const std::vector<ProgramInfo>& programs, ErrorCode result) {
        // TODO: 处理扫描完成
    }
    
    void MainWindow::OnUninstallProgress(int percentage, const String& message) {
        // TODO: 处理卸载进度
    }
    
    void MainWindow::OnUninstallCompleted(const UninstallResult& result) {
        // TODO: 处理卸载完成
    }
    
    void MainWindow::ShowError(const String& title, const String& message, ErrorCode errorCode) {
        ErrorHandler::ShowErrorDialog(title, message, errorCode);
    }
    
    bool MainWindow::ShowConfirmation(const String& title, const String& message) {
        int result = MessageBoxW(m_hWnd, message.c_str(), title.c_str(), MB_YESNO | MB_ICONQUESTION);
        return (result == IDYES);
    }
    
    void MainWindow::AddSamplePrograms() {
        // 添加一些示例程序到列表中
        struct SampleProgram {
            const wchar_t* name;
            const wchar_t* version;
            const wchar_t* publisher;
            const wchar_t* size;
            const wchar_t* date;
        };
        
        SampleProgram samples[] = {
            {L"Google Chrome", L"119.0.6045.160", L"Google LLC", L"280 MB", L"2023-11-15"},
            {L"Microsoft Office", L"16.0.16827.20166", L"Microsoft Corporation", L"3.2 GB", L"2023-10-20"},
            {L"Adobe Acrobat Reader", L"23.008.20458", L"Adobe Inc.", L"156 MB", L"2023-11-01"},
            {L"VLC Media Player", L"3.0.18", L"VideoLAN", L"42 MB", L"2023-09-15"},
            {L"7-Zip", L"22.01", L"Igor Pavlov", L"12 MB", L"2023-08-10"},
            {L"Notepad++", L"8.5.8", L"Notepad++ Team", L"8.5 MB", L"2023-10-05"},
            {L"WinRAR", L"6.24", L"win.rar GmbH", L"3.8 MB", L"2023-09-28"},
            {L"Steam", L"3.4.5.76", L"Valve Corporation", L"3.1 MB", L"2023-11-10"},
        };
        
        // 清空现有项目
        ListView_DeleteAllItems(m_hListView);
        
        // 添加示例程序
        LVITEMW item = {};
        item.mask = LVIF_TEXT;
        
        for (int i = 0; i < sizeof(samples) / sizeof(samples[0]); i++) {
            item.iItem = i;
            item.iSubItem = 0;
            item.pszText = (LPWSTR)samples[i].name;
            int index = ListView_InsertItem(m_hListView, &item);
            
            if (index != -1) {
                // 设置子项
                ListView_SetItemText(m_hListView, index, 1, (LPWSTR)samples[i].version);
                ListView_SetItemText(m_hListView, index, 2, (LPWSTR)samples[i].publisher);
                ListView_SetItemText(m_hListView, index, 3, (LPWSTR)samples[i].size);
                ListView_SetItemText(m_hListView, index, 4, (LPWSTR)samples[i].date);
            }
        }
        
        // 更新状态栏
        SetStatusText(L"已加载 " + std::to_wstring(sizeof(samples) / sizeof(samples[0])) + L" 个程序");
    }
    
} // namespace YG
