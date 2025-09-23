/**
 * @file MainWindowLogs.cpp
 * @brief 主窗口日志管理功能实现
 * @author gmrchzh@gmail.com
 * @version 1.0.1
 * @date 2025-09-21
 */

#include "ui/MainWindowLogs.h"
#include "ui/MainWindow.h"
#include "core/Logger.h"
#include "core/Common.h"
#include "utils/UIUtils.h"
#include "../resources/resource.h"
#include <windows.h>
#include <commctrl.h>
#include <shellapi.h>

namespace YG {

    MainWindowLogs::MainWindowLogs(MainWindow* mainWindow) 
        : m_mainWindow(mainWindow) {
    }

    void MainWindowLogs::ShowLogManagerDialog() {
        YG_LOG_INFO(L"显示日志管理对话框");
        
        HWND hWnd = m_mainWindow->GetWindowHandle();
        
        INT_PTR result = DialogBoxParamW(
            GetModuleHandleW(nullptr),
            MAKEINTRESOURCEW(IDD_LOG_MANAGER),
            hWnd,
            LogManagerDlgProc,
            (LPARAM)this
        );
        
        if (result > 0) {
            YG_LOG_INFO(L"日志管理对话框已关闭");
        } else {
            YG_LOG_ERROR(L"创建日志管理对话框失败");
        }
    }

    void MainWindowLogs::ClearLogFiles() {
        YG_LOG_INFO(L"开始清理日志文件");
        
        try {
            // 获取日志目录 - 使用更可靠的方法
            String logDir;
            String logFilePath = Logger::GetInstance().GetLogFilePath();
            
            if (!logFilePath.empty()) {
                size_t lastSlash = logFilePath.find_last_of(L"\\/");
                if (lastSlash != String::npos) {
                    logDir = logFilePath.substr(0, lastSlash);
                }
            }
            
            // 如果无法从Logger获取路径，使用默认路径
            if (logDir.empty()) {
                logDir = GetApplicationPath() + L"\\logs";
            }
            
            YG_LOG_INFO(L"清理日志文件，目录: " + logDir);
            
            // 清理日志文件
            WIN32_FIND_DATAW findData;
            String searchPattern = logDir + L"\\*.log";
            HANDLE hFind = FindFirstFileW(searchPattern.c_str(), &findData);
            
            int deletedCount = 0;
            int failedCount = 0;
            
            bool logSystemShutdown = false;
            
            if (hFind != INVALID_HANDLE_VALUE) {
                do {
                    if (!(findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
                        String logFile = logDir + L"\\" + findData.cFileName;
                        bool isCurrentLog = (String(findData.cFileName) == L"yguninstaller.log");
                        
                        // 如果当前日志文件正在使用，先关闭日志系统
                        if (isCurrentLog && !logSystemShutdown) {
                            Logger::GetInstance().Shutdown();
                            Sleep(200); // 等待文件句柄释放
                            logSystemShutdown = true;
                        }
                        
                        if (DeleteFileW(logFile.c_str())) {
                            deletedCount++;
                            YG_LOG_INFO(L"已删除日志文件: " + String(findData.cFileName));
                        } else {
                            DWORD error = GetLastError();
                            
                            // 如果是当前日志文件且仍然失败，尝试延迟删除
                            if (error == ERROR_SHARING_VIOLATION && isCurrentLog) {
                                bool deleted = false;
                                for (int i = 0; i < 5; i++) {
                                    Sleep(100);
                                    if (DeleteFileW(logFile.c_str())) {
                                        deleted = true;
                                        break;
                                    }
                                }
                                
                                if (deleted) {
                                    deletedCount++;
                                    YG_LOG_INFO(L"延迟删除日志文件成功: " + String(findData.cFileName));
                                } else {
                                    failedCount++;
                                    YG_LOG_ERROR(L"当前日志文件删除失败: " + String(findData.cFileName) + L", 错误代码: " + std::to_wstring(error));
                                }
                            } else {
                                failedCount++;
                                YG_LOG_ERROR(L"删除日志文件失败: " + String(findData.cFileName) + L", 错误代码: " + std::to_wstring(error));
                            }
                        }
                    }
                } while (FindNextFileW(hFind, &findData));
                FindClose(hFind);
                
                // 如果关闭了日志系统，重新初始化
                if (logSystemShutdown) {
                    String newLogPath = logDir + L"\\yguninstaller.log";
                    Logger::GetInstance().Initialize(newLogPath, LogLevel::Info);
                }
            } else {
                DWORD error = GetLastError();
                if (error == ERROR_PATH_NOT_FOUND) {
                    // 目录不存在，尝试创建
                    if (CreateDirectoryW(logDir.c_str(), nullptr) || GetLastError() == ERROR_ALREADY_EXISTS) {
                        YG_LOG_INFO(L"已创建日志目录: " + logDir);
                    }
                } else {
                    YG_LOG_ERROR(L"无法搜索日志文件，错误代码: " + std::to_wstring(error));
                }
            }
            
            YG_LOG_INFO(L"日志清理完成，成功删除 " + std::to_wstring(deletedCount) + L" 个文件，失败 " + std::to_wstring(failedCount) + L" 个文件");
            
        } catch (...) {
            YG_LOG_ERROR(L"清理日志文件时发生异常");
        }
    }

    bool MainWindowLogs::IsFileInUse(const String& filePath) {
        HANDLE hFile = CreateFileW(filePath.c_str(), 
                                  GENERIC_READ | GENERIC_WRITE,
                                  0,  // 不共享
                                  nullptr,
                                  OPEN_EXISTING,
                                  0,
                                  nullptr);
        
        if (hFile == INVALID_HANDLE_VALUE) {
            DWORD error = GetLastError();
            // 如果文件被占用或访问被拒绝，说明文件正在使用
            return (error == ERROR_SHARING_VIOLATION || error == ERROR_ACCESS_DENIED);
        } else {
            CloseHandle(hFile);
            return false; // 文件没有被占用
        }
    }

    void MainWindowLogs::RefreshLogList(HWND hDlg) {
        HWND hLogList = GetDlgItem(hDlg, IDC_LOG_LIST);
        if (!hLogList) {
            YG_LOG_ERROR(L"日志列表控件未找到");
            return;
        }
        
        // 清空列表
        ListView_DeleteAllItems(hLogList);
        
        // 获取日志目录 - 使用更可靠的方法
        String logDir;
        String logFilePath = Logger::GetInstance().GetLogFilePath();
        
        if (!logFilePath.empty()) {
            // 从日志文件路径提取目录
            size_t lastSlash = logFilePath.find_last_of(L"\\/");
            if (lastSlash != String::npos) {
                logDir = logFilePath.substr(0, lastSlash);
            }
        }
        
        // 如果无法从Logger获取路径，使用默认路径
        if (logDir.empty()) {
            logDir = GetApplicationPath() + L"\\logs";
        }
        
        YG_LOG_INFO(L"刷新日志列表，日志目录: " + logDir);
        
        // 搜索日志文件
        String searchPattern = logDir + L"\\*.log";
        WIN32_FIND_DATAW findData;
        HANDLE hFind = FindFirstFileW(searchPattern.c_str(), &findData);
        
        int index = 0;
        if (hFind != INVALID_HANDLE_VALUE) {
            do {
                if (!(findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
                    // 添加文件到列表
                    LVITEMW lvi = {0};
                    lvi.mask = LVIF_TEXT;
                    lvi.iItem = index;
                    lvi.iSubItem = 0;
                    lvi.pszText = findData.cFileName;
                    int result = ListView_InsertItem(hLogList, &lvi);
                    
                    if (result != -1) {
                        // 设置文件大小
                        String sizeStr = FormatFileSize(findData.nFileSizeLow);
                        ListView_SetItemText(hLogList, index, 1, const_cast<LPWSTR>(sizeStr.c_str()));
                        
                        // 设置修改时间
                        SYSTEMTIME st;
                        FileTimeToSystemTime(&findData.ftLastWriteTime, &st);
                        wchar_t timeStr[100];
                        swprintf_s(timeStr, L"%04d-%02d-%02d %02d:%02d:%02d",
                                  st.wYear, st.wMonth, st.wDay,
                                  st.wHour, st.wMinute, st.wSecond);
                        ListView_SetItemText(hLogList, index, 2, timeStr);
                        
                        index++;
                        YG_LOG_INFO(L"添加日志文件到列表: " + String(findData.cFileName));
                    }
                }
            } while (FindNextFileW(hFind, &findData));
            FindClose(hFind);
        } else {
            YG_LOG_WARNING(L"无法搜索日志文件，目录可能不存在: " + logDir);
            DWORD error = GetLastError();
            if (error == ERROR_PATH_NOT_FOUND) {
                // 目录不存在，尝试创建
                if (CreateDirectoryW(logDir.c_str(), nullptr) || GetLastError() == ERROR_ALREADY_EXISTS) {
                    YG_LOG_INFO(L"已创建日志目录: " + logDir);
                }
            }
        }
        
        // 自动调整列宽
        ListView_SetColumnWidth(hLogList, 0, LVSCW_AUTOSIZE);
        ListView_SetColumnWidth(hLogList, 1, LVSCW_AUTOSIZE);
        ListView_SetColumnWidth(hLogList, 2, LVSCW_AUTOSIZE);
        
        YG_LOG_INFO(L"日志列表刷新完成，共 " + std::to_wstring(index) + L" 个文件");
    }

    String MainWindowLogs::FormatFileSize(DWORD fileSize) {
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

    INT_PTR CALLBACK MainWindowLogs::LogManagerDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
        UNREFERENCED_PARAMETER(lParam);
        
        switch (message) {
            case WM_INITDIALOG:
                {
                    // 获取主窗口句柄并居中显示日志管理对话框
                    MainWindowLogs* pLogs = reinterpret_cast<MainWindowLogs*>(lParam);
                    if (pLogs && pLogs->m_mainWindow) {
                        HWND hParentWnd = pLogs->m_mainWindow->GetWindowHandle();
                        if (hParentWnd) {
                            // 使用UIUtils的通用居中函数
                            UIUtils::CenterWindow(hDlg, hParentWnd);
                            YG_LOG_INFO(L"日志管理对话框已居中显示");
                        }
                    }
                    
                    // 初始化日志列表
                    HWND hLogList = GetDlgItem(hDlg, IDC_LOG_LIST);
                    if (hLogList) {
                        // 设置ListView样式
                        ListView_SetExtendedListViewStyle(hLogList, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
                        
                        // 添加列
                        LVCOLUMNW lvc = {0};
                        lvc.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
                        lvc.cx = 200;
                        lvc.pszText = const_cast<LPWSTR>(L"日志文件");
                        ListView_InsertColumn(hLogList, 0, &lvc);
                        
                        lvc.cx = 100;
                        lvc.pszText = const_cast<LPWSTR>(L"大小");
                        ListView_InsertColumn(hLogList, 1, &lvc);
                        
                        lvc.cx = 150;
                        lvc.pszText = const_cast<LPWSTR>(L"修改时间");
                        ListView_InsertColumn(hLogList, 2, &lvc);
                        
                        // 刷新日志列表
                        RefreshLogList(hDlg);
                    }
                    
                    // 设置日志信息
                    SetDlgItemTextW(hDlg, IDC_STATIC_LOG_INFO, L"日志管理 - 查看和删除程序日志文件");
                }
                    return TRUE;
                
            case WM_COMMAND:
                switch (LOWORD(wParam)) {
                    case IDC_BUTTON_VIEW_LOG:
                        {
                            HWND hLogList = GetDlgItem(hDlg, IDC_LOG_LIST);
                            int selected = ListView_GetNextItem(hLogList, -1, LVNI_SELECTED);
                            if (selected != -1) {
                                // 获取选中的日志文件路径
                                wchar_t fileName[MAX_PATH] = {0};
                                LVITEMW lvi = {0};
                                lvi.mask = LVIF_TEXT;
                                lvi.iItem = selected;
                                lvi.iSubItem = 0;
                                lvi.pszText = fileName;
                                lvi.cchTextMax = MAX_PATH;
                                
                                if (ListView_GetItem(hLogList, &lvi)) {
                                    // 获取日志目录
                                    String logFilePath = Logger::GetInstance().GetLogFilePath();
                                    String logDir = logFilePath.substr(0, logFilePath.find_last_of(L"\\/"));
                                    
                                    // 构建完整路径
                                    String fullPath = logDir + L"\\" + fileName;
                                    
                                    // 使用记事本打开日志文件
                                    ShellExecuteW(hDlg, L"open", L"notepad.exe", fullPath.c_str(), nullptr, SW_SHOWNORMAL);
                                }
                            } else {
                                MessageBoxW(hDlg, L"请先选择要查看的日志文件。", L"提示", MB_OK | MB_ICONWARNING);
                            }
                        }
                        break;
                        
                    case IDC_BUTTON_DELETE_LOG:
                        {
                            HWND hLogList = GetDlgItem(hDlg, IDC_LOG_LIST);
                            int selected = ListView_GetNextItem(hLogList, -1, LVNI_SELECTED);
                            if (selected != -1) {
                                // 获取选中的日志文件名
                                wchar_t fileName[MAX_PATH] = {0};
                                LVITEMW lvi = {0};
                                lvi.mask = LVIF_TEXT;
                                lvi.iItem = selected;
                                lvi.iSubItem = 0;
                                lvi.pszText = fileName;
                                lvi.cchTextMax = MAX_PATH;
                                
                                if (ListView_GetItem(hLogList, &lvi)) {
                                    String message = L"确定要删除日志文件 \"" + String(fileName) + L"\" 吗？\n\n此操作不可撤销。";
                                    if (MessageBoxW(hDlg, message.c_str(), L"确认删除", MB_YESNO | MB_ICONWARNING) == IDYES) {
                                        // 获取日志目录 - 使用更可靠的方法
                                        String logDir;
                                        String logFilePath = Logger::GetInstance().GetLogFilePath();
                                        
                                        if (!logFilePath.empty()) {
                                            size_t lastSlash = logFilePath.find_last_of(L"\\/");
                                            if (lastSlash != String::npos) {
                                                logDir = logFilePath.substr(0, lastSlash);
                                            }
                                        }
                                        
                                        // 如果无法从Logger获取路径，使用默认路径
                                        if (logDir.empty()) {
                                            logDir = GetApplicationPath() + L"\\logs";
                                        }
                                        
                                        // 构建完整路径
                                        String fullPath = logDir + L"\\" + fileName;
                                        
                                        YG_LOG_INFO(L"尝试删除日志文件: " + fullPath);
                                        
                                        // 检查是否是当前日志文件
                                        bool isCurrentLog = (String(fileName) == L"yguninstaller.log");
                                        
                                        // 检查文件是否被占用
                                        bool fileInUse = IsFileInUse(fullPath);
                                        
                                        // 如果是当前日志文件，需要特殊处理
                                        if (isCurrentLog || fileInUse) {
                                            // 先关闭日志系统
                                            Logger::GetInstance().Shutdown();
                                            Sleep(200); // 等待文件句柄释放
                                        }
                                        
                                        // 删除文件
                                        if (DeleteFileW(fullPath.c_str())) {
                                            MessageBoxW(hDlg, L"日志文件删除成功。", L"成功", MB_OK | MB_ICONINFORMATION);
                                            RefreshLogList(hDlg);  // 刷新列表
                                            YG_LOG_INFO(L"日志文件删除成功: " + String(fileName));
                                        } else {
                                            DWORD error = GetLastError();
                                            
                                            // 如果仍然失败，尝试强制删除
                                            if (error == ERROR_SHARING_VIOLATION && isCurrentLog) {
                                                // 尝试多次删除，等待文件句柄释放
                                                bool deleted = false;
                                                for (int i = 0; i < 5; i++) {
                                                    Sleep(100);
                                                    if (DeleteFileW(fullPath.c_str())) {
                                                        deleted = true;
                                                        break;
                                                    }
                                                }
                                                
                                                if (deleted) {
                                                    MessageBoxW(hDlg, L"日志文件删除成功（延迟删除）。", L"成功", MB_OK | MB_ICONINFORMATION);
                                                    RefreshLogList(hDlg);
                                                    YG_LOG_INFO(L"日志文件延迟删除成功: " + String(fileName));
                                                } else {
                                                    String errorMsg = L"无法删除当前日志文件。\n\n"
                                                                     L"文件正在被程序使用，请稍后重试。\n\n"
                                                                     L"提示：您可以在程序退出后手动删除此文件。";
                                                    MessageBoxW(hDlg, errorMsg.c_str(), L"文件被占用", MB_OK | MB_ICONWARNING);
                                                    YG_LOG_ERROR(L"当前日志文件删除失败: " + String(fileName));
                                                }
                                            } else {
                                                String errorMsg = L"删除日志文件失败。\n\n错误代码: " + std::to_wstring(error);
                                                
                                                if (error == ERROR_SHARING_VIOLATION) {
                                                    errorMsg += L"\n\n文件可能正在被其他程序使用。";
                                                } else if (error == ERROR_ACCESS_DENIED) {
                                                    errorMsg += L"\n\n没有足够的权限删除此文件。";
                                                } else if (error == ERROR_FILE_NOT_FOUND) {
                                                    errorMsg += L"\n\n文件不存在或已被删除。";
                                                }
                                                
                                                MessageBoxW(hDlg, errorMsg.c_str(), L"错误", MB_OK | MB_ICONERROR);
                                                YG_LOG_ERROR(L"删除日志文件失败: " + String(fileName) + L", 错误代码: " + std::to_wstring(error));
                                            }
                                        }
                                        
                                        // 如果删除了当前日志文件，重新初始化日志系统
                                        if (isCurrentLog) {
                                            String newLogPath = logDir + L"\\yguninstaller.log";
                                            Logger::GetInstance().Initialize(newLogPath, LogLevel::Info);
                                        }
                                    }
                                }
                            } else {
                                MessageBoxW(hDlg, L"请先选择要删除的日志文件。", L"提示", MB_OK | MB_ICONWARNING);
                            }
                        }
                        break;
                        
                    case IDC_BUTTON_CLEAR_ALL_LOGS:
                        {
                            if (MessageBoxW(hDlg, L"确定要删除所有日志文件吗？\n\n此操作不可撤销。", 
                                           L"确认删除", MB_YESNO | MB_ICONWARNING) == IDYES) {
                                // 获取日志目录 - 使用更可靠的方法
                                String logDir;
                                String logFilePath = Logger::GetInstance().GetLogFilePath();
                                
                                if (!logFilePath.empty()) {
                                    size_t lastSlash = logFilePath.find_last_of(L"\\/");
                                    if (lastSlash != String::npos) {
                                        logDir = logFilePath.substr(0, lastSlash);
                                    }
                                }
                                
                                // 如果无法从Logger获取路径，使用默认路径
                                if (logDir.empty()) {
                                    logDir = GetApplicationPath() + L"\\logs";
                                }
                                
                                YG_LOG_INFO(L"开始清空所有日志文件，目录: " + logDir);
                                
                                // 删除所有日志文件
                                String searchPattern = logDir + L"\\*.log";
                                WIN32_FIND_DATAW findData;
                                HANDLE hFind = FindFirstFileW(searchPattern.c_str(), &findData);
                                
                                int deletedCount = 0;
                                int failedCount = 0;
                                
                                bool logSystemShutdown = false;
                                
                                if (hFind != INVALID_HANDLE_VALUE) {
                                    do {
                                        if (!(findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
                                            String fullPath = logDir + L"\\" + findData.cFileName;
                                            bool isCurrentLog = (String(findData.cFileName) == L"yguninstaller.log");
                                            
                                            // 如果当前日志文件正在使用，先关闭日志系统
                                            if (isCurrentLog && !logSystemShutdown) {
                                                Logger::GetInstance().Shutdown();
                                                Sleep(200); // 等待文件句柄释放
                                                logSystemShutdown = true;
                                            }
                                            
                                            if (DeleteFileW(fullPath.c_str())) {
                                                deletedCount++;
                                                YG_LOG_INFO(L"删除日志文件成功: " + String(findData.cFileName));
                                            } else {
                                                DWORD error = GetLastError();
                                                
                                                // 如果是当前日志文件且仍然失败，尝试延迟删除
                                                if (error == ERROR_SHARING_VIOLATION && isCurrentLog) {
                                                    bool deleted = false;
                                                    for (int i = 0; i < 5; i++) {
                                                        Sleep(100);
                                                        if (DeleteFileW(fullPath.c_str())) {
                                                            deleted = true;
                                                            break;
                                                        }
                                                    }
                                                    
                                                    if (deleted) {
                                                        deletedCount++;
                                                        YG_LOG_INFO(L"延迟删除日志文件成功: " + String(findData.cFileName));
                                                    } else {
                                                        failedCount++;
                                                        YG_LOG_ERROR(L"当前日志文件删除失败: " + String(findData.cFileName) + L", 错误代码: " + std::to_wstring(error));
                                                    }
                                                } else {
                                                    failedCount++;
                                                    YG_LOG_ERROR(L"删除日志文件失败: " + String(findData.cFileName) + L", 错误代码: " + std::to_wstring(error));
                                                }
                                            }
                                        }
                                    } while (FindNextFileW(hFind, &findData));
                                    FindClose(hFind);
                                    
                                    // 如果关闭了日志系统，重新初始化
                                    if (logSystemShutdown) {
                                        String newLogPath = logDir + L"\\yguninstaller.log";
                                        Logger::GetInstance().Initialize(newLogPath, LogLevel::Info);
                                    }
                                }
                                
                                String message;
                                if (failedCount > 0) {
                                    message = L"删除完成！\n\n成功删除: " + std::to_wstring(deletedCount) + L" 个文件\n失败: " + std::to_wstring(failedCount) + L" 个文件\n\n部分文件可能正在被使用。";
                                    MessageBoxW(hDlg, message.c_str(), L"删除完成", MB_OK | MB_ICONWARNING);
                                } else {
                                    message = L"已成功删除 " + std::to_wstring(deletedCount) + L" 个日志文件。";
                                    MessageBoxW(hDlg, message.c_str(), L"删除完成", MB_OK | MB_ICONINFORMATION);
                                }
                                
                                RefreshLogList(hDlg);  // 刷新列表
                                YG_LOG_INFO(L"清空日志文件完成，成功: " + std::to_wstring(deletedCount) + L", 失败: " + std::to_wstring(failedCount));
                            }
                        }
                        break;
                        
                    case IDC_BUTTON_REFRESH_LOGS:
                        RefreshLogList(hDlg);
                        break;
                        
                    case IDOK:
                    case IDCANCEL:
                        EndDialog(hDlg, LOWORD(wParam));
                        return TRUE;
                }
                break;
        }
        return FALSE;
    }

} // namespace YG
