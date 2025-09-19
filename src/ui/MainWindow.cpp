/**
 * @file MainWindow.cpp
 * @brief 主窗口实现
 * @author gmrchzh@gmail.com
 * @version 1.0.0
 * @date 2025-09-17
 */

#include "ui/MainWindow.h"
#include "core/Logger.h"
#include "core/ErrorHandler.h"
#include "services/DirectProgramScanner.h"
#include "../resources/resource.h"  // 包含资源定义
#include <cstdio>  // 为swprintf_s提供支持
#include <commctrl.h>  // ListView控件支持
#include <windowsx.h>  // 窗口消息宏
#include <algorithm>   // 为std::sort提供支持

namespace YG {
    
    MainWindow::MainWindow() : m_hWnd(nullptr), m_hInstance(nullptr),
                              m_hMenu(nullptr), m_hContextMenu(nullptr),
                              m_hToolbar(nullptr), m_hStatusBar(nullptr), m_hListView(nullptr),
                              m_hSearchEdit(nullptr), m_hProgressBar(nullptr), m_hImageList(nullptr),
                              m_includeSystemComponents(false), m_isInTray(false),
                              m_isScanning(false), m_isUninstalling(false), m_isListViewMode(false),
                              m_sortColumn(0), m_sortAscending(true) {
        
        // 初始化托盘数据结构
        ZeroMemory(&m_nid, sizeof(NOTIFYICONDATAW));
    }
    
    MainWindow::~MainWindow() {
        // 清理系统托盘
        if (m_isInTray) {
            ShowSystemTray(false);
        }
        
        if (m_hImageList) {
            ImageList_Destroy(m_hImageList);
            m_hImageList = nullptr;
        }
        if (m_hWnd) {
            DestroyWindow(m_hWnd);
        }
    }
    
    ErrorCode MainWindow::Create(HINSTANCE hInstance) {
        m_hInstance = hInstance;
        
        YG_LOG_INFO(L"开始创建主窗口");
        
        // 注册窗口类
        WNDCLASSEXW wcex;
        ZeroMemory(&wcex, sizeof(WNDCLASSEXW));
        wcex.cbSize = sizeof(WNDCLASSEXW);
        wcex.style = CS_HREDRAW | CS_VREDRAW;
        wcex.lpfnWndProc = WindowProc;
        wcex.cbClsExtra = 0;
        wcex.cbWndExtra = 0;
        wcex.hInstance = hInstance;
        // 加载程序图标
        HICON hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_MAIN_ICON));
        if (!hIcon) {
            // 如果加载失败，使用默认应用程序图标
            hIcon = LoadIcon(nullptr, IDI_APPLICATION);
            YG_LOG_WARNING(L"无法加载程序图标，使用默认图标");
        } else {
            YG_LOG_INFO(L"程序图标加载成功");
        }
        wcex.hIcon = hIcon;       // 大图标
        wcex.hIconSm = hIcon;     // 小图标
        wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
        wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
        wcex.lpszMenuName = nullptr;
        wcex.lpszClassName = WINDOW_CLASS_NAME;
        
        if (!RegisterClassExW(&wcex)) {
            DWORD error = GetLastError();
            YG_LOG_ERROR(L"窗口类注册失败，错误代码: " + std::to_wstring(error));
            return ErrorCode::GeneralError;
        }
        
        YG_LOG_INFO(L"窗口类注册成功");
        
        // 创建窗口
        YG_LOG_INFO(L"开始创建窗口");
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
            DWORD error = GetLastError();
            YG_LOG_ERROR(L"窗口创建失败，错误代码: " + std::to_wstring(error));
            return ErrorCode::GeneralError;
        }
        
        // 明确设置窗口图标（确保标题栏显示图标）
        if (hIcon) {
            SendMessage(m_hWnd, WM_SETICON, ICON_BIG, (LPARAM)hIcon);    // 大图标（标题栏和任务栏）
            SendMessage(m_hWnd, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);  // 小图标（标题栏）
            YG_LOG_INFO(L"窗口图标设置成功");
        } else {
            YG_LOG_WARNING(L"无法加载窗口图标");
        }
        
        YG_LOG_INFO(L"窗口创建成功，句柄: " + std::to_wstring(reinterpret_cast<uintptr_t>(m_hWnd)));
        
        // 窗口创建成功后，创建简单的代码菜单来替代资源文件菜单
        ErrorCode menuResult = CreateMenus();
        if (menuResult != ErrorCode::Success) {
            YG_LOG_WARNING(L"菜单创建失败，但程序继续运行");
        }
        
        YG_LOG_INFO(L"主窗口创建成功");
        return ErrorCode::Success;
    }
    
    void MainWindow::Show(int nCmdShow) {
        YG_LOG_INFO(L"尝试显示窗口，显示模式: " + std::to_wstring(nCmdShow));
        if (m_hWnd) {
            BOOL showResult = ShowWindow(m_hWnd, nCmdShow);
            YG_LOG_INFO(L"ShowWindow返回值: " + std::to_wstring(showResult));
            
            BOOL updateResult = UpdateWindow(m_hWnd);
            YG_LOG_INFO(L"UpdateWindow返回值: " + std::to_wstring(updateResult));
            
            // 确保窗口在前台
            SetForegroundWindow(m_hWnd);
            YG_LOG_INFO(L"窗口显示完成");
        } else {
            YG_LOG_ERROR(L"窗口句柄为空，无法显示");
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
        
        YG_LOG_INFO(L"开始刷新程序列表，包含系统组件: " + String(includeSystemComponents ? L"是" : L"否"));
        
        SetStatusText(L"正在扫描已安装的程序...");
        
        // 显示进度条
        UpdateProgress(0, true);
        
        // 在编辑框中显示扫描状态
        if (m_hListView) {
            SetWindowTextW(m_hListView, L"正在扫描64位 Windows 系统上的已安装程序...\r\n\r\n请耐心等待，这可能需要几秒钟时间。");
        }
        
        // 使用ProgramDetector进行扫描
        if (!m_programDetector) {
            m_programDetector = YG::MakeUnique<ProgramDetector>();
        }
        
        std::vector<ProgramInfo> programs;
        ErrorCode result = m_programDetector->ScanSync(includeSystemComponents, programs);
        
        if (result == ErrorCode::Success) {
            YG_LOG_INFO(L"扫描完成，找到 " + std::to_wstring(programs.size()) + L" 个程序");
            
            // 更新进度到100%
            UpdateProgress(100, true);
            
            // 短暂显示完成状态，然后隐藏进度条
            Sleep(500);
            UpdateProgress(0, false);
            
            PopulateProgramList(programs);
        } else {
            YG_LOG_ERROR(L"程序扫描失败，错误代码: " + std::to_wstring(static_cast<int>(result)));
            
            // 隐藏进度条
            UpdateProgress(0, false);
            
            // 显示扫描失败信息
            if (m_hListView) {
                SetWindowTextW(m_hListView, L"程序扫描失败！\r\n\r\n可能的原因：\r\n• 缺少管理员权限\r\n• 注册表访问被限制\r\n• 系统安全软件阻止\r\n\r\n请尝试以管理员身份运行程序，或检查系统安全设置。");
            }
            SetStatusText(L"程序扫描失败 - 请检查权限和系统设置");
        }
        
        YG_LOG_INFO(L"程序列表刷新完成");
    }
    
    void MainWindow::SearchPrograms(const String& keyword) {
        m_currentSearchKeyword = keyword;
        YG_LOG_INFO(L"搜索程序: " + keyword);
        
        if (keyword.empty()) {
            // 空搜索，显示所有程序
            m_filteredPrograms = m_programs;
        } else {
            // 执行搜索过滤
            m_filteredPrograms.clear();
            String lowerKeyword = keyword;
            std::transform(lowerKeyword.begin(), lowerKeyword.end(), lowerKeyword.begin(), ::towlower);
            
            for (const auto& program : m_programs) {
                // 获取程序名称
                String programName = !program.displayName.empty() ? program.displayName : program.name;
                String lowerName = programName;
                std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::towlower);
                
                // 获取发布者名称
                String lowerPublisher = program.publisher;
                std::transform(lowerPublisher.begin(), lowerPublisher.end(), lowerPublisher.begin(), ::towlower);
                
                // 检查是否匹配
                if (lowerName.find(lowerKeyword) != String::npos ||
                    lowerPublisher.find(lowerKeyword) != String::npos ||
                    program.version.find(keyword) != String::npos) {
                    m_filteredPrograms.push_back(program);
                }
            }
        }
        
        // 更新显示
        PopulateProgramList(m_filteredPrograms);
        
        // 更新状态栏
        if (keyword.empty()) {
            SetStatusText(L"显示所有程序 - " + std::to_wstring(m_filteredPrograms.size()) + L" 个程序");
        } else {
            SetStatusText(L"搜索结果 - 找到 " + std::to_wstring(m_filteredPrograms.size()) + L" 个匹配的程序");
        }
        
        YG_LOG_INFO(L"搜索完成，找到 " + std::to_wstring(m_filteredPrograms.size()) + L" 个匹配程序");
    }
    
    void MainWindow::UninstallSelectedProgram(UninstallMode mode) {
        ProgramInfo selectedProgram;
        if (!GetSelectedProgram(selectedProgram)) {
            MessageBoxW(m_hWnd, L"请先选择要卸载的程序。", L"提示", MB_OK | MB_ICONINFORMATION);
            return;
        }
        
        // 确认卸载
        String programName = !selectedProgram.displayName.empty() ? selectedProgram.displayName : selectedProgram.name;
        String confirmMessage;
        if (mode == UninstallMode::Force) {
            confirmMessage = L"确定要强制卸载 \"" + programName + L"\" 吗？\n\n注意：强制卸载可能导致系统不稳定。";
        } else {
            confirmMessage = L"确定要卸载 \"" + programName + L"\" 吗？";
        }
        
        int result = MessageBoxW(m_hWnd, confirmMessage.c_str(), L"确认卸载", MB_YESNO | MB_ICONQUESTION);
        if (result != IDYES) {
            return;
        }
        
        // 检查是否有卸载字符串
        if (selectedProgram.uninstallString.empty()) {
            MessageBoxW(m_hWnd, L"该程序没有提供卸载信息，无法自动卸载。", L"无法卸载", MB_OK | MB_ICONWARNING);
            return;
        }
        
        // 创建卸载服务
        if (!m_uninstallerService) {
            m_uninstallerService = YG::MakeUnique<UninstallerService>();
        }
        
        // 启动卸载
        SetStatusText(L"正在卸载 " + programName + L"...");
        ErrorCode uninstallResult = m_uninstallerService->UninstallProgram(selectedProgram, mode);
        
        if (uninstallResult == ErrorCode::Success) {
            MessageBoxW(m_hWnd, (L"程序 \"" + programName + L"\" 卸载完成！").c_str(), 
                       L"卸载成功", MB_OK | MB_ICONINFORMATION);
            // 刷新程序列表
            RefreshProgramList(m_includeSystemComponents);
            SetStatusText(L"卸载完成");
        } else {
            String errorMsg = L"程序卸载失败！\n\n程序：" + programName;
            MessageBoxW(m_hWnd, errorMsg.c_str(), L"卸载失败", MB_OK | MB_ICONERROR);
            SetStatusText(L"卸载失败");
        }
        
        YG_LOG_INFO(L"开始卸载程序: " + programName);
    }
    
    void MainWindow::ShowProgramDetails(const ProgramInfo& program) {
        YG_LOG_INFO(L"显示程序详细信息: " + program.name);
        
        // 构建详细信息文本
        String programName = !program.displayName.empty() ? program.displayName : program.name;
        String details = L"程序详细信息\n";
        details += L"━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n\n";
        
        details += L"📋 基本信息:\n";
        details += L"  程序名称: " + programName + L"\n";
        if (!program.version.empty()) {
            details += L"  版本号: " + program.version + L"\n";
        }
        if (!program.publisher.empty()) {
            details += L"  发布者: " + program.publisher + L"\n";
        }
        
        details += L"\n📁 安装信息:\n";
        if (!program.installLocation.empty()) {
            details += L"  安装位置: " + program.installLocation + L"\n";
        }
        if (!program.installDate.empty()) {
            String formattedDate = FormatInstallDate(program.installDate);
            details += L"  安装日期: " + formattedDate + L"\n";
        }
        if (program.estimatedSize > 0) {
            details += L"  程序大小: " + FormatFileSize(program.estimatedSize) + L"\n";
        }
        
        details += L"\n🔧 卸载信息:\n";
        if (!program.uninstallString.empty()) {
            details += L"  卸载命令: " + program.uninstallString + L"\n";
        }
        if (!program.iconPath.empty()) {
            details += L"  图标路径: " + program.iconPath + L"\n";
        }
        
        details += L"\n🏷️ 程序属性:\n";
        details += L"  系统组件: " + String(program.isSystemComponent ? L"是" : L"否") + L"\n";
        
        // 显示详细信息对话框
        MessageBoxW(m_hWnd, details.c_str(), (L"程序详细信息 - " + programName).c_str(), 
                   MB_OK | MB_ICONINFORMATION);
    }
    
    void MainWindow::SetStatusText(const String& text) {
        if (m_hStatusBar) {
            // 设置状态栏文本
            SendMessageW(m_hStatusBar, SB_SETTEXTW, 0, (LPARAM)text.c_str());
        }
        YG_LOG_INFO(L"状态文本: " + text);
    }
    
    void MainWindow::UpdateProgress(int percentage, bool visible) {
        if (!m_hProgressBar) {
            return;
        }
        
        // 显示或隐藏进度条
        ShowWindow(m_hProgressBar, visible ? SW_SHOW : SW_HIDE);
        
        if (visible) {
            // 设置进度值
            SendMessage(m_hProgressBar, PBM_SETPOS, percentage, 0);
            
            // 更新状态栏文本
            String progressText = L"进度: " + std::to_wstring(percentage) + L"%";
            SetStatusText(progressText);
            
            YG_LOG_INFO(L"进度更新: " + std::to_wstring(percentage) + L"%");
        }
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
            case WM_TRAYICON:
                OnTrayNotify(wParam, lParam);
                return 0;
            case WM_CLOSE:
                // 关闭窗口时最小化到托盘而不是退出
                MinimizeToTray();
                return 0;
            case WM_SHOWWINDOW:
                {
                    if (wParam && !m_hListView) {  // 窗口显示且控件还未创建
                        YG_LOG_INFO(L"窗口显示，开始创建控件");
                        ErrorCode result = CreateControls();
                        if (result == ErrorCode::Success) {
                            YG_LOG_INFO(L"控件创建成功，开始扫描真实程序列表");
                            
                            // 立即开始真实的程序扫描
                            RefreshProgramList(m_includeSystemComponents);
                            
                        } else {
                            YG_LOG_ERROR(L"控件创建失败");
                            MessageBoxW(m_hWnd, L"程序初始化失败，无法创建程序列表显示控件。", L"错误", MB_OK | MB_ICONERROR);
                        }
                    }
                }
                return DefWindowProc(hWnd, uMsg, wParam, lParam);
            case WM_TIMER:
                {
                    if (wParam == 1) {
                        // 定时器触发，开始扫描真实程序列表
                        KillTimer(m_hWnd, 1);  // 停止定时器
                        YG_LOG_INFO(L"定时器触发，开始扫描真实程序列表");
                        PostMessage(m_hWnd, WM_COMMAND, ID_ACTION_REFRESH, 0);
                    }
                }
                return 0;
            default:
                return DefWindowProc(hWnd, uMsg, wParam, lParam);
        }
    }
    
    LRESULT MainWindow::OnCreate(HWND hWnd, LPARAM lParam) {
        (void)hWnd;    // 抑制警告
        (void)lParam;
        
        YG_LOG_INFO(L"主窗口创建开始");
        
        // 直接创建控件，但先等待程序完全初始化
        YG_LOG_INFO(L"将在窗口显示时创建控件");
        
        return 0;
    }
    
    LRESULT MainWindow::OnDestroy() {
        PostQuitMessage(0);
        return 0;
    }
    
    LRESULT MainWindow::OnSize(int width, int height) {
        (void)width;   // 抑制警告
        (void)height;
        
        // 调整控件布局
        AdjustLayout();
        return 0;
    }
    
    LRESULT MainWindow::OnCommand(WORD commandId, WORD notificationCode, HWND controlHandle) {
        // 处理搜索框文本变化
        if (commandId == ID_SEARCH_EDIT && notificationCode == EN_CHANGE) {
            // 获取搜索框文本
            wchar_t searchText[256] = {0};
            GetWindowTextW(m_hSearchEdit, searchText, sizeof(searchText)/sizeof(wchar_t));
            
            // 执行搜索
            SearchPrograms(String(searchText));
            return 0;
        }
        
        switch (commandId) {
            case 1001:  // 扫描已安装的程序按钮
                {
                    YG_LOG_INFO(L"用户点击了扫描按钮");
                    SetStatusText(L"正在扫描已安装的程序...");
                    
                    // 在编辑框中显示扫描结果
                    String scanResult = L"\r\n\r\n===== 扫描结果 =====\r\n";
                    scanResult += L"扫描时间: 2025-09-19 11:45:00\r\n";
                    scanResult += L"扫描路径: 注册表 + 程序文件夹\r\n";
                    scanResult += L"扫描结果: 找到 8 个已安装的程序\r\n";
                    scanResult += L"\r\n详细列表如上所示。";
                    
                    // 获取现有文本并添加扫描结果
                    int textLen = GetWindowTextLengthW(m_hListView);
                    if (textLen > 0) {
                        wchar_t* buffer = new wchar_t[textLen + 1];
                        GetWindowTextW(m_hListView, buffer, textLen + 1);
                        String currentText(buffer);
                        delete[] buffer;
                        
                        SetWindowTextW(m_hListView, (currentText + scanResult).c_str());
                    }
                    
                    SetStatusText(L"扫描完成 - 找到 8 个程序");
                    YG_LOG_INFO(L"扫描模拟完成");
                }
                break;
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
                    ProgramInfo selectedProgram;
                    if (GetSelectedProgram(selectedProgram)) {
                        UninstallSelectedProgram(UninstallMode::Standard);
                    } else {
                        MessageBoxW(m_hWnd, L"请先选择要卸载的程序。", L"提示", MB_OK | MB_ICONWARNING);
                    }
                }
                break;
            case ID_ACTION_FORCE_UNINSTALL:
            case ID_TB_FORCE_UNINSTALL:
                {
                    ProgramInfo selectedProgram;
                    if (GetSelectedProgram(selectedProgram)) {
                        UninstallSelectedProgram(UninstallMode::Force);
                    } else {
                        MessageBoxW(m_hWnd, L"请先选择要强制卸载的程序。", L"提示", MB_OK | MB_ICONWARNING);
                    }
                }
                break;
            case ID_ACTION_DEEP_UNINSTALL:
                {
                    ProgramInfo selectedProgram;
                    if (GetSelectedProgram(selectedProgram)) {
                        UninstallSelectedProgram(UninstallMode::Deep);
                    } else {
                        MessageBoxW(m_hWnd, L"请先选择要深度卸载的程序。", L"提示", MB_OK | MB_ICONWARNING);
                    }
                }
                break;
            case ID_ACTION_BATCH_UNINSTALL:
                BatchUninstallSelectedPrograms(UninstallMode::Standard);
                break;
            case ID_ACTION_SELECT_ALL:
                SelectAllPrograms(true);
                break;
            case ID_TB_SEARCH:
                // 搜索按钮点击，触发搜索
                if (m_hSearchEdit) {
                    wchar_t searchText[256] = {0};
                    GetWindowTextW(m_hSearchEdit, searchText, 255);
                    SearchPrograms(String(searchText));
                }
                break;
                
            // 操作菜单 - 其他项目
            case ID_ACTION_PROPERTIES:
                {
                    ProgramInfo selectedProgram;
                    if (GetSelectedProgram(selectedProgram)) {
                        String programName = !selectedProgram.displayName.empty() ? selectedProgram.displayName : selectedProgram.name;
                        String propText = L"程序属性：\n\n";
                        propText += L"程序名称：" + programName + L"\n";
                        propText += L"版本：" + selectedProgram.version + L"\n";
                        propText += L"发布者：" + selectedProgram.publisher + L"\n";
                        propText += L"安装日期：" + selectedProgram.installDate + L"\n";
                        if (selectedProgram.estimatedSize > 0) {
                            propText += L"大小：" + FormatFileSize(selectedProgram.estimatedSize) + L"\n";
                        }
                        propText += L"\n实际项目中这里会显示详细属性对话框。";
                        MessageBoxW(m_hWnd, propText.c_str(), L"程序属性", MB_OK | MB_ICONINFORMATION);
                    } else {
                        MessageBoxW(m_hWnd, L"请先选择一个程序。", L"提示", MB_OK | MB_ICONWARNING);
                    }
                }
                break;
            case ID_ACTION_OPEN_LOCATION:
                {
                    ProgramInfo selectedProgram;
                    if (GetSelectedProgram(selectedProgram)) {
                        String programName = !selectedProgram.displayName.empty() ? selectedProgram.displayName : selectedProgram.name;
                        String locationText = L"打开安装位置：\n\n";
                        locationText += L"程序：" + programName + L"\n";
                        locationText += L"位置：" + selectedProgram.installLocation + L"\n";
                        locationText += L"\n实际项目中这里会打开文件资源管理器。";
                        MessageBoxW(m_hWnd, locationText.c_str(), L"打开位置", MB_OK | MB_ICONINFORMATION);
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
                
            // 设置菜单
            case ID_SETTINGS_GENERAL:
            case ID_SETTINGS_ADVANCED:
                ShowSettingsDialog();
                break;
                
            // 托盘菜单
            case ID_TRAY_RESTORE:
                RestoreFromTray();
                break;
            case ID_TRAY_EXIT:
                // 真正退出程序
                ShowSystemTray(false);
                PostQuitMessage(0);
                break;
                
            default:
                return DefWindowProc(m_hWnd, WM_COMMAND, MAKEWPARAM(commandId, notificationCode), (LPARAM)controlHandle);
        }
        return 0;
    }
    
    LRESULT MainWindow::OnNotify(LPNMHDR pNMHDR) {
        if (!pNMHDR) return 0;
        
        // 处理ListView通知消息
        if (pNMHDR->hwndFrom == m_hListView) {
            switch (pNMHDR->code) {
                case LVN_COLUMNCLICK:
                {
                    // 列标题点击事件
                    LPNMLISTVIEW pNMLV = (LPNMLISTVIEW)pNMHDR;
                    OnColumnHeaderClick(pNMLV->iSubItem);
                    return 0;
                }
                default:
                    break;
            }
        }
        
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
                if (GetKeyState(VK_CONTROL) & 0x8000) {
                    // Ctrl+Delete - 批量卸载
                    SendMessage(m_hWnd, WM_COMMAND, ID_ACTION_BATCH_UNINSTALL, 0);
                } else if (GetKeyState(VK_SHIFT) & 0x8000) {
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
            case 'A':
                // Ctrl+A - 全选
                if (GetKeyState(VK_CONTROL) & 0x8000) {
                    SendMessage(m_hWnd, WM_COMMAND, ID_ACTION_SELECT_ALL, 0);
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
        
        // 检查是否有选中的项目（适应列表框）
        int selectedIndex = static_cast<int>(SendMessage(m_hListView, LB_GETCURSEL, 0, 0));
        if (selectedIndex != LB_ERR) {
            // 显示右键菜单
            TrackPopupMenu(m_hContextMenu, TPM_RIGHTBUTTON, x, y, 0, m_hWnd, nullptr);
        }
        
        return 0;
    }
    
    ErrorCode MainWindow::CreateControls() {
        YG_LOG_INFO(L"开始创建控件...");
        
        // 等待窗口完全初始化
        Sleep(100);  // 等待100毫秒
        
        // 初始化Common Controls
        INITCOMMONCONTROLSEX icex;
        icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
        icex.dwICC = ICC_LISTVIEW_CLASSES | ICC_BAR_CLASSES;
        InitCommonControlsEx(&icex);
        
        // 获取窗口客户区宽度
        RECT clientRect;
        GetClientRect(m_hWnd, &clientRect);
        int windowWidth = clientRect.right - clientRect.left;
        int initialWidth = windowWidth - 10; // 减去左右边距
        
        // 确保最小宽度
        int minWidth = CalculateTableWidth();
        if (initialWidth < minWidth) {
            initialWidth = minWidth;
        }
        
        // 创建 ListView 控件用于表格显示（无边框，动态宽度，5px边距）
        m_hListView = CreateWindowExW(
            0,  // 移除 WS_EX_CLIENTEDGE 扩展边框样式
            WC_LISTVIEWW,
            L"",
            WS_CHILD | WS_VISIBLE | LVS_REPORT,  // 移除LVS_SINGLESEL，支持多选
            5,       // x (左侧5px边距)
            GetSystemMetrics(SM_CYMENU) + 3,  // y (菜单栏下方，为进度条留出空间)
            initialWidth,  // width - 动态宽度
            500,     // height
            m_hWnd,  // 父窗口句柄
            (HMENU)2001,  // 控件ID
            GetModuleHandle(NULL),  // 实例
            NULL     // 参数
        );
        
        if (!m_hListView) {
            DWORD error = GetLastError();
            YG_LOG_ERROR(L"ListView创建失败，错误: " + std::to_wstring(error));
            
            // 如果ListView创建失败，使用编辑框作为备用方案（无边框，动态宽度，5px边距）
            YG_LOG_INFO(L"使用编辑框作为备用方案");
            m_hListView = CreateWindowExW(
                0,  // 移除 WS_EX_CLIENTEDGE 边框样式
                L"EDIT",
                L"程序已启动，正在初始化表格显示...",
                WS_CHILD | WS_VISIBLE | ES_MULTILINE | ES_READONLY | WS_VSCROLL | ES_AUTOVSCROLL,
                5, 0, initialWidth, 500,  // 左侧5px边距，使用动态宽度
                m_hWnd,
                (HMENU)2001,
                GetModuleHandle(NULL),
                NULL
            );
            
            if (!m_hListView) {
                DWORD error2 = GetLastError();
                YG_LOG_ERROR(L"备用编辑框也创建失败，错误: " + std::to_wstring(error2));
                return ErrorCode::GeneralError;
            }
            m_isListViewMode = false; // 记录使用的是编辑框模式
        } else {
            m_isListViewMode = true; // 记录使用的是ListView模式
            // 设置ListView的扩展样式以优化用户体验
            ListView_SetExtendedListViewStyle(m_hListView, 
                LVS_EX_FULLROWSELECT |      // 整行选中高亮
                LVS_EX_GRIDLINES |          // 网格线
                LVS_EX_INFOTIP |            // 信息提示
                LVS_EX_DOUBLEBUFFER |       // 双缓冲，减少闪烁
                LVS_EX_SUBITEMIMAGES);      // 子项图标支持
            
            // 创建图标列表
            CreateImageList();
            
            // 创建列表头
            InitializeListViewColumns();
        }
        
        YG_LOG_INFO(L"程序列表显示控件创建成功");
        
        // 创建状态栏
        m_hStatusBar = CreateWindowExW(
            0,
            STATUSCLASSNAMEW,
            L"",
            WS_CHILD | WS_VISIBLE | SBARS_SIZEGRIP,
            0, 0, 0, 0,
            m_hWnd,
            (HMENU)2002,
            GetModuleHandle(NULL),
            NULL
        );
        
        if (m_hStatusBar) {
            SetStatusText(L"程序已启动 - 表格初始化完成");
            YG_LOG_INFO(L"状态栏创建成功");
        } else {
            YG_LOG_WARNING(L"状态栏创建失败，但继续运行");
        }
        
        YG_LOG_INFO(L"所有控件创建完成");
        return ErrorCode::Success;
    }
    
    ErrorCode MainWindow::CreateMenus() {
        YG_LOG_INFO(L"开始创建代码菜单...");
        
        // 创建主菜单
        m_hMenu = CreateMenu();
        if (!m_hMenu) {
            DWORD error = GetLastError();
            YG_LOG_ERROR(L"创建主菜单失败，错误代码: " + std::to_wstring(error));
            return ErrorCode::GeneralError;
        }
        
        // 创建“文件”菜单
        HMENU hFileMenu = CreatePopupMenu();
        if (hFileMenu) {
            AppendMenuW(hFileMenu, MF_STRING, ID_FILE_EXIT, L"退出(&X)");
            AppendMenuW(m_hMenu, MF_POPUP, (UINT_PTR)hFileMenu, L"文件(&F)");
        }
        
        // 创建“操作”菜单
        HMENU hActionMenu = CreatePopupMenu();
        if (hActionMenu) {
            AppendMenuW(hActionMenu, MF_STRING, ID_ACTION_REFRESH, L"刷新程序列表(&R)\tF5");
            AppendMenuW(hActionMenu, MF_SEPARATOR, 0, nullptr);
            AppendMenuW(hActionMenu, MF_STRING, ID_ACTION_UNINSTALL, L"卸载程序(&U)\tDel");
            AppendMenuW(hActionMenu, MF_STRING, ID_ACTION_FORCE_UNINSTALL, L"强制卸载(&F)\tShift+Del");
            AppendMenuW(hActionMenu, MF_SEPARATOR, 0, nullptr);
            AppendMenuW(hActionMenu, MF_STRING, ID_ACTION_DEEP_UNINSTALL, L"深度卸载(&D)\tCtrl+Shift+Del");
            AppendMenuW(hActionMenu, MF_SEPARATOR, 0, nullptr);
            AppendMenuW(hActionMenu, MF_STRING, ID_ACTION_BATCH_UNINSTALL, L"批量卸载(&B)\tCtrl+Del");
            AppendMenuW(hActionMenu, MF_STRING, ID_ACTION_SELECT_ALL, L"全选(&A)\tCtrl+A");
            AppendMenuW(hActionMenu, MF_SEPARATOR, 0, nullptr);
            AppendMenuW(hActionMenu, MF_STRING, ID_ACTION_PROPERTIES, L"属性(&P)\tAlt+Enter");
            AppendMenuW(hActionMenu, MF_STRING, ID_ACTION_OPEN_LOCATION, L"打开安装位置(&L)\tCtrl+O");
            AppendMenuW(m_hMenu, MF_POPUP, (UINT_PTR)hActionMenu, L"操作(&A)");
        }
        
        // 创建“查看”菜单
        HMENU hViewMenu = CreatePopupMenu();
        if (hViewMenu) {
            AppendMenuW(hViewMenu, MF_STRING | MF_CHECKED, ID_VIEW_TOOLBAR, L"工具栏(&T)");
            AppendMenuW(hViewMenu, MF_STRING | MF_CHECKED, ID_VIEW_STATUSBAR, L"状态栏(&S)");
            AppendMenuW(hViewMenu, MF_SEPARATOR, 0, nullptr);
            AppendMenuW(hViewMenu, MF_STRING, ID_VIEW_SHOW_SYSTEM, L"显示系统组件(&C)");
            AppendMenuW(hViewMenu, MF_STRING, ID_VIEW_REFRESH, L"刷新(&R)\tF5");
            AppendMenuW(m_hMenu, MF_POPUP, (UINT_PTR)hViewMenu, L"查看(&V)");
        }
        
        // 创建"设置"菜单
        HMENU hSettingsMenu = CreatePopupMenu();
        if (hSettingsMenu) {
            AppendMenuW(hSettingsMenu, MF_STRING, ID_SETTINGS_GENERAL, L"常规设置(&G)");
            AppendMenuW(hSettingsMenu, MF_STRING, ID_SETTINGS_ADVANCED, L"高级设置(&A)");
            AppendMenuW(hSettingsMenu, MF_SEPARATOR, 0, nullptr);
            AppendMenuW(hSettingsMenu, MF_STRING, ID_VIEW_SHOW_SYSTEM, L"显示系统组件(&S)");
            AppendMenuW(m_hMenu, MF_POPUP, (UINT_PTR)hSettingsMenu, L"设置(&S)");
        }
        
        // 创建"帮助"菜单
        HMENU hHelpMenu = CreatePopupMenu();
        if (hHelpMenu) {
            AppendMenuW(hHelpMenu, MF_STRING, ID_HELP_HELP, L"帮助主题(&H)\tF1");
            AppendMenuW(hHelpMenu, MF_SEPARATOR, 0, nullptr);
            AppendMenuW(hHelpMenu, MF_STRING, ID_HELP_WEBSITE, L"访问官网(&W)");
            AppendMenuW(hHelpMenu, MF_STRING, ID_HELP_CHECK_UPDATE, L"检查更新(&U)");
            AppendMenuW(hHelpMenu, MF_SEPARATOR, 0, nullptr);
            AppendMenuW(hHelpMenu, MF_STRING, ID_HELP_ABOUT, L"关于 YG Uninstaller(&A)");
            AppendMenuW(m_hMenu, MF_POPUP, (UINT_PTR)hHelpMenu, L"帮助(&H)");
        }
        
        // 设置窗口菜单
        if (m_hWnd && m_hMenu) {
            SetMenu(m_hWnd, m_hMenu);
            DrawMenuBar(m_hWnd);  // 强制重绘菜单栏
            YG_LOG_INFO(L"菜单设置成功");
        }
        
        // 创建菜单栏右侧的搜索控件
        CreateMenuBarSearchControls();
        
        // 创建系统托盘
        CreateSystemTray();
        
        YG_LOG_INFO(L"代码菜单创建完成");
        return ErrorCode::Success;
    }
    
    void MainWindow::CreateMenuBarSearchControls() {
        YG_LOG_INFO(L"开始创建菜单栏嵌入式搜索控件...");
        
        // 获取窗口客户区大小
        RECT clientRect;
        GetClientRect(m_hWnd, &clientRect);
        int windowWidth = clientRect.right - clientRect.left;
        
        // 获取菜单栏高度
        int menuHeight = GetSystemMetrics(SM_CYMENU);
        
        // 计算搜索控件的位置（嵌入菜单栏内部）
        int searchWidth = 180;      // 搜索框宽度（稍微减小以适应菜单栏）
        int buttonWidth = 24;       // 搜索按钮宽度（减小以适应菜单栏）
        int spacing = 3;            // 间距（减小）
        int rightMargin = 8;        // 右边距（减小）
        
        int searchX = windowWidth - rightMargin - searchWidth - buttonWidth - spacing;
        int searchY = 2;            // 菜单栏内的垂直位置（更紧贴顶部）
        int controlHeight = menuHeight - 4;  // 控件高度（菜单栏高度减去更小的边距）
        
        // 创建搜索框（无边框，融入菜单栏）
        m_hSearchEdit = CreateWindowExW(
            0,  // 移除边框，让控件融入菜单栏
            L"EDIT",
            L"",
            WS_CHILD | WS_VISIBLE | ES_LEFT | WS_BORDER,  // 添加简单边框
            searchX,
            searchY,
            searchWidth,
            controlHeight,
            m_hWnd,
            (HMENU)ID_SEARCH_EDIT,
            m_hInstance,
            nullptr
        );
        
        if (m_hSearchEdit) {
            // 设置搜索框提示文本
            SendMessageW(m_hSearchEdit, EM_SETCUEBANNER, TRUE, (LPARAM)L"搜索程序...");
            
            // 设置字体（使用更小的字体以适应菜单栏）
            HFONT hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
            SendMessage(m_hSearchEdit, WM_SETFONT, (WPARAM)hFont, TRUE);
            
            // 创建圆角区域
            HRGN hRgn = CreateRoundRectRgn(0, 0, searchWidth + 1, controlHeight + 1, 8, 8);
            if (hRgn) {
                SetWindowRgn(m_hSearchEdit, hRgn, TRUE);
                YG_LOG_INFO(L"搜索框圆角效果设置成功");
            } else {
                YG_LOG_WARNING(L"搜索框圆角效果设置失败");
            }
            
            // 设置搜索框的Z-order，确保在菜单栏之上
            SetWindowPos(m_hSearchEdit, HWND_TOP, 0, 0, 0, 0, 
                        SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
            
            YG_LOG_INFO(L"菜单栏嵌入式搜索框创建成功");
        } else {
            YG_LOG_ERROR(L"菜单栏嵌入式搜索框创建失败");
        }
        
        // 创建搜索按钮（平面按钮样式，融入菜单栏）
        HWND hSearchButton = CreateWindowExW(
            0,
            L"BUTTON",
            L"🔍",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | BS_FLAT,  // 平面按钮样式
            searchX + searchWidth + spacing,
            searchY,
            buttonWidth,
            controlHeight,
            m_hWnd,
            (HMENU)ID_TB_SEARCH,
            m_hInstance,
            nullptr
        );
        
        if (hSearchButton) {
            // 设置按钮字体
            HFONT hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
            SendMessage(hSearchButton, WM_SETFONT, (WPARAM)hFont, TRUE);
            
            // 创建圆角区域（按钮）
            HRGN hBtnRgn = CreateRoundRectRgn(0, 0, buttonWidth + 1, controlHeight + 1, 8, 8);
            if (hBtnRgn) {
                SetWindowRgn(hSearchButton, hBtnRgn, TRUE);
                YG_LOG_INFO(L"搜索按钮圆角效果设置成功");
            } else {
                YG_LOG_WARNING(L"搜索按钮圆角效果设置失败");
            }
            
            // 设置按钮的Z-order，确保在菜单栏之上
            SetWindowPos(hSearchButton, HWND_TOP, 0, 0, 0, 0, 
                        SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
            
            YG_LOG_INFO(L"菜单栏嵌入式搜索按钮创建成功");
        } else {
            YG_LOG_ERROR(L"菜单栏嵌入式搜索按钮创建失败");
        }
        
        // 创建进度条（放在菜单栏下方）
        m_hProgressBar = CreateWindowExW(
            0,
            PROGRESS_CLASSW,
            L"",
            WS_CHILD | PBS_SMOOTH,  // 初始隐藏，平滑进度条
            5,  // 左边距
            menuHeight,  // 紧贴菜单栏下方
            windowWidth - 10,  // 全宽度（减去左右边距）
            3,       // 很细的进度条
            m_hWnd,
            (HMENU)ID_PROGRESS_BAR,
            m_hInstance,
            nullptr
        );
        
        if (m_hProgressBar) {
            // 设置进度条范围
            SendMessage(m_hProgressBar, PBM_SETRANGE, 0, MAKELPARAM(0, 100));
            YG_LOG_INFO(L"菜单栏下方进度条创建成功");
        }
        
        YG_LOG_INFO(L"菜单栏嵌入式搜索控件创建完成");
    }
    
    ErrorCode MainWindow::CreateToolbar() {
        // 创建工具栏
        m_hToolbar = CreateWindowExW(
            0,
            TOOLBARCLASSNAMEW,
            L"",
            WS_CHILD | WS_VISIBLE | TBSTYLE_FLAT | TBSTYLE_TOOLTIPS,
            0, 0, 0, 0,
            m_hWnd,
            (HMENU)ID_TOOLBAR,
            m_hInstance,
            nullptr
        );
        
        if (!m_hToolbar) {
            YG_LOG_ERROR(L"创建工具栏失败");
            return ErrorCode::GeneralError;
        }
        
        // 设置工具栏按钮大小
        SendMessage(m_hToolbar, TB_BUTTONSTRUCTSIZE, sizeof(TBBUTTON), 0);
        SendMessage(m_hToolbar, TB_SETBITMAPSIZE, 0, MAKELONG(16, 16));
        
        // 定义工具栏按钮
        TBBUTTON buttons[] = {
            {0, ID_TB_REFRESH, TBSTATE_ENABLED, TBSTYLE_BUTTON, {0}, 0, (INT_PTR)L"刷新程序列表"},
            {0, 0, 0, TBSTYLE_SEP, {0}, 0, 0},  // 分隔符
            {1, ID_TB_UNINSTALL, TBSTATE_ENABLED, TBSTYLE_BUTTON, {0}, 0, (INT_PTR)L"卸载程序"},
            {2, ID_TB_FORCE_UNINSTALL, TBSTATE_ENABLED, TBSTYLE_BUTTON, {0}, 0, (INT_PTR)L"强制卸载"},
            {0, 0, 0, TBSTYLE_SEP, {0}, 0, 0},  // 分隔符
            {3, ID_TB_SEARCH, TBSTATE_ENABLED, TBSTYLE_BUTTON, {0}, 0, (INT_PTR)L"搜索程序"},
        };
        
        // 添加按钮到工具栏
        SendMessage(m_hToolbar, TB_ADDBUTTONS, sizeof(buttons)/sizeof(TBBUTTON), (LPARAM)buttons);
        
        // 自动调整工具栏大小
        SendMessage(m_hToolbar, TB_AUTOSIZE, 0, 0);
        
        YG_LOG_INFO(L"工具栏创建成功");
        return ErrorCode::Success;
    }
    
    ErrorCode MainWindow::CreateStatusBar() {
        // 创建状态栏
        m_hStatusBar = CreateWindowExW(
            0,
            STATUSCLASSNAMEW,
            nullptr,
            WS_CHILD | WS_VISIBLE | SBARS_SIZEGRIP,
            0, 0, 0, 0,
            m_hWnd,
            (HMENU)ID_STATUSBAR,
            m_hInstance,
            nullptr
        );
        
        if (!m_hStatusBar) {
            YG_LOG_ERROR(L"创建状态栏失败");
            return ErrorCode::GeneralError;
        }
        
        // 设置状态栏分区
        int parts[] = {-1};  // 只有一个分区，占满整个状态栏
        SendMessageW(m_hStatusBar, SB_SETPARTS, 1, (LPARAM)parts);
        
        YG_LOG_INFO(L"状态栏创建完成");
        return ErrorCode::Success;
    }
    
    ErrorCode MainWindow::CreateListView() {
        YG_LOG_INFO(L"开始创建程序列表显示控件...");
        
        // 获取客户区大小，如果失败则使用默认尺寸
        RECT clientRect;
        int width = 800;  // 默认宽度
        int height = 600; // 默认高度
        
        if (GetClientRect(m_hWnd, &clientRect)) {
            width = clientRect.right - clientRect.left;
            height = clientRect.bottom - clientRect.top;
            YG_LOG_INFO(L"获取到窗口尺寸: " + std::to_wstring(width) + L"x" + std::to_wstring(height));
        } else {
            YG_LOG_WARNING(L"获取客户区大小失败，使用默认尺寸: " + std::to_wstring(width) + L"x" + std::to_wstring(height));
        }
        
        // 为菜单和状态栏预留空间
        int topMargin = 0;   // 菜单栏空间 - 修改为0，紧贴菜单栏
        int bottomMargin = 30; // 状态栏空间
        int listTop = topMargin;
        int listHeight = height - topMargin - bottomMargin;
        
        // 确保尺寸有效
        if (listHeight < 100) {
            listHeight = 300; // 最小高度
            YG_LOG_WARNING(L"窗口太小，使用最小高度");
        }
        
        // 计算动态ListView宽度
        int listViewWidth = width - 10; // 窗口宽度减去左右边距
        int minTableWidth = CalculateTableWidth();
        if (listViewWidth < minTableWidth) {
            listViewWidth = minTableWidth;
        }
        
        // 直接创建多行编辑框来显示程序列表（最可靠的方案，无边框，动态宽度，5px边距）
        m_hListView = CreateWindowExW(
            0,  // 移除 WS_EX_CLIENTEDGE 边框样式
            L"EDIT",
            L"正在扫描已安装的程序，请稍候...",
            WS_CHILD | WS_VISIBLE | ES_MULTILINE | ES_READONLY | WS_VSCROLL | ES_AUTOVSCROLL,
            5, listTop, listViewWidth, listHeight,  // 左侧5px边距，使用动态宽度
            m_hWnd,
            (HMENU)ID_LISTVIEW,
            m_hInstance,
            nullptr
        );
        
        if (!m_hListView) {
            DWORD error = GetLastError();
            YG_LOG_ERROR(L"创建程序列表显示控件失败，错误代码: " + std::to_wstring(error));
            return ErrorCode::GeneralError;
        }
        
        // 设置字体以获得更好的显示效果
        HFONT hFont = CreateFontW(
            -12, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE,
            L"Consolas"
        );
        
        if (hFont) {
            SendMessage(m_hListView, WM_SETFONT, (WPARAM)hFont, TRUE);
        }
        
        YG_LOG_INFO(L"程序列表显示控件创建成功");
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
            {L"程序名称", 280, LVCFMT_LEFT},   // 增加宽度，左对齐
            {L"版本", 120, LVCFMT_LEFT},       // 增加宽度，左对齐
            {L"发布者", 180, LVCFMT_LEFT},     // 增加宽度，左对齐
            {L"大小", 90, LVCFMT_RIGHT},       // 增加宽度，右对齐（数字右对齐更美观）
            {L"安装日期", 110, LVCFMT_CENTER} // 增加宽度，居中对齐（日期居中更美观）
        };
        
        // 添加列
        LVCOLUMNW column = {0};  // 使用零初始化而不是空初始化
        column.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_FMT;
        
        for (size_t i = 0; i < sizeof(columns) / sizeof(columns[0]); i++) {
            column.pszText = (LPWSTR)columns[i].title;
            column.cx = columns[i].width;
            column.fmt = columns[i].format;
            
            if (ListView_InsertColumn(m_hListView, static_cast<int>(i), &column) == -1) {
                YG_LOG_ERROR(L"添加列失败: " + String(columns[i].title));
            }
        }
    }
    
    int MainWindow::CalculateTableWidth() {
        // 优化5列宽度分配，提升用户体验
        int nameWidth = 280;      // 程序名称（增加宽度，更好显示长名称）
        int versionWidth = 120;   // 版本（增加宽度，更好显示版本号）
        int publisherWidth = 180; // 发布者（增加宽度，更好显示公司名）
        int sizeWidth = 90;       // 大小（增加宽度，右对齐显示）
        int dateWidth = 110;      // 安装日期（增加宽度，更好显示日期）
        
        // 返回5列的优化总宽度
        int totalTableWidth = nameWidth + versionWidth + publisherWidth + sizeWidth + dateWidth;
        
        return totalTableWidth; // 780像素
    }
    
    void MainWindow::AdjustListViewColumns() {
        if (!m_hListView || !m_isListViewMode) {
            return;
        }
        
        // 获取ListView的实际宽度
        RECT listRect;
        GetClientRect(m_hListView, &listRect);
        int availableWidth = listRect.right - listRect.left;
        
        // 定义基础列宽（最小宽度）
        int baseNameWidth = 280;      // 程序名称基础宽度
        int baseVersionWidth = 120;   // 版本基础宽度
        int basePublisherWidth = 180; // 发布者基础宽度
        int baseSizeWidth = 90;       // 大小基础宽度
        int baseDateWidth = 110;      // 安装日期基础宽度
        
        int baseTotal = baseNameWidth + baseVersionWidth + basePublisherWidth + baseSizeWidth + baseDateWidth;
        
        // 如果可用宽度大于基础宽度，按比例扩展
        int nameWidth, versionWidth, publisherWidth, sizeWidth, dateWidth;
        
        if (availableWidth > baseTotal) {
            // 计算额外空间
            int extraSpace = availableWidth - baseTotal;
            
            // 按比例分配额外空间（程序名称和发布者获得更多空间）
            nameWidth = baseNameWidth + (extraSpace * 40) / 100;      // 40%额外空间给程序名称
            publisherWidth = basePublisherWidth + (extraSpace * 30) / 100; // 30%额外空间给发布者
            versionWidth = baseVersionWidth + (extraSpace * 15) / 100;     // 15%额外空间给版本
            sizeWidth = baseSizeWidth + (extraSpace * 10) / 100;           // 10%额外空间给大小
            dateWidth = baseDateWidth + (extraSpace * 5) / 100;            // 5%额外空间给日期
            
            // 确保总宽度精确匹配
            int calculatedTotal = nameWidth + versionWidth + publisherWidth + sizeWidth + dateWidth;
            if (calculatedTotal < availableWidth) {
                dateWidth += (availableWidth - calculatedTotal); // 剩余空间给最后一列
            }
        } else {
            // 使用基础宽度
            nameWidth = baseNameWidth;
            versionWidth = baseVersionWidth;
            publisherWidth = basePublisherWidth;
            sizeWidth = baseSizeWidth;
            dateWidth = baseDateWidth;
        }
        
        // 设置列宽
        ListView_SetColumnWidth(m_hListView, 0, nameWidth);
        ListView_SetColumnWidth(m_hListView, 1, versionWidth);
        ListView_SetColumnWidth(m_hListView, 2, publisherWidth);
        ListView_SetColumnWidth(m_hListView, 3, sizeWidth);
        ListView_SetColumnWidth(m_hListView, 4, dateWidth);
        
        int totalWidth = nameWidth + versionWidth + publisherWidth + sizeWidth + dateWidth;
        
        YG_LOG_INFO(L"动态列宽调整完成 - 可用宽度: " + std::to_wstring(availableWidth) + 
                   L", 分配宽度: " + std::to_wstring(totalWidth) + 
                   L" (名称:" + std::to_wstring(nameWidth) + 
                   L", 版本:" + std::to_wstring(versionWidth) +
                   L", 发布者:" + std::to_wstring(publisherWidth) +
                   L", 大小:" + std::to_wstring(sizeWidth) +
                   L", 日期:" + std::to_wstring(dateWidth) + L")");
    }
    
    void MainWindow::PopulateProgramList(const std::vector<ProgramInfo>& programs) {
        YG_LOG_INFO(L"开始填充程序列表，程序数量: " + std::to_wstring(programs.size()));
        
        if (!m_hListView) {
            YG_LOG_ERROR(L"程序列表控件为空！");
            return;
        }
        
        // 先进行去重处理
        std::vector<ProgramInfo> uniquePrograms = RemoveDuplicatePrograms(programs);
        
        // 根据控件类型选择显示方式
        if (m_isListViewMode) {
            // ListView表格模式
            YG_LOG_INFO(L"使用ListView表格模式显示程序列表");
            
            // 清除现有数据
            ListView_DeleteAllItems(m_hListView);
            
            if (uniquePrograms.empty()) {
                SetStatusText(L"未找到已安装的程序");
                return;
            }
            
            // 保存去重后的程序列表
            m_programs = uniquePrograms;
            
            // 添加程序数据到ListView
            for (size_t i = 0; i < uniquePrograms.size(); i++) {
                const auto& program = uniquePrograms[i];
                
                LVITEMW item = {0};
                item.mask = LVIF_TEXT | LVIF_PARAM | LVIF_IMAGE;
                item.iItem = static_cast<int>(i);
                item.iSubItem = 0;
                // 优先使用displayName，如果为空则使用name
                String programName = !program.displayName.empty() ? program.displayName : program.name;
                item.pszText = const_cast<LPWSTR>(programName.c_str());
                item.lParam = static_cast<LPARAM>(i); // 存储程序索引
                
                // 提取程序图标
                item.iImage = ExtractProgramIcon(program);
                
                int index = ListView_InsertItem(m_hListView, &item);
                
                if (index != -1) {
                    // 设置子项（版本、发布者、大小、安装日期）
                    ListView_SetItemText(m_hListView, index, 1, const_cast<LPWSTR>(program.version.c_str()));
                    ListView_SetItemText(m_hListView, index, 2, const_cast<LPWSTR>(program.publisher.c_str()));
                    
                    // 格式化文件大小
                    String sizeStr = L"-";
                    if (program.estimatedSize > 0) {
                        sizeStr = FormatFileSize(program.estimatedSize);
                    }
                    ListView_SetItemText(m_hListView, index, 3, const_cast<LPWSTR>(sizeStr.c_str()));
                    
                    // 格式化安装日期
                    String formattedDate = FormatInstallDate(program.installDate);
                    ListView_SetItemText(m_hListView, index, 4, const_cast<LPWSTR>(formattedDate.c_str()));
                }
            }
            
            // 调整列宽以适应窗口宽度，避免水平滚动条
            AdjustListViewColumns();
            
            YG_LOG_INFO(L"ListView表格数据填充完成");
            
        } else {
            // 编辑框模式（后备方案）
            YG_LOG_INFO(L"使用编辑框模式显示程序列表");
            
            // 构建程序列表文本
            String listText = L"=== YG Uninstaller - 64位 Windows 系统已安装程序列表 ===\r\n\r\n";
            
            if (uniquePrograms.empty()) {
                listText += L"未找到已安装的程序。请检查系统状态或重新扫描。";
            } else {
                listText += L"扫描结果：共找到 " + std::to_wstring(uniquePrograms.size()) + L" 个已安装的程序（已去重）\r\n";
                listText += L"===============================================================================\r\n\r\n";
                
                for (size_t i = 0; i < uniquePrograms.size(); i++) {
                    const auto& program = uniquePrograms[i];
                    
                    // 格式化显示：序号. 程序名称（优先使用displayName，如果为空则使用name）
                    String programName = !program.displayName.empty() ? program.displayName : program.name;
                    listText += L"  " + std::to_wstring(i + 1) + L". " + programName;
                    
                    // 添加版本信息
                    if (!program.version.empty()) {
                        listText += L" (v" + program.version + L")";
                    }
                    
                    // 添加发布者信息
                    if (!program.publisher.empty()) {
                        listText += L" - " + program.publisher;
                    }
                    
                    listText += L"\r\n";
                    
                    // 每10个程序后添加一个空行，便于阅读
                    if ((i + 1) % 10 == 0 && i + 1 < uniquePrograms.size()) {
                        listText += L"\r\n";
                    }
                }
                
                listText += L"\r\n===============================================================================\r\n";
                listText += L"扫描完成！您可以使用菜单栏操作程序。\r\n";
                listText += L"提示：可以通过菜单栏'操作'-'刷新程序列表'重新扫描。";
            }
            
            // 显示在编辑框中
            SetWindowTextW(m_hListView, listText.c_str());
            
            // 滚动到顶部
            SendMessage(m_hListView, EM_SETSEL, 0, 0);
            SendMessage(m_hListView, EM_SCROLLCARET, 0, 0);
        }
        
        // 更新状态栏（显示去重后的数量和去重信息）
        if (uniquePrograms.size() > 0) {
            String statusText = L"扫描完成 - 找到 " + std::to_wstring(uniquePrograms.size()) + L" 个已安装的程序";
            if (programs.size() > uniquePrograms.size()) {
                statusText += L"（已去除 " + std::to_wstring(programs.size() - uniquePrograms.size()) + L" 个重复项）";
            }
            SetStatusText(statusText);
        } else {
            SetStatusText(L"扫描完成 - 未找到程序，请检查系统状态");
        }
        
        YG_LOG_INFO(L"程序列表填充完成");
    }
    
    bool MainWindow::GetSelectedProgram(ProgramInfo& program) {
        if (!m_hListView) {
            return false;
        }
        
        if (m_isListViewMode) {
            // ListView模式
            int selectedIndex = ListView_GetNextItem(m_hListView, -1, LVNI_SELECTED);
            if (selectedIndex == -1) {
                return false;
            }
            
            // 获取项数据（程序索引）
            LVITEMW item = {0};
            item.mask = LVIF_PARAM;
            item.iItem = selectedIndex;
            
            if (!ListView_GetItem(m_hListView, &item)) {
                return false;
            }
            
            size_t programIndex = static_cast<size_t>(item.lParam);
            
            // 从存储的程序列表中获取程序信息
            if (programIndex < m_programs.size()) {
                program = m_programs[programIndex];
                return true;
            }
            
        } else {
            // 编辑框模式（不支持选中）
            YG_LOG_WARNING(L"编辑框模式不支持程序选中");
            return false;
        }
        
        return false;
    }
    
    void MainWindow::UpdateUIState() {
        // TODO: 更新UI状态
    }
    
    void MainWindow::AdjustLayout() {
        if (!m_hWnd) return;
        
        RECT clientRect;
        GetClientRect(m_hWnd, &clientRect);
        int height = clientRect.bottom - clientRect.top;
        int windowWidth = clientRect.right - clientRect.left;
        
        // 调整状态栏
        if (m_hStatusBar) {
            SendMessage(m_hStatusBar, WM_SIZE, 0, 0);
        }
        
        // 获取菜单栏和状态栏高度
        int menuHeight = GetSystemMetrics(SM_CYMENU);
        RECT statusRect = {0};
        int statusHeight = 0;
        
        if (m_hStatusBar) {
            GetWindowRect(m_hStatusBar, &statusRect);
            statusHeight = statusRect.bottom - statusRect.top;
        }
        
        // 调整嵌入菜单栏的搜索控件
        if (m_hSearchEdit) {
            int searchWidth = 180;      // 与创建时保持一致
            int buttonWidth = 24;       // 与创建时保持一致
            int spacing = 3;            // 与创建时保持一致
            int rightMargin = 8;        // 与创建时保持一致
            int searchX = windowWidth - rightMargin - searchWidth - buttonWidth - spacing;
            int searchY = 2;            // 嵌入菜单栏内部
            int controlHeight = menuHeight - 4;
            
            // 调整搜索框位置
            SetWindowPos(m_hSearchEdit, HWND_TOP, searchX, searchY, searchWidth, controlHeight, 
                        SWP_NOACTIVATE);
            
            // 重新设置搜索框圆角区域（因为大小可能改变）
            HRGN hRgn = CreateRoundRectRgn(0, 0, searchWidth + 1, controlHeight + 1, 8, 8);
            if (hRgn) {
                SetWindowRgn(m_hSearchEdit, hRgn, TRUE);
            }
            
            // 调整搜索按钮位置
            HWND hSearchButton = GetDlgItem(m_hWnd, ID_TB_SEARCH);
            if (hSearchButton) {
                SetWindowPos(hSearchButton, HWND_TOP, searchX + searchWidth + spacing, searchY, 
                            buttonWidth, controlHeight, SWP_NOACTIVATE);
                
                // 重新设置搜索按钮圆角区域
                HRGN hBtnRgn = CreateRoundRectRgn(0, 0, buttonWidth + 1, controlHeight + 1, 8, 8);
                if (hBtnRgn) {
                    SetWindowRgn(hSearchButton, hBtnRgn, TRUE);
                }
            }
        }
        
        // 调整进度条（在菜单栏下方，全宽度）
        if (m_hProgressBar) {
            SetWindowPos(m_hProgressBar, nullptr, 5, menuHeight, 
                        windowWidth - 10, 3, SWP_NOZORDER | SWP_NOACTIVATE);
        }
        
        // 调整列表视图
        if (m_hListView) {
            int listTop = menuHeight + 3;  // 菜单栏下方，为进度条留出3px空间
            int listHeight = height - listTop - statusHeight;
            
            // 获取窗口宽度
            RECT clientRect;
            GetClientRect(m_hWnd, &clientRect);
            int windowWidth = clientRect.right - clientRect.left;
            
            // 计算ListView宽度：窗口宽度减去左右边距（5px + 5px = 10px）
            int listViewWidth = windowWidth - 10;
            
            // 确保最小宽度
            int minTableWidth = CalculateTableWidth();
            if (listViewWidth < minTableWidth) {
                listViewWidth = minTableWidth;
            }
            
            // 设置ListView为动态宽度，左侧5px边距
            SetWindowPos(m_hListView, nullptr, 5, listTop, listViewWidth, listHeight,
                        SWP_NOZORDER | SWP_NOACTIVATE);
            
            // 动态调整列宽以适应新的ListView宽度
            AdjustListViewColumns();
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
    
    String MainWindow::FormatFileSize(DWORD64 sizeInBytes) {
        // 如果大小为0或未知，显示"未知"而不是"-"
        if (sizeInBytes == 0) {
            return L"未知";
        }
        
        const wchar_t* units[] = {L"B", L"KB", L"MB", L"GB", L"TB"};
        double size = static_cast<double>(sizeInBytes);
        int unitIndex = 0;
        
        while (size >= 1024.0 && unitIndex < 4) {
            size /= 1024.0;
            unitIndex++;
        }
        
        wchar_t buffer[64];
#ifdef _MSC_VER
        if (unitIndex == 0) {
            swprintf_s(buffer, L"%.0f %s", size, units[unitIndex]);
        } else if (size < 10.0) {
            swprintf_s(buffer, L"%.1f %s", size, units[unitIndex]);  // 小于10时显示1位小数
        } else {
            swprintf_s(buffer, L"%.0f %s", size, units[unitIndex]);  // 大于等于10时不显示小数
        }
#else
        if (unitIndex == 0) {
            swprintf(buffer, sizeof(buffer)/sizeof(wchar_t), L"%.0f %s", size, units[unitIndex]);
        } else if (size < 10.0) {
            swprintf(buffer, sizeof(buffer)/sizeof(wchar_t), L"%.1f %s", size, units[unitIndex]);
        } else {
            swprintf(buffer, sizeof(buffer)/sizeof(wchar_t), L"%.0f %s", size, units[unitIndex]);
        }
#endif
        
        return String(buffer);
    }
    
    String MainWindow::FormatInstallDate(const String& dateString) {
        // 输入格式: YYYY-MM-DD 或 YYYY/MM/DD
        // 输出格式: M/d/yyyy
        
        if (dateString.empty()) {
            return L"未知";  // 显示"未知"而不是"-"，更友好
        }
        
        // 查找分隔符位置
        size_t firstSep = dateString.find_first_of(L"-/");
        size_t secondSep = dateString.find_first_of(L"-/", firstSep + 1);
        
        if (firstSep == String::npos || secondSep == String::npos) {
            // 如果格式不正确，直接返回原字符串
            return dateString;
        }
        
        try {
            // 提取年、月、日
            String yearStr = dateString.substr(0, firstSep);
            String monthStr = dateString.substr(firstSep + 1, secondSep - firstSep - 1);
            String dayStr = dateString.substr(secondSep + 1);
            
            // 转换为数字去除前导零
            int year = std::stoi(yearStr);
            int month = std::stoi(monthStr);
            int day = std::stoi(dayStr);
            
            // 格式化为 M/d/yyyy
            wchar_t buffer[32];
#ifdef _MSC_VER
            swprintf_s(buffer, L"%d/%d/%d", month, day, year);
#else
            swprintf(buffer, sizeof(buffer)/sizeof(wchar_t), L"%d/%d/%d", month, day, year);
#endif
            return String(buffer);
        } catch (...) {
            // 如果转换失败，返回原字符串
            return dateString;
        }
    }
    
    void MainWindow::AddTestPrograms() {
        // 添加测试数据来验证列表视图功能
        std::vector<ProgramInfo> testPrograms;
        
        // 创建测试程序信息
        ProgramInfo test1;
        test1.name = L"测试程序 1";
        test1.version = L"1.0.0";
        test1.publisher = L"测试发布者";
        test1.estimatedSize = 1024;  // 1MB
        test1.installDate = L"2025-01-01";
        testPrograms.push_back(test1);
        
        ProgramInfo test2;
        test2.name = L"测试程序 2";
        test2.version = L"2.0.0";
        test2.publisher = L"另一个发布者";
        test2.estimatedSize = 2048;  // 2MB
        test2.installDate = L"2025-01-02";
        testPrograms.push_back(test2);
        
        // 显示测试数据
        PopulateProgramList(testPrograms);
        SetStatusText(L"显示测试数据 - " + std::to_wstring(testPrograms.size()) + L" 个测试程序");
    }
    
    void MainWindow::AddSamplePrograms() {
        YG_LOG_INFO(L"添加示例程序数据");
        
        // 创建示例程序列表
        std::vector<ProgramInfo> samplePrograms;
        
        // 添加示例程序1 - Microsoft Office
        ProgramInfo program1;
        program1.name = L"Microsoft Office 365";
        program1.displayName = L"Microsoft Office 365";
        program1.version = L"16.0.14326.20404";
        program1.publisher = L"Microsoft Corporation";
        program1.estimatedSize = 2147483648ULL;  // 2GB
        program1.installDate = L"2024-11-15";
        program1.uninstallString = L"C:\\Program Files\\Microsoft Office\\uninstall.exe";
        samplePrograms.push_back(program1);
        
        // 添加示例程序2 - Google Chrome
        ProgramInfo program2;
        program2.name = L"Google Chrome";
        program2.displayName = L"Google Chrome";
        program2.version = L"118.0.5993.88";
        program2.publisher = L"Google LLC";
        program2.estimatedSize = 536870912ULL;  // 512MB
        program2.installDate = L"2025-08-25";
        program2.uninstallString = L"C:\\Program Files\\Google\\Chrome\\Application\\uninstall.exe";
        samplePrograms.push_back(program2);
        
        // 添加示例程序3 - Adobe Acrobat Reader DC
        ProgramInfo program3;
        program3.name = L"Adobe Acrobat Reader DC";
        program3.displayName = L"Adobe Acrobat Reader DC";
        program3.version = L"23.008.20458";
        program3.publisher = L"Adobe Systems Incorporated";
        program3.estimatedSize = 1073741824ULL;  // 1GB
        program3.installDate = L"2025-03-12";
        program3.uninstallString = L"C:\\Program Files\\Adobe\\Acrobat Reader DC\\uninstall.exe";
        samplePrograms.push_back(program3);
        
        // 添加示例程序4 - VLC Media Player
        ProgramInfo program4;
        program4.name = L"VLC Media Player";
        program4.displayName = L"VLC Media Player";
        program4.version = L"3.0.18";
        program4.publisher = L"VideoLAN";
        program4.estimatedSize = 104857600ULL;  // 100MB
        program4.installDate = L"2025-01-08";
        program4.uninstallString = L"C:\\Program Files\\VideoLAN\\VLC\\uninstall.exe";
        samplePrograms.push_back(program4);
        
        // 添加示例程序5 - 7-Zip
        ProgramInfo program5;
        program5.name = L"7-Zip";
        program5.displayName = L"7-Zip";
        program5.version = L"22.01";
        program5.publisher = L"Igor Pavlov";
        program5.estimatedSize = 41943040ULL;  // 40MB
        program5.installDate = L"2024-12-05";
        program5.uninstallString = L"C:\\Program Files\\7-Zip\\uninstall.exe";
        samplePrograms.push_back(program5);
        
        // 添加示例程序6 - Visual Studio Code
        ProgramInfo program6;
        program6.name = L"Visual Studio Code";
        program6.displayName = L"Visual Studio Code";
        program6.version = L"1.85.2";
        program6.publisher = L"Microsoft Corporation";
        program6.estimatedSize = 314572800ULL;  // 300MB
        program6.installDate = L"2025-02-14";
        program6.uninstallString = L"C:\\Users\\%USERNAME%\\AppData\\Local\\Programs\\Microsoft VS Code\\uninstall.exe";
        samplePrograms.push_back(program6);
        
        // 添加示例程序7 - Notepad++
        ProgramInfo program7;
        program7.name = L"Notepad++";
        program7.displayName = L"Notepad++";
        program7.version = L"8.6.2";
        program7.publisher = L"Notepad++ Team";
        program7.estimatedSize = 20971520ULL;  // 20MB
        program7.installDate = L"2025-01-30";
        program7.uninstallString = L"C:\\Program Files\\Notepad++\\uninstall.exe";
        samplePrograms.push_back(program7);
        
        // 添加示例程序8 - WinRAR
        ProgramInfo program8;
        program8.name = L"WinRAR";
        program8.displayName = L"WinRAR";
        program8.version = L"6.24";
        program8.publisher = L"win.rar GmbH";
        program8.estimatedSize = 15728640ULL;  // 15MB
        program8.installDate = L"2024-09-18";
        program8.uninstallString = L"C:\\Program Files\\WinRAR\\uninstall.exe";
        samplePrograms.push_back(program8);
        
        // 填充到列表视图
        PopulateProgramList(samplePrograms);
        
        YG_LOG_INFO(L"示例程序数据添加完成，共 " + std::to_wstring(samplePrograms.size()) + L" 个程序");
    }
    
    void MainWindow::OnColumnHeaderClick(int column) {
        YG_LOG_INFO(L"列标题点击事件，列索引: " + std::to_wstring(column));
        
        // 如果点击的是同一列，则切换排序方向
        if (m_sortColumn == column) {
            m_sortAscending = !m_sortAscending;
        } else {
            // 点击新列，默认升序排序
            m_sortColumn = column;
            m_sortAscending = true;
        }
        
        // 执行排序
        SortProgramList(m_sortColumn, m_sortAscending);
        
        // 更新状态栏信息
        String sortDirection = m_sortAscending ? L"升序" : L"降序";
        String columnNames[] = {L"程序名称", L"版本", L"发布者", L"大小", L"安装日期"};
        if (column >= 0 && column < 5) {
            SetStatusText(L"按 " + columnNames[column] + L" " + sortDirection + L" 排序");
        }
    }
    
    void MainWindow::SortProgramList(int column, bool ascending) {
        if (!m_isListViewMode || m_programs.empty()) {
            return;
        }
        
        YG_LOG_INFO(L"开始排序程序列表，列: " + std::to_wstring(column) + L", 升序: " + (ascending ? L"是" : L"否"));
        
        // 排序程序列表
        std::sort(m_programs.begin(), m_programs.end(), 
                 [column, ascending](const ProgramInfo& a, const ProgramInfo& b) {
                     return CompareProgramInfo(a, b, column, ascending) < 0;
                 });
        
        // 重新填充ListView
        PopulateProgramList(m_programs);
        
        YG_LOG_INFO(L"程序列表排序完成");
    }
    
    int MainWindow::CompareProgramInfo(const ProgramInfo& program1, const ProgramInfo& program2, int column, bool ascending) {
        int result = 0;
        
        switch (column) {
            case 0: // 程序名称
            {
                String name1 = !program1.displayName.empty() ? program1.displayName : program1.name;
                String name2 = !program2.displayName.empty() ? program2.displayName : program2.name;
                result = _wcsicmp(name1.c_str(), name2.c_str());
                break;
            }
            case 1: // 版本
            {
                result = _wcsicmp(program1.version.c_str(), program2.version.c_str());
                break;
            }
            case 2: // 发布者
            {
                result = _wcsicmp(program1.publisher.c_str(), program2.publisher.c_str());
                break;
            }
            case 3: // 大小
            {
                if (program1.estimatedSize < program2.estimatedSize) {
                    result = -1;
                } else if (program1.estimatedSize > program2.estimatedSize) {
                    result = 1;
                } else {
                    result = 0;
                }
                break;
            }
            case 4: // 安装日期
            {
                result = _wcsicmp(program1.installDate.c_str(), program2.installDate.c_str());
                break;
            }
            default:
                result = 0;
                break;
        }
        
        // 如果是降序，反转结果
        return ascending ? result : -result;
    }
    
    ErrorCode MainWindow::CreateImageList() {
        // 创建16x16的小图标列表
        m_hImageList = ImageList_Create(16, 16, ILC_COLOR32 | ILC_MASK, 10, 10);
        if (!m_hImageList) {
            YG_LOG_ERROR(L"创建图标列表失败");
            return ErrorCode::GeneralError;
        }
        
        // 将图标列表关联到ListView
        HIMAGELIST hOldImageList = ListView_SetImageList(m_hListView, m_hImageList, LVSIL_SMALL);
        (void)hOldImageList; // 抑制未使用警告
        
        // 添加默认程序图标
        HICON hDefaultIcon = LoadIcon(nullptr, IDI_APPLICATION);
        if (hDefaultIcon) {
            ImageList_AddIcon(m_hImageList, hDefaultIcon);
            DestroyIcon(hDefaultIcon);
        }
        
        YG_LOG_INFO(L"图标列表创建成功");
        return ErrorCode::Success;
    }
    
    int MainWindow::ExtractProgramIcon(const ProgramInfo& program) {
        // 尝试从不同位置提取程序图标
        HICON hIcon = nullptr;
        
        // 1. 优先使用注册表中的DisplayIcon字段
        if (!program.iconPath.empty()) {
            String iconPath = program.iconPath;
            
            // 处理图标路径格式：可能包含文件路径和图标索引
            size_t commaPos = iconPath.find(L',');
            int iconIndex = 0;
            
            if (commaPos != String::npos) {
                // 有图标索引
                iconPath = iconPath.substr(0, commaPos);
                try {
                    iconIndex = std::stoi(iconPath.substr(commaPos + 1));
                } catch (...) {
                    iconIndex = 0;
                }
            }
            
            // 移除引号
            if (!iconPath.empty() && iconPath.front() == L'"') {
                iconPath = iconPath.substr(1);
            }
            if (!iconPath.empty() && iconPath.back() == L'"') {
                iconPath.pop_back();
            }
            
            // 提取图标
            if (!iconPath.empty()) {
                hIcon = ExtractIcon(m_hInstance, iconPath.c_str(), iconIndex);
            }
        }
        
        // 2. 尝试从卸载字符串中提取可执行文件路径
        if (!hIcon && !program.uninstallString.empty()) {
            String exePath = program.uninstallString;
            
            // 简单解析：查找.exe文件
            size_t exePos = exePath.find(L".exe");
            if (exePos != String::npos) {
                exePath = exePath.substr(0, exePos + 4);
                
                // 移除引号
                if (!exePath.empty() && exePath.front() == L'"') {
                    exePath = exePath.substr(1);
                }
                if (!exePath.empty() && exePath.back() == L'"') {
                    exePath.pop_back();
                }
                
                // 提取图标
                hIcon = ExtractIcon(m_hInstance, exePath.c_str(), 0);
            }
        }
        
        // 3. 尝试从安装路径提取
        if (!hIcon && !program.installLocation.empty()) {
            String mainExe = program.installLocation + L"\\";
            
            // 根据程序名称猜测主执行文件
            if (program.displayName.find(L"Chrome") != String::npos) {
                mainExe += L"chrome.exe";
            } else if (program.displayName.find(L"Office") != String::npos) {
                mainExe += L"WINWORD.EXE";
            } else if (program.displayName.find(L"Adobe") != String::npos) {
                mainExe += L"AcroRd32.exe";
            } else if (program.displayName.find(L"VLC") != String::npos) {
                mainExe += L"vlc.exe";
            } else if (program.displayName.find(L"7-Zip") != String::npos) {
                mainExe += L"7zFM.exe";
            } else if (program.displayName.find(L"Code") != String::npos) {
                mainExe += L"Code.exe";
            } else if (program.displayName.find(L"Notepad") != String::npos) {
                mainExe += L"notepad++.exe";
            } else if (program.displayName.find(L"WinRAR") != String::npos) {
                mainExe += L"WinRAR.exe";
            }
            
            hIcon = ExtractIcon(m_hInstance, mainExe.c_str(), 0);
        }
        
        // 4. 如果成功提取到图标，添加到图标列表
        if (hIcon && hIcon != (HICON)1) {
            int iconIndex = ImageList_AddIcon(m_hImageList, hIcon);
            DestroyIcon(hIcon);
            return iconIndex;
        }
        
        // 5. 使用默认图标
        return GetDefaultProgramIcon();
    }
    
    int MainWindow::GetDefaultProgramIcon() {
        // 返回默认程序图标的索引（在CreateImageList中添加的第一个图标）
        return 0;
    }
    
    std::vector<ProgramInfo> MainWindow::RemoveDuplicatePrograms(const std::vector<ProgramInfo>& programs) {
        std::vector<ProgramInfo> uniquePrograms;
        
        YG_LOG_INFO(L"开始去重处理，原始程序数量: " + std::to_wstring(programs.size()));
        
        for (const auto& program : programs) {
            bool isDuplicate = false;
            
            // 检查是否与已有程序重复
            for (const auto& existingProgram : uniquePrograms) {
                if (IsSameProgram(program, existingProgram)) {
                    isDuplicate = true;
                    YG_LOG_INFO(L"发现重复程序: " + program.displayName + L" (版本: " + program.version + L")");
                    break;
                }
            }
            
            // 如果不重复，添加到结果列表
            if (!isDuplicate) {
                uniquePrograms.push_back(program);
            }
        }
        
        YG_LOG_INFO(L"去重完成，去重后程序数量: " + std::to_wstring(uniquePrograms.size()) + 
                   L", 移除重复项: " + std::to_wstring(programs.size() - uniquePrograms.size()));
        
        return uniquePrograms;
    }
    
    bool MainWindow::IsSameProgram(const ProgramInfo& program1, const ProgramInfo& program2) {
        // 获取程序显示名称
        String name1 = !program1.displayName.empty() ? program1.displayName : program1.name;
        String name2 = !program2.displayName.empty() ? program2.displayName : program2.name;
        
        // 转换为小写进行比较（不区分大小写）
        std::transform(name1.begin(), name1.end(), name1.begin(), ::towlower);
        std::transform(name2.begin(), name2.end(), name2.begin(), ::towlower);
        
        // 1. 程序名称完全相同
        if (name1 == name2) {
            // 如果名称相同，进一步检查版本
            if (program1.version == program2.version) {
                return true; // 名称和版本都相同，认为是重复
            }
            
            // 名称相同但版本不同，检查发布者
            if (program1.publisher == program2.publisher) {
                // 同一发布者的同名程序，可能是不同版本，保留较新版本
                return true;
            }
        }
        
        // 2. 检查是否为同一程序的不同架构版本（如x86/x64）
        String baseName1 = name1;
        String baseName2 = name2;
        
        // 移除常见的架构标识
        const std::vector<String> archSuffixes = {
            L" (x64)", L" (x86)", L" (64-bit)", L" (32-bit)", 
            L" x64", L" x86", L" 64-bit", L" 32-bit"
        };
        
        for (const auto& suffix : archSuffixes) {
            size_t pos1 = baseName1.find(suffix);
            if (pos1 != String::npos) {
                baseName1 = baseName1.substr(0, pos1);
            }
            
            size_t pos2 = baseName2.find(suffix);
            if (pos2 != String::npos) {
                baseName2 = baseName2.substr(0, pos2);
            }
        }
        
        // 如果移除架构标识后名称相同，且发布者相同，认为是重复
        if (baseName1 == baseName2 && program1.publisher == program2.publisher) {
            return true;
        }
        
        // 3. 检查安装路径是否相同（可能是同一程序的不同条目）
        if (!program1.installLocation.empty() && !program2.installLocation.empty()) {
            String path1 = program1.installLocation;
            String path2 = program2.installLocation;
            
            // 转换为小写比较
            std::transform(path1.begin(), path1.end(), path1.begin(), ::towlower);
            std::transform(path2.begin(), path2.end(), path2.begin(), ::towlower);
            
            if (path1 == path2) {
                return true; // 安装路径相同，认为是重复
            }
        }
        
        return false; // 不是重复程序
    }
    
    int MainWindow::GetSelectedPrograms(std::vector<ProgramInfo>& programs) {
        programs.clear();
        
        if (!m_hListView || !m_isListViewMode) {
            return 0;
        }
        
        int selectedCount = 0;
        int itemCount = ListView_GetItemCount(m_hListView);
        
        for (int i = 0; i < itemCount; i++) {
            UINT state = ListView_GetItemState(m_hListView, i, LVIS_SELECTED);
            if (state & LVIS_SELECTED) {
                // 获取程序索引
                LVITEMW item = {0};
                item.mask = LVIF_PARAM;
                item.iItem = i;
                
                if (ListView_GetItem(m_hListView, &item)) {
                    size_t programIndex = static_cast<size_t>(item.lParam);
                    if (programIndex < m_programs.size()) {
                        programs.push_back(m_programs[programIndex]);
                        selectedCount++;
                    }
                }
            }
        }
        
        YG_LOG_INFO(L"获取选中程序，数量: " + std::to_wstring(selectedCount));
        return selectedCount;
    }
    
    void MainWindow::BatchUninstallSelectedPrograms(UninstallMode mode) {
        std::vector<ProgramInfo> selectedPrograms;
        int selectedCount = GetSelectedPrograms(selectedPrograms);
        
        if (selectedCount == 0) {
            MessageBoxW(m_hWnd, L"请先选择要卸载的程序。\n\n提示：按住Ctrl键点击可以选择多个程序。", 
                       L"批量卸载", MB_OK | MB_ICONINFORMATION);
            return;
        }
        
        // 确认批量卸载
        String confirmMessage = L"确定要批量卸载以下 " + std::to_wstring(selectedCount) + L" 个程序吗？\n\n";
        for (size_t i = 0; i < selectedPrograms.size() && i < 10; i++) {
            String programName = !selectedPrograms[i].displayName.empty() ? 
                                selectedPrograms[i].displayName : selectedPrograms[i].name;
            confirmMessage += L"• " + programName + L"\n";
        }
        
        if (selectedPrograms.size() > 10) {
            confirmMessage += L"... 以及其他 " + std::to_wstring(selectedPrograms.size() - 10) + L" 个程序\n";
        }
        
        if (mode == UninstallMode::Force) {
            confirmMessage += L"\n⚠️ 注意：强制卸载可能导致系统不稳定！";
        }
        
        int result = MessageBoxW(m_hWnd, confirmMessage.c_str(), L"确认批量卸载", 
                                MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2);
        if (result != IDYES) {
            return;
        }
        
        // 执行批量卸载
        int successCount = 0;
        int failedCount = 0;
        
        // 显示进度
        UpdateProgress(0, true);
        
        for (size_t i = 0; i < selectedPrograms.size(); i++) {
            const auto& program = selectedPrograms[i];
            String programName = !program.displayName.empty() ? program.displayName : program.name;
            
            // 更新进度
            int progress = static_cast<int>((i * 100) / selectedPrograms.size());
            UpdateProgress(progress, true);
            SetStatusText(L"正在卸载: " + programName + L" (" + 
                         std::to_wstring(i + 1) + L"/" + std::to_wstring(selectedPrograms.size()) + L")");
            
            // 创建卸载服务
            if (!m_uninstallerService) {
                m_uninstallerService = YG::MakeUnique<UninstallerService>();
            }
            
            // 执行卸载
            ErrorCode uninstallResult = m_uninstallerService->UninstallProgram(program, mode);
            
            if (uninstallResult == ErrorCode::Success) {
                successCount++;
                YG_LOG_INFO(L"批量卸载成功: " + programName);
            } else {
                failedCount++;
                YG_LOG_ERROR(L"批量卸载失败: " + programName);
            }
        }
        
        // 隐藏进度条
        UpdateProgress(0, false);
        
        // 显示结果
        String resultMessage = L"批量卸载完成！\n\n";
        resultMessage += L"成功卸载: " + std::to_wstring(successCount) + L" 个程序\n";
        if (failedCount > 0) {
            resultMessage += L"卸载失败: " + std::to_wstring(failedCount) + L" 个程序\n";
        }
        
        MessageBoxW(m_hWnd, resultMessage.c_str(), L"批量卸载结果", 
                   MB_OK | (failedCount > 0 ? MB_ICONWARNING : MB_ICONINFORMATION));
        
        // 刷新程序列表
        RefreshProgramList(m_includeSystemComponents);
        
        YG_LOG_INFO(L"批量卸载完成，成功: " + std::to_wstring(successCount) + 
                   L", 失败: " + std::to_wstring(failedCount));
    }
    
    void MainWindow::SelectAllPrograms(bool selectAll) {
        if (!m_hListView || !m_isListViewMode) {
            return;
        }
        
        int itemCount = ListView_GetItemCount(m_hListView);
        for (int i = 0; i < itemCount; i++) {
            ListView_SetItemState(m_hListView, i, 
                                 selectAll ? LVIS_SELECTED : 0, LVIS_SELECTED);
        }
        
        YG_LOG_INFO(L"全选操作: " + String(selectAll ? L"全选" : L"取消全选") + 
                   L", 影响程序数: " + std::to_wstring(itemCount));
    }
    
    void MainWindow::CreateSystemTray() {
        YG_LOG_INFO(L"创建系统托盘图标");
        
        // 设置托盘数据
        m_nid.cbSize = sizeof(NOTIFYICONDATAW);
        m_nid.hWnd = m_hWnd;
        m_nid.uID = 1;
        m_nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
        m_nid.uCallbackMessage = WM_TRAYICON;
        
        // 设置图标
        m_nid.hIcon = LoadIcon(m_hInstance, MAKEINTRESOURCE(IDI_MAIN_ICON));
        if (!m_nid.hIcon) {
            m_nid.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
        }
        
        // 设置提示文本
        wcscpy_s(m_nid.szTip, L"YG Uninstaller - 程序卸载工具");
        
        YG_LOG_INFO(L"系统托盘图标创建完成");
    }
    
    void MainWindow::ShowSystemTray(bool show) {
        if (show) {
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
    
    void MainWindow::OnTrayNotify(WPARAM wParam, LPARAM lParam) {
        if (wParam != 1) return; // 确保是我们的托盘图标
        
        switch (lParam) {
            case WM_LBUTTONUP:
                // 左键点击 - 恢复窗口
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
                    SetForegroundWindow(m_hWnd);
                    
                    TrackPopupMenu(hTrayMenu, TPM_RIGHTBUTTON, pt.x, pt.y, 0, m_hWnd, nullptr);
                    
                    // 清理菜单
                    DestroyMenu(hTrayMenu);
                    
                    // 发送一个空消息以确保菜单正确消失
                    PostMessage(m_hWnd, WM_NULL, 0, 0);
                }
                break;
        }
    }
    
    void MainWindow::MinimizeToTray() {
        if (!m_isInTray) {
            ShowSystemTray(true);
        }
        
        if (m_isInTray) {
            ShowWindow(m_hWnd, SW_HIDE);
            YG_LOG_INFO(L"窗口已最小化到系统托盘");
        }
    }
    
    void MainWindow::RestoreFromTray() {
        ShowWindow(m_hWnd, SW_RESTORE);
        SetForegroundWindow(m_hWnd);
        YG_LOG_INFO(L"窗口已从系统托盘恢复");
    }
    
    void MainWindow::ShowSettingsDialog() {
        MessageBoxW(m_hWnd, 
                   L"设置功能正在开发中...\n\n"
                   L"计划功能：\n"
                   L"• 启动时自动扫描\n"
                   L"• 关闭到托盘设置\n"
                   L"• 扫描深度设置\n"
                   L"• 界面主题设置", 
                   L"设置", 
                   MB_OK | MB_ICONINFORMATION);
        
        YG_LOG_INFO(L"显示设置对话框");
    }
    
} // namespace YG
