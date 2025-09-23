/**
 * @file MainWindow.cpp
 * @brief 主窗口实现
 * @author gmrchzh@gmail.com
 * @version 1.0.1
 * @date 2025-09-17
 */

#include "ui/MainWindow.h"
#include "ui/MainWindowSettings.h"
#include "ui/MainWindowLogs.h"
#include "ui/MainWindowTray.h"
#include "ui/ResourceManager.h"
#include "ui/CleanupDialog.h"
#include "services/ResidualScanner.h"
#include "utils/UIUtils.h"
#include "utils/StringUtils.h"
#include "utils/RegistryHelper.h"
#include "core/Logger.h"
#include "core/ErrorHandler.h"
#include "core/Config.h"
#include "services/DirectProgramScanner.h"
#include "../resources/resource.h"  // 包含资源定义
#include <cstdio>  // 为swprintf_s提供支持
#include <commctrl.h>  // ListView控件支持
#include <windowsx.h>  // 窗口消息宏
#include <algorithm>   // 为std::sort提供支持
#include <shellapi.h>  // 为ShellExecute提供支持
#include <cwctype>      // iswspace
#include <uxtheme.h>   // SetWindowTheme
#ifdef _MSC_VER
#pragma comment(lib, "uxtheme.lib")
#endif

// 强制退出定时器回调函数
VOID CALLBACK ForceExitTimerProc(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime) {
    UNREFERENCED_PARAMETER(hwnd);
    UNREFERENCED_PARAMETER(uMsg);
    UNREFERENCED_PARAMETER(idEvent);
    UNREFERENCED_PARAMETER(dwTime);
    
    YG_LOG_WARNING(L"2秒超时，开始强制终止进程");
    OutputDebugStringW(L"窗口关闭2秒后强制退出\n");
    
    // 多重强制退出方法
    try {
        // 方法1：TerminateProcess
        TerminateProcess(GetCurrentProcess(), 0);
    } catch (...) {
        // 方法2：ExitProcess
        try {
            ExitProcess(0);
        } catch (...) {
            // 方法3：系统调用
            _exit(0);
        }
    }
}

// 前向声明自定义属性对话框类
namespace YG {
    class CustomPropertyDialog;
    class ClickablePropertyDialog;
}

namespace YG {
    
    MainWindow::MainWindow() : m_hWnd(nullptr), m_hInstance(nullptr),
                              m_hMenu(nullptr), m_hContextMenu(nullptr),
                              m_hToolbar(nullptr), m_hStatusBar(nullptr), m_hListView(nullptr),
                              m_hSearchEdit(nullptr), m_hProgressBar(nullptr), m_hLeftPanel(nullptr),
                              m_hRightPanel(nullptr), m_hDetailsEdit(nullptr), m_hBottomSearchEdit(nullptr), m_hImageList(nullptr),
                              m_includeSystemComponents(false), m_showWindowsUpdates(false),
                              m_isScanning(false), m_isUninstalling(false), m_isListViewMode(false),
                              m_scrollBarsHidden(false), m_originalListViewProc(nullptr), m_sortColumn(0), m_sortAscending(true) {
        
        // 创建资源管理器（最先创建）
        m_resourceManager = YG::MakeUnique<ResourceManager>();
        
        // 初始化日志管理器
        m_logManager = YG::MakeUnique<MainWindowLogs>(this);
        
        // 初始化系统托盘管理器
        m_trayManager = YG::MakeUnique<MainWindowTray>(this);
        
        // 初始化设置管理器
        m_settingsManager = YG::MakeUnique<MainWindowSettings>(this);
        
        // 初始化残留扫描器
        m_residualScanner = std::shared_ptr<ResidualScanner>(new ResidualScanner());
        
        YG_LOG_INFO(L"MainWindow构造函数完成，所有管理器已创建");
    }
    
    MainWindow::~MainWindow() {
        YG_LOG_INFO(L"MainWindow析构函数开始 - 使用RAII资源管理");
        
        try {
            // 设置清理函数到资源管理器
            if (m_resourceManager) {
                // 设置服务清理函数
                m_resourceManager->SetServiceCleanup(
                    [this]() {
        if (m_programDetector) {
                            YG_LOG_INFO(L"停止程序检测器");
            m_programDetector->StopScan();
            m_programDetector.reset();
        }
                    },
                    [this]() {
        if (m_uninstallerService) {
                            YG_LOG_INFO(L"释放卸载服务");
            m_uninstallerService.reset();
                        }
                    }
                );
                
                // 设置系统托盘清理函数
                m_resourceManager->SetTrayCleanup([this]() {
                    if (m_trayManager && m_trayManager->IsInTray()) {
                        YG_LOG_INFO(L"清理系统托盘");
                        m_trayManager->ShowSystemTray(false);
                        m_trayManager->Cleanup();
                    }
                });
                
                // 设置窗口和控件资源
                m_resourceManager->SetMainWindow(m_hWnd);
                m_resourceManager->SetMenus(m_hMenu, m_hContextMenu);
                m_resourceManager->SetControls(m_hListView, m_originalListViewProc);
                m_resourceManager->SetImageList(m_hImageList);
                
                // 触发清理（ResourceManager析构函数会自动调用）
                YG_LOG_INFO(L"资源管理器将自动清理所有资源");
            }
            
            // 等待一小段时间确保清理完成
            Sleep(100);
            
        } catch (const std::exception& e) {
            YG_LOG_ERROR(L"析构函数中发生标准异常: " + StringToWString(e.what()));
        } catch (...) {
            YG_LOG_ERROR(L"析构函数中发生未知异常");
        }
        
        YG_LOG_INFO(L"MainWindow析构函数完成");
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
        // 使用经典卸载工具的背景颜色
        wcex.hbrBackground = CreateSolidBrush(RGB(255, 255, 255)); // 白色背景
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
        
        // 计算屏幕中央位置 - 设置窗口尺寸为900x750
        int screenWidth = GetSystemMetrics(SM_CXSCREEN);
        int screenHeight = GetSystemMetrics(SM_CYSCREEN);
        int windowWidth = 900;   // 窗口宽度900像素
        int windowHeight = 750;  // 窗口高度750像素
        int x = (screenWidth - windowWidth) / 2;
        int y = (screenHeight - windowHeight) / 2;
        
        YG_LOG_INFO(L"屏幕尺寸: " + std::to_wstring(screenWidth) + L"x" + std::to_wstring(screenHeight) + 
                   L", 窗口位置: (" + std::to_wstring(x) + L"," + std::to_wstring(y) + L")");
        
        m_hWnd = CreateWindowW(
            WINDOW_CLASS_NAME,
            WINDOW_TITLE,
            WS_OVERLAPPEDWINDOW,
            x, y,  // 使用计算的中央位置
            windowWidth, windowHeight,
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
        YG_LOG_INFO(L"开始消息循环");
        MSG msg;
        int exitCode = 0;
        
        while (true) {
            // 使用PeekMessage而不是GetMessage，避免阻塞
            BOOL bRet = PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE);
            
            if (bRet) {
            // 检查是否是退出消息
            if (msg.message == WM_QUIT) {
                YG_LOG_INFO(L"收到WM_QUIT消息，退出码: " + std::to_wstring(msg.wParam));
                exitCode = static_cast<int>(msg.wParam);
                break;
            }
            
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            } else {
                // 没有消息时，检查窗口是否还存在
                if (!m_hWnd || !IsWindow(m_hWnd)) {
                    YG_LOG_INFO(L"窗口已销毁，退出消息循环");
                    break;
                }
                
                // 短暂休眠，避免CPU占用过高
                Sleep(1);
            }
        }
        
        YG_LOG_INFO(L"消息循环结束，退出码: " + std::to_wstring(exitCode));
        
        // 强制等待确保所有消息处理完毕
        Sleep(100);
        
        return exitCode;
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
            
            // 扫描完成后，确保隐藏水平滚动条
            m_scrollBarsHidden = false;  // 重置状态，允许重新隐藏滚动条
            ForceHideScrollBars();
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
        // 将仅空白的输入视为“空搜索”
        bool isBlank = keyword.empty() || std::all_of(keyword.begin(), keyword.end(), [](wchar_t ch){ return std::iswspace(ch) != 0; });
        m_currentSearchKeyword = isBlank ? L"" : keyword;
        YG_LOG_INFO(L"搜索程序: " + (isBlank ? String(L"<空>") : keyword));
        
        if (isBlank) {
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
        
        // 更新状态栏 - 搜索结果特殊处理
        if (!isBlank) {
            // 搜索模式下的状态栏显示
            String statusText;
            if (m_filteredPrograms.empty()) {
                statusText = L"搜索结果: 未找到匹配的程序";
            } else {
                DWORD64 totalSize = 0;
                for (const auto& program : m_filteredPrograms) {
                    totalSize += program.estimatedSize;
                }
                
                statusText = L"搜索结果: 找到 " + std::to_wstring(m_filteredPrograms.size()) + L" 个程序";
                if (totalSize > 0) {
                    statusText += L", 占用空间: " + FormatFileSize(totalSize);
                }
            }
            SetStatusText(statusText);
        } else {
            // 非搜索模式，使用标准状态栏逻辑
            UpdateStatusBarForSelection();
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
            
            // 设置卸载完成回调
            m_uninstallerService->SetUninstallCompleteCallback(
                [this](const ProgramInfo& program, bool success) {
                    OnUninstallComplete(program, success);
                });
            
            // 设置残留扫描器
            m_uninstallerService->SetResidualScanner(m_residualScanner);
        }
        
        // 保存当前卸载的程序信息
        m_currentUninstallingProgram = selectedProgram;
        
        // 启动卸载
        SetStatusText(L"正在卸载 " + programName + L"...");
        ErrorCode uninstallResult = m_uninstallerService->UninstallProgram(selectedProgram, mode);
        
        if (uninstallResult == ErrorCode::Success) {
            MessageBoxW(m_hWnd, (L"程序 \"" + programName + L"\" 卸载完成！").c_str(), 
                       L"卸载成功", MB_OK | MB_ICONINFORMATION);
            // 注意：不在这里刷新程序列表，通过回调机制处理
            SetStatusText(L"卸载完成，等待后续处理...");
        } else {
            String errorMsg = L"程序卸载失败！\n\n程序：" + programName;
            MessageBoxW(m_hWnd, errorMsg.c_str(), L"卸载失败", MB_OK | MB_ICONERROR);
            SetStatusText(L"卸载失败");
        }
        
        YG_LOG_INFO(L"开始卸载程序: " + programName);
    }
    
    
    void MainWindow::SetStatusText(const String& text) {
        if (m_hStatusBar) {
            // 设置状态栏文本
            SendMessageW(m_hStatusBar, SB_SETTEXTW, 0, (LPARAM)text.c_str());
        }
        YG_LOG_INFO(L"状态文本: " + text);
    }
    
    void MainWindow::UpdateStatusBarForSelection() {
        if (!m_hListView || !m_hStatusBar) {
            return;
        }
        
        // 获取选中的项目数量
        int selectedCount = ListView_GetSelectedCount(m_hListView);
        
        if (selectedCount == 0) {
            // 没有选中程序，显示总体统计信息
            // 使用当前显示的程序列表（可能是搜索结果或全部程序）
            const auto& displayPrograms = m_filteredPrograms.empty() ? m_programs : m_filteredPrograms;
            int totalCount = static_cast<int>(displayPrograms.size());
            
            if (totalCount == 0) {
                // 如果还没有程序数据，显示0个程序
                SetStatusText(L"共计 0 个程序");
                return;
            }
            
            DWORD64 totalSize = 0;
            
            // 计算总占用空间
            for (const auto& program : displayPrograms) {
                totalSize += program.estimatedSize; // estimatedSize已经是字节单位
            }
            
            String statusText = L"共计 " + std::to_wstring(totalCount) + L" 个程序";
            if (totalSize > 0) {
                statusText += L", 占用空间: " + FormatFileSize(totalSize);
            }
            
            SetStatusText(statusText);
            
        } else if (selectedCount == 1) {
            // 选中了一个程序，显示该程序的详细信息
            ProgramInfo selectedProgram;
            if (GetSelectedProgram(selectedProgram)) {
                String programName = !selectedProgram.displayName.empty() ? 
                                   selectedProgram.displayName : selectedProgram.name;
                
                String statusText = L"已选中: " + programName;
                if (selectedProgram.estimatedSize > 0) {
                    statusText += L", 大小: " + FormatFileSize(selectedProgram.estimatedSize);
                }
                
                SetStatusText(statusText);
            }
            
        } else {
            // 选中了多个程序，显示多选统计
            DWORD64 selectedSize = 0;
            std::vector<ProgramInfo> selectedPrograms;
            GetSelectedPrograms(selectedPrograms);
            
            for (const auto& program : selectedPrograms) {
                selectedSize += program.estimatedSize; // estimatedSize已经是字节单位
            }
            
            String statusText = L"已选中 " + std::to_wstring(selectedCount) + L" 个程序";
            if (selectedSize > 0) {
                statusText += L", 总大小: " + FormatFileSize(selectedSize);
            }
            
            SetStatusText(statusText);
        }
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
                {
                    LPNMHDR pNMHDR = (LPNMHDR)lParam;
                    YG_LOG_INFO(L"WM_NOTIFY消息：控件ID=" + std::to_wstring(pNMHDR->idFrom) + 
                               L", 通知代码=" + std::to_wstring(pNMHDR->code) + 
                               L", 来源句柄=" + std::to_wstring(reinterpret_cast<uintptr_t>(pNMHDR->hwndFrom)));
                    return OnNotify(pNMHDR);
                }
            case WM_CONTEXTMENU:
                return OnContextMenu(LOWORD(lParam), HIWORD(lParam));
            case WM_TRAYICON:
                if (m_trayManager) {
                    m_trayManager->OnTrayNotify(wParam, lParam);
                }
                return 0;
            case WM_CLOSE:
                // 根据设置决定关闭行为
                if (m_settingsManager && m_settingsManager->GetSettings().closeToTray) {
                    // 始终最小化到托盘（满足你的需求）
                    m_trayManager->MinimizeToTray();
                    return 0;
                } else {
                    // 显示退出确认对话框
                    int result = MessageBoxW(m_hWnd, 
                        L"确定要退出YG Uninstaller吗？\r\n\r\n"
                        L"退出后将无法继续管理已安装的程序。",
                        L"退出确认",
                        MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2);
                    
                    if (result == IDYES) {
                        // 用户确认退出 - 优雅清理所有资源
                        YG_LOG_INFO(L"WM_CLOSE: 用户确认退出，开始优雅退出流程");
                        
                        // 停止所有定时器
                        KillTimer(m_hWnd, 1);  // 停止扫描定时器
                        KillTimer(m_hWnd, 2);  // 停止滚动条监控定时器
                        
                        // 恢复ListView的原始窗口过程
                        if (m_hListView && m_originalListViewProc) {
                            SetWindowLongPtr(m_hListView, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(m_originalListViewProc));
                            m_originalListViewProc = nullptr;
                        }
                    
                    // 强制停止所有后台操作
                    if (m_programDetector) {
                        YG_LOG_INFO(L"WM_CLOSE: 停止程序检测器");
                        m_programDetector->StopScan();
                        m_programDetector.reset();
                    }
                    
                    if (m_uninstallerService) {
                        YG_LOG_INFO(L"WM_CLOSE: 释放卸载服务");
                        m_uninstallerService.reset();
                    }
                    
                    // 清理托盘图标
                    if (m_trayManager && m_trayManager->IsInTray()) {
                        YG_LOG_INFO(L"WM_CLOSE: 清理系统托盘");
                        m_trayManager->ShowSystemTray(false);
                        m_trayManager->Cleanup();
                    }
                        
                        // 清理图标列表
                        if (m_hImageList) {
                            YG_LOG_INFO(L"WM_CLOSE: 清理图标列表");
                            ImageList_Destroy(m_hImageList);
                            m_hImageList = nullptr;
                        }
                        
                        // 强制刷新日志
                        Logger::GetInstance().Flush();
                    
                    // 强制等待资源释放
                        Sleep(300);
                        
                        YG_LOG_INFO(L"WM_CLOSE: 销毁窗口");
                        // 先销毁窗口
                        if (m_hWnd) {
                            DestroyWindow(m_hWnd);
                            m_hWnd = nullptr;
                        }
                    
                    YG_LOG_INFO(L"WM_CLOSE: 发送退出消息");
                    PostQuitMessage(0);
                    
                        // 备用：如果消息循环未能正常退出，直接终止进程
                        YG_LOG_INFO(L"WM_CLOSE: 触发PostQuitMessage(0)并准备直接退出");
                        ExitProcess(0);
                    } else {
                        // 用户取消退出，不做任何操作
                        YG_LOG_INFO(L"WM_CLOSE: 用户取消退出");
                    }
                }
                return 0;
            case WM_SHOWWINDOW:
                {
                    if (wParam && !m_hListView) {  // 窗口显示且控件还未创建
                        YG_LOG_INFO(L"窗口显示，开始创建控件");
                        ErrorCode result = CreateControls();
                        if (result == ErrorCode::Success) {
                            YG_LOG_INFO(L"控件创建成功，开始扫描真实程序列表");
                            
                            // 立即更新状态栏显示初始状态
                            UpdateStatusBarForSelection();
                            
                            // 第一次启动时自动扫描程序列表
                            SetStatusText(L"正在扫描已安装的程序...");
                            
                            // 在ListView中显示扫描状态
                            if (m_hListView) {
                                SetWindowTextW(m_hListView, L"YG Uninstaller - 程序卸载工具\r\n\r\n"
                                                           L"正在扫描已安装的程序...\r\n\r\n"
                                                           L"请耐心等待，这可能需要几秒钟时间。\r\n\r\n"
                                                           L"扫描完成后您将看到：\r\n"
                                                           L"• 完整的已安装程序列表\r\n"
                                                           L"• 程序版本、发布者、大小等信息\r\n"
                                                           L"• 支持卸载、批量卸载等功能\r\n"
                                                           L"• 可按程序名称进行搜索");
                            }
                            
                            // 延迟一下再开始扫描，让用户看到提示信息
                            Sleep(500);
                            
                            // 开始自动扫描程序列表
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
                        // 定时器触发，但不自动刷新程序列表
                        KillTimer(m_hWnd, 1);  // 停止定时器
                        YG_LOG_INFO(L"定时器1触发，但不自动刷新程序列表");
                        // 不再自动刷新，等待用户手动操作
                    } else if (wParam == 2) {
                        // 滚动条监控定时器
                        ForceHideScrollBars();  // 隐藏水平滚动条，保留垂直滚动条
                    } else if (wParam == 3) {
                        // 显示清理对话框定时器
                        KillTimer(m_hWnd, 3);  // 停止定时器
                        YG_LOG_INFO(L"定时器3触发，显示清理对话框");
                        ShowCleanupDialog(m_currentUninstallingProgram);
                    } else if (wParam == 4) {
                        // 延迟刷新程序列表定时器
                        KillTimer(m_hWnd, 4);  // 停止定时器
                        YG_LOG_INFO(L"定时器4触发，刷新程序列表");
                        RefreshProgramList(m_includeSystemComponents);
                    }
                }
                return 0;
            case WM_MOUSEWHEEL:
                {
                    // 处理鼠标滚轮消息，转发给ListView进行正常滚动
                    if (m_hListView && m_isListViewMode) {
                        // 将鼠标滚轮消息转发给ListView控件
                        SendMessage(m_hListView, WM_MOUSEWHEEL, wParam, lParam);
                        
                        // 立即隐藏水平滚动条（如果出现的话）
                        ShowScrollBar(m_hListView, SB_HORZ, FALSE);
                        
                        // 强制隐藏水平滚动条（如果出现的话）
                        m_scrollBarsHidden = false;  // 重置状态，允许重新隐藏滚动条
                        ForceHideScrollBars();
                        
                        YG_LOG_INFO(L"鼠标滚轮滚动已转发给ListView，强制隐藏水平滚动条");
                    }
                    return 0;
                }
            case WM_USER + 100:
                {
                    // 处理卸载完成消息
                    bool success = (lParam != 0);
                    HandleUninstallComplete(success);
                    return 0;
                }
            case WM_USER + 101:
                {
                    // 处理残留扫描进度消息
                    int percentage = static_cast<int>(wParam);
                    int foundCount = static_cast<int>(lParam);
                    HandleResidualScanProgress(percentage, foundCount);
                    return 0;
                }
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
        YG_LOG_INFO(L"主窗口正在销毁，开始彻底清理资源");
        
        // 停止所有定时器（包括强制退出定时器）
        KillTimer(m_hWnd, 1);  // 停止扫描定时器
        KillTimer(m_hWnd, 2);  // 停止滚动条监控定时器
        KillTimer(m_hWnd, 9999);  // 停止强制退出定时器
        
        // 恢复ListView的原始窗口过程
        if (m_hListView && m_originalListViewProc) {
            SetWindowLongPtr(m_hListView, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(m_originalListViewProc));
            m_originalListViewProc = nullptr;
            YG_LOG_INFO(L"OnDestroy: ListView窗口过程已恢复");
        }
        
        // 确保停止所有后台操作
        if (m_programDetector) {
            YG_LOG_INFO(L"OnDestroy: 停止程序检测器");
            m_programDetector->StopScan();
            m_programDetector.reset();
        }
        
        // 确保停止卸载服务
        if (m_uninstallerService) {
            YG_LOG_INFO(L"OnDestroy: 停止卸载服务");
            // 如果有正在进行的卸载操作，等待完成
            m_uninstallerService.reset();
        }
        
        // 清理系统托盘
        if (m_trayManager && m_trayManager->IsInTray()) {
            YG_LOG_INFO(L"OnDestroy: 清理系统托盘");
            m_trayManager->ShowSystemTray(false);
            m_trayManager->Cleanup();
        }
        
        // 清理图标列表
        if (m_hImageList) {
            YG_LOG_INFO(L"OnDestroy: 清理图标列表");
            ImageList_Destroy(m_hImageList);
            m_hImageList = nullptr;
        }
        
        // 清理菜单
        if (m_hMenu) {
            YG_LOG_INFO(L"OnDestroy: 清理主菜单");
            DestroyMenu(m_hMenu);
            m_hMenu = nullptr;
        }
        
        if (m_hContextMenu) {
            YG_LOG_INFO(L"OnDestroy: 清理上下文菜单");
            DestroyMenu(m_hContextMenu);
            m_hContextMenu = nullptr;
        }
        
        // 强制刷新日志
        Logger::GetInstance().Flush();
        
        YG_LOG_INFO(L"OnDestroy: 资源清理完成");
        
        // 清理完成，正常退出
        YG_LOG_INFO(L"OnDestroy: 资源清理完成，程序正常退出");
        
        return 0;
    }
    
    LRESULT MainWindow::OnSize(int width, int height) {
        (void)width;   // 抑制警告
        (void)height;
        
        // 调整控件布局
        AdjustLayout();
        
        // 窗口大小变化时重置滚动条状态并强制隐藏滚动条
        m_scrollBarsHidden = false;  // 重置状态，允许重新处理滚动条
        ForceHideScrollBars();  // 隐藏水平滚动条，保留垂直滚动条
        
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
        
        // 处理顶部搜索框
        if (commandId == 3001 && notificationCode == EN_CHANGE) {
            // 获取搜索框文本
            wchar_t searchText[256] = {0};
            GetWindowTextW(m_hSearchEdit, searchText, sizeof(searchText)/sizeof(wchar_t));
            
            // 执行搜索
            SearchPrograms(String(searchText));
            return 0;
        }
        
        // 处理底部搜索框
        if (commandId == 4021 && notificationCode == EN_CHANGE) {
            // 获取底部搜索框文本
            wchar_t searchText[256] = {0};
            GetWindowTextW(m_hBottomSearchEdit, searchText, sizeof(searchText)/sizeof(wchar_t));
            
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
                {
                    // 即使开启了“关闭最小化到托盘”，也执行真正退出
                    int result = MessageBoxW(m_hWnd, 
                        L"确定要退出YG Uninstaller吗？\r\n\r\n"
                        L"退出后将无法继续管理已安装的程序。",
                        L"退出确认",
                        MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2);
                    if (result != IDYES) {
                        break;
                    }

                    // 停止所有定时器
                    KillTimer(m_hWnd, 1);  // 停止扫描定时器
                    KillTimer(m_hWnd, 2);  // 停止滚动条监控定时器
                    KillTimer(m_hWnd, 9999);  // 停止强制退出定时器（若存在）

                    // 恢复ListView的原始窗口过程
                    if (m_hListView && m_originalListViewProc) {
                        SetWindowLongPtr(m_hListView, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(m_originalListViewProc));
                        m_originalListViewProc = nullptr;
                    }

                    // 停止后台操作
                    if (m_programDetector) {
                        m_programDetector->StopScan();
                        m_programDetector.reset();
                    }
                    if (m_uninstallerService) {
                        m_uninstallerService.reset();
                    }

                    // 清理托盘
                    if (m_trayManager && m_trayManager->IsInTray()) {
                        m_trayManager->ShowSystemTray(false);
                        m_trayManager->Cleanup();
                    }

                    // 清理图标列表
                    if (m_hImageList) {
                        ImageList_Destroy(m_hImageList);
                        m_hImageList = nullptr;
                    }

                    // 刷新日志
                    Logger::GetInstance().Flush();
                    Sleep(300);

                    // 销毁窗口并退出
                    if (m_hWnd) {
                        DestroyWindow(m_hWnd);
                        m_hWnd = nullptr;
                    }
                    PostQuitMessage(0);

                    // 备用强制退出机制
                    SetTimer(m_hWnd, 9999, 2000, ForceExitTimerProc);
                }
                break;
                
            // 操作菜单
            case ID_ACTION_REFRESH:
                RefreshProgramList(m_includeSystemComponents);
                break;
            case ID_ACTION_UNINSTALL:
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
                    YG_LOG_INFO(L"=== ID_ACTION_PROPERTIES被触发 ===");
                    
                    try {
                        // 先用简单的MessageBox测试
                    ProgramInfo selectedProgram;
                        YG_LOG_INFO(L"开始调用GetSelectedProgram");
                        
                    if (GetSelectedProgram(selectedProgram)) {
                            YG_LOG_INFO(L"找到选中程序: " + selectedProgram.name);
                            YG_LOG_INFO(L"开始调用ShowCustomPropertyDialog");
                            
                            // 使用新的自定义属性对话框
                            ShowCustomPropertyDialog(selectedProgram);
                            
                            YG_LOG_INFO(L"ShowCustomPropertyDialog调用完成");
                            
                    } else {
                            YG_LOG_WARNING(L"没有选中程序");
                            MessageBox(m_hWnd, L"请先选择一个程序。", L"提示", MB_OK | MB_ICONWARNING);
                        }
                    } catch (...) {
                        YG_LOG_ERROR(L"属性对话框出现异常");
                        MessageBox(m_hWnd, L"属性对话框出现错误，请查看日志。", L"错误", MB_OK | MB_ICONERROR);
                    }
                    
                    YG_LOG_INFO(L"=== ID_ACTION_PROPERTIES处理完成 ===");
                }
                break;
            case ID_ACTION_OPEN_LOCATION:
                {
                    ProgramInfo selectedProgram;
                    if (GetSelectedProgram(selectedProgram)) {
                        String programName = !selectedProgram.displayName.empty() ? selectedProgram.displayName : selectedProgram.name;
                        
                        // 检查安装位置是否有效，如果为空则尝试多种方法获取安装路径
                        String installPath = selectedProgram.installLocation;
                        
                        // 方法1: 尝试从卸载字符串中提取安装路径
                        if (installPath.empty() && !selectedProgram.uninstallString.empty()) {
                            installPath = ExtractPathFromUninstallString(selectedProgram.uninstallString);
                        }
                        
                        // 方法2: 尝试从图标路径中提取安装路径
                        if (installPath.empty() && !selectedProgram.iconPath.empty()) {
                            installPath = ExtractPathFromIconPath(selectedProgram.iconPath);
                        }
                        
                        // 方法3: 尝试从程序名称推导常见安装路径
                        if (installPath.empty()) {
                            installPath = GuessCommonInstallPath(selectedProgram);
                        }
                        
                        // 如果仍然没有安装路径，显示详细的错误信息
                        if (installPath.empty()) {
                            String message = L"程序 \"" + programName + L"\" 无法自动定位安装位置。\n\n"
                                           L"已尝试以下方法：\n"
                                           L"✓ 注册表InstallLocation字段\n"
                                           L"✓ 从卸载字符串提取路径\n"
                                           L"✓ 从图标路径提取路径\n"
                                           L"✓ 根据程序名称猜测常见路径\n\n"
                                           L"可能的原因：\n"
                                           L"• 程序未正确安装或注册表信息损坏\n"
                                           L"• 便携式程序（无固定安装位置）\n"
                                           L"• 系统组件或Windows更新\n"
                                           L"• 程序安装在其他非标准位置\n\n"
                                           L"建议：\n"
                                           L"• 检查程序是否正常安装\n"
                                           L"• 尝试重新安装程序\n"
                                           L"• 手动在文件资源管理器中搜索程序";
                            
                            MessageBoxW(m_hWnd, message.c_str(), L"无法定位安装位置", MB_OK | MB_ICONINFORMATION);
                            break;
                        }
                        
                        // 验证路径是否存在
                        DWORD attributes = GetFileAttributesW(installPath.c_str());
                        if (attributes == INVALID_FILE_ATTRIBUTES || !(attributes & FILE_ATTRIBUTE_DIRECTORY)) {
                            MessageBoxW(m_hWnd, 
                                (L"程序 \"" + programName + L"\" 的安装位置不存在：\n\n"
                                 L"路径：" + installPath + L"\n\n"
                                 L"这可能是因为：\n"
                                 L"• 程序已被卸载但注册表信息未清理\n"
                                 L"• 安装路径已更改\n"
                                 L"• 目录已被删除").c_str(), 
                                L"安装位置不存在", MB_OK | MB_ICONWARNING);
                            break;
                        }
                        
                        // 使用ShellExecuteW打开文件资源管理器并定位到安装目录
                        HINSTANCE result = ShellExecuteW(
                            m_hWnd,                    // 父窗口句柄
                            L"explore",               // 操作：打开并浏览
                            installPath.c_str(),       // 目标路径
                            nullptr,                   // 参数
                            nullptr,                   // 工作目录
                            SW_SHOWNORMAL             // 显示方式
                        );
                        
                        // 检查执行结果
                        if ((INT_PTR)result <= 32) {
                            String errorMsg = L"无法打开安装位置：\n\n";
                            errorMsg += L"程序：" + programName + L"\n";
                            errorMsg += L"路径：" + installPath + L"\n\n";
                            
                            // 根据错误代码提供相应的错误信息
                            if ((INT_PTR)result == ERROR_FILE_NOT_FOUND || (INT_PTR)result == SE_ERR_FNF) {
                                errorMsg += L"错误：文件或目录未找到";
                            } else if ((INT_PTR)result == ERROR_PATH_NOT_FOUND || (INT_PTR)result == SE_ERR_PNF) {
                                errorMsg += L"错误：路径未找到";
                            } else if ((INT_PTR)result == ERROR_ACCESS_DENIED) {
                                errorMsg += L"错误：访问被拒绝";
                            } else if ((INT_PTR)result == ERROR_BAD_FORMAT) {
                                errorMsg += L"错误：文件格式无效";
                            } else if ((INT_PTR)result == SE_ERR_ASSOCINCOMPLETE) {
                                errorMsg += L"错误：文件关联不完整";
                            } else if ((INT_PTR)result == SE_ERR_NOASSOC) {
                                errorMsg += L"错误：没有关联的程序";
                            } else if ((INT_PTR)result == SE_ERR_DDEBUSY) {
                                errorMsg += L"错误：DDE事务无法完成";
                            } else if ((INT_PTR)result == SE_ERR_DDEFAIL) {
                                errorMsg += L"错误：DDE事务失败";
                            } else if ((INT_PTR)result == SE_ERR_DDETIMEOUT) {
                                errorMsg += L"错误：DDE事务超时";
                            } else if ((INT_PTR)result == SE_ERR_DLLNOTFOUND) {
                                errorMsg += L"错误：找不到指定的DLL";
                            } else if ((INT_PTR)result == SE_ERR_OOM) {
                                errorMsg += L"错误：内存不足";
                            } else if ((INT_PTR)result == SE_ERR_SHARE) {
                                errorMsg += L"错误：共享冲突";
                            } else {
                                errorMsg += L"错误代码：" + std::to_wstring((INT_PTR)result);
                            }
                            
                            MessageBoxW(m_hWnd, errorMsg.c_str(), L"打开失败", MB_OK | MB_ICONERROR);
                        } else {
                            // 成功打开，记录日志
                            YG_LOG_INFO(L"成功打开程序安装位置: " + programName + L" -> " + installPath);
                            
                            // 显示成功消息（可选）
                            SetStatusText(L"已打开安装位置：" + installPath);
                        }
                    } else {
                        MessageBoxW(m_hWnd, L"请先选择一个程序。", L"提示", MB_OK | MB_ICONWARNING);
                    }
                }
                break;
                
            // 右键菜单处理
            case ID_CM_UNINSTALL:
                SendMessage(m_hWnd, WM_COMMAND, ID_ACTION_UNINSTALL, 0);
                break;
            case ID_CM_FORCE_UNINSTALL:
                SendMessage(m_hWnd, WM_COMMAND, ID_ACTION_FORCE_UNINSTALL, 0);
                break;
            case ID_CM_PROPERTIES:
                SendMessage(m_hWnd, WM_COMMAND, ID_ACTION_PROPERTIES, 0);
                break;
            case ID_CM_OPEN_LOCATION:
                SendMessage(m_hWnd, WM_COMMAND, ID_ACTION_OPEN_LOCATION, 0);
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
            case ID_VIEW_LARGE_ICONS:
                SetListViewMode(LV_VIEW_ICON);
                UpdateViewMenu(ID_VIEW_LARGE_ICONS);
                SetStatusText(L"切换到大图标视图");
                break;
            case ID_VIEW_SMALL_ICONS:
                SetListViewMode(LV_VIEW_SMALLICON);
                UpdateViewMenu(ID_VIEW_SMALL_ICONS);
                SetStatusText(L"切换到小图标视图");
                break;
            case ID_VIEW_LIST:
                SetListViewMode(LV_VIEW_LIST);
                UpdateViewMenu(ID_VIEW_LIST);
                SetStatusText(L"切换到列表视图");
                break;
            case ID_VIEW_DETAILS:
                SetListViewMode(LV_VIEW_DETAILS);
                UpdateViewMenu(ID_VIEW_DETAILS);
                SetStatusText(L"切换到详细信息视图");
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
            case ID_VIEW_SHOW_UPDATES:
                {
                    HMENU hMenu = GetMenu(m_hWnd);
                    HMENU hViewMenu = GetSubMenu(hMenu, 2);
                    UINT state = GetMenuState(hViewMenu, ID_VIEW_SHOW_UPDATES, MF_BYCOMMAND);
                    bool isChecked = (state & MF_CHECKED) != 0;
                    
                    CheckMenuItem(hViewMenu, ID_VIEW_SHOW_UPDATES, MF_BYCOMMAND | (isChecked ? MF_UNCHECKED : MF_CHECKED));
                    m_showWindowsUpdates = !isChecked;
                    RefreshProgramList(m_includeSystemComponents);
                    SetStatusText(m_showWindowsUpdates ? L"显示Windows更新" : L"隐藏Windows更新");
                }
                break;
                
            // 帮助菜单
            case ID_HELP_HELP:
                MessageBoxW(m_hWnd, L"YG Uninstaller 帮助\n\n这是一个高效的程序卸载工具。\n\n功能:\n• 扫描已安装程序\n• 标准卸载\n• 强制卸载\n• 深度清理", L"帮助", MB_OK | MB_ICONINFORMATION);
                break;
            case ID_HELP_WEBSITE:
                OpenWebsite();
                break;
            case ID_HELP_CHECK_UPDATE:
                CheckForUpdates();
                break;
            case ID_HELP_ABOUT:
                MessageBoxW(m_hWnd, L"关于 YG Uninstaller\n\n版本：1.0.1\n\n一个高效、轻量级的Windows程序卸载工具。\n\n© 2025 YG Software", L"关于", MB_OK | MB_ICONINFORMATION);
                break;
                
            // 工具菜单
            case ID_TOOLS_SETTINGS:
                if (m_settingsManager) {
                    m_settingsManager->ShowSettingsDialog();
                }
                break;
            case ID_TOOLS_LOG_MANAGER:
                if (m_logManager) {
                    m_logManager->ShowLogManagerDialog();
                }
                break;
                
                
                
            // 托盘菜单
            case ID_TRAY_RESTORE:
                if (m_trayManager) {
                    m_trayManager->RestoreFromTray();
                }
                break;
            case ID_TRAY_EXIT:
                {
                    // 托盘菜单“退出”走真正退出流程
                    int result = MessageBoxW(m_hWnd, 
                        L"确定要退出YG Uninstaller吗？\r\n\r\n"
                        L"退出后将无法继续管理已安装的程序。",
                        L"退出确认",
                        MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2);
                    if (result != IDYES) {
                        break;
                    }

                    // 停止所有定时器
                    KillTimer(m_hWnd, 1);
                    KillTimer(m_hWnd, 2);
                    KillTimer(m_hWnd, 9999);

                    // 恢复ListView过程
                    if (m_hListView && m_originalListViewProc) {
                        SetWindowLongPtr(m_hListView, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(m_originalListViewProc));
                        m_originalListViewProc = nullptr;
                    }

                    // 停止后台
                    if (m_programDetector) { m_programDetector->StopScan(); m_programDetector.reset(); }
                    if (m_uninstallerService) { m_uninstallerService.reset(); }

                    // 清理托盘
                    if (m_trayManager && m_trayManager->IsInTray()) {
                        m_trayManager->ShowSystemTray(false);
                        m_trayManager->Cleanup();
                    }

                    // 清理图标列表
                    if (m_hImageList) { ImageList_Destroy(m_hImageList); m_hImageList = nullptr; }

                    Logger::GetInstance().Flush();
                    Sleep(300);

                    if (m_hWnd) { DestroyWindow(m_hWnd); m_hWnd = nullptr; }
                    PostQuitMessage(0);
                }
                break;
                
            default:
                return DefWindowProc(m_hWnd, WM_COMMAND, MAKEWPARAM(commandId, notificationCode), (LPARAM)controlHandle);
        }
        return 0;
    }
    
    LRESULT MainWindow::OnNotify(LPNMHDR pNMHDR) {
        if (!pNMHDR) return 0;
        
        YG_LOG_INFO(L"OnNotify: 检查ListView句柄匹配 - 消息句柄:" + std::to_wstring(reinterpret_cast<uintptr_t>(pNMHDR->hwndFrom)) + 
                   L", ListView句柄:" + std::to_wstring(reinterpret_cast<uintptr_t>(m_hListView)));
        
        // 处理ListView通知消息 - 只匹配ListView的控件ID 2001
        if (pNMHDR->idFrom == 2001) {
            YG_LOG_INFO(L"OnNotify: ListView匹配成功(ID=" + std::to_wstring(pNMHDR->idFrom) + L"), 处理通知代码: " + std::to_wstring(pNMHDR->code) + L" (十六进制: " + std::to_wstring(static_cast<int>(pNMHDR->code)) + L")");
            switch (pNMHDR->code) {
                case LVN_COLUMNCLICK:
                {
                    // 列标题点击事件
                    LPNMLISTVIEW pNMLV = (LPNMLISTVIEW)pNMHDR;
                    OnColumnHeaderClick(pNMLV->iSubItem);
                    return 0;
                }
                case LVN_ITEMCHANGED:
                {
                    // 列表项选择变化事件
                    LPNMLISTVIEW pNMLV = (LPNMLISTVIEW)pNMHDR;
                    if ((pNMLV->uChanged & LVIF_STATE) && 
                        ((pNMLV->uNewState & LVIS_SELECTED) != (pNMLV->uOldState & LVIS_SELECTED))) {
                        // 选择状态发生了变化，更新状态栏
                        UpdateStatusBarForSelection();
                        
                        // 程序选中事件 - 仅记录日志，不更新详情面板
                        if (pNMLV->uNewState & LVIS_SELECTED) {
                            YG_LOG_INFO(L"检测到程序选中事件，项目索引: " + std::to_wstring(pNMLV->iItem));
                        } else {
                            YG_LOG_INFO(L"程序取消选中");
                        }
                    }
                    return 0;
                }
                case NM_CLICK:
                {
                    // ListView单击事件 - 仅记录日志
                    YG_LOG_INFO(L"检测到ListView单击事件");
                    return 0;
                }
                default:
                    break;
            }
        }
        
        // 处理表头控件的通知消息 - 监听列宽变化
        HWND hHeader = ListView_GetHeader(m_hListView);
        if (hHeader && pNMHDR->hwndFrom == hHeader) {
            switch (pNMHDR->code) {
                case HDN_ITEMCHANGED:
                case HDN_ITEMCHANGING:
                {
                    // 表头列宽正在变化或已变化
                    LPNMHEADERW pNMHeader = (LPNMHEADERW)pNMHDR;
                    if (pNMHeader->pitem && (pNMHeader->pitem->mask & HDI_WIDTH)) {
                        // 列宽发生变化，同步更新内容列宽
                        OnHeaderColumnWidthChanged(pNMHeader->iItem, pNMHeader->pitem->cxy);
                    }
                    return 0;
                }
                case HDN_ENDTRACK:
                {
                    // 用户完成列宽拖动
                    LPNMHEADERW pNMHeader = (LPNMHEADERW)pNMHDR;
                    OnHeaderColumnResizeEnd(pNMHeader->iItem);
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
            case 'R':
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
            AppendMenuW(m_hContextMenu, MF_STRING, ID_CM_UNINSTALL, L"卸载程序");
            AppendMenuW(m_hContextMenu, MF_STRING, ID_CM_FORCE_UNINSTALL, L"强制卸载");
            AppendMenuW(m_hContextMenu, MF_SEPARATOR, 0, nullptr);
            AppendMenuW(m_hContextMenu, MF_STRING, ID_CM_PROPERTIES, L"属性");
            AppendMenuW(m_hContextMenu, MF_STRING, ID_CM_OPEN_LOCATION, L"打开安装位置");
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
        
        // 计算简化的单栏布局
        GetClientRect(m_hWnd, &clientRect);
        int totalWidth = clientRect.right - clientRect.left;
        int totalHeight = clientRect.bottom - clientRect.top;
        
        int statusBarHeight = 25;
        int menuBarHeight = 30;
        int searchBarHeight = 40;  // 搜索栏高度
        int availableHeight = totalHeight - menuBarHeight - statusBarHeight - searchBarHeight;
        
        // 创建搜索区域
        CreateSearchArea(totalWidth, menuBarHeight, searchBarHeight);
        
        // 创建ListView，无边框，左边距20px，右边距0
        m_hListView = CreateWindowExW(
            0,  // 移除WS_EX_CLIENTEDGE边框样式
            WC_LISTVIEWW,
            L"",
            WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SINGLESEL | LVS_SHOWSELALWAYS | WS_VSCROLL,
            20, menuBarHeight + searchBarHeight,  // 左边距20px
            totalWidth - 20, availableHeight,  // 宽度 = 总宽度 - 左边距20px，右边距为0
            m_hWnd,
            (HMENU)2001,
            GetModuleHandle(NULL),
            NULL
        );
        
        // 设置ListView扩展样式
        if (m_hListView) {
            ListView_SetExtendedListViewStyle(m_hListView, 
                LVS_EX_FULLROWSELECT |      // 整行选中高亮
                LVS_EX_INFOTIP |            // 信息提示
                LVS_EX_SUBITEMIMAGES |      // 子项图标支持
                LVS_EX_HEADERDRAGDROP);     // 表头拖放和调整
                // 移除LVS_EX_GRIDLINES网格线以获得更简洁的外观
            
            YG_LOG_INFO(L"ListView创建成功，无边框，简洁外观");
        }
        
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
            // 设置经典卸载工具风格的ListView扩展样式
            ListView_SetExtendedListViewStyle(m_hListView, 
                LVS_EX_FULLROWSELECT |      // 整行选中高亮
                LVS_EX_GRIDLINES |          // 网格线
                LVS_EX_INFOTIP |            // 信息提示
                LVS_EX_SUBITEMIMAGES |      // 子项图标支持
                LVS_EX_HEADERDRAGDROP);     // 表头拖放和调整
            
            // 彻底隐藏水平滚动条，保留垂直滚动条，但保持列宽调整功能
            ShowScrollBar(m_hListView, SB_HORZ, FALSE);   // 隐藏水平滚动条
            ShowScrollBar(m_hListView, SB_BOTH, FALSE);   // 隐藏所有滚动条
            ShowScrollBar(m_hListView, SB_HORZ, FALSE);   // 再次确认隐藏水平滚动条
            
            // 使用SetWindowLong移除水平滚动条样式，保留垂直滚动条
            LONG_PTR style = GetWindowLongPtr(m_hListView, GWL_STYLE);
            style &= ~WS_HSCROLL;  // 移除水平滚动条样式
            // 确保垂直滚动条样式存在
            if (!(style & WS_VSCROLL)) {
                style |= WS_VSCROLL;  // 添加垂直滚动条样式
            }
            SetWindowLongPtr(m_hListView, GWL_STYLE, style);
            
            // 强制禁用水平滚动条，启用垂直滚动条
            EnableScrollBar(m_hListView, SB_HORZ, ESB_DISABLE_BOTH);
            EnableScrollBar(m_hListView, SB_VERT, ESB_ENABLE_BOTH);
            
            // 设置水平滚动条范围为0，彻底禁用
            SCROLLINFO si = {0};
            si.cbSize = sizeof(SCROLLINFO);
            si.fMask = SIF_ALL | SIF_DISABLENOSCROLL;
            si.nMin = 0;
            si.nMax = 0;
            si.nPage = 0;
            si.nPos = 0;
            SetScrollInfo(m_hListView, SB_HORZ, &si, TRUE);  // 强制设置水平滚动条为0
            
            // 使用SendMessage强制隐藏水平滚动条
            SendMessage(m_hListView, WM_HSCROLL, SB_TOP, 0);
            
            // 强制重绘ListView以应用样式变更
            SetWindowPos(m_hListView, NULL, 0, 0, 0, 0, 
                        SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED | SWP_DRAWFRAME);
            
            // 再次确认隐藏水平滚动条
            ShowScrollBar(m_hListView, SB_HORZ, FALSE);
            
            // 子类化ListView以拦截水平滚动消息
            SetWindowLongPtr(m_hListView, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
            m_originalListViewProc = (WNDPROC)SetWindowLongPtr(m_hListView, GWLP_WNDPROC, 
                                                              reinterpret_cast<LONG_PTR>(ListViewSubclassProc));
            
            YG_LOG_INFO(L"ListView扩展样式设置完成，水平滚动条已彻底隐藏，垂直滚动条已保留，支持列宽调整");
            
            // 启动定时器持续监控和隐藏滚动条（大幅降低频率减少闪烁）
            SetTimer(m_hWnd, 2, 5000, NULL);  // 每5秒检查一次滚动条
            
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
            SetStatusText(L"共计 0 个程序");
            YG_LOG_INFO(L"状态栏创建成功");
        } else {
            YG_LOG_WARNING(L"状态栏创建失败，但继续运行");
        }
        
        YG_LOG_INFO(L"所有控件创建完成");
        return ErrorCode::Success;
    }
    
    ErrorCode MainWindow::CreateMenus() {
        YG_LOG_INFO(L"开始创建优化的菜单结构...");
        
        // 创建主菜单
        m_hMenu = CreateMenu();
        if (!m_hMenu) {
            DWORD error = GetLastError();
            YG_LOG_ERROR(L"创建主菜单失败，错误代码: " + std::to_wstring(error));
            return ErrorCode::GeneralError;
        }
        
        // 创建"文件"菜单 - 文件操作
        HMENU hFileMenu = CreatePopupMenu();
        if (hFileMenu) {
            AppendMenuW(hFileMenu, MF_STRING, ID_ACTION_REFRESH, L"刷新程序列表(&R)\tF5");
            AppendMenuW(hFileMenu, MF_SEPARATOR, 0, nullptr);
            AppendMenuW(hFileMenu, MF_STRING, ID_FILE_EXIT, L"退出(&X)");
            AppendMenuW(m_hMenu, MF_POPUP, (UINT_PTR)hFileMenu, L"文件(&F)");
        }
        
        // 创建"操作"菜单 - 程序操作
        HMENU hActionMenu = CreatePopupMenu();
        if (hActionMenu) {
            // 单个程序操作
            AppendMenuW(hActionMenu, MF_STRING, ID_ACTION_UNINSTALL, L"卸载程序(&U)\tDel");
            AppendMenuW(hActionMenu, MF_STRING, ID_ACTION_FORCE_UNINSTALL, L"强制卸载(&F)\tShift+Del");
            AppendMenuW(hActionMenu, MF_STRING, ID_ACTION_DEEP_UNINSTALL, L"深度卸载(&D)\tCtrl+Shift+Del");
            AppendMenuW(hActionMenu, MF_SEPARATOR, 0, nullptr);
            
            // 批量操作
            AppendMenuW(hActionMenu, MF_STRING, ID_ACTION_SELECT_ALL, L"全选程序(&A)\tCtrl+A");
            AppendMenuW(hActionMenu, MF_STRING, ID_ACTION_BATCH_UNINSTALL, L"批量卸载(&B)\tCtrl+Del");
            AppendMenuW(hActionMenu, MF_SEPARATOR, 0, nullptr);
            
            // 程序信息
            AppendMenuW(hActionMenu, MF_STRING, ID_ACTION_PROPERTIES, L"程序属性(&P)\tAlt+Enter");
            AppendMenuW(hActionMenu, MF_STRING, ID_ACTION_OPEN_LOCATION, L"打开安装位置(&O)\tCtrl+O");
            
            AppendMenuW(m_hMenu, MF_POPUP, (UINT_PTR)hActionMenu, L"操作(&A)");
        }
        
        // 创建"查看"菜单 - 重新组织，去除重复功能
        HMENU hViewMenu = CreatePopupMenu();
        if (hViewMenu) {
            // 视图模式分组
            AppendMenuW(hViewMenu, MF_STRING, ID_VIEW_LARGE_ICONS, L"大图标(&L)");
            AppendMenuW(hViewMenu, MF_STRING, ID_VIEW_SMALL_ICONS, L"小图标(&S)");
            AppendMenuW(hViewMenu, MF_STRING, ID_VIEW_LIST, L"列表(&I)");
            AppendMenuW(hViewMenu, MF_STRING | MF_CHECKED, ID_VIEW_DETAILS, L"详细信息(&D)");
            AppendMenuW(hViewMenu, MF_SEPARATOR, 0, nullptr);
            
            // 界面元素控制
            AppendMenuW(hViewMenu, MF_STRING | MF_CHECKED, ID_VIEW_TOOLBAR, L"工具栏(&T)");
            AppendMenuW(hViewMenu, MF_STRING | MF_CHECKED, ID_VIEW_STATUSBAR, L"状态栏(&B)");
            AppendMenuW(hViewMenu, MF_SEPARATOR, 0, nullptr);
            
            // 显示选项
            AppendMenuW(hViewMenu, MF_STRING, ID_VIEW_SHOW_SYSTEM, L"显示系统组件(&C)");
            AppendMenuW(hViewMenu, MF_STRING, ID_VIEW_SHOW_UPDATES, L"显示Windows更新(&W)");
            
            AppendMenuW(m_hMenu, MF_POPUP, (UINT_PTR)hViewMenu, L"查看(&V)");
        }
        
        // 创建"工具"菜单 - 包含设置和其他工具功能
        HMENU hToolsMenu = CreatePopupMenu();
        if (hToolsMenu) {
            AppendMenuW(hToolsMenu, MF_STRING, ID_TOOLS_SETTINGS, L"设置(&S)");
            AppendMenuW(hToolsMenu, MF_SEPARATOR, 0, nullptr);
            AppendMenuW(hToolsMenu, MF_STRING, ID_TOOLS_LOG_MANAGER, L"日志管理(&L)");
            AppendMenuW(m_hMenu, MF_POPUP, (UINT_PTR)hToolsMenu, L"工具(&T)");
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
        
        // 系统托盘由MainWindowTray管理
        
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
        
        // 计算搜索控件的位置（通栏输入，无按钮）
        int leftMargin = 0;         // 左右通栏
        int rightMargin = 0;
        
        int searchY = 2;            // 菜单栏内的垂直位置
        int controlHeight = menuHeight - 4;  // 控件高度
        int searchX = leftMargin;   // 从左侧开始
        int searchWidth = windowWidth - leftMargin - rightMargin; // 全宽度
        
        // 创建现代化搜索框 - Windows 11 风格
        m_hSearchEdit = CreateWindowExW(
            WS_EX_COMPOSITED,  // 去掉ClientEdge以移除粗边框
            L"EDIT",
            L"",
            WS_CHILD | WS_VISIBLE | ES_CENTER | ES_AUTOHSCROLL,  // 单行文本输入，水平居中
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
            
            // 取消主题避免蓝色下边框/高亮
            SetWindowTheme(m_hSearchEdit, L" ", L" ");

            // 启用左右内边距，避免提示文本靠边裁切
            const int padding = 8;
            SendMessage(m_hSearchEdit, EM_SETMARGINS, EC_LEFTMARGIN | EC_RIGHTMARGIN, MAKELPARAM(padding, padding));
            
            // 设置搜索框的Z-order，确保在菜单栏之上
            SetWindowPos(m_hSearchEdit, HWND_TOP, 0, 0, 0, 0, 
                        SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
            
            YG_LOG_INFO(L"菜单栏嵌入式搜索框创建成功");
        } else {
            YG_LOG_ERROR(L"菜单栏嵌入式搜索框创建失败");
        }
        
        // 不再创建搜索按钮，实现纯输入框通栏样式
        
        // 创建进度条（放在菜单栏下方）
        m_hProgressBar = CreateWindowExW(
            0,
            PROGRESS_CLASSW,
            L"",
            WS_CHILD | PBS_SMOOTH,  // 初始隐藏，平滑进度条
            20,  // 左边距与ListView对齐
            menuHeight,  // 紧贴菜单栏下方
            windowWidth - 20,  // 与ListView右边对齐
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
        int listViewWidth = width - 40; // 窗口宽度减去左右边距（20px + 20px）
        int minTableWidth = CalculateTableWidth();
        if (listViewWidth < minTableWidth) {
            listViewWidth = minTableWidth;
        }
        
        // 直接创建多行编辑框来显示程序列表（最可靠的方案，无边框，动态宽度，20px边距）
        m_hListView = CreateWindowExW(
            0,  // 移除 WS_EX_CLIENTEDGE 边框样式
            L"EDIT",
            L"正在扫描已安装的程序，请稍候...",
            WS_CHILD | WS_VISIBLE | ES_MULTILINE | ES_READONLY | WS_VSCROLL | ES_AUTOVSCROLL,
            20, listTop, listViewWidth, listHeight,  // 左侧20px边距，使用动态宽度
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
            {L"大小", 90, LVCFMT_LEFT},        // 左对齐，节省空间
            {L"安装日期", 110, LVCFMT_LEFT}    // 左对齐，节省空间
        };
        
        // 获取Header控件
        HWND hHeader = ListView_GetHeader(m_hListView);
        if (hHeader) {
            int beforeCount = Header_GetItemCount(hHeader);
            YG_LOG_INFO(L"删除列之前的列数: " + std::to_wstring(beforeCount));
        }
        
        // 添加列
        LVCOLUMNW column = {0};  // 使用零初始化而不是空初始化
        column.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_FMT;
        
        // 强制清除所有现有列
        int deletedCount = 0;
        while (ListView_DeleteColumn(m_hListView, 0)) {
            deletedCount++;
            if (deletedCount > 20) break; // 防止无限循环
        }
        YG_LOG_INFO(L"删除了 " + std::to_wstring(deletedCount) + L" 个旧列");
        
        // 确保ListView处于详细信息模式
        DWORD style = GetWindowLongPtrW(m_hListView, GWL_STYLE);
        style = (style & ~LVS_TYPEMASK) | LVS_REPORT;
        SetWindowLongPtrW(m_hListView, GWL_STYLE, style);
        
        // 添加新列
        for (size_t i = 0; i < sizeof(columns) / sizeof(columns[0]); i++) {
            column.pszText = (LPWSTR)columns[i].title;
            column.cx = columns[i].width;
            column.fmt = columns[i].format;
            
            if (ListView_InsertColumn(m_hListView, static_cast<int>(i), &column) == -1) {
                YG_LOG_ERROR(L"添加列失败: " + String(columns[i].title));
            } else {
                YG_LOG_INFO(L"成功添加列: " + String(columns[i].title) + L" (宽度: " + std::to_wstring(columns[i].width) + L")");
            }
        }
        
        // 验证最终列数
        if (hHeader) {
            int finalCount = Header_GetItemCount(hHeader);
            YG_LOG_INFO(L"最终列数: " + std::to_wstring(finalCount));
            
            if (finalCount != 5) {
                YG_LOG_WARNING(L"警告：列数不正确！预期5列，实际" + std::to_wstring(finalCount) + L"列");
                
                // 如果列数不正确，再次清理并重新添加
                while (ListView_DeleteColumn(m_hListView, 0)) {}
                
                for (size_t i = 0; i < sizeof(columns) / sizeof(columns[0]); i++) {
                    column.pszText = (LPWSTR)columns[i].title;
                    column.cx = columns[i].width;
                    column.fmt = columns[i].format;
                    ListView_InsertColumn(m_hListView, static_cast<int>(i), &column);
                }
                
                YG_LOG_INFO(L"重新添加列后的列数: " + std::to_wstring(Header_GetItemCount(hHeader)));
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
        
        // 获取ListView的客户区宽度
        RECT listViewRect;
        GetClientRect(m_hListView, &listViewRect);
        int listViewWidth = listViewRect.right - listViewRect.left;
        
        // 减去垂直滚动条宽度，右边距为0，完全贴边
        int scrollBarWidth = GetSystemMetrics(SM_CXVSCROLL);
        int availableWidth = listViewWidth - scrollBarWidth; // 右边距为0，完全贴边
        
        // 检查是否需要初始化列宽（只在列宽为0时初始化）
        bool needInitialize = false;
        for (int i = 0; i < 5; i++) {
            if (ListView_GetColumnWidth(m_hListView, i) == 0) {
                needInitialize = true;
                break;
            }
        }
        
        if (needInitialize) {
            // 定义前4列的固定宽度
            int nameWidth = 280;      // 程序名称
            int versionWidth = 120;   // 版本
            int publisherWidth = 180; // 发布者
            int sizeWidth = 90;       // 大小
            
            // 计算前4列的总宽度
            int fixedColumnsWidth = nameWidth + versionWidth + publisherWidth + sizeWidth;
            
            // 最后一列（安装日期）使用剩余空间，但至少110像素
            int dateWidth = availableWidth - fixedColumnsWidth;
            if (dateWidth < 110) {
                dateWidth = 110;
            }
            
            // 设置列宽
            ListView_SetColumnWidth(m_hListView, 0, nameWidth);
            ListView_SetColumnWidth(m_hListView, 1, versionWidth);
            ListView_SetColumnWidth(m_hListView, 2, publisherWidth);
            ListView_SetColumnWidth(m_hListView, 3, sizeWidth);
            ListView_SetColumnWidth(m_hListView, 4, dateWidth);
            
            int totalWidth = nameWidth + versionWidth + publisherWidth + sizeWidth + dateWidth;
            
            YG_LOG_INFO(L"自适应列宽设置完成 - ListView宽度: " + std::to_wstring(listViewWidth) + 
                       L", 可用宽度: " + std::to_wstring(availableWidth) +
                       L", 总列宽: " + std::to_wstring(totalWidth) + 
                       L" (名称:" + std::to_wstring(nameWidth) + 
                       L", 版本:" + std::to_wstring(versionWidth) +
                       L", 发布者:" + std::to_wstring(publisherWidth) +
                       L", 大小:" + std::to_wstring(sizeWidth) +
                       L", 日期:" + std::to_wstring(dateWidth) + L")");
        } else {
            // 如果列宽已存在，检查是否需要调整最后一列以消除空白区域
            int currentTotalWidth = 0;
            for (int i = 0; i < 5; i++) {
                currentTotalWidth += ListView_GetColumnWidth(m_hListView, i);
            }
            
            // 如果总宽度小于可用宽度，扩展最后一列
            if (currentTotalWidth < availableWidth) {
                int currentDateWidth = ListView_GetColumnWidth(m_hListView, 4);
                int newDateWidth = currentDateWidth + (availableWidth - currentTotalWidth);
                ListView_SetColumnWidth(m_hListView, 4, newDateWidth);
                
                YG_LOG_INFO(L"扩展最后一列消除空白区域 - 原宽度: " + std::to_wstring(currentDateWidth) + 
                           L", 新宽度: " + std::to_wstring(newDateWidth));
            }
        }
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
            
            // 程序列表填充完成后，强制隐藏水平滚动条
            m_scrollBarsHidden = false;  // 重置状态，允许重新隐藏滚动条
            ForceHideScrollBars();
            
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
        
        // 更新状态栏 - 使用新的状态栏逻辑
        UpdateStatusBarForSelection();
        
        YG_LOG_INFO(L"程序列表填充完成");
    }
    
    bool MainWindow::GetSelectedProgram(ProgramInfo& program) {
        if (!m_hListView) {
            YG_LOG_WARNING(L"ListView句柄无效");
            return false;
        }
        
        YG_LOG_INFO(L"开始获取选中程序，程序总数: " + std::to_wstring(m_programs.size()));
        
        // 获取选中项的索引
        int selectedIndex = ListView_GetNextItem(m_hListView, -1, LVNI_SELECTED);
        YG_LOG_INFO(L"ListView选中索引: " + std::to_wstring(selectedIndex));
        
        if (selectedIndex == -1) {
            YG_LOG_INFO(L"没有选中的程序");
            return false;
        }
        
        // 获取ListView中显示的文本（用于调试）
        wchar_t displayText[512] = {0};
        LVITEMW textItem = {0};
        textItem.mask = LVIF_TEXT;
        textItem.iItem = selectedIndex;
        textItem.iSubItem = 0;
        textItem.pszText = displayText;
        textItem.cchTextMax = 512;
        if (ListView_GetItem(m_hListView, &textItem)) {
            YG_LOG_INFO(L"ListView显示的程序名称: " + String(displayText));
        }
        
        // 获取项数据（程序索引）
        LVITEMW item = {0};
        item.mask = LVIF_PARAM;
        item.iItem = selectedIndex;
        
        if (!ListView_GetItem(m_hListView, &item)) {
            YG_LOG_WARNING(L"无法获取ListView项数据");
            return false;
        }
        
        size_t programIndex = static_cast<size_t>(item.lParam);
        YG_LOG_INFO(L"存储的程序索引: " + std::to_wstring(programIndex));
        
        // 从存储的程序列表中获取程序信息
        if (programIndex < m_programs.size()) {
            program = m_programs[programIndex];
            YG_LOG_INFO(L"成功获取选中程序: " + program.name);
            YG_LOG_INFO(L"选中程序的注册表路径: " + program.registryKey);
            
            // 添加详细调试信息
            String debugMsg = L"程序选择调试信息:\n";
            debugMsg += L"ListView选中索引: " + std::to_wstring(selectedIndex) + L"\n";
            debugMsg += L"ListView显示名称: " + String(displayText) + L"\n";
            debugMsg += L"存储的程序索引: " + std::to_wstring(programIndex) + L"\n";
            debugMsg += L"实际程序名称: " + program.name + L"\n";
            debugMsg += L"实际程序显示名称: " + program.displayName + L"\n";
            debugMsg += L"实际注册表路径: " + program.registryKey + L"\n";
            YG_LOG_INFO(debugMsg);
            
            return true;
        } else {
            YG_LOG_WARNING(L"程序索引超出范围: " + std::to_wstring(programIndex) + 
                         L", 总数: " + std::to_wstring(m_programs.size()));
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
        
        // 调整嵌入菜单栏的搜索控件（纯输入通栏，无按钮与圆角）
        if (m_hSearchEdit) {
            int leftMargin = 0;
            int rightMargin = 0;
            int searchX = leftMargin;
            int searchY = 2;            // 嵌入菜单栏内部
            int controlHeight = menuHeight - 4;
            int searchWidth = windowWidth - leftMargin - rightMargin;

            if (searchWidth < 50) searchWidth = 50; // 兜底，避免过窄

            // 调整搜索框位置
            SetWindowPos(m_hSearchEdit, HWND_TOP, searchX, searchY, searchWidth, controlHeight,
                        SWP_NOACTIVATE);

            // 移除任何圆角区域（如果此前设置过）
            SetWindowRgn(m_hSearchEdit, NULL, TRUE);
        }
        
        // 调整进度条（在菜单栏下方，全宽度）
        if (m_hProgressBar) {
            SetWindowPos(m_hProgressBar, nullptr, 20, menuHeight, 
                        windowWidth - 20, 3, SWP_NOZORDER | SWP_NOACTIVATE);
        }
        
        // 调整列表视图
        if (m_hListView) {
            int listTop = menuHeight + 3;  // 菜单栏下方，为进度条留出3px空间
            int listHeight = height - listTop - statusHeight;
            
            // 获取窗口宽度
            RECT clientRect;
            GetClientRect(m_hWnd, &clientRect);
            int windowWidth = clientRect.right - clientRect.left;
            
            // 计算ListView宽度：左边距20px，右边距0
            int listViewWidth = windowWidth - 20;
            
            // 确保最小宽度
            int minTableWidth = CalculateTableWidth();
            if (listViewWidth < minTableWidth) {
                listViewWidth = minTableWidth;
            }
            
            // 设置ListView位置和尺寸：左边距20px，右边距0
            SetWindowPos(m_hListView, nullptr, 20, listTop, listViewWidth, listHeight,
                        SWP_NOZORDER | SWP_NOACTIVATE);
            
            // 动态调整列宽以适应新的ListView宽度
            AdjustListViewColumns();
        }
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
        // 支持多种输入格式: YYYYMMDD, YYYY-MM-DD, YYYY/MM/DD, MM/DD/YYYY等
        // 统一输出格式: d/M/yyyy (如: 20/9/2025)
        
        if (dateString.empty()) {
            return L"未知";
        }
        
        int year = 0, month = 0, day = 0;
        bool parsed = false;
        
        // 尝试解析不同的日期格式
        
        // 格式1: YYYYMMDD (注册表中常见格式，如: 20250920)
        if (!parsed && dateString.length() == 8) {
            try {
                year = std::stoi(dateString.substr(0, 4));
                month = std::stoi(dateString.substr(4, 2));
                day = std::stoi(dateString.substr(6, 2));
                if (year >= 1990 && year <= 2050 && month >= 1 && month <= 12 && day >= 1 && day <= 31) {
                    parsed = true;
                }
            } catch (...) {}
        }
        
        // 格式2: YYYY-MM-DD 或 YYYY/MM/DD
        if (!parsed) {
            size_t firstSep = dateString.find_first_of(L"-/");
            size_t secondSep = dateString.find_first_of(L"-/", firstSep + 1);
            
            if (firstSep != String::npos && secondSep != String::npos) {
                try {
                    String yearStr = dateString.substr(0, firstSep);
                    String monthStr = dateString.substr(firstSep + 1, secondSep - firstSep - 1);
                    String dayStr = dateString.substr(secondSep + 1);
                    
                    year = std::stoi(yearStr);
                    month = std::stoi(monthStr);
                    day = std::stoi(dayStr);
                    
                    if (year >= 1990 && year <= 2050 && month >= 1 && month <= 12 && day >= 1 && day <= 31) {
                        parsed = true;
                    }
                } catch (...) {}
            }
        }
        
        // 格式3: MM/DD/YYYY 或 DD/MM/YYYY (尝试美式和欧式格式)
        if (!parsed) {
            size_t firstSep = dateString.find_first_of(L"-/");
            size_t secondSep = dateString.find_first_of(L"-/", firstSep + 1);
            
            if (firstSep != String::npos && secondSep != String::npos) {
                try {
                    String part1 = dateString.substr(0, firstSep);
                    String part2 = dateString.substr(firstSep + 1, secondSep - firstSep - 1);
                    String part3 = dateString.substr(secondSep + 1);
                    
                    int p1 = std::stoi(part1);
                    int p2 = std::stoi(part2);
                    int p3 = std::stoi(part3);
                    
                    // 判断是否为 MM/DD/YYYY 格式
                    if (p3 >= 1990 && p3 <= 2050 && p1 >= 1 && p1 <= 12 && p2 >= 1 && p2 <= 31) {
                        month = p1;
                        day = p2;
                        year = p3;
                        parsed = true;
                    }
                    // 判断是否为 DD/MM/YYYY 格式
                    else if (p3 >= 1990 && p3 <= 2050 && p2 >= 1 && p2 <= 12 && p1 >= 1 && p1 <= 31) {
                        day = p1;
                        month = p2;
                        year = p3;
                        parsed = true;
                    }
                } catch (...) {}
            }
        }
        
        // 如果成功解析，格式化输出为 d/M/yyyy
        if (parsed) {
            wchar_t buffer[32];
#ifdef _MSC_VER
            swprintf_s(buffer, L"%d/%d/%d", day, month, year);
#else
            swprintf(buffer, sizeof(buffer)/sizeof(wchar_t), L"%d/%d/%d", day, month, year);
#endif
            return String(buffer);
        }
        
        // 如果所有格式都解析失败，返回"未知"
        return L"未知";
    }
    
    void MainWindow::AddTestPrograms() {
        // 添加测试数据来验证列表视图功能
        std::vector<ProgramInfo> testPrograms;
        
        // 创建测试程序信息
        ProgramInfo test1;
        test1.name = L"测试程序 1";
        test1.version = L"1.0.1";
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
    
    // 版本比较辅助函数
    bool MainWindow::CompareVersions(const String& version1, const String& version2) {
        if (version1.empty() || version2.empty()) {
            return false; // 如果任一版本为空，不认为相同
        }
        
        // 简单的版本比较：完全相同的版本字符串
        return version1 == version2;
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
            // 如果名称和版本都相同，认为是重复
            if (program1.version == program2.version) {
                // 进一步检查发布者，确保是同一程序
            if (program1.publisher == program2.publisher) {
                    return true; // 名称、版本、发布者都相同，认为是重复
            }
            }
            // 注意：不再将同名但不同版本的程序视为重复，保留不同版本
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
        
        // 如果移除架构标识后名称相同，且发布者相同，进一步检查版本
        if (baseName1 == baseName2 && program1.publisher == program2.publisher) {
            // 只有版本也相同时才认为是重复（避免移除不同版本）
            if (program1.version == program2.version) {
            return true;
            }
            // 如果版本不同，保留两个版本（可能是不同架构的同一程序）
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
                
                // 设置卸载完成回调
                m_uninstallerService->SetUninstallCompleteCallback(
                    [this](const ProgramInfo& program, bool success) {
                        OnUninstallComplete(program, success);
                    });
                
                // 设置残留扫描器
                m_uninstallerService->SetResidualScanner(m_residualScanner);
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
    
    
    
    
    
    
    
    void MainWindow::SetListViewMode(DWORD mode) {
        if (!m_hListView || !m_isListViewMode) {
            return;
        }
        
        // 设置ListView视图模式
        ListView_SetView(m_hListView, mode);
        
        // 根据不同视图模式调整布局
        switch (mode) {
            case LV_VIEW_ICON:
                {
                    // 大图标模式：创建大图标列表
                    if (m_hImageList) {
                        ImageList_Destroy(m_hImageList);
                    }
                    m_hImageList = ImageList_Create(32, 32, ILC_COLOR32 | ILC_MASK, 10, 10);
                    HIMAGELIST hOldImageList = ListView_SetImageList(m_hListView, m_hImageList, LVSIL_NORMAL);
                    (void)hOldImageList; // 抑制未使用警告
                    break;
                }
                
            case LV_VIEW_SMALLICON:
            case LV_VIEW_LIST:
                {
                    // 小图标和列表模式：使用16x16图标
                    if (m_hImageList) {
                        ImageList_Destroy(m_hImageList);
                    }
                    m_hImageList = ImageList_Create(16, 16, ILC_COLOR32 | ILC_MASK, 10, 10);
                    HIMAGELIST hOldImageList2 = ListView_SetImageList(m_hListView, m_hImageList, LVSIL_SMALL);
                    (void)hOldImageList2; // 抑制未使用警告
                    break;
                }
                
            case LV_VIEW_DETAILS:
                {
                    // 详细信息模式：确保列存在
                    InitializeListViewColumns();
                    break;
                }
        }
        
        // 重新填充程序列表以适应新视图
        PopulateProgramList(m_filteredPrograms.empty() ? m_programs : m_filteredPrograms);
        
        YG_LOG_INFO(L"ListView视图模式已切换");
    }
    
    void MainWindow::UpdateViewMenu(int selectedId) {
        HMENU hMenu = GetMenu(m_hWnd);
        if (!hMenu) return;
        
        HMENU hViewMenu = GetSubMenu(hMenu, 2); // 查看菜单是第3个
        if (!hViewMenu) return;
        
        // 取消所有视图模式的选中状态
        CheckMenuItem(hViewMenu, ID_VIEW_LARGE_ICONS, MF_BYCOMMAND | MF_UNCHECKED);
        CheckMenuItem(hViewMenu, ID_VIEW_SMALL_ICONS, MF_BYCOMMAND | MF_UNCHECKED);
        CheckMenuItem(hViewMenu, ID_VIEW_LIST, MF_BYCOMMAND | MF_UNCHECKED);
        CheckMenuItem(hViewMenu, ID_VIEW_DETAILS, MF_BYCOMMAND | MF_UNCHECKED);
        
        // 选中当前视图模式
        CheckMenuItem(hViewMenu, selectedId, MF_BYCOMMAND | MF_CHECKED);
    }
    
    
    void MainWindow::OpenWebsite() {
        // 实际的网站访问功能
        String websiteUrl = L"https://github.com/gitchzh/YG-uninstaller";
        
        // 使用ShellExecute打开默认浏览器
        HINSTANCE result = ShellExecuteW(
            m_hWnd,
            L"open",
            websiteUrl.c_str(),
            nullptr,
            nullptr,
            SW_SHOWNORMAL
        );
        
        if ((INT_PTR)result <= 32) {
            // 如果打开失败，显示错误信息
            String errorMsg = L"无法打开网站：" + websiteUrl + L"\n\n";
            errorMsg += L"请手动复制链接到浏览器中打开。";
            
            MessageBoxW(m_hWnd, errorMsg.c_str(), L"打开网站失败", MB_OK | MB_ICONWARNING);
        } else {
            SetStatusText(L"已在默认浏览器中打开官网");
        }
        
        YG_LOG_INFO(L"尝试打开官网: " + websiteUrl);
    }
    
    void MainWindow::CheckForUpdates() {
        SetStatusText(L"正在检查更新...");
        
        // 模拟检查更新过程
        Sleep(1000); // 模拟网络延迟
        
        // 当前版本信息
        String currentVersion = L"1.0.1";
        String latestVersion = L"1.0.1"; // 在实际应用中这里会从服务器获取
        
        String updateMessage;
        if (currentVersion == latestVersion) {
            updateMessage = L"更新检查完成\n\n";
            updateMessage += L"当前版本：" + currentVersion + L"\n";
            updateMessage += L"最新版本：" + latestVersion + L"\n\n";
            updateMessage += L"您使用的已是最新版本！";
            
            MessageBoxW(m_hWnd, updateMessage.c_str(), L"检查更新", MB_OK | MB_ICONINFORMATION);
        } else {
            updateMessage = L"发现新版本！\n\n";
            updateMessage += L"当前版本：" + currentVersion + L"\n";
            updateMessage += L"最新版本：" + latestVersion + L"\n\n";
            updateMessage += L"是否前往下载页面？";
            
            int result = MessageBoxW(m_hWnd, updateMessage.c_str(), L"发现更新", MB_YESNO | MB_ICONQUESTION);
            if (result == IDYES) {
                OpenWebsite(); // 打开下载页面
            }
        }
        
        SetStatusText(L"更新检查完成");
        YG_LOG_INFO(L"更新检查完成，当前版本: " + currentVersion);
    }
    
    
    
    
    
    
    
    void MainWindow::CreateCleanLeftPanel(int panelWidth, int windowHeight) {
        YG_LOG_INFO(L"创建简洁的左侧面板 - 1:3布局");
        
        int statusBarHeight = 25;
        int availableHeight = windowHeight - statusBarHeight - 50; // 减去状态栏和菜单栏高度
        
        // 创建左侧面板背景，确保不超出1/4边界
        m_hLeftPanel = CreateWindowExW(
            WS_EX_CLIENTEDGE,  // 3D边框效果
            L"STATIC",
            L"",
            WS_CHILD | WS_VISIBLE | SS_NOTIFY,
            2, 30,  // 位置：左边留2px边距，菜单栏下方
            panelWidth - 4, availableHeight,  // 宽度减去边距，确保不超出边界
            m_hWnd,
            (HMENU)4001,
            m_hInstance,
            nullptr
        );
        
        if (m_hLeftPanel) {
            // 设置面板背景色为浅蓝色
            SetClassLongPtr(m_hLeftPanel, GCLP_HBRBACKGROUND, (LONG_PTR)CreateSolidBrush(RGB(221, 239, 247)));
        }
        
        // 上半部分：程序详情显示区域 (占左侧面板的2/3)
        int detailsHeight = availableHeight * 2 / 3;
        m_hDetailsEdit = CreateWindowExW(
            WS_EX_CLIENTEDGE,
            L"EDIT",
            L"选择一个程序查看详细信息",
            WS_CHILD | WS_VISIBLE | ES_MULTILINE | ES_READONLY | WS_VSCROLL,
            10, 10,  // 相对于左侧面板的位置
            panelWidth - 30, detailsHeight - 20,
            m_hLeftPanel,
            (HMENU)4002,
            m_hInstance,
            nullptr
        );
        
        if (m_hDetailsEdit) {
            // 设置字体
            HFONT hFont = CreateFontW(
                -12, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE,
                L"MS Shell Dlg"
            );
            if (hFont) {
                SendMessage(m_hDetailsEdit, WM_SETFONT, (WPARAM)hFont, TRUE);
            }
        }
        
        // 下半部分：搜索区域 (占左侧面板的1/3)
        int searchAreaY = detailsHeight + 10;
        
        // 创建搜索标签
        CreateWindowExW(
            0,
            L"STATIC",
            L"搜索:",
            WS_CHILD | WS_VISIBLE | SS_LEFT,
            10, searchAreaY,
            40, 20,
            m_hLeftPanel,
            (HMENU)4020,
            m_hInstance,
            nullptr
        );
        
        // 创建搜索框
        m_hBottomSearchEdit = CreateWindowExW(
            WS_EX_CLIENTEDGE,
            L"EDIT",
            L"",
            WS_CHILD | WS_VISIBLE | ES_LEFT | ES_AUTOHSCROLL,
            10, searchAreaY + 25,
            panelWidth - 30, 24,
            m_hLeftPanel,
            (HMENU)4021,
            m_hInstance,
            nullptr
        );
        
        if (m_hBottomSearchEdit) {
            // 设置提示文本
            SendMessageW(m_hBottomSearchEdit, EM_SETCUEBANNER, TRUE, (LPARAM)L"输入程序名称...");
            
            // 设置字体
            HFONT hFont = CreateFontW(
                -12, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE,
                L"MS Shell Dlg"
            );
            if (hFont) {
                SendMessage(m_hBottomSearchEdit, WM_SETFONT, (WPARAM)hFont, TRUE);
            }
        }
        
        YG_LOG_INFO(L"简洁左侧面板创建完成");
    }
    
    void MainWindow::CreateTwoColumnLayout(int leftWidth, int rightWidth, int totalHeight, int menuHeight, int statusHeight) {
        YG_LOG_INFO(L"创建完全独立的两栏布局");
        
        int availableHeight = totalHeight - menuHeight - statusHeight;
        
        // 创建左侧容器 - 完全独立的第一栏
        m_hLeftPanel = CreateWindowExW(
            WS_EX_CLIENTEDGE,
            L"STATIC",
            L"",
            WS_CHILD | WS_VISIBLE | SS_NOTIFY,
            0, menuHeight,  // 从窗口左边开始，菜单栏下方
            leftWidth, availableHeight,  // 严格的1/4宽度
            m_hWnd,
            (HMENU)4001,
            m_hInstance,
            nullptr
        );
        
        if (m_hLeftPanel) {
            // 设置左侧面板背景色
            SetClassLongPtr(m_hLeftPanel, GCLP_HBRBACKGROUND, (LONG_PTR)CreateSolidBrush(RGB(221, 239, 247)));
            
            // 在左侧容器内创建详情显示区域
            int detailsHeight = availableHeight * 2 / 3;
            m_hDetailsEdit = CreateWindowExW(
                WS_EX_CLIENTEDGE,
                L"EDIT",
                L"选择一个程序查看详细信息",
                WS_CHILD | WS_VISIBLE | ES_MULTILINE | ES_READONLY | WS_VSCROLL,
                5, 5,
                leftWidth - 15, detailsHeight - 10,
                m_hLeftPanel,  // 父窗口是左侧容器
                (HMENU)4002,
                m_hInstance,
                nullptr
            );
            
            // 在左侧容器内创建搜索区域
            int searchY = detailsHeight + 5;
            CreateWindowExW(
                0, L"STATIC", L"搜索:",
                WS_CHILD | WS_VISIBLE | SS_LEFT,
                5, searchY, 40, 20,
                m_hLeftPanel,
                (HMENU)4020,
                m_hInstance, nullptr
            );
            
            m_hBottomSearchEdit = CreateWindowExW(
                WS_EX_CLIENTEDGE, L"EDIT", L"",
                WS_CHILD | WS_VISIBLE | ES_LEFT | ES_AUTOHSCROLL,
                5, searchY + 25, leftWidth - 15, 24,
                m_hLeftPanel,
                (HMENU)4021,
                m_hInstance, nullptr
            );
            
            if (m_hBottomSearchEdit) {
                SendMessageW(m_hBottomSearchEdit, EM_SETCUEBANNER, TRUE, (LPARAM)L"输入程序名称...");
            }
        }
        
        // 创建右侧容器 - 完全独立的第二栏
        m_hRightPanel = CreateWindowExW(
            WS_EX_CLIENTEDGE,
            L"STATIC",
            L"",
            WS_CHILD | WS_VISIBLE | SS_NOTIFY,
            leftWidth, menuHeight,  // 从左侧容器右边开始
            rightWidth, availableHeight,  // 严格的3/4宽度
            m_hWnd,
            (HMENU)4100,
            m_hInstance,
            nullptr
        );
        
        if (m_hRightPanel) {
            // 设置右侧面板背景色
            SetClassLongPtr(m_hRightPanel, GCLP_HBRBACKGROUND, (LONG_PTR)CreateSolidBrush(RGB(255, 255, 255)));
            
            // 在右侧容器内创建ListView - 直接设置主窗口为父窗口以确保事件路由
            m_hListView = CreateWindowExW(
                0,  // 不需要额外边框，容器已有边框
                WC_LISTVIEWW,
                L"",
                WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SINGLESEL | LVS_SHOWSELALWAYS,
                leftWidth + 5, menuHeight + 5,  // 直接相对于主窗口的位置
                rightWidth - 10, availableHeight - 10,  // 填满右侧区域
                m_hWnd,  // 父窗口是主窗口，确保事件路由正确
                (HMENU)2001,
                GetModuleHandle(NULL),
                NULL
            );
            
            // 设置ListView扩展样式
            if (m_hListView) {
                ListView_SetExtendedListViewStyle(m_hListView, 
                    LVS_EX_FULLROWSELECT |      // 整行选中高亮
                    LVS_EX_GRIDLINES |          // 网格线
                    LVS_EX_INFOTIP |            // 信息提示
                    LVS_EX_SUBITEMIMAGES |      // 子项图标支持
                    LVS_EX_HEADERDRAGDROP);     // 表头拖放和调整
                
                YG_LOG_INFO(L"ListView扩展样式设置完成");
            }
        }
        
        YG_LOG_INFO(L"完全独立的两栏布局创建完成");
    }
    
    void MainWindow::CreateSearchArea(int totalWidth, int yPosition, int searchBarHeight) {
        YG_LOG_INFO(L"创建搜索区域");
        
        // 计算搜索框的位置和尺寸
        int margin = 0;  // 左右通栏
        int searchBoxWidth = totalWidth - (margin * 2);
        int searchBoxHeight = 24;  // 搜索框高度
        int searchBoxY = yPosition + (searchBarHeight - searchBoxHeight) / 2;  // 垂直居中
        
        // 创建搜索输入框
        m_hSearchEdit = CreateWindowExW(
            0,
            L"EDIT",
            L"",
            WS_CHILD | WS_VISIBLE | ES_CENTER | ES_AUTOHSCROLL,
            margin, searchBoxY,
            searchBoxWidth, searchBoxHeight,
            m_hWnd,
            (HMENU)3001,  // 搜索框控件ID
            m_hInstance,
            nullptr
        );
        
        if (m_hSearchEdit) {
            // 设置搜索提示文本
            SendMessageW(m_hSearchEdit, EM_SETCUEBANNER, TRUE, (LPARAM)L"输入程序名称进行搜索...");
            
            // 设置字体
            HFONT hFont = CreateFontW(
                -12, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE,
                L"Segoe UI"
            );
            SendMessage(m_hSearchEdit, WM_SETFONT, (WPARAM)hFont, TRUE);

            // 取消主题避免蓝色下边框/高亮
            SetWindowTheme(m_hSearchEdit, L" ", L" ");

            // 设置输入内边距，确保提示文本完整显示
            const int padding = 8;
            SendMessage(m_hSearchEdit, EM_SETMARGINS, EC_LEFTMARGIN | EC_RIGHTMARGIN, MAKELPARAM(padding, padding));
            
            YG_LOG_INFO(L"搜索框创建成功，位置: (" + std::to_wstring(margin) + L", " + 
                       std::to_wstring(searchBoxY) + L"), 尺寸: " + std::to_wstring(searchBoxWidth) + 
                       L"x" + std::to_wstring(searchBoxHeight));
        } else {
            YG_LOG_ERROR(L"搜索框创建失败");
        }
    }
    
    
    void MainWindow::UpdateDetailsPanel(const ProgramInfo& program) {
        if (!m_hDetailsEdit) {
            return;
        }
        
        // 构建详情文本，格式化显示
        String detailsText;
        
        // 程序名称（标题）
        String displayName = !program.displayName.empty() ? program.displayName : program.name;
        detailsText += displayName + L"\r\n";
        detailsText += String(displayName.length(), L'=') + L"\r\n\r\n";
        
        // 版本信息
        detailsText += L"版本:\r\n";
        if (!program.version.empty()) {
            detailsText += L"  " + program.version + L"\r\n\r\n";
        } else {
            detailsText += L"  未知\r\n\r\n";
        }
        
        // 发行商
        detailsText += L"发行商:\r\n";
        if (!program.publisher.empty()) {
            detailsText += L"  " + program.publisher + L"\r\n\r\n";
        } else {
            detailsText += L"  未知\r\n\r\n";
        }
        
        // 安装日期
        detailsText += L"安装日期:\r\n";
        if (!program.installDate.empty()) {
            detailsText += L"  " + program.installDate + L"\r\n\r\n";
        } else {
            detailsText += L"  未知\r\n\r\n";
        }
        
        // 程序大小
        detailsText += L"大小:\r\n";
        if (program.estimatedSize > 0) {
            detailsText += L"  " + FormatFileSize(program.estimatedSize) + L"\r\n\r\n";
        } else {
            detailsText += L"  未知\r\n\r\n";
        }
        
        // 安装位置
        detailsText += L"安装位置:\r\n";
        if (!program.installLocation.empty()) {
            detailsText += L"  " + program.installLocation + L"\r\n\r\n";
        } else {
            detailsText += L"  未知\r\n\r\n";
        }
        
        // 卸载信息
        detailsText += L"卸载信息:\r\n";
        if (!program.uninstallString.empty()) {
            detailsText += L"  可以卸载\r\n";
            detailsText += L"  卸载命令: " + program.uninstallString.substr(0, 50);
            if (program.uninstallString.length() > 50) {
                detailsText += L"...";
            }
            detailsText += L"\r\n\r\n";
        } else {
            detailsText += L"  无法自动卸载\r\n\r\n";
        }
        
        // 注册表信息
        if (!program.registryKey.empty()) {
            detailsText += L"注册表位置:\r\n";
            detailsText += L"  " + program.registryKey + L"\r\n\r\n";
        }
        
        // 设置文本到编辑框
        SetWindowTextW(m_hDetailsEdit, detailsText.c_str());
        
        YG_LOG_INFO(L"详情面板已更新: " + displayName);
    }
    
    
    
    
    
    
    
    
    
    
    
    String MainWindow::ExtractPathFromUninstallString(const String& uninstallString) {
        if (uninstallString.empty()) {
            return L"";
        }
        
        // 查找.exe文件路径
        size_t exePos = uninstallString.find(L".exe");
        if (exePos != String::npos) {
            // 向前查找路径的开始
            size_t pathStart = uninstallString.rfind(L'"', exePos);
            if (pathStart != String::npos) {
                // 从引号后开始到.exe结束
                String fullPath = uninstallString.substr(pathStart + 1, exePos + 4 - pathStart - 1);
                
                // 提取目录路径（去掉文件名）
                size_t lastSlash = fullPath.find_last_of(L"\\/");
                if (lastSlash != String::npos) {
                    return fullPath.substr(0, lastSlash);
                }
            } else {
                // 没有引号，直接查找.exe
                String fullPath = uninstallString.substr(0, exePos + 4);
                
                // 提取目录路径（去掉文件名）
                size_t lastSlash = fullPath.find_last_of(L"\\/");
                if (lastSlash != String::npos) {
                    return fullPath.substr(0, lastSlash);
                }
            }
        }
        
        return L"";
    }
    
    String MainWindow::ExtractPathFromIconPath(const String& iconPath) {
        if (iconPath.empty()) {
            return L"";
        }
        
        // 从图标路径中提取可执行文件路径（去掉图标索引）
        String executablePath = iconPath;
        size_t commaPos = executablePath.find(L',');
        if (commaPos != String::npos) {
            executablePath = executablePath.substr(0, commaPos);
        }
        
        // 去掉引号
        if (executablePath.front() == L'"') {
            executablePath.erase(0, 1);
        }
        if (executablePath.back() == L'"') {
            executablePath.pop_back();
        }
        
        // 提取目录路径（去掉文件名）
        size_t lastSlash = executablePath.find_last_of(L"\\/");
        if (lastSlash != String::npos) {
            return executablePath.substr(0, lastSlash);
        }
        
        return L"";
    }
    
    String MainWindow::GuessCommonInstallPath(const ProgramInfo& program) {
        String programName = !program.displayName.empty() ? program.displayName : program.name;
        String publisher = program.publisher;
        
        // 转换为小写进行比较
        String lowerName = programName;
        String lowerPublisher = publisher;
        std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::towlower);
        std::transform(lowerPublisher.begin(), lowerPublisher.end(), lowerPublisher.begin(), ::towlower);
        
        // 常见程序的安装路径映射
        struct CommonProgram {
            String namePattern;
            String publisherPattern;
            String installPath;
        };
        
        static const CommonProgram commonPrograms[] = {
            // Microsoft Office
            {L"microsoft office", L"microsoft corporation", L"C:\\Program Files\\Microsoft Office"},
            {L"office", L"microsoft corporation", L"C:\\Program Files\\Microsoft Office"},
            {L"word", L"microsoft corporation", L"C:\\Program Files\\Microsoft Office"},
            {L"excel", L"microsoft corporation", L"C:\\Program Files\\Microsoft Office"},
            {L"powerpoint", L"microsoft corporation", L"C:\\Program Files\\Microsoft Office"},
            
            // Adobe Creative Suite
            {L"adobe", L"adobe systems", L"C:\\Program Files\\Adobe"},
            {L"photoshop", L"adobe systems", L"C:\\Program Files\\Adobe\\Adobe Photoshop"},
            {L"illustrator", L"adobe systems", L"C:\\Program Files\\Adobe\\Adobe Illustrator"},
            {L"acrobat", L"adobe systems", L"C:\\Program Files\\Adobe\\Acrobat"},
            
            // Google软件
            {L"google chrome", L"google llc", L"C:\\Program Files\\Google\\Chrome\\Application"},
            {L"chrome", L"google llc", L"C:\\Program Files\\Google\\Chrome\\Application"},
            {L"google drive", L"google llc", L"C:\\Program Files\\Google\\Drive File Stream"},
            
            // Mozilla软件
            {L"firefox", L"mozilla foundation", L"C:\\Program Files\\Mozilla Firefox"},
            {L"thunderbird", L"mozilla foundation", L"C:\\Program Files\\Mozilla Thunderbird"},
            
            // 其他常见软件
            {L"winrar", L"win.rar gmbh", L"C:\\Program Files\\WinRAR"},
            {L"7-zip", L"igor pavlov", L"C:\\Program Files\\7-Zip"},
            {L"notepad++", L"notepad++ team", L"C:\\Program Files\\Notepad++"},
            {L"visual studio", L"microsoft corporation", L"C:\\Program Files\\Microsoft Visual Studio"},
            {L"git", L"git for windows", L"C:\\Program Files\\Git"},
            {L"node.js", L"node.js foundation", L"C:\\Program Files\\nodejs"},
            {L"python", L"python software foundation", L"C:\\Program Files\\Python"},
            {L"java", L"oracle corporation", L"C:\\Program Files\\Java"},
            {L"eclipse", L"eclipse foundation", L"C:\\Program Files\\Eclipse"},
            
            // 游戏平台
            {L"steam", L"valve corporation", L"C:\\Program Files (x86)\\Steam"},
            {L"origin", L"electronic arts", L"C:\\Program Files (x86)\\Origin"},
            {L"epic games", L"epic games", L"C:\\Program Files\\Epic Games"},
            
            // 系统工具
            {L"ccleaner", L"piriform ltd", L"C:\\Program Files\\CCleaner"},
            {L"malwarebytes", L"malwarebytes", L"C:\\Program Files\\Malwarebytes"},
            {L"nvidia", L"nvidia corporation", L"C:\\Program Files\\NVIDIA Corporation"},
            {L"amd", L"advanced micro devices", L"C:\\Program Files\\AMD"},
        };
        
        // 检查程序名称和发布者是否匹配
        for (const auto& common : commonPrograms) {
            String lowerPattern = common.namePattern;
            String lowerPubPattern = common.publisherPattern;
            std::transform(lowerPattern.begin(), lowerPattern.end(), lowerPattern.begin(), ::towlower);
            std::transform(lowerPubPattern.begin(), lowerPubPattern.end(), lowerPubPattern.begin(), ::towlower);
            
            // 检查名称匹配（包含匹配）
            bool nameMatch = lowerName.find(lowerPattern) != String::npos;
            bool publisherMatch = lowerPublisher.find(lowerPubPattern) != String::npos;
            
            if (nameMatch && publisherMatch) {
                // 验证路径是否存在
                DWORD attributes = GetFileAttributesW(common.installPath.c_str());
                if (attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_DIRECTORY)) {
                    return common.installPath;
                }
            }
        }
        
        // 如果都不匹配，尝试通用的Program Files路径
        String genericPath = L"C:\\Program Files\\" + programName;
        DWORD attributes = GetFileAttributesW(genericPath.c_str());
        if (attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_DIRECTORY)) {
            return genericPath;
        }
        
        // 尝试Program Files (x86)
        String genericPath86 = L"C:\\Program Files (x86)\\" + programName;
        attributes = GetFileAttributesW(genericPath86.c_str());
        if (attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_DIRECTORY)) {
            return genericPath86;
        }
        
        return L"";
    }
    
    
    
    
    
    void MainWindow::ShowProgramDetails(const ProgramInfo& program) {
        // 创建增强的程序属性对话框
        String details = L"═══ 程序详细信息 ═══\n\n";
        
        // 基本信息
        details += L"📋 基本信息\n";
        details += L"├─ 程序名称: " + (!program.displayName.empty() ? program.displayName : program.name) + L"\n";
        details += L"├─ 内部名称: " + program.name + L"\n";
        details += L"├─ 版本: " + (program.version.empty() ? L"未知" : program.version) + L"\n";
        details += L"├─ 发布者: " + (program.publisher.empty() ? L"未知" : program.publisher) + L"\n";
        details += L"└─ 大小: " + FormatFileSize(program.estimatedSize) + L"\n\n";
        
        // 安装信息
        details += L"💾 安装信息\n";
        details += L"├─ 安装日期: " + FormatInstallDate(program.installDate) + L"\n";
        details += L"├─ 安装位置: " + (program.installLocation.empty() ? L"未知" : program.installLocation) + L"\n";
        details += L"└─ 图标路径: " + (program.iconPath.empty() ? L"默认" : program.iconPath) + L"\n\n";
        
        // 卸载信息
        details += L"🗑️ 卸载信息\n";
        details += L"├─ 卸载命令: " + (program.uninstallString.empty() ? L"无" : program.uninstallString) + L"\n";
        details += L"├─ 系统组件: " + String(program.isSystemComponent ? L"是" : L"否") + L"\n";
        details += L"└─ 可卸载: " + String(program.uninstallString.empty() ? L"否" : L"是") + L"\n\n";
        
        // 技术信息
        details += L"🔧 技术信息\n";
        details += L"├─ 程序ID: " + program.name + L"\n";
        details += L"├─ 检测方式: " + String(program.isSystemComponent ? L"系统扫描" : L"用户程序") + L"\n";
        details += L"└─ 数据完整性: " + String(program.uninstallString.empty() ? L"不完整" : L"完整");
        
        // 使用增强的消息框显示
        MessageBoxW(m_hWnd, details.c_str(), 
                   (L"程序属性 - " + (!program.displayName.empty() ? program.displayName : program.name)).c_str(), 
                   MB_OK | MB_ICONINFORMATION);
        
        YG_LOG_INFO(L"显示程序详细信息: " + program.name);
    }
    
    void MainWindow::OnHeaderColumnWidthChanged(int columnIndex, int newWidth) {
        if (!m_hListView || !m_isListViewMode || columnIndex < 0 || columnIndex >= 5) {
            return;
        }
        
        // 实时同步表头和内容的列宽
        // 注意：这里不使用ListView_SetColumnWidth，因为它会触发递归通知
        // 而是直接让表头的变化自然影响ListView的显示
        
        YG_LOG_INFO(L"表头列宽变化 - 列" + std::to_wstring(columnIndex) + 
                   L": 新宽度=" + std::to_wstring(newWidth));
        
        // 强制重绘ListView以确保内容与表头同步
        InvalidateRect(m_hListView, NULL, FALSE);
    }
    
    void MainWindow::OnHeaderColumnResizeEnd(int columnIndex) {
        if (!m_hListView || !m_isListViewMode || columnIndex < 0 || columnIndex >= 5) {
            return;
        }
        
        // 用户完成列宽拖动，获取最终宽度并确保同步
        int finalWidth = ListView_GetColumnWidth(m_hListView, columnIndex);
        
        YG_LOG_INFO(L"列宽调整完成 - 列" + std::to_wstring(columnIndex) + 
                   L": 最终宽度=" + std::to_wstring(finalWidth));
        
        // 如果调整的不是最后一列，需要重新计算最后一列的宽度来填满剩余空间
        if (columnIndex != 4) {
            // 获取ListView的客户区宽度
            RECT listViewRect;
            GetClientRect(m_hListView, &listViewRect);
            int listViewWidth = listViewRect.right - listViewRect.left;
            
            // 减去滚动条宽度和边距
            int scrollBarWidth = GetSystemMetrics(SM_CXVSCROLL);
            int availableWidth = listViewWidth - scrollBarWidth - 10;
            
            // 计算前4列的总宽度
            int totalUsedWidth = 0;
            for (int i = 0; i < 4; i++) {
                totalUsedWidth += ListView_GetColumnWidth(m_hListView, i);
            }
            
            // 计算最后一列应有的宽度（剩余空间，但至少110像素）
            int newDateWidth = availableWidth - totalUsedWidth;
            if (newDateWidth < 110) {
                newDateWidth = 110;
            }
            
            // 设置最后一列的宽度
            ListView_SetColumnWidth(m_hListView, 4, newDateWidth);
            
            YG_LOG_INFO(L"自动调整最后一列宽度: " + std::to_wstring(newDateWidth) + 
                       L"px (可用宽度: " + std::to_wstring(availableWidth) + 
                       L"px, 前4列总宽度: " + std::to_wstring(totalUsedWidth) + L"px)");
        }
        
        // 强制刷新整个ListView确保完全同步
        InvalidateRect(m_hListView, NULL, TRUE);
        UpdateWindow(m_hListView);
        
        // 更新状态栏显示当前列宽信息
        const wchar_t* columnNames[] = {L"程序名称", L"版本", L"发布者", L"大小", L"安装日期"};
        if (columnIndex >= 0 && columnIndex < 5) {
            SetStatusText(columnNames[columnIndex] + String(L"列宽度已调整为: ") + std::to_wstring(finalWidth) + L"px");
        }
    }
    
    void MainWindow::ForceHideScrollBars() {
        if (!m_hListView || !m_isListViewMode) {
            return;
        }
        
        // 如果滚动条已经隐藏，避免重复操作
        if (m_scrollBarsHidden) {
            return;
        }
        
        // 强制隐藏水平滚动条，使用多种方法确保彻底移除
        ShowScrollBar(m_hListView, SB_HORZ, FALSE);  // 方法1：隐藏水平滚动条
        ShowScrollBar(m_hListView, SB_BOTH, FALSE);  // 方法2：隐藏所有滚动条
        ShowScrollBar(m_hListView, SB_HORZ, FALSE);  // 方法3：再次确认隐藏水平滚动条
        
        // 获取当前样式
        LONG_PTR currentStyle = GetWindowLongPtr(m_hListView, GWL_STYLE);
        LONG_PTR style = currentStyle;
        bool needsUpdate = false;
        
        // 强制移除水平滚动条样式
        if (style & WS_HSCROLL) {
            style &= ~WS_HSCROLL;  // 移除水平滚动条样式
            needsUpdate = true;
        }
        
        // 确保垂直滚动条样式存在
        if (!(style & WS_VSCROLL)) {
            style |= WS_VSCROLL;  // 添加垂直滚动条样式
            needsUpdate = true;
        }
        
        // 应用样式变更
        if (needsUpdate) {
            SetWindowLongPtr(m_hListView, GWL_STYLE, style);
            
            // 强制应用样式变更
            SetWindowPos(m_hListView, NULL, 0, 0, 0, 0, 
                        SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED | SWP_DRAWFRAME);
        }
        
        // 强制禁用水平滚动条
        EnableScrollBar(m_hListView, SB_HORZ, ESB_DISABLE_BOTH);
        EnableScrollBar(m_hListView, SB_VERT, ESB_ENABLE_BOTH);
        
        // 设置水平滚动条范围为0，彻底禁用
        SCROLLINFO si = {0};
        si.cbSize = sizeof(SCROLLINFO);
        si.fMask = SIF_ALL | SIF_DISABLENOSCROLL;
        si.nMin = 0;
        si.nMax = 0;
        si.nPage = 0;
        si.nPos = 0;
        SetScrollInfo(m_hListView, SB_HORZ, &si, TRUE);  // 强制设置水平滚动条为0
        
        // 使用SendMessage强制隐藏水平滚动条
        SendMessage(m_hListView, WM_HSCROLL, SB_TOP, 0);
        
        // 只在需要更新时才重绘，避免闪烁
        if (needsUpdate) {
            // 强制重绘ListView确保水平滚动条完全隐藏
            InvalidateRect(m_hListView, NULL, TRUE);
            UpdateWindow(m_hListView);
        }
        
        // 再次确认隐藏水平滚动条
        ShowScrollBar(m_hListView, SB_HORZ, FALSE);
        
        // 标记滚动条已隐藏
        m_scrollBarsHidden = true;
        
        YG_LOG_INFO(L"已彻底隐藏水平滚动条，保留垂直滚动条");
    }
    
    LRESULT CALLBACK MainWindow::ListViewSubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
        MainWindow* pThis = reinterpret_cast<MainWindow*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
        
        // 拦截水平滚动消息，防止水平滚动条显示
        if (uMsg == WM_HSCROLL) {
            // 直接返回，不处理水平滚动
            return 0;
        }
        
        // 拦截垂直滚动消息，在垂直滚动后隐藏水平滚动条
        if (uMsg == WM_VSCROLL) {
            // 先调用原始窗口过程处理垂直滚动
            LRESULT result = 0;
            if (pThis && pThis->m_originalListViewProc) {
                result = CallWindowProc(pThis->m_originalListViewProc, hWnd, uMsg, wParam, lParam);
            } else {
                result = DefWindowProc(hWnd, uMsg, wParam, lParam);
            }
            
            // 垂直滚动后立即隐藏水平滚动条
            ShowScrollBar(hWnd, SB_HORZ, FALSE);
            return result;
        }
        
        // 拦截鼠标滚轮消息，在滚轮滚动后隐藏水平滚动条
        if (uMsg == WM_MOUSEWHEEL) {
            // 先调用原始窗口过程处理鼠标滚轮
            LRESULT result = 0;
            if (pThis && pThis->m_originalListViewProc) {
                result = CallWindowProc(pThis->m_originalListViewProc, hWnd, uMsg, wParam, lParam);
            } else {
                result = DefWindowProc(hWnd, uMsg, wParam, lParam);
            }
            
            // 鼠标滚轮滚动后立即隐藏水平滚动条
            ShowScrollBar(hWnd, SB_HORZ, FALSE);
            return result;
        }
        
        // 拦截水平滚动条相关的消息
        if (uMsg == WM_PAINT) {
            // 在绘制前强制隐藏水平滚动条（减少频率避免闪烁）
            if (pThis && !pThis->m_scrollBarsHidden) {
                ShowScrollBar(hWnd, SB_HORZ, FALSE);
            }
        }
        
        // 拦截窗口大小变化消息，在大小变化后隐藏水平滚动条
        if (uMsg == WM_SIZE) {
            // 先调用原始窗口过程处理大小变化
            LRESULT result = 0;
            if (pThis && pThis->m_originalListViewProc) {
                result = CallWindowProc(pThis->m_originalListViewProc, hWnd, uMsg, wParam, lParam);
            } else {
                result = DefWindowProc(hWnd, uMsg, wParam, lParam);
            }
            
            // 大小变化后立即隐藏水平滚动条
            ShowScrollBar(hWnd, SB_HORZ, FALSE);
            return result;
        }
        
        // 拦截列表项选择变化消息，在选择变化后隐藏水平滚动条
        if (uMsg == LVM_SETITEMSTATE || uMsg == LVM_SETITEM || uMsg == LVM_INSERTITEM || uMsg == LVM_DELETEITEM) {
            // 先调用原始窗口过程处理列表项操作
            LRESULT result = 0;
            if (pThis && pThis->m_originalListViewProc) {
                result = CallWindowProc(pThis->m_originalListViewProc, hWnd, uMsg, wParam, lParam);
            } else {
                result = DefWindowProc(hWnd, uMsg, wParam, lParam);
            }
            
            // 列表项操作后立即隐藏水平滚动条
            ShowScrollBar(hWnd, SB_HORZ, FALSE);
            return result;
        }
        
        // 调用原始窗口过程
        if (pThis && pThis->m_originalListViewProc) {
            return CallWindowProc(pThis->m_originalListViewProc, hWnd, uMsg, wParam, lParam);
        }
        
        return DefWindowProc(hWnd, uMsg, wParam, lParam);
    }
    
    String MainWindow::FormatFileSize(DWORD fileSize) {
        if (fileSize == 0) return L"0 B";
        
        const wchar_t* units[] = {L"B", L"KB", L"MB", L"GB"};
        double size = static_cast<double>(fileSize);
        int unit = 0;
        
        while (size >= 1024 && unit < 3) {
            size /= 1024;
            unit++;
        }
        
        wchar_t buffer[50];
        if (unit == 0) {
            swprintf_s(buffer, L"%.0f %s", size, units[unit]);
        } else {
            swprintf_s(buffer, L"%.1f %s", size, units[unit]);
        }
        
        return String(buffer);
    }
    
    
    
    String MainWindow::GetProgramWebsite(const ProgramInfo& program) {
        YG_LOG_INFO(L"获取程序官网信息: " + program.name);
        
        String websiteURL;
        
        try {
            // 方法1: 从注册表获取官网信息
            if (!program.registryKey.empty()) {
                String regPath = program.registryKey;
                
                // 如果是完整路径，提取键名部分
                size_t lastSlash = regPath.find_last_of(L'\\');
                if (lastSlash != String::npos) {
                    regPath = L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\" + 
                             regPath.substr(lastSlash + 1);
                } else {
                    regPath = L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\" + regPath;
                }
                
                HKEY hKey;
                if (RegistryHelper::OpenKey(HKEY_LOCAL_MACHINE, regPath, KEY_READ, hKey) == ErrorCode::Success) {
                    // 尝试获取各种网站字段
                    String helpLink, urlInfoAbout, urlUpdateInfo, publisherURL;
                    RegistryHelper::ReadString(hKey, L"HelpLink", helpLink);
                    RegistryHelper::ReadString(hKey, L"URLInfoAbout", urlInfoAbout);
                    RegistryHelper::ReadString(hKey, L"URLUpdateInfo", urlUpdateInfo);
                    RegistryHelper::ReadString(hKey, L"Publisher", publisherURL);
                    
                    RegCloseKey(hKey);
                    
                    // 选择最合适的网站链接
                    if (!helpLink.empty() && IsValidURL(helpLink)) {
                        websiteURL = helpLink;
                        YG_LOG_INFO(L"从HelpLink获取网站: " + websiteURL);
                    } else if (!urlInfoAbout.empty() && IsValidURL(urlInfoAbout)) {
                        websiteURL = urlInfoAbout;
                        YG_LOG_INFO(L"从URLInfoAbout获取网站: " + websiteURL);
                    } else if (!urlUpdateInfo.empty() && IsValidURL(urlUpdateInfo)) {
                        websiteURL = urlUpdateInfo;
                        YG_LOG_INFO(L"从URLUpdateInfo获取网站: " + websiteURL);
                    }
                }
            }
            
            // 方法2: 根据发布者推测官网
            if (websiteURL.empty() && !program.publisher.empty()) {
                websiteURL = GuessWebsiteFromPublisher(program.publisher);
                if (!websiteURL.empty()) {
                    YG_LOG_INFO(L"从发布者推测网站: " + websiteURL);
                }
            }
            
        } catch (const std::exception& e) {
            YG_LOG_WARNING(L"获取官网信息时发生异常: " + StringToWString(e.what()));
        } catch (...) {
            YG_LOG_WARNING(L"获取官网信息时发生未知异常");
        }
        
        return websiteURL;
    }
    
    bool MainWindow::IsValidURL(const String& url) {
        if (url.empty() || url.length() < 7) {
            return false;
        }
        
        // 检查是否以http://或https://开头
        String lowerURL = StringUtils::ToLower(url);
        return StringUtils::StartsWith(lowerURL, L"http://") || 
               StringUtils::StartsWith(lowerURL, L"https://") ||
               StringUtils::StartsWith(lowerURL, L"www.");
    }
    
    String MainWindow::GuessWebsiteFromPublisher(const String& publisher) {
        if (publisher.empty()) {
            return L"";
        }
        
        String lowerPublisher = StringUtils::ToLower(publisher);
        
        // 常见软件厂商的官网映射
        if (StringUtils::Contains(lowerPublisher, L"microsoft")) {
            return L"https://www.microsoft.com";
        } else if (StringUtils::Contains(lowerPublisher, L"google")) {
            return L"https://www.google.com";
        } else if (StringUtils::Contains(lowerPublisher, L"adobe")) {
            return L"https://www.adobe.com";
        } else if (StringUtils::Contains(lowerPublisher, L"mozilla")) {
            return L"https://www.mozilla.org";
        } else if (StringUtils::Contains(lowerPublisher, L"oracle")) {
            return L"https://www.oracle.com";
        } else if (StringUtils::Contains(lowerPublisher, L"apple")) {
            return L"https://www.apple.com";
        } else if (StringUtils::Contains(lowerPublisher, L"nvidia")) {
            return L"https://www.nvidia.com";
        } else if (StringUtils::Contains(lowerPublisher, L"intel")) {
            return L"https://www.intel.com";
        } else if (StringUtils::Contains(lowerPublisher, L"amd")) {
            return L"https://www.amd.com";
        } else if (StringUtils::Contains(lowerPublisher, L"steam") || StringUtils::Contains(lowerPublisher, L"valve")) {
            return L"https://store.steampowered.com";
        } else if (StringUtils::Contains(lowerPublisher, L"epic") || StringUtils::Contains(lowerPublisher, L"epic games")) {
            return L"https://www.epicgames.com";
        } else if (StringUtils::Contains(lowerPublisher, L"腾讯") || StringUtils::Contains(lowerPublisher, L"tencent")) {
            return L"https://www.tencent.com";
        } else if (StringUtils::Contains(lowerPublisher, L"百度") || StringUtils::Contains(lowerPublisher, L"baidu")) {
            return L"https://www.baidu.com";
        } else if (StringUtils::Contains(lowerPublisher, L"阿里") || StringUtils::Contains(lowerPublisher, L"alibaba")) {
            return L"https://www.alibaba.com";
        } else if (StringUtils::Contains(lowerPublisher, L"网易") || StringUtils::Contains(lowerPublisher, L"netease")) {
            return L"https://www.163.com";
        }
        
        return L""; // 无法推测
    }
    
    // 自定义属性对话框类定义
    class CustomPropertyDialog {
    private:
        HWND m_hParent;
        HWND m_hDialog;
        ProgramInfo m_program;
        String m_websiteURL;
        
        // 控件句柄
        HWND m_hPathLink;
        HWND m_hWebsiteLink;
        HWND m_hCloseButton;
        HWND m_hHeaderPanel;
        bool m_dialogClosed;
        
        // 现代化设计常量
        static constexpr int DIALOG_WIDTH = 580;
        static constexpr int DIALOG_HEIGHT = 420;
        static constexpr int MARGIN = 24;
        static constexpr int LINE_HEIGHT = 32;
        static constexpr int ICON_SIZE = 20;
        static constexpr int HEADER_HEIGHT = 60;
        static constexpr int BUTTON_HEIGHT = 36;
        
        // 现代化配色方案
        static constexpr COLORREF BG_COLOR = RGB(248, 249, 250);
        static constexpr COLORREF HEADER_COLOR = RGB(255, 255, 255);
        static constexpr COLORREF TEXT_PRIMARY = RGB(33, 37, 41);
        static constexpr COLORREF TEXT_SECONDARY = RGB(108, 117, 125);
        static constexpr COLORREF LINK_COLOR = RGB(13, 110, 253);
        static constexpr COLORREF LINK_HOVER = RGB(10, 88, 202);
        static constexpr COLORREF BORDER_COLOR = RGB(222, 226, 230);
        static constexpr COLORREF BUTTON_COLOR = RGB(13, 110, 253);
        static constexpr COLORREF BUTTON_HOVER = RGB(10, 88, 202);
        
    public:
        CustomPropertyDialog(HWND hParent, const ProgramInfo& program, const String& websiteURL)
            : m_hParent(hParent), m_hDialog(nullptr), m_program(program), m_websiteURL(websiteURL),
              m_hPathLink(nullptr), m_hWebsiteLink(nullptr), m_hCloseButton(nullptr), 
              m_hHeaderPanel(nullptr), m_dialogClosed(false) {
        }
        
        void Show() {
            YG_LOG_INFO(L"开始显示自定义属性对话框");
            
            // 注册现代化窗口类
            static bool classRegistered = false;
            if (!classRegistered) {
                WNDCLASSEX wc = {};
                wc.cbSize = sizeof(WNDCLASSEX);
                wc.style = CS_HREDRAW | CS_VREDRAW | CS_DROPSHADOW; // 添加阴影效果
                wc.lpfnWndProc = DialogProc;
                wc.hInstance = GetModuleHandle(nullptr);
                wc.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
                wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
                wc.hbrBackground = CreateSolidBrush(BG_COLOR); // 使用自定义背景色
                wc.lpszClassName = L"YGModernPropertyDialog";
                
                RegisterClassEx(&wc);
                classRegistered = true;
            }
            
            // 创建窗口标题
            String windowTitle = (!m_program.displayName.empty() ? m_program.displayName : m_program.name) + L" - 属性";
            
            // 创建现代化对话框
            m_hDialog = CreateWindowEx(
                WS_EX_DLGMODALFRAME | WS_EX_LAYERED, // 添加分层窗口支持
                L"YGModernPropertyDialog",
                windowTitle.c_str(),
                WS_POPUP | WS_CAPTION | WS_SYSMENU,
                CW_USEDEFAULT, CW_USEDEFAULT,
                DIALOG_WIDTH, DIALOG_HEIGHT,
                m_hParent,
                nullptr,
                GetModuleHandle(nullptr),
                this
            );
            
            if (!m_hDialog) {
                DWORD error = GetLastError();
                YG_LOG_ERROR(L"自定义属性对话框创建失败，错误代码: " + std::to_wstring(error));
                return;
            }
            
            YG_LOG_INFO(L"对话框创建成功，句柄: " + std::to_wstring(reinterpret_cast<uintptr_t>(m_hDialog)));
            
            // 居中显示
            CenterWindow();
            
            // 创建控件
            CreateControls();
            
            // 显示对话框
            ShowWindow(m_hDialog, SW_SHOW);
            UpdateWindow(m_hDialog);
            
            // 设置为模态
            EnableWindow(m_hParent, FALSE);
            
            // 使用更安全的消息循环
            YG_LOG_INFO(L"进入模态消息循环");
            MSG msg;
            while (!m_dialogClosed && IsWindow(m_hDialog)) {
                BOOL bRet = GetMessage(&msg, nullptr, 0, 0);
                
                if (bRet == 0) {
                    // WM_QUIT消息
                    YG_LOG_INFO(L"接收到WM_QUIT消息，退出对话框");
                    PostQuitMessage((int)msg.wParam); // 重新发送WM_QUIT
                    break;
                } else if (bRet == -1) {
                    // 错误
                    YG_LOG_ERROR(L"GetMessage发生错误: " + std::to_wstring(GetLastError()));
                    break;
                } else {
                    // 正常消息处理
                    TranslateMessage(&msg);
                    DispatchMessage(&msg);
                }
            }
            
            YG_LOG_INFO(L"退出模态消息循环，m_dialogClosed=" + std::to_wstring(m_dialogClosed));
            
            // 恢复父窗口
            if (IsWindow(m_hParent)) {
                EnableWindow(m_hParent, TRUE);
                SetForegroundWindow(m_hParent);
            }
        }
        
    private:
        void CenterWindow() {
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
        
        void CreateControls() {
            // 创建现代化字体
            HFONT hTitleFont = CreateFont(-18, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE,
                                        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
            
            HFONT hBodyFont = CreateFont(-14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                                       DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                       CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
            
            HFONT hLabelFont = CreateFont(-12, 0, 0, 0, FW_MEDIUM, FALSE, FALSE, FALSE,
                                        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
            
            // 创建头部面板
            m_hHeaderPanel = CreateWindow(L"STATIC", L"",
                                        WS_CHILD | WS_VISIBLE | SS_OWNERDRAW,
                                        0, 0, DIALOG_WIDTH, HEADER_HEIGHT,
                                        m_hDialog, (HMENU)2000, GetModuleHandle(nullptr), nullptr);
            
            int currentY = HEADER_HEIGHT + MARGIN;
            
            // 程序名称（大标题）
            String programName = !m_program.displayName.empty() ? m_program.displayName : m_program.name;
            HWND hProgramTitle = CreateWindow(L"STATIC", programName.c_str(),
                                            WS_CHILD | WS_VISIBLE | SS_LEFT,
                                            MARGIN, 15, DIALOG_WIDTH - 2 * MARGIN, 30,
                                            m_hDialog, nullptr, GetModuleHandle(nullptr), nullptr);
            SendMessage(hProgramTitle, WM_SETFONT, (WPARAM)hTitleFont, TRUE);
            
            // 创建属性信息区域
            CreateModernInfoLine(L"📋", L"版本", m_program.version.empty() ? L"未知" : m_program.version, 
                               currentY, hLabelFont, hBodyFont, false, 0);
            currentY += LINE_HEIGHT;
            
            CreateModernInfoLine(L"👤", L"发布者", m_program.publisher.empty() ? L"未知" : m_program.publisher,
                               currentY, hLabelFont, hBodyFont, false, 0);
            currentY += LINE_HEIGHT;
            
            CreateModernInfoLine(L"📅", L"安装时间", m_program.installDate.empty() ? L"未知" : m_program.installDate,
                               currentY, hLabelFont, hBodyFont, false, 0);
            currentY += LINE_HEIGHT;
            
            String sizeText = m_program.estimatedSize > 0 ? 
                StringUtils::FormatFileSize(m_program.estimatedSize) : L"未知";
            CreateModernInfoLine(L"💾", L"程序大小", sizeText,
                               currentY, hLabelFont, hBodyFont, false, 0);
            currentY += LINE_HEIGHT;
            
            // 可点击的链接区域
            if (!m_program.installLocation.empty()) {
                m_hPathLink = CreateModernInfoLine(L"📁", L"安装路径", m_program.installLocation,
                                                 currentY, hLabelFont, hBodyFont, true, 1001);
                currentY += LINE_HEIGHT;
            }
            
            if (!m_websiteURL.empty()) {
                m_hWebsiteLink = CreateModernInfoLine(L"🌐", L"官方网站", m_websiteURL,
                                                    currentY, hLabelFont, hBodyFont, true, 1002);
                currentY += LINE_HEIGHT;
            }
            
            // 创建现代化按钮
            currentY += MARGIN;
            m_hCloseButton = CreateWindow(L"BUTTON", L"关闭",
                                        WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
                                        (DIALOG_WIDTH - 100) / 2, currentY, 100, BUTTON_HEIGHT,
                                        m_hDialog, (HMENU)IDOK, GetModuleHandle(nullptr), nullptr);
            
            SendMessage(m_hCloseButton, WM_SETFONT, (WPARAM)hBodyFont, TRUE);
        }
        
        HWND CreateModernInfoLine(const String& icon, const String& label, const String& value, 
                                 int y, HFONT hLabelFont, HFONT hValueFont, bool isClickable, int id) {
            // 创建图标（使用真正的emoji）
            CreateWindow(L"STATIC", icon.c_str(),
                        WS_CHILD | WS_VISIBLE | SS_LEFT | SS_CENTERIMAGE,
                        MARGIN, y, ICON_SIZE + 8, LINE_HEIGHT,
                        m_hDialog, nullptr, GetModuleHandle(nullptr), nullptr);
            
            // 创建标签
            HWND hLabel = CreateWindow(L"STATIC", label.c_str(),
                                     WS_CHILD | WS_VISIBLE | SS_LEFT | SS_CENTERIMAGE,
                                     MARGIN + ICON_SIZE + 12, y, 100, LINE_HEIGHT,
                                     m_hDialog, nullptr, GetModuleHandle(nullptr), nullptr);
            SendMessage(hLabel, WM_SETFONT, (WPARAM)hLabelFont, TRUE);
            
            // 创建值控件
            DWORD valueStyle = WS_CHILD | WS_VISIBLE | SS_LEFT | SS_CENTERIMAGE;
            if (isClickable) {
                valueStyle |= SS_NOTIFY;
            }
            
            HWND hValue = CreateWindow(L"STATIC", value.c_str(),
                                     valueStyle,
                                     MARGIN + ICON_SIZE + 120, y, 
                                     DIALOG_WIDTH - MARGIN - ICON_SIZE - 130, LINE_HEIGHT,
                                     m_hDialog, isClickable ? (HMENU)(UINT_PTR)id : nullptr, 
                                     GetModuleHandle(nullptr), nullptr);
            
            SendMessage(hValue, WM_SETFONT, (WPARAM)hValueFont, TRUE);
            
            // 如果是可点击的，创建下划线字体
            if (isClickable) {
                LOGFONT lf;
                GetObject(hValueFont, sizeof(LOGFONT), &lf);
                lf.lfUnderline = TRUE;
                HFONT hLinkFont = CreateFontIndirect(&lf);
                SendMessage(hValue, WM_SETFONT, (WPARAM)hLinkFont, TRUE);
            }
            
            return hValue;
        }
        
        void CreateInfoLine(const String& icon, const String& label, const String& value, int y, bool clickable) {
            HFONT hDefaultFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
            
            // 图标
            CreateWindow(L"STATIC", icon.c_str(),
                        WS_CHILD | WS_VISIBLE | SS_LEFT,
                        MARGIN, y, ICON_SIZE + 5, LINE_HEIGHT,
                        m_hDialog, nullptr, GetModuleHandle(nullptr), nullptr);
            
            // 标签
            CreateWindow(L"STATIC", label.c_str(),
                        WS_CHILD | WS_VISIBLE | SS_LEFT,
                        MARGIN + ICON_SIZE + 10, y, 80, LINE_HEIGHT,
                        m_hDialog, nullptr, GetModuleHandle(nullptr), nullptr);
            
            // 值
            HWND hValue = CreateWindow(L"STATIC", value.c_str(),
                                     WS_CHILD | WS_VISIBLE | SS_LEFT,
                                     MARGIN + ICON_SIZE + 100, y, DIALOG_WIDTH - MARGIN - ICON_SIZE - 110, LINE_HEIGHT,
                                     m_hDialog, nullptr, GetModuleHandle(nullptr), nullptr);
            
            SendMessage(hValue, WM_SETFONT, (WPARAM)hDefaultFont, TRUE);
        }
        
        HWND CreateClickableInfoLine(const String& icon, const String& label, const String& value, int y, int id) {
            HFONT hDefaultFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
            
            // 图标
            CreateWindow(L"STATIC", icon.c_str(),
                        WS_CHILD | WS_VISIBLE | SS_LEFT,
                        MARGIN, y, ICON_SIZE + 5, LINE_HEIGHT,
                        m_hDialog, nullptr, GetModuleHandle(nullptr), nullptr);
            
            // 标签
            CreateWindow(L"STATIC", label.c_str(),
                        WS_CHILD | WS_VISIBLE | SS_LEFT,
                        MARGIN + ICON_SIZE + 10, y, 80, LINE_HEIGHT,
                        m_hDialog, nullptr, GetModuleHandle(nullptr), nullptr);
            
            // 可点击的值（链接样式）
            HWND hLink = CreateWindow(L"STATIC", value.c_str(),
                                    WS_CHILD | WS_VISIBLE | SS_LEFT | SS_NOTIFY,
                                    MARGIN + ICON_SIZE + 100, y, DIALOG_WIDTH - MARGIN - ICON_SIZE - 110, LINE_HEIGHT,
                                    m_hDialog, (HMENU)(UINT_PTR)id, GetModuleHandle(nullptr), nullptr);
            
            // 创建下划线字体
            LOGFONT lf;
            GetObject(hDefaultFont, sizeof(LOGFONT), &lf);
            lf.lfUnderline = TRUE;
            HFONT hLinkFont = CreateFontIndirect(&lf);
            
            SendMessage(hLink, WM_SETFONT, (WPARAM)hLinkFont, TRUE);
            
            return hLink;
        }
        
        static LRESULT CALLBACK DialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
            CustomPropertyDialog* dialog = nullptr;
            
            if (message == WM_NCCREATE) {
                CREATESTRUCT* pCreate = (CREATESTRUCT*)lParam;
                dialog = (CustomPropertyDialog*)pCreate->lpCreateParams;
                SetWindowLongPtr(hDlg, GWLP_USERDATA, (LONG_PTR)dialog);
                dialog->m_hDialog = hDlg;
            } else {
                dialog = (CustomPropertyDialog*)GetWindowLongPtr(hDlg, GWLP_USERDATA);
            }
            
            if (dialog) {
                return dialog->HandleMessage(message, wParam, lParam);
            }
            
            return DefWindowProc(hDlg, message, wParam, lParam);
        }
        
        LRESULT HandleMessage(UINT message, WPARAM wParam, LPARAM lParam) {
            switch (message) {
                case WM_DRAWITEM:
                    return OnDrawItem((DRAWITEMSTRUCT*)lParam);
                    
                case WM_COMMAND:
                    if (LOWORD(wParam) == IDOK) {
                        // 关闭按钮
                        m_dialogClosed = true;
                        DestroyWindow(m_hDialog);
                        return 0;
                    } else if (LOWORD(wParam) == 1001) {
                        // 安装路径链接被点击
                        if (HIWORD(wParam) == STN_CLICKED) {
                            OpenInstallPath();
                        }
                        return 0;
                    } else if (LOWORD(wParam) == 1002) {
                        // 网站链接被点击
                        if (HIWORD(wParam) == STN_CLICKED) {
                            OpenWebsite();
                        }
                        return 0;
                    }
                    break;
                    
                case WM_CLOSE:
                    m_dialogClosed = true;
                    DestroyWindow(m_hDialog);
                    return 0;
                    
                case WM_DESTROY:
                    // 设置关闭标志，不要调用PostQuitMessage
                    m_dialogClosed = true;
                    return 0;
                    
                case WM_CTLCOLORSTATIC:
                    return OnCtlColorStatic((HDC)wParam, (HWND)lParam);
                    
                case WM_SETCURSOR:
                    // 为链接设置手型光标
                    if ((HWND)wParam == m_hPathLink || (HWND)wParam == m_hWebsiteLink) {
                        SetCursor(LoadCursor(nullptr, IDC_HAND));
                    return TRUE;
                    }
                    break;
            }
            
            return DefWindowProc(m_hDialog, message, wParam, lParam);
        }
        
        void OpenInstallPath() {
            if (m_program.installLocation.empty()) {
                return;
            }
            
            YG_LOG_INFO(L"点击打开安装路径: " + m_program.installLocation);
            
            // 使用explorer /select定位到安装目录
            String command = L"/select,\"" + m_program.installLocation + L"\"";
            HINSTANCE result = ShellExecute(m_hDialog, L"open", L"explorer.exe", 
                                          command.c_str(), nullptr, SW_SHOWNORMAL);
            
            if ((INT_PTR)result <= 32) {
                // 如果失败，尝试直接打开目录
                ShellExecute(m_hDialog, L"open", m_program.installLocation.c_str(), 
                           nullptr, nullptr, SW_SHOWNORMAL);
            }
        }
        
        void OpenWebsite() {
            if (m_websiteURL.empty()) {
                return;
            }
            
            YG_LOG_INFO(L"点击打开官方网站: " + m_websiteURL);
            
            ShellExecute(m_hDialog, L"open", m_websiteURL.c_str(), 
                       nullptr, nullptr, SW_SHOWNORMAL);
        }
        
        // 自定义绘制方法
        LRESULT OnDrawItem(DRAWITEMSTRUCT* pDIS) {
            if (pDIS->CtlID == IDOK) { // 关闭按钮
                return DrawModernButton(pDIS);
            } else if (pDIS->CtlID == 2000) { // 头部面板
                return DrawHeaderPanel(pDIS);
            }
            return 0;
        }
        
        LRESULT DrawModernButton(DRAWITEMSTRUCT* pDIS) {
            HDC hdc = pDIS->hDC;
            RECT rect = pDIS->rcItem;
            
            // 判断按钮状态
            bool isPressed = (pDIS->itemState & ODS_SELECTED) != 0;
            
            // 绘制圆角矩形背景
            HBRUSH hBrush = CreateSolidBrush(isPressed ? BUTTON_HOVER : BUTTON_COLOR);
            HPEN hPen = CreatePen(PS_SOLID, 1, BUTTON_COLOR);
            
            SelectObject(hdc, hBrush);
            SelectObject(hdc, hPen);
            
            // 绘制圆角矩形
            RoundRect(hdc, rect.left, rect.top, rect.right, rect.bottom, 6, 6);
            
            // 绘制文本
            SetBkMode(hdc, TRANSPARENT);
            SetTextColor(hdc, RGB(255, 255, 255));
            
            HFONT hFont = CreateFont(-14, 0, 0, 0, FW_MEDIUM, FALSE, FALSE, FALSE,
                                   DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                   CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
            SelectObject(hdc, hFont);
            
            DrawText(hdc, L"关闭", -1, &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
            
            // 清理资源
            DeleteObject(hBrush);
            DeleteObject(hPen);
            DeleteObject(hFont);
                    
                    return TRUE;
                }
                
        LRESULT DrawHeaderPanel(DRAWITEMSTRUCT* pDIS) {
            HDC hdc = pDIS->hDC;
            RECT rect = pDIS->rcItem;
            
            // 绘制白色背景
            HBRUSH hBrush = CreateSolidBrush(HEADER_COLOR);
            FillRect(hdc, &rect, hBrush);
            
            // 绘制底部边框
            HPEN hPen = CreatePen(PS_SOLID, 1, BORDER_COLOR);
            SelectObject(hdc, hPen);
            MoveToEx(hdc, rect.left, rect.bottom - 1, nullptr);
            LineTo(hdc, rect.right, rect.bottom - 1);
            
            DeleteObject(hBrush);
            DeleteObject(hPen);
            
                            return TRUE;
                    }
        
        LRESULT OnCtlColorStatic(HDC hdc, HWND hWnd) {
            // 设置透明背景
            SetBkMode(hdc, TRANSPARENT);
            
            // 为链接设置特殊颜色
            if (hWnd == m_hPathLink || hWnd == m_hWebsiteLink) {
                SetTextColor(hdc, LINK_COLOR);
                return (LRESULT)GetStockObject(NULL_BRUSH);
            }
            
            // 为其他静态控件设置正常颜色
            SetTextColor(hdc, TEXT_PRIMARY);
            return (LRESULT)GetStockObject(NULL_BRUSH);
        }
    };
    
    // 可点击链接的属性对话框类定义
    class ClickablePropertyDialog {
    private:
        HWND m_hParent;
        HWND m_hDialog;
        ProgramInfo m_program;
        String m_installPath;
        String m_websiteURL;
        
        // 控件句柄
        HWND m_hInstallPathLink;
        HWND m_hWebsiteLink;
        HWND m_hCloseButton;
        bool m_dialogClosed;
        WINDOWPLACEMENT m_parentOriginalState;
        
        static constexpr int DIALOG_WIDTH = 520;
        static constexpr int DIALOG_HEIGHT = 350;  // 适中的对话框高度
        static constexpr int MARGIN = 20;
        static constexpr int LINE_HEIGHT = 32;     // 适中的行高
        
        // 现代化配色
        static constexpr COLORREF BG_COLOR = RGB(248, 249, 250);
        static constexpr COLORREF TEXT_COLOR = RGB(33, 37, 41);
        static constexpr COLORREF LINK_COLOR = RGB(13, 110, 253);
        static constexpr COLORREF BUTTON_COLOR = RGB(13, 110, 253);
        
    public:
        ClickablePropertyDialog(HWND hParent, const ProgramInfo& program, const String& installPath, const String& websiteURL)
            : m_hParent(hParent), m_hDialog(nullptr), m_program(program), m_installPath(installPath), m_websiteURL(websiteURL),
              m_hInstallPathLink(nullptr), m_hWebsiteLink(nullptr), m_hCloseButton(nullptr), m_dialogClosed(false) {
        }
        
        void Show() {
            YG_LOG_INFO(L"开始显示可点击链接属性对话框");
            
            // 保存主窗口当前状态
            m_parentOriginalState.length = sizeof(WINDOWPLACEMENT);
            GetWindowPlacement(m_hParent, &m_parentOriginalState);
            YG_LOG_INFO(L"保存主窗口状态，showCmd=" + std::to_wstring(m_parentOriginalState.showCmd));
            
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
                wc.lpszClassName = L"YGClickablePropertyDialog";
                
                RegisterClassEx(&wc);
                classRegistered = true;
            }
            
            // 创建窗口标题
            String windowTitle = (!m_program.displayName.empty() ? m_program.displayName : m_program.name) + L" - 属性";
        
        // 创建对话框
            m_hDialog = CreateWindowEx(
                WS_EX_DLGMODALFRAME,
                L"YGClickablePropertyDialog",
                windowTitle.c_str(),
                WS_POPUP | WS_CAPTION | WS_SYSMENU,
                CW_USEDEFAULT, CW_USEDEFAULT,
                DIALOG_WIDTH, DIALOG_HEIGHT,
                m_hParent,
                nullptr,
                GetModuleHandle(nullptr),
                this
            );
            
            if (!m_hDialog) {
                YG_LOG_ERROR(L"可点击属性对话框创建失败");
                return;
            }
            
            YG_LOG_INFO(L"对话框创建成功");
            
            // 居中显示
            CenterWindow();
            
            // 创建控件
            CreateControls();
            
            // 显示对话框
            ShowWindow(m_hDialog, SW_SHOW);
            UpdateWindow(m_hDialog);
            
            // 设置为模态
            EnableWindow(m_hParent, FALSE);
            
            // 使用标准模态对话框消息循环，避免影响主窗口
            MSG msg;
            while (!m_dialogClosed && IsWindow(m_hDialog)) {
                BOOL bRet = GetMessage(&msg, nullptr, 0, 0);
                
                if (bRet == 0) {
                    // WM_QUIT消息 - 直接退出对话框循环，不影响主程序
                    YG_LOG_INFO(L"接收到WM_QUIT消息，退出对话框循环");
                    break;
                } else if (bRet == -1) {
                    // 错误
                    YG_LOG_ERROR(L"GetMessage发生错误");
                    break;
                } else {
                    // 处理所有消息，但不干扰主窗口状态
                    TranslateMessage(&msg);
                    DispatchMessage(&msg);
                }
            }
            
            // 恢复父窗口到原始状态
            if (IsWindow(m_hParent)) {
                // 重新启用父窗口
                EnableWindow(m_hParent, TRUE);
                
                // 恢复到保存的原始状态
                YG_LOG_INFO(L"恢复主窗口到原始状态，showCmd=" + std::to_wstring(m_parentOriginalState.showCmd));
                
                // 确保恢复到原始状态，而不是被最小化
                if (m_parentOriginalState.showCmd == SW_SHOWMINIMIZED) {
                    // 如果原来就是最小化的，保持最小化
                    SetWindowPlacement(m_hParent, &m_parentOriginalState);
        } else {
                    // 如果原来不是最小化的，确保不被最小化
                    if (IsIconic(m_hParent)) {
                        ShowWindow(m_hParent, SW_RESTORE);
                    }
                    
                    // 恢复原始位置和大小，但确保窗口可见
                    WINDOWPLACEMENT restoreState = m_parentOriginalState;
                    if (restoreState.showCmd == SW_SHOWMINIMIZED) {
                        restoreState.showCmd = SW_SHOWNORMAL;
                    }
                    SetWindowPlacement(m_hParent, &restoreState);
                }
                
                // 确保主窗口在前台
                SetForegroundWindow(m_hParent);
                BringWindowToTop(m_hParent);
                
                YG_LOG_INFO(L"主窗口状态已完全恢复");
            }
            
            YG_LOG_INFO(L"可点击属性对话框已关闭");
        }
        
    private:
        void CenterWindow() {
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
        
        void CreateControls() {
            HFONT hFont = CreateFont(-14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                                   DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                   CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
            
            HFONT hLinkFont = CreateFont(-14, 0, 0, 0, FW_NORMAL, FALSE, TRUE, FALSE,
                                       DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                       CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
            
            int currentY = MARGIN;
            String programName = !m_program.displayName.empty() ? m_program.displayName : m_program.name;
            
            // 程序名称 - 适中的间距处理
            CreateProgramNameLabel(L"📦 程序名称:", programName, currentY, hFont);
            currentY += (programName.length() > 50 ? LINE_HEIGHT + 8 : LINE_HEIGHT); // 长名称额外增加8px
            
            // 版本
            CreateInfoLabel(L"🏷️ 版本:", m_program.version.empty() ? L"未知" : m_program.version, currentY, hFont);
            currentY += LINE_HEIGHT;
            
            // 发布者
            CreateInfoLabel(L"👤 发布者:", m_program.publisher.empty() ? L"未知" : m_program.publisher, currentY, hFont);
            currentY += LINE_HEIGHT;
            
            // 安装时间
            CreateInfoLabel(L"📅 安装时间:", m_program.installDate.empty() ? L"未知" : m_program.installDate, currentY, hFont);
            currentY += LINE_HEIGHT;
            
            // 程序大小 - estimatedSize已经是字节单位，不需要再乘以1024
            String sizeText = m_program.estimatedSize > 0 ? 
                StringUtils::FormatFileSize(m_program.estimatedSize) : L"未知";
            CreateInfoLabel(L"💾 程序大小:", sizeText, currentY, hFont);
            currentY += LINE_HEIGHT;
            
            // 安装路径（可点击链接）
            CreateInfoLabel(L"📁 安装路径:", L"", currentY, hFont);
            if (!m_installPath.empty()) {
                m_hInstallPathLink = CreateClickableLink(m_installPath, 140, currentY, 1001, hLinkFont);
                                        } else {
                CreateInfoLabel(L"", L"未知", currentY, hFont);
            }
            currentY += LINE_HEIGHT;
            
            // 官方网站（可点击链接）
            if (!m_websiteURL.empty()) {
                CreateInfoLabel(L"🌐 官方网站:", L"", currentY, hFont);
                m_hWebsiteLink = CreateClickableLink(m_websiteURL, 140, currentY, 1002, hLinkFont);
                currentY += LINE_HEIGHT;
            }
            
            // 去掉关闭按钮 - 用户可以通过窗口右上角的 X 按钮关闭
        }
        
        void CreateProgramNameLabel(const String& label, const String& value, int y, HFONT hFont) {
            // 标签 - 垂直居中对齐
            HWND hLabel = CreateWindow(L"STATIC", label.c_str(),
                                     WS_CHILD | WS_VISIBLE | SS_LEFT | SS_CENTERIMAGE,
                                     MARGIN, y, 120, LINE_HEIGHT,
                                     m_hDialog, nullptr, GetModuleHandle(nullptr), nullptr);
            SendMessage(hLabel, WM_SETFONT, (WPARAM)hFont, TRUE);
            
            // 程序名称值 - 支持多行显示和自动换行
            if (!value.empty()) {
                int valueHeight = value.length() > 50 ? LINE_HEIGHT + 8 : LINE_HEIGHT; // 长名称使用稍大高度
                HWND hValue = CreateWindow(L"STATIC", value.c_str(),
                                         WS_CHILD | WS_VISIBLE | SS_LEFT | SS_WORDELLIPSIS,
                                         MARGIN + 140, y, DIALOG_WIDTH - MARGIN - 160, valueHeight,
                                         m_hDialog, nullptr, GetModuleHandle(nullptr), nullptr);
                SendMessage(hValue, WM_SETFONT, (WPARAM)hFont, TRUE);
            }
        }
        
        void CreateInfoLabel(const String& label, const String& value, int y, HFONT hFont) {
            // 标签 - 垂直居中对齐
            HWND hLabel = CreateWindow(L"STATIC", label.c_str(),
                                     WS_CHILD | WS_VISIBLE | SS_LEFT | SS_CENTERIMAGE,
                                     MARGIN, y, 120, LINE_HEIGHT,
                                     m_hDialog, nullptr, GetModuleHandle(nullptr), nullptr);
            SendMessage(hLabel, WM_SETFONT, (WPARAM)hFont, TRUE);
            
            // 值 - 支持多行显示，增加行间距
            if (!value.empty()) {
                HWND hValue = CreateWindow(L"STATIC", value.c_str(),
                                         WS_CHILD | WS_VISIBLE | SS_LEFT | SS_WORDELLIPSIS,
                                         MARGIN + 140, y, DIALOG_WIDTH - MARGIN - 160, LINE_HEIGHT,
                                         m_hDialog, nullptr, GetModuleHandle(nullptr), nullptr);
                SendMessage(hValue, WM_SETFONT, (WPARAM)hFont, TRUE);
            }
        }
        
        HWND CreateClickableLink(const String& text, int x, int y, int id, HFONT hFont) {
            HWND hLink = CreateWindow(L"STATIC", text.c_str(),
                                    WS_CHILD | WS_VISIBLE | SS_LEFT | SS_NOTIFY | SS_WORDELLIPSIS,
                                    MARGIN + x, y, DIALOG_WIDTH - MARGIN - x - 20, LINE_HEIGHT,
                                    m_hDialog, (HMENU)(UINT_PTR)id, GetModuleHandle(nullptr), nullptr);
            
            SendMessage(hLink, WM_SETFONT, (WPARAM)hFont, TRUE);
            return hLink;
        }
        
        static LRESULT CALLBACK DialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
            ClickablePropertyDialog* dialog = nullptr;
            
            if (message == WM_NCCREATE) {
                CREATESTRUCT* pCreate = (CREATESTRUCT*)lParam;
                dialog = (ClickablePropertyDialog*)pCreate->lpCreateParams;
                SetWindowLongPtr(hDlg, GWLP_USERDATA, (LONG_PTR)dialog);
                dialog->m_hDialog = hDlg;
            } else {
                dialog = (ClickablePropertyDialog*)GetWindowLongPtr(hDlg, GWLP_USERDATA);
            }
            
            if (dialog) {
                return dialog->HandleMessage(message, wParam, lParam);
            }
            
            return DefWindowProc(hDlg, message, wParam, lParam);
        }
        
        LRESULT HandleMessage(UINT message, WPARAM wParam, LPARAM lParam) {
            switch (message) {
                case WM_COMMAND:
                    if (LOWORD(wParam) == 1001) {
                        // 安装路径链接被点击
                        if (HIWORD(wParam) == STN_CLICKED) {
                            OpenInstallPath();
                        }
                        return 0;
                    } else if (LOWORD(wParam) == 1002) {
                        // 网站链接被点击
                        if (HIWORD(wParam) == STN_CLICKED) {
                            OpenWebsite();
                        }
                        return 0;
                        }
                        break;
                        
                case WM_CLOSE:
                    m_dialogClosed = true;
                    DestroyWindow(m_hDialog);
                    return 0;
                    
                case WM_DESTROY:
                    m_dialogClosed = true;
                    return 0;
                    
                case WM_KEYDOWN:
                    // 支持ESC键关闭对话框
                    if (wParam == VK_ESCAPE) {
                        m_dialogClosed = true;
                        DestroyWindow(m_hDialog);
                        return 0;
                    }
                        break;
                        
                case WM_CTLCOLORSTATIC:
                    // 为链接设置蓝色文本
                    if ((HWND)lParam == m_hInstallPathLink || (HWND)lParam == m_hWebsiteLink) {
                        HDC hdc = (HDC)wParam;
                        SetTextColor(hdc, LINK_COLOR);
                        SetBkMode(hdc, TRANSPARENT);
                        return (LRESULT)GetStockObject(NULL_BRUSH);
                    }
                    // 为其他控件设置正常颜色
                    else {
                        HDC hdc = (HDC)wParam;
                        SetTextColor(hdc, TEXT_COLOR);
                        SetBkMode(hdc, TRANSPARENT);
                        return (LRESULT)GetStockObject(NULL_BRUSH);
                    }
                    
                case WM_SETCURSOR:
                    // 为链接设置手型光标
                    if ((HWND)wParam == m_hInstallPathLink || (HWND)wParam == m_hWebsiteLink) {
                        SetCursor(LoadCursor(nullptr, IDC_HAND));
                        return TRUE;
                }
                break;
        }
            
            return DefWindowProc(m_hDialog, message, wParam, lParam);
        }
        
        void OpenInstallPath() {
            if (m_installPath.empty()) {
            return;
        }
        
            YG_LOG_INFO(L"点击打开安装路径: " + m_installPath);
            
            // 使用explorer /select定位到安装目录
            String command = L"/select,\"" + m_installPath + L"\"";
            HINSTANCE result = ShellExecute(m_hDialog, L"open", L"explorer.exe", 
                                          command.c_str(), nullptr, SW_SHOWNORMAL);
            
            if ((INT_PTR)result <= 32) {
                // 如果失败，尝试直接打开目录
                ShellExecute(m_hDialog, L"open", m_installPath.c_str(), 
                           nullptr, nullptr, SW_SHOWNORMAL);
            }
        }
        
        void OpenWebsite() {
            if (m_websiteURL.empty()) {
                return;
            }
            
            YG_LOG_INFO(L"点击打开官方网站: " + m_websiteURL);
            
            ShellExecute(m_hDialog, L"open", m_websiteURL.c_str(), 
                       nullptr, nullptr, SW_SHOWNORMAL);
        }
    };
    
    void MainWindow::ShowCustomPropertyDialog(const ProgramInfo& program) {
        YG_LOG_INFO(L"显示自定义属性对话框: " + program.name);
        
        // 获取程序信息
        String programName = !program.displayName.empty() ? program.displayName : program.name;
        
        // 获取安装路径信息
        String installPath = program.installLocation;
        if (installPath.empty() && !program.uninstallString.empty()) {
            installPath = ExtractPathFromUninstallString(program.uninstallString);
        }
        if (installPath.empty() && !program.iconPath.empty()) {
            installPath = ExtractPathFromIconPath(program.iconPath);
        }
        
        // 获取官网信息
        String websiteURL = GetProgramWebsite(program);
        
        // 创建可点击链接的自定义属性对话框
        ClickablePropertyDialog dialog(m_hWnd, program, installPath, websiteURL);
        dialog.Show();
    }
    
    void MainWindow::OnUninstallComplete(const ProgramInfo& program, bool success) {
        YG_LOG_INFO(L"卸载完成回调: " + program.name + L", 成功: " + std::to_wstring(success));
        
        // 使用PostMessage确保在主线程中安全处理
        LPARAM lParam = success ? 1 : 0;
        PostMessage(m_hWnd, WM_USER + 100, 0, lParam);
    }
    
    void MainWindow::HandleUninstallComplete(bool success) {
        YG_LOG_INFO(L"处理卸载完成消息，成功: " + std::to_wstring(success));
        
        if (success) {
            // 卸载成功，启动残留扫描
            SetStatusText(L"卸载完成，正在扫描残留文件...");
            
            if (m_residualScanner) {
                try {
                    // 设置扫描进度回调
                    auto progressCallback = [this](int percentage, const String& currentPath, int foundCount) {
                        // 使用PostMessage确保线程安全
                        PostMessage(m_hWnd, WM_USER + 101, percentage, foundCount);
                    };
                    
                    // 启动残留扫描
                    ErrorCode scanResult = m_residualScanner->StartScan(m_currentUninstallingProgram, progressCallback);
                    
                    if (scanResult == ErrorCode::Success) {
                        YG_LOG_INFO(L"残留扫描已启动");
                    } else {
                        YG_LOG_ERROR(L"残留扫描启动失败");
                        SetStatusText(L"残留扫描启动失败");
                        // 延迟刷新程序列表
                        SetTimer(m_hWnd, 4, 1000, nullptr);
                    }
                } catch (const std::exception& e) {
                    (void)e; // 避免未使用变量警告
                    YG_LOG_ERROR(L"残留扫描发生异常");
                    SetStatusText(L"残留扫描发生异常");
                    // 延迟刷新程序列表
                    SetTimer(m_hWnd, 4, 1000, nullptr);
                } catch (...) {
                    YG_LOG_ERROR(L"残留扫描发生未知异常");
                    SetStatusText(L"残留扫描发生未知异常");
                    // 延迟刷新程序列表
                    SetTimer(m_hWnd, 4, 1000, nullptr);
                }
            } else {
                YG_LOG_WARNING(L"残留扫描器未初始化");
                SetStatusText(L"卸载完成");
                // 延迟刷新程序列表
                SetTimer(m_hWnd, 4, 1000, nullptr);
            }
        } else {
            YG_LOG_WARNING(L"卸载失败，跳过残留扫描");
            SetStatusText(L"卸载失败");
            // 延迟刷新程序列表
            SetTimer(m_hWnd, 4, 1000, nullptr);
        }
    }
    
    void MainWindow::HandleResidualScanProgress(int percentage, int foundCount) {
        YG_LOG_INFO(L"处理残留扫描进度消息: " + std::to_wstring(percentage) + L"%, 找到: " + std::to_wstring(foundCount));
        
        // 在状态栏显示扫描进度
        String statusText = L"扫描残留文件... " + std::to_wstring(percentage) + L"% | " +
                          L"已找到 " + std::to_wstring(foundCount) + L" 项";
        
        SetStatusText(statusText);
        
        // 如果扫描完成，显示清理对话框
        if (percentage >= 100) {
            YG_LOG_INFO(L"残留扫描完成，找到 " + std::to_wstring(foundCount) + L" 项残留");
            
            if (foundCount > 0) {
                SetStatusText(L"发现残留文件，准备显示清理对话框...");
                // 延迟显示清理对话框，让用户看到状态更新
                SetTimer(m_hWnd, 3, 1000, nullptr); // 1秒后显示清理对话框
            } else {
                SetStatusText(L"未发现残留文件，系统已清理干净");
                MessageBox(m_hWnd, L"恭喜！未发现任何残留文件，系统已清理干净。", L"清理完成", MB_OK | MB_ICONINFORMATION);
                // 延迟刷新程序列表
                SetTimer(m_hWnd, 4, 2000, nullptr);
            }
        }
    }
    
    void MainWindow::OnResidualScanProgress(int percentage, const String& currentPath, int foundCount) {
        // 在状态栏显示扫描进度
        String statusText = L"扫描残留文件... " + std::to_wstring(percentage) + L"% | " +
                          L"已找到 " + std::to_wstring(foundCount) + L" 项 | " +
                          L"当前: " + currentPath;
        
        SetStatusText(statusText);
        
        // 如果扫描完成，显示清理对话框
        if (percentage >= 100) {
            YG_LOG_INFO(L"残留扫描完成，找到 " + std::to_wstring(foundCount) + L" 项残留");
            
            if (foundCount > 0) {
                // 获取扫描结果并显示清理对话框
                auto scanResults = m_residualScanner->GetScanResults();
                if (!scanResults.empty()) {
                    SetStatusText(L"发现残留文件，准备显示清理对话框...");
                    
                    // 延迟显示清理对话框，让用户看到状态更新
                    SetTimer(m_hWnd, 3, 1000, nullptr); // 1秒后显示清理对话框
                } else {
                    SetStatusText(L"未发现残留文件");
                }
            } else {
                SetStatusText(L"未发现残留文件，系统已清理干净");
                MessageBox(m_hWnd, L"恭喜！未发现任何残留文件，系统已清理干净。", L"清理完成", MB_OK | MB_ICONINFORMATION);
            }
        }
    }
    
    void MainWindow::ShowCleanupDialog(const ProgramInfo& program) {
        YG_LOG_INFO(L"显示清理对话框: " + program.name);
        
        if (!m_residualScanner) {
            YG_LOG_ERROR(L"残留扫描器未初始化");
            return;
        }
        
        // 创建清理对话框
        CleanupDialog cleanupDialog(m_hWnd, program, m_residualScanner);
        
        // 设置扫描结果
        auto scanResults = m_residualScanner->GetScanResults();
        cleanupDialog.SetResidualData(scanResults);
        
        // 显示对话框
        CleanupResult result = cleanupDialog.ShowDialog();
        
        // 处理结果
        switch (result) {
            case CleanupResult::Completed:
                SetStatusText(L"残留清理完成");
                MessageBox(m_hWnd, L"残留清理操作已完成！", L"清理完成", MB_OK | MB_ICONINFORMATION);
                break;
            case CleanupResult::Cancelled:
                SetStatusText(L"用户取消了残留清理");
                break;
            default:
                SetStatusText(L"清理对话框已关闭");
                break;
        }
        
        YG_LOG_INFO(L"清理对话框已关闭，结果: " + std::to_wstring(static_cast<int>(result)));
    }
    
} // namespace YG
