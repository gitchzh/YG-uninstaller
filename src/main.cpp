/**
 * @file main.cpp
 * @brief 应用程序入口点
 * @author gmrchzh@gmail.com
 * @version 1.0.1
 * @date 2025-09-17
 */

#include "core/Common.h"
#include "core/Config.h"
#include "core/Logger.h"
#include "core/ErrorHandler.h"
#include "ui/MainWindow.h"
#include <windows.h>
#include <commctrl.h>
#include <objbase.h>
#include <memory>

// 链接必要的库（MSVC专用，GCC通过编译器参数链接）
#ifdef _MSC_VER
#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "advapi32.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "kernel32.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "oleaut32.lib")
#pragma comment(lib, "uuid.lib")
#pragma comment(lib, "gdi32.lib")
#endif

using namespace YG;

/**
 * @brief 应用程序初始化
 * @return ErrorCode 初始化结果
 */
ErrorCode InitializeApplication() {
    // 初始化COM
    HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    if (FAILED(hr)) {
        YG_LOG_ERROR(L"COM初始化失败: " + std::to_wstring(hr));
        return ErrorCode::GeneralError;
    }
    
    // 初始化公共控件 - 使用更简单的方法
    InitCommonControls();
    
    // 然后使用扩展初始化
    INITCOMMONCONTROLSEX icex;
    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC = ICC_LISTVIEW_CLASSES | ICC_TREEVIEW_CLASSES | ICC_BAR_CLASSES | 
                 ICC_TAB_CLASSES | ICC_PROGRESS_CLASS | ICC_COOL_CLASSES |
                 ICC_WIN95_CLASSES | ICC_STANDARD_CLASSES;
    
    BOOL initResult = InitCommonControlsEx(&icex);
    YG_LOG_INFO(L"公共控件初始化结果: " + std::to_wstring(initResult));
    
    if (!initResult) {
        YG_LOG_WARNING(L"扩展公共控件初始化失败，但继续运行");
        // 不要因为这个失败就退出程序
    }
    
    // 初始化日志系统
    String logPath = GetApplicationPath() + L"\\logs\\yguninstaller.log";
    ErrorCode logResult = Logger::GetInstance().Initialize(logPath, LogLevel::Info);
    if (logResult != ErrorCode::Success) {
        // 日志初始化失败不是致命错误，继续运行
        OutputDebugStringW(L"日志系统初始化失败\n");
    }
    
    YG_LOG_INFO(L"=== YG Uninstaller 启动 ===");
    YG_LOG_INFO(L"版本: " + String(YG_VERSION_STRING));
    YG_LOG_INFO(L"系统: " + GetWindowsVersion());
    YG_LOG_INFO(L"管理员权限: " + String(IsRunningAsAdmin() ? L"是" : L"否"));
    
    // 加载配置
    ErrorCode configResult = Config::GetInstance().Load();
    if (configResult != ErrorCode::Success) {
        YG_LOG_WARNING(L"配置加载失败，使用默认配置");
    }
    
    // 设置错误处理器
    ErrorHandler::GetInstance().EnableAutoLogging(true);
    ErrorHandler::GetInstance().EnableErrorDialog(true);
    
    YG_LOG_INFO(L"应用程序初始化完成");
    return ErrorCode::Success;
}

/**
 * @brief 应用程序清理
 */
void CleanupApplication() {
    YG_LOG_INFO(L"开始应用程序清理");
    
    try {
        // 保存配置
        Config::GetInstance().Save();
        YG_LOG_INFO(L"配置已保存");
    } catch (...) {
        // 忽略配置保存错误，确保清理继续进行
        OutputDebugStringW(L"配置保存失败\n");
    }
    
    // 强制等待确保所有线程和操作完成
    Sleep(300);
    
    try {
        // 强制刷新所有日志
        Logger::GetInstance().Flush();
        YG_LOG_INFO(L"日志已刷新");
        
        // 关闭日志系统
        Logger::GetInstance().Shutdown();
    } catch (...) {
        // 忽略日志系统错误，确保清理继续进行
        OutputDebugStringW(L"日志系统关闭失败\n");
    }
    
    try {
        // 清理COM
        CoUninitialize();
        OutputDebugStringW(L"COM已清理\n");
    } catch (...) {
        OutputDebugStringW(L"COM清理失败\n");
    }
    
    // 强制等待一小段时间，确保所有资源都被释放
    Sleep(200);
    
    OutputDebugStringW(L"应用程序清理完成\n");
}

/**
 * @brief 检查单实例运行
 * @return bool 是否为单实例
 */
bool CheckSingleInstance() {
    // 先尝试查找已运行的实例
    HWND hExistingWnd = FindWindowW(L"YGUninstallerMainWindow", nullptr);
    if (hExistingWnd) {
        // 找到已运行的实例，激活它
        if (IsIconic(hExistingWnd)) {
            ShowWindow(hExistingWnd, SW_RESTORE);
        }
        SetForegroundWindow(hExistingWnd);
        return false; // 不允许运行新实例
    }
    
    // 没有找到运行的实例，允许运行
    return true;
}

/**
 * @brief Windows应用程序入口点
 * @param hInstance 应用程序实例句柄
 * @param hPrevInstance 前一个实例句柄（总是NULL）
 * @param lpCmdLine 命令行参数
 * @param nCmdShow 显示方式
 * @return int 应用程序退出代码
 */
int WINAPI wWinMain(_In_ HINSTANCE hInstance,
                   _In_opt_ HINSTANCE hPrevInstance,
                   _In_ LPWSTR lpCmdLine,
                   _In_ int nCmdShow) {
    
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);
    
    int exitCode = 0;
    
    try {
        // 检查单实例运行
        if (!CheckSingleInstance()) {
            return 0;  // 已有实例在运行
        }
        
        // 初始化应用程序
        ErrorCode initResult = InitializeApplication();
        if (initResult != ErrorCode::Success) {
            MessageBoxW(nullptr, 
                       L"应用程序初始化失败！", 
                       L"YG Uninstaller", 
                       MB_OK | MB_ICONERROR);
            return static_cast<int>(initResult);
        }
        
        // 创建主窗口
        auto mainWindow = YG::MakeUnique<MainWindow>();
        ErrorCode createResult = mainWindow->Create(hInstance);
        if (createResult != ErrorCode::Success) {
            YG_LOG_FATAL(L"主窗口创建失败");
            ErrorHandler::GetInstance().ShowErrorDialog(
                L"错误", 
                L"主窗口创建失败！", 
                createResult);
            return static_cast<int>(createResult);
        }
        
        // 显示主窗口
        mainWindow->Show(nCmdShow);
        
        YG_LOG_INFO(L"主窗口已显示，开始消息循环");
        
        // 运行消息循环
        exitCode = mainWindow->RunMessageLoop();
        
        YG_LOG_INFO(L"消息循环结束，退出代码: " + std::to_wstring(exitCode));
    }
    catch (const YGException& ex) {
        YG_LOG_FATAL(L"未处理的YG异常: " + ex.GetWhatW());
        ErrorHandler::GetInstance().HandleException(ex, true);
        exitCode = static_cast<int>(ex.GetErrorCode());
    }
    catch (const std::exception& ex) {
        YG_LOG_FATAL(L"未处理的标准异常: " + StringToWString(ex.what()));
        ErrorHandler::GetInstance().HandleStdException(ex, true);
        exitCode = static_cast<int>(ErrorCode::UnknownError);
    }
    catch (...) {
        YG_LOG_FATAL(L"未处理的未知异常");
        ErrorHandler::GetInstance().HandleUnknownException(true);
        exitCode = static_cast<int>(ErrorCode::UnknownError);
    }
    
    // 清理应用程序
    CleanupApplication();
    
    // 强制终止所有可能残留的线程和资源
    OutputDebugStringW(L"开始强制进程清理\n");
    
    // 强制等待所有线程结束
    Sleep(100);
    
    // 最激进的强制退出方法
    OutputDebugStringW(L"开始最激进的强制退出流程\n");
    
    // 方法1：直接调用系统退出
    try {
        OutputDebugStringW(L"使用ExitProcess强制退出\n");
        ExitProcess(exitCode);
    } catch (...) {
        OutputDebugStringW(L"ExitProcess失败，尝试其他方法\n");
    }
    
    // 方法2：如果ExitProcess失败，使用TerminateProcess
    try {
        HANDLE hCurrentProcess = GetCurrentProcess();
        OutputDebugStringW(L"使用TerminateProcess强制结束进程\n");
        TerminateProcess(hCurrentProcess, exitCode);
    } catch (...) {
        OutputDebugStringW(L"TerminateProcess失败\n");
    }
    
    // 方法3：最后的手段 - 系统调用
    OutputDebugStringW(L"使用系统调用强制退出\n");
    _exit(exitCode);
    
    return exitCode;
}

/**
 * @brief 控制台应用程序入口点（用于调试）
 * @param argc 参数个数
 * @param argv 参数数组
 * @return int 应用程序退出代码
 */
int wmain(int argc, wchar_t* argv[]) {
    // 如果是从控制台启动的，显示版本信息
    if (argc > 1) {
        for (int i = 1; i < argc; i++) {
            String arg(argv[i]);
            if (arg == L"--version" || arg == L"-v") {
                wprintf(L"%s %s\n", YG_APP_NAME, YG_VERSION_STRING);
                wprintf(L"%s\n", YG_APP_DESCRIPTION);
                wprintf(L"Copyright (c) 2025 %s\n", YG_COMPANY_NAME);
                return 0;
            } else if (arg == L"--help" || arg == L"-h") {
                wprintf(L"用法: YGUninstaller.exe [选项]\n");
                wprintf(L"选项:\n");
                wprintf(L"  --version, -v    显示版本信息\n");
                wprintf(L"  --help, -h       显示此帮助信息\n");
                return 0;
            }
        }
    }
    
    // 否则启动GUI应用程序
    return wWinMain(GetModuleHandleW(nullptr), nullptr, GetCommandLineW(), SW_SHOW);
}
