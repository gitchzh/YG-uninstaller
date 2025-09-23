/**
 * @file MainWindowSettings.cpp
 * @brief 主窗口设置管理功能实现
 * @author gmrchzh@gmail.com
 * @version 1.0.1
 * @date 2025-09-21
 */

#include "ui/MainWindowSettings.h"
#include "ui/MainWindow.h"
#include "core/Config.h"
#include "core/Logger.h"
#include "../resources/resource.h"
#include <windows.h>
#include <commctrl.h>
#include <commdlg.h>
#include <shlobj.h>

namespace YG {

    MainWindowSettings::MainWindowSettings(MainWindow* mainWindow) 
        : m_mainWindow(mainWindow) {
        LoadSettings();
    }

    void MainWindowSettings::ShowSettingsDialog() {
        ShowTabbedSettingsDialog();
    }

    void MainWindowSettings::ShowGeneralSettingsDialog() {
        // 显示常规设置对话框
        INT_PTR result = DialogBoxParamW(
            GetModuleHandleW(nullptr),
            MAKEINTRESOURCEW(IDD_SETTINGS_GENERAL),
            m_mainWindow->GetWindowHandle(),
            GeneralSettingsDlgProc,
            (LPARAM)this
        );
        
        if (result == IDOK) {
            // 用户点击确定，应用设置
            ApplySettings();
            YG_LOG_INFO(L"常规设置已保存并应用");
        } else {
            YG_LOG_INFO(L"常规设置已取消");
        }
    }

    void MainWindowSettings::ShowAdvancedSettingsDialog() {
        // 显示高级设置对话框
        INT_PTR result = DialogBoxParamW(
            GetModuleHandleW(nullptr),
            MAKEINTRESOURCEW(IDD_SETTINGS_ADVANCED),
            m_mainWindow->GetWindowHandle(),
            AdvancedSettingsDlgProc,
            (LPARAM)this
        );
        
        if (result == IDOK) {
            // 用户点击确定，应用设置
            ApplySettings();
            YG_LOG_INFO(L"高级设置已保存并应用");
        } else {
            YG_LOG_INFO(L"高级设置已取消");
        }
    }

    void MainWindowSettings::ShowTabbedSettingsDialog() {
        YG_LOG_INFO(L"显示选项卡式设置对话框");
        
        // 创建对话框
        HWND hDlg = CreateDialogParamW(
            GetModuleHandleW(nullptr),
            MAKEINTRESOURCEW(IDD_SETTINGS_TABBED),
            m_mainWindow->GetWindowHandle(),
            TabbedSettingsDlgProc,
            (LPARAM)this
        );
        
        if (hDlg) {
            // 计算对话框在主窗口中央的位置
            RECT mainRect, dlgRect;
            GetWindowRect(m_mainWindow->GetWindowHandle(), &mainRect);
            GetWindowRect(hDlg, &dlgRect);
            
            int dlgWidth = dlgRect.right - dlgRect.left;
            int dlgHeight = dlgRect.bottom - dlgRect.top;
            int mainWidth = mainRect.right - mainRect.left;
            int mainHeight = mainRect.bottom - mainRect.top;
            
            int x = mainRect.left + (mainWidth - dlgWidth) / 2;
            int y = mainRect.top + (mainHeight - dlgHeight) / 2;
            
            // 设置对话框位置
            SetWindowPos(hDlg, HWND_TOP, x, y, 0, 0, SWP_NOSIZE | SWP_SHOWWINDOW);
            
            // 运行消息循环
            MSG msg;
            while (GetMessageW(&msg, nullptr, 0, 0)) {
                if (!IsDialogMessageW(hDlg, &msg)) {
                    TranslateMessage(&msg);
                    DispatchMessageW(&msg);
                }
            }
            
            YG_LOG_INFO(L"选项卡式设置对话框已关闭");
        } else {
            YG_LOG_ERROR(L"创建选项卡式设置对话框失败");
        }
    }

    void MainWindowSettings::LoadSettings() {
        // 从配置文件加载设置
        YG_LOG_INFO(L"从配置文件加载设置...");
        
        Config& config = Config::GetInstance();
        
        // 加载各种设置
        m_settings.autoScanOnStartup = config.GetBool(L"AutoScanOnStartup", true);
        m_settings.showHiddenPrograms = config.GetBool(L"ShowHiddenPrograms", false);
        m_settings.startMinimized = config.GetBool(L"StartMinimized", false);
        m_settings.closeToTray = config.GetBool(L"CloseToTray", false);
        m_settings.confirmUninstall = config.GetBool(L"ConfirmUninstall", true);
        m_settings.autoRefreshAfterUninstall = config.GetBool(L"AutoRefreshAfterUninstall", true);
        m_settings.keepUninstallLogs = config.GetBool(L"KeepUninstallLogs", true);
        m_settings.includeSystemComponents = config.GetBool(L"IncludeSystemComponents", false);
        
        // 加载高级设置
        m_settings.enableDeepClean = config.GetBool(L"EnableDeepClean", false);
        m_settings.cleanRegistry = config.GetBool(L"CleanRegistry", false);
        m_settings.cleanFolders = config.GetBool(L"CleanFolders", false);
        m_settings.cleanStartMenu = config.GetBool(L"CleanStartMenu", false);
        m_settings.createRestorePoint = config.GetBool(L"CreateRestorePoint", false);
        m_settings.backupRegistry = config.GetBool(L"BackupRegistry", false);
        m_settings.verifySignature = config.GetBool(L"VerifySignature", false);
        m_settings.enableMultiThread = config.GetBool(L"EnableMultiThread", true);
        m_settings.verboseLogging = config.GetBool(L"VerboseLogging", false);
        
        // 加载性能设置
        m_settings.threadCount = config.GetInt(L"ThreadCount", 4);
        m_settings.cacheSize = config.GetInt(L"CacheSize", 100);
        m_settings.logLevel = config.GetInt(L"LogLevel", 2);
        
        YG_LOG_INFO(L"设置加载完成");
    }

    void MainWindowSettings::SaveSettings() {
        // 保存设置到配置文件
        YG_LOG_INFO(L"保存设置到配置文件...");
        
        Config& config = Config::GetInstance();
        
        // 保存各种设置
        config.SetBool(L"AutoScanOnStartup", m_settings.autoScanOnStartup);
        config.SetBool(L"ShowHiddenPrograms", m_settings.showHiddenPrograms);
        config.SetBool(L"StartMinimized", m_settings.startMinimized);
        config.SetBool(L"CloseToTray", m_settings.closeToTray);
        config.SetBool(L"ConfirmUninstall", m_settings.confirmUninstall);
        config.SetBool(L"AutoRefreshAfterUninstall", m_settings.autoRefreshAfterUninstall);
        config.SetBool(L"KeepUninstallLogs", m_settings.keepUninstallLogs);
        config.SetBool(L"IncludeSystemComponents", m_settings.includeSystemComponents);
        
        // 保存高级设置
        config.SetBool(L"EnableDeepClean", m_settings.enableDeepClean);
        config.SetBool(L"CleanRegistry", m_settings.cleanRegistry);
        config.SetBool(L"CleanFolders", m_settings.cleanFolders);
        config.SetBool(L"CleanStartMenu", m_settings.cleanStartMenu);
        config.SetBool(L"CreateRestorePoint", m_settings.createRestorePoint);
        config.SetBool(L"BackupRegistry", m_settings.backupRegistry);
        config.SetBool(L"VerifySignature", m_settings.verifySignature);
        config.SetBool(L"EnableMultiThread", m_settings.enableMultiThread);
        config.SetBool(L"VerboseLogging", m_settings.verboseLogging);
        
        // 保存性能设置
        config.SetInt(L"ThreadCount", m_settings.threadCount);
        config.SetInt(L"CacheSize", m_settings.cacheSize);
        config.SetInt(L"LogLevel", m_settings.logLevel);
        
        // 保存到文件
        config.Save();
        
        YG_LOG_INFO(L"设置保存完成");
    }

    void MainWindowSettings::ApplySettings() {
        // 应用设置到主窗口
        YG_LOG_INFO(L"应用设置...");
        
        // 这里可以添加具体的设置应用逻辑
        // 例如：更新UI状态、重新初始化某些组件等
        
        YG_LOG_INFO(L"设置应用完成");
    }

    void MainWindowSettings::ExportSettings() {
        // 导出设置到配置文件
        YG_LOG_INFO(L"导出设置...");
        
        try {
            // 先保存当前设置
            SaveSettings();
            
            // 获取配置文件路径
            String configPath = Config::GetInstance().GetConfigFilePath();
            
            // 检查配置文件是否存在
            if (GetFileAttributesW(configPath.c_str()) == INVALID_FILE_ATTRIBUTES) {
                YG_LOG_ERROR(L"配置文件不存在: " + configPath);
                MessageBoxW(m_mainWindow->GetWindowHandle(), L"配置文件不存在，无法导出设置！", L"导出设置", MB_OK | MB_ICONERROR);
                return;
            }
            
            // 获取桌面路径
            String desktopPath;
            wchar_t desktopPathBuffer[MAX_PATH];
            if (SUCCEEDED(SHGetFolderPathW(nullptr, CSIDL_DESKTOP, nullptr, 0, desktopPathBuffer))) {
                desktopPath = String(desktopPathBuffer) + L"\\YGUninstaller_Settings.ini";
            } else {
                // 如果无法获取桌面路径，使用当前用户目录
                desktopPath = L"C:\\Users\\" + GetCurrentUserName() + L"\\Desktop\\YGUninstaller_Settings.ini";
            }
            
            // 复制配置文件到桌面
            if (CopyFileW(configPath.c_str(), desktopPath.c_str(), FALSE)) {
                YG_LOG_INFO(L"设置已导出到桌面: " + desktopPath);
                MessageBoxW(m_mainWindow->GetWindowHandle(), (L"设置已导出到桌面！\n\n文件位置：\n" + desktopPath).c_str(), L"导出设置", MB_OK | MB_ICONINFORMATION);
            } else {
                DWORD error = GetLastError();
                String errorMsg = L"导出设置失败！\n\n错误代码: " + std::to_wstring(error);
                if (error == ERROR_ACCESS_DENIED) {
                    errorMsg += L"\n\n没有足够的权限写入桌面目录。";
                } else if (error == ERROR_DISK_FULL) {
                    errorMsg += L"\n\n磁盘空间不足。";
                }
                YG_LOG_ERROR(L"导出设置失败: " + errorMsg);
                MessageBoxW(m_mainWindow->GetWindowHandle(), errorMsg.c_str(), L"导出设置", MB_OK | MB_ICONERROR);
            }
        }
        catch (const std::exception& ex) {
            String errorMsg = L"导出设置时发生异常: " + StringToWString(ex.what());
            YG_LOG_ERROR(errorMsg);
            MessageBoxW(m_mainWindow->GetWindowHandle(), errorMsg.c_str(), L"导出设置", MB_OK | MB_ICONERROR);
        }
    }

    bool MainWindowSettings::ImportSettings() {
        // 从配置文件导入设置
        YG_LOG_INFO(L"导入设置...");
        
        try {
            // 打开文件选择对话框
            OPENFILENAMEW ofn = {0};
            wchar_t szFile[MAX_PATH] = {0};
            
            ofn.lStructSize = sizeof(ofn);
            ofn.hwndOwner = m_mainWindow->GetWindowHandle();  // 设置父窗口
            ofn.lpstrFile = szFile;
            ofn.nMaxFile = sizeof(szFile) / sizeof(szFile[0]);
            ofn.lpstrFilter = L"配置文件\0*.ini\0所有文件\0*.*\0";
            ofn.nFilterIndex = 1;
            ofn.lpstrTitle = L"选择要导入的配置文件";
            ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
            
            if (GetOpenFileNameW(&ofn)) {
                String sourceFile = szFile;
                
                // 检查源文件是否存在
                if (GetFileAttributesW(sourceFile.c_str()) == INVALID_FILE_ATTRIBUTES) {
                    YG_LOG_ERROR(L"选择的文件不存在: " + sourceFile);
                    MessageBoxW(m_mainWindow->GetWindowHandle(), L"选择的文件不存在！", L"导入设置", MB_OK | MB_ICONERROR);
                    return false;
                }
                
                // 获取配置文件路径
                String configPath = Config::GetInstance().GetConfigFilePath();
                
                // 备份当前配置文件
                String backupPath = configPath + L".backup";
                if (GetFileAttributesW(configPath.c_str()) != INVALID_FILE_ATTRIBUTES) {
                    CopyFileW(configPath.c_str(), backupPath.c_str(), FALSE);
                }
                
                // 复制选中的文件到配置文件位置
                if (CopyFileW(sourceFile.c_str(), configPath.c_str(), FALSE)) {
                    // 重新加载配置
                    ErrorCode loadResult = Config::GetInstance().Load();
                    if (loadResult == ErrorCode::Success) {
                        LoadSettings(); // 重新加载设置到成员变量
                        YG_LOG_INFO(L"设置导入成功: " + sourceFile);
                        
                        // 删除备份文件
                        DeleteFileW(backupPath.c_str());
                        return true;
                    } else {
                        // 配置加载失败，恢复备份
                        YG_LOG_ERROR(L"配置加载失败，恢复备份");
                        if (GetFileAttributesW(backupPath.c_str()) != INVALID_FILE_ATTRIBUTES) {
                            CopyFileW(backupPath.c_str(), configPath.c_str(), FALSE);
                        }
                        MessageBoxW(m_mainWindow->GetWindowHandle(), L"配置文件格式错误，导入失败！", L"导入设置", MB_OK | MB_ICONERROR);
                        return false;
                    }
                } else {
                    DWORD error = GetLastError();
                    String errorMsg = L"复制配置文件失败！\n\n错误代码: " + std::to_wstring(error);
                    if (error == ERROR_ACCESS_DENIED) {
                        errorMsg += L"\n\n没有足够的权限写入配置文件。";
                    }
                    YG_LOG_ERROR(L"复制配置文件失败: " + errorMsg);
                    MessageBoxW(m_mainWindow->GetWindowHandle(), errorMsg.c_str(), L"导入设置", MB_OK | MB_ICONERROR);
                    return false;
                }
            }
            
            return false;
        }
        catch (const std::exception& ex) {
            String errorMsg = L"导入设置时发生异常: " + StringToWString(ex.what());
            YG_LOG_ERROR(errorMsg);
            MessageBoxW(m_mainWindow->GetWindowHandle(), errorMsg.c_str(), L"导入设置", MB_OK | MB_ICONERROR);
            return false;
        }
    }

    void MainWindowSettings::ResetToDefaultSettings() {
        // 重置所有设置为默认值
        YG_LOG_INFO(L"重置设置为默认值...");
        
        // 重置为默认值
        m_settings.autoScanOnStartup = true;
        m_settings.showHiddenPrograms = false;
        m_settings.startMinimized = false;
        m_settings.closeToTray = false;
        m_settings.confirmUninstall = true;
        m_settings.autoRefreshAfterUninstall = true;
        m_settings.keepUninstallLogs = true;
        m_settings.includeSystemComponents = false;
        m_settings.enableMultiThread = true;
        m_settings.verboseLogging = false;
        m_settings.threadCount = 4;
        m_settings.cacheSize = 100;
        m_settings.logLevel = 2; // 信息级别
        
        YG_LOG_INFO(L"设置已重置为默认值");
    }

    String MainWindowSettings::GetCurrentUserName() const {
        // 获取当前用户名
        wchar_t userName[256];
        DWORD size = sizeof(userName) / sizeof(userName[0]);
        
        if (GetUserNameW(userName, &size)) {
            return String(userName);
        }
        
        return L"User";
    }

    // 对话框过程函数实现
    INT_PTR CALLBACK MainWindowSettings::GeneralSettingsDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
        static MainWindowSettings* pThis = nullptr;
        
        switch (message) {
            case WM_INITDIALOG:
                {
                    pThis = (MainWindowSettings*)lParam;
                    if (!pThis) return FALSE;
                    
                    // 初始化常规设置控件状态
                    CheckDlgButton(hDlg, IDC_CHECK_AUTO_SCAN, pThis->m_settings.autoScanOnStartup ? BST_CHECKED : BST_UNCHECKED);
                    CheckDlgButton(hDlg, IDC_CHECK_SYSTEM_COMPONENTS, pThis->m_settings.includeSystemComponents ? BST_CHECKED : BST_UNCHECKED);
                    CheckDlgButton(hDlg, IDC_CHECK_WINDOWS_UPDATES, false); // 默认不显示
                    CheckDlgButton(hDlg, IDC_CHECK_HIDDEN_PROGRAMS, pThis->m_settings.showHiddenPrograms ? BST_CHECKED : BST_UNCHECKED);
                    CheckDlgButton(hDlg, IDC_CHECK_START_MINIMIZED, pThis->m_settings.startMinimized ? BST_CHECKED : BST_UNCHECKED);
                    CheckDlgButton(hDlg, IDC_CHECK_CLOSE_TO_TRAY, pThis->m_settings.closeToTray ? BST_CHECKED : BST_UNCHECKED);
                    CheckDlgButton(hDlg, IDC_CHECK_CONFIRM_UNINSTALL, pThis->m_settings.confirmUninstall ? BST_CHECKED : BST_UNCHECKED);
                    CheckDlgButton(hDlg, IDC_CHECK_AUTO_REFRESH, pThis->m_settings.autoRefreshAfterUninstall ? BST_CHECKED : BST_UNCHECKED);
                    CheckDlgButton(hDlg, IDC_CHECK_KEEP_LOGS, pThis->m_settings.keepUninstallLogs ? BST_CHECKED : BST_UNCHECKED);
                    
                    return TRUE;
                }
                
            case WM_COMMAND:
                {
                    if (LOWORD(wParam) == IDOK) {
                        if (!pThis) return FALSE;
                        
                        // 保存常规设置
                        pThis->m_settings.autoScanOnStartup = IsDlgButtonChecked(hDlg, IDC_CHECK_AUTO_SCAN) == BST_CHECKED;
                        pThis->m_settings.includeSystemComponents = IsDlgButtonChecked(hDlg, IDC_CHECK_SYSTEM_COMPONENTS) == BST_CHECKED;
                        pThis->m_settings.showHiddenPrograms = IsDlgButtonChecked(hDlg, IDC_CHECK_HIDDEN_PROGRAMS) == BST_CHECKED;
                        pThis->m_settings.startMinimized = IsDlgButtonChecked(hDlg, IDC_CHECK_START_MINIMIZED) == BST_CHECKED;
                        pThis->m_settings.closeToTray = IsDlgButtonChecked(hDlg, IDC_CHECK_CLOSE_TO_TRAY) == BST_CHECKED;
                        pThis->m_settings.confirmUninstall = IsDlgButtonChecked(hDlg, IDC_CHECK_CONFIRM_UNINSTALL) == BST_CHECKED;
                        pThis->m_settings.autoRefreshAfterUninstall = IsDlgButtonChecked(hDlg, IDC_CHECK_AUTO_REFRESH) == BST_CHECKED;
                        pThis->m_settings.keepUninstallLogs = IsDlgButtonChecked(hDlg, IDC_CHECK_KEEP_LOGS) == BST_CHECKED;
                        
                        // 保存设置到配置文件
                        pThis->SaveSettings();
                        
                        EndDialog(hDlg, IDOK);
                        return TRUE;
                    } else if (LOWORD(wParam) == IDCANCEL) {
                        EndDialog(hDlg, IDCANCEL);
                        return TRUE;
                    }
                }
                break;
        }
        
        return FALSE;
    }

    INT_PTR CALLBACK MainWindowSettings::AdvancedSettingsDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
        static MainWindowSettings* pThis = nullptr;
        
        switch (message) {
            case WM_INITDIALOG:
                {
                    pThis = (MainWindowSettings*)lParam;
                    if (!pThis) return FALSE;
                    
                    // 初始化高级设置控件状态
                    CheckDlgButton(hDlg, IDC_CHECK_DEEP_CLEAN, pThis->m_settings.enableDeepClean ? BST_CHECKED : BST_UNCHECKED);
                    CheckDlgButton(hDlg, IDC_CHECK_CLEAN_REGISTRY, pThis->m_settings.cleanRegistry ? BST_CHECKED : BST_UNCHECKED);
                    CheckDlgButton(hDlg, IDC_CHECK_CLEAN_FOLDERS, pThis->m_settings.cleanFolders ? BST_CHECKED : BST_UNCHECKED);
                    CheckDlgButton(hDlg, IDC_CHECK_CLEAN_STARTMENU, pThis->m_settings.cleanStartMenu ? BST_CHECKED : BST_UNCHECKED);
                    CheckDlgButton(hDlg, IDC_CHECK_CREATE_RESTORE, pThis->m_settings.createRestorePoint ? BST_CHECKED : BST_UNCHECKED);
                    CheckDlgButton(hDlg, IDC_CHECK_BACKUP_REGISTRY, pThis->m_settings.backupRegistry ? BST_CHECKED : BST_UNCHECKED);
                    CheckDlgButton(hDlg, IDC_CHECK_VERIFY_SIGNATURE, pThis->m_settings.verifySignature ? BST_CHECKED : BST_UNCHECKED);
                    CheckDlgButton(hDlg, IDC_CHECK_MULTI_THREAD, pThis->m_settings.enableMultiThread ? BST_CHECKED : BST_UNCHECKED);
                    CheckDlgButton(hDlg, IDC_CHECK_VERBOSE_LOG, pThis->m_settings.verboseLogging ? BST_CHECKED : BST_UNCHECKED);
                    
                    // 初始化线程数下拉框
                    HWND hThreadCombo = GetDlgItem(hDlg, IDC_COMBO_THREAD_COUNT);
                    SendMessageW(hThreadCombo, CB_ADDSTRING, 0, (LPARAM)L"1");
                    SendMessageW(hThreadCombo, CB_ADDSTRING, 0, (LPARAM)L"2");
                    SendMessageW(hThreadCombo, CB_ADDSTRING, 0, (LPARAM)L"4");
                    SendMessageW(hThreadCombo, CB_ADDSTRING, 0, (LPARAM)L"8");
                    SendMessageW(hThreadCombo, CB_SETCURSEL, 2, 0); // 默认4线程
                    
                    // 初始化日志级别下拉框
                    HWND hLogCombo = GetDlgItem(hDlg, IDC_COMBO_LOG_LEVEL);
                    SendMessageW(hLogCombo, CB_ADDSTRING, 0, (LPARAM)L"错误");
                    SendMessageW(hLogCombo, CB_ADDSTRING, 0, (LPARAM)L"警告");
                    SendMessageW(hLogCombo, CB_ADDSTRING, 0, (LPARAM)L"信息");
                    SendMessageW(hLogCombo, CB_ADDSTRING, 0, (LPARAM)L"调试");
                    SendMessageW(hLogCombo, CB_SETCURSEL, pThis->m_settings.logLevel, 0);
                    
                    // 设置缓存大小
                    SetDlgItemInt(hDlg, IDC_EDIT_CACHE_SIZE, pThis->m_settings.cacheSize, FALSE);
                    
                    return TRUE;
                }
                
            case WM_COMMAND:
                {
                    if (LOWORD(wParam) == IDOK) {
                        if (!pThis) return FALSE;
                        
                        // 保存高级设置
                        pThis->m_settings.enableDeepClean = IsDlgButtonChecked(hDlg, IDC_CHECK_DEEP_CLEAN) == BST_CHECKED;
                        pThis->m_settings.cleanRegistry = IsDlgButtonChecked(hDlg, IDC_CHECK_CLEAN_REGISTRY) == BST_CHECKED;
                        pThis->m_settings.cleanFolders = IsDlgButtonChecked(hDlg, IDC_CHECK_CLEAN_FOLDERS) == BST_CHECKED;
                        pThis->m_settings.cleanStartMenu = IsDlgButtonChecked(hDlg, IDC_CHECK_CLEAN_STARTMENU) == BST_CHECKED;
                        pThis->m_settings.createRestorePoint = IsDlgButtonChecked(hDlg, IDC_CHECK_CREATE_RESTORE) == BST_CHECKED;
                        pThis->m_settings.backupRegistry = IsDlgButtonChecked(hDlg, IDC_CHECK_BACKUP_REGISTRY) == BST_CHECKED;
                        pThis->m_settings.verifySignature = IsDlgButtonChecked(hDlg, IDC_CHECK_VERIFY_SIGNATURE) == BST_CHECKED;
                        pThis->m_settings.enableMultiThread = IsDlgButtonChecked(hDlg, IDC_CHECK_MULTI_THREAD) == BST_CHECKED;
                        pThis->m_settings.verboseLogging = IsDlgButtonChecked(hDlg, IDC_CHECK_VERBOSE_LOG) == BST_CHECKED;
                        
                        // 获取线程数和缓存大小
                        HWND hThreadCombo = GetDlgItem(hDlg, IDC_COMBO_THREAD_COUNT);
                        int threadSel = (int)SendMessageW(hThreadCombo, CB_GETCURSEL, 0, 0);
                        int threadValues[] = {1, 2, 4, 8};
                        if (threadSel >= 0 && threadSel < 4) {
                            pThis->m_settings.threadCount = threadValues[threadSel];
                        }
                        
                        HWND hLogCombo = GetDlgItem(hDlg, IDC_COMBO_LOG_LEVEL);
                        pThis->m_settings.logLevel = (int)SendMessageW(hLogCombo, CB_GETCURSEL, 0, 0);
                        
                        pThis->m_settings.cacheSize = GetDlgItemInt(hDlg, IDC_EDIT_CACHE_SIZE, NULL, FALSE);
                        
                        // 保存设置到配置文件
                        pThis->SaveSettings();
                        
                        EndDialog(hDlg, IDOK);
                        return TRUE;
                    } else if (LOWORD(wParam) == IDCANCEL) {
                        EndDialog(hDlg, IDCANCEL);
                        return TRUE;
                    }
                }
                break;
        }
        
        return FALSE;
    }

    INT_PTR CALLBACK MainWindowSettings::TabbedSettingsDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
        static MainWindowSettings* pThis = nullptr;
        
        switch (message) {
            case WM_INITDIALOG:
                {
                    pThis = (MainWindowSettings*)lParam;
                    if (!pThis) return FALSE;
                    
                    // 初始化设置控件的值
                    pThis->LoadSettingsFromDialog(hDlg);
                    
                    // 初始化组合框
                    pThis->InitializeSettingsControls(hDlg);
                    
                    return TRUE;
                }
                
            case WM_COMMAND:
                {
                    switch (LOWORD(wParam)) {
                        case IDOK:
                            // 保存所有设置
                            pThis->SaveSettingsToDialog(hDlg);
                            // 应用设置
                            pThis->ApplySettings();
                            YG_LOG_INFO(L"设置已保存并应用");
                            DestroyWindow(hDlg);
                            PostQuitMessage(0);
                            return TRUE;
                            
                        case IDCANCEL:
                            DestroyWindow(hDlg);
                            PostQuitMessage(0);
                            return TRUE;
                            
                        case IDC_BUTTON_RESET_DEFAULT:
                            // 重置为默认设置
                            if (MessageBoxW(hDlg, L"确定要重置所有设置为默认值吗？", L"重置设置", 
                                           MB_YESNO | MB_ICONQUESTION) == IDYES) {
                                pThis->ResetToDefaultSettings();
                                pThis->LoadSettingsFromDialog(hDlg);
                                pThis->InitializeSettingsControls(hDlg);
                            }
                            return TRUE;
                            
                        case IDC_BUTTON_EXPORT_SETTINGS:
                            // 导出设置
                            {
                                // 禁用按钮防止重复点击
                                HWND hButton = GetDlgItem(hDlg, IDC_BUTTON_EXPORT_SETTINGS);
                                EnableWindow(hButton, FALSE);
                                
                                try {
                                    pThis->ExportSettings();
                                    // 注意：ExportSettings函数内部已经显示成功消息，这里不需要再显示
                                }
                                catch (const std::exception& ex) {
                                    String errorMsg = L"导出设置时发生异常: " + StringToWString(ex.what());
                                    MessageBoxW(hDlg, errorMsg.c_str(), L"导出设置", MB_OK | MB_ICONERROR);
                                }
                                
                                // 重新启用按钮
                                EnableWindow(hButton, TRUE);
                            }
                            return TRUE;
                            
                        case IDC_BUTTON_IMPORT_SETTINGS:
                            // 导入设置
                            {
                                // 禁用按钮防止重复点击
                                HWND hButton = GetDlgItem(hDlg, IDC_BUTTON_IMPORT_SETTINGS);
                                EnableWindow(hButton, FALSE);
                                
                                try {
                                    if (pThis->ImportSettings()) {
                                        pThis->LoadSettingsFromDialog(hDlg);
                                        MessageBoxW(hDlg, L"设置已从配置文件导入！", L"导入设置", MB_OK | MB_ICONINFORMATION);
                                    } else {
                                        MessageBoxW(hDlg, L"导入设置失败！", L"导入设置", MB_OK | MB_ICONERROR);
                                    }
                                }
                                catch (const std::exception& ex) {
                                    String errorMsg = L"导入设置时发生异常: " + StringToWString(ex.what());
                                    MessageBoxW(hDlg, errorMsg.c_str(), L"导入设置", MB_OK | MB_ICONERROR);
                                }
                                
                                // 重新启用按钮
                                EnableWindow(hButton, TRUE);
                            }
                            return TRUE;
                    }
                    break;
                }
                
        }
        
        return FALSE;
    }

    // 私有方法实现
    void MainWindowSettings::LoadSettingsFromDialog(HWND hDlg) {
        // 从成员变量加载设置到对话框控件
        YG_LOG_INFO(L"加载设置到对话框控件...");
        
        // 设置复选框状态
        CheckDlgButton(hDlg, IDC_CHECK_AUTO_SCAN, m_settings.autoScanOnStartup ? BST_CHECKED : BST_UNCHECKED);
        CheckDlgButton(hDlg, IDC_CHECK_HIDDEN_PROGRAMS, m_settings.showHiddenPrograms ? BST_CHECKED : BST_UNCHECKED);
        CheckDlgButton(hDlg, IDC_CHECK_START_MINIMIZED, m_settings.startMinimized ? BST_CHECKED : BST_UNCHECKED);
        CheckDlgButton(hDlg, IDC_CHECK_CLOSE_TO_TRAY, m_settings.closeToTray ? BST_CHECKED : BST_UNCHECKED);
        CheckDlgButton(hDlg, IDC_CHECK_CONFIRM_UNINSTALL, m_settings.confirmUninstall ? BST_CHECKED : BST_UNCHECKED);
        CheckDlgButton(hDlg, IDC_CHECK_AUTO_REFRESH, m_settings.autoRefreshAfterUninstall ? BST_CHECKED : BST_UNCHECKED);
        CheckDlgButton(hDlg, IDC_CHECK_KEEP_LOGS, m_settings.keepUninstallLogs ? BST_CHECKED : BST_UNCHECKED);
        CheckDlgButton(hDlg, IDC_CHECK_SYSTEM_COMPONENTS, m_settings.includeSystemComponents ? BST_CHECKED : BST_UNCHECKED);
        CheckDlgButton(hDlg, IDC_CHECK_MULTI_THREAD, m_settings.enableMultiThread ? BST_CHECKED : BST_UNCHECKED);
        CheckDlgButton(hDlg, IDC_CHECK_VERBOSE_LOG, m_settings.verboseLogging ? BST_CHECKED : BST_UNCHECKED);
        
        YG_LOG_INFO(L"设置已加载到对话框控件");
    }

    void MainWindowSettings::SaveSettingsToDialog(HWND hDlg) {
        // 从对话框控件保存设置到成员变量
        YG_LOG_INFO(L"从对话框控件保存设置...");
        
        // 保存复选框状态
        m_settings.autoScanOnStartup = (IsDlgButtonChecked(hDlg, IDC_CHECK_AUTO_SCAN) == BST_CHECKED);
        m_settings.showHiddenPrograms = (IsDlgButtonChecked(hDlg, IDC_CHECK_HIDDEN_PROGRAMS) == BST_CHECKED);
        m_settings.startMinimized = (IsDlgButtonChecked(hDlg, IDC_CHECK_START_MINIMIZED) == BST_CHECKED);
        m_settings.closeToTray = (IsDlgButtonChecked(hDlg, IDC_CHECK_CLOSE_TO_TRAY) == BST_CHECKED);
        m_settings.confirmUninstall = (IsDlgButtonChecked(hDlg, IDC_CHECK_CONFIRM_UNINSTALL) == BST_CHECKED);
        m_settings.autoRefreshAfterUninstall = (IsDlgButtonChecked(hDlg, IDC_CHECK_AUTO_REFRESH) == BST_CHECKED);
        m_settings.keepUninstallLogs = (IsDlgButtonChecked(hDlg, IDC_CHECK_KEEP_LOGS) == BST_CHECKED);
        m_settings.includeSystemComponents = (IsDlgButtonChecked(hDlg, IDC_CHECK_SYSTEM_COMPONENTS) == BST_CHECKED);
        m_settings.enableMultiThread = (IsDlgButtonChecked(hDlg, IDC_CHECK_MULTI_THREAD) == BST_CHECKED);
        m_settings.verboseLogging = (IsDlgButtonChecked(hDlg, IDC_CHECK_VERBOSE_LOG) == BST_CHECKED);
        
        // 保存组合框状态
        m_settings.threadCount = (int)SendDlgItemMessage(hDlg, IDC_COMBO_THREAD_COUNT, CB_GETCURSEL, 0, 0) + 1;
        m_settings.logLevel = (int)SendDlgItemMessage(hDlg, IDC_COMBO_LOG_LEVEL, CB_GETCURSEL, 0, 0);
        
        YG_LOG_INFO(L"设置已从对话框控件保存");
    }

    void MainWindowSettings::InitializeSettingsControls(HWND hDlg) {
        // 初始化组合框控件
        YG_LOG_INFO(L"初始化设置控件...");
        
        // 初始化线程数下拉框
        HWND hThreadCombo = GetDlgItem(hDlg, IDC_COMBO_THREAD_COUNT);
        if (hThreadCombo) {
            SendMessageW(hThreadCombo, CB_ADDSTRING, 0, (LPARAM)L"1");
            SendMessageW(hThreadCombo, CB_ADDSTRING, 0, (LPARAM)L"2");
            SendMessageW(hThreadCombo, CB_ADDSTRING, 0, (LPARAM)L"4");
            SendMessageW(hThreadCombo, CB_ADDSTRING, 0, (LPARAM)L"8");
            SendMessageW(hThreadCombo, CB_SETCURSEL, m_settings.threadCount - 1, 0);
        }
        
        // 初始化日志级别下拉框
        HWND hLogCombo = GetDlgItem(hDlg, IDC_COMBO_LOG_LEVEL);
        if (hLogCombo) {
            SendMessageW(hLogCombo, CB_ADDSTRING, 0, (LPARAM)L"错误");
            SendMessageW(hLogCombo, CB_ADDSTRING, 0, (LPARAM)L"警告");
            SendMessageW(hLogCombo, CB_ADDSTRING, 0, (LPARAM)L"信息");
            SendMessageW(hLogCombo, CB_ADDSTRING, 0, (LPARAM)L"调试");
            SendMessageW(hLogCombo, CB_SETCURSEL, m_settings.logLevel, 0);
        }
        
        YG_LOG_INFO(L"设置控件初始化完成");
    }

    // 其他私有方法的简化实现
    void MainWindowSettings::ShowSettingsTabContent(HWND hDlg, int tabIndex) {
        // 简化实现，根据选项卡索引显示对应的控件
        // 这里可以根据需要实现具体的选项卡内容显示逻辑
    }

    void MainWindowSettings::SaveTabbedSettings(HWND hDlg) {
        SaveSettingsToDialog(hDlg);
        SaveSettings();
    }

    void MainWindowSettings::LoadTabbedSettings(HWND hDlg) {
        LoadSettingsFromDialog(hDlg);
        InitializeSettingsControls(hDlg);
    }

    void MainWindowSettings::ResizeTabbedSettingsDialog(HWND hDlg, int width, int height) {
        // 简化实现，调整对话框大小
        SetWindowPos(hDlg, nullptr, 0, 0, width, height, SWP_NOMOVE | SWP_NOZORDER);
    }

} // namespace YG
