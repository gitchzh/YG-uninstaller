/**
 * @file UIUtils.cpp
 * @brief UI工具类实现
 * @author YG Software
 * @version 1.0.1
 * @date 2025-01-21
 */

#include "utils/UIUtils.h"
#include "core/Logger.h"

// 移除shellscalingapi.h依赖以减小体积

namespace YG {
    
    void UIUtils::CenterWindow(HWND hWnd, HWND hParent) {
        if (!hWnd || !IsWindow(hWnd)) {
            return;
        }
        
        RECT rcWindow, rcParent;
        ::GetWindowRect(hWnd, &rcWindow);
        
        int windowWidth = rcWindow.right - rcWindow.left;
        int windowHeight = rcWindow.bottom - rcWindow.top;
        
        if (hParent && IsWindow(hParent)) {
            ::GetWindowRect(hParent, &rcParent);
        } else {
            // 使用屏幕尺寸
            rcParent.left = 0;
            rcParent.top = 0;
            rcParent.right = GetSystemMetrics(SM_CXSCREEN);
            rcParent.bottom = GetSystemMetrics(SM_CYSCREEN);
        }
        
        int parentWidth = rcParent.right - rcParent.left;
        int parentHeight = rcParent.bottom - rcParent.top;
        
        int x = rcParent.left + (parentWidth - windowWidth) / 2;
        int y = rcParent.top + (parentHeight - windowHeight) / 2;
        
        SetWindowPos(hWnd, nullptr, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
    }
    
    bool UIUtils::SetWindowIcon(HWND hWnd, int iconId, HINSTANCE hInstance) {
        if (!hWnd || !IsWindow(hWnd)) {
            return false;
        }
        
        HICON hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(iconId));
        if (!hIcon) {
            return false;
        }
        
        SendMessage(hWnd, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
        SendMessage(hWnd, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
        
        return true;
    }
    
    HWND UIUtils::CreateToolTip(HWND hParent, HWND hControl, const String& text) {
        if (!hParent || !hControl || text.empty()) {
            return nullptr;
        }
        
        HWND hTooltip = CreateWindowEx(
            WS_EX_TOPMOST,
            TOOLTIPS_CLASS,
            nullptr,
            WS_POPUP | TTS_NOPREFIX | TTS_ALWAYSTIP,
            CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
            hParent,
            nullptr,
            GetModuleHandle(nullptr),
            nullptr
        );
        
        if (!hTooltip) {
            return nullptr;
        }
        
        TOOLINFO ti = {};
        ti.cbSize = sizeof(TOOLINFO);
        ti.uFlags = TTF_IDISHWND | TTF_SUBCLASS;
        ti.hwnd = hParent;
        ti.uId = (UINT_PTR)hControl;
        ti.lpszText = const_cast<wchar_t*>(text.c_str());
        
        SendMessage(hTooltip, TTM_ADDTOOL, 0, (LPARAM)&ti);
        
        return hTooltip;
    }
    
    bool UIUtils::SetListViewColumn(HWND hListView, int columnIndex, const String& text, 
                                  int width, int format) {
        if (!hListView || !IsWindow(hListView)) {
            return false;
        }
        
        LVCOLUMN lvc = {};
        lvc.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_FMT;
        lvc.fmt = format;
        lvc.cx = width;
        lvc.pszText = const_cast<wchar_t*>(text.c_str());
        
        return ListView_InsertColumn(hListView, columnIndex, &lvc) != -1;
    }
    
    bool UIUtils::SetListViewItem(HWND hListView, int itemIndex, int subItemIndex, 
                                const String& text, int imageIndex) {
        if (!hListView || !IsWindow(hListView)) {
            return false;
        }
        
        LVITEM lvi = {};
        lvi.mask = LVIF_TEXT;
        lvi.iItem = itemIndex;
        lvi.iSubItem = subItemIndex;
        lvi.pszText = const_cast<wchar_t*>(text.c_str());
        
        if (imageIndex >= 0 && subItemIndex == 0) {
            lvi.mask |= LVIF_IMAGE;
            lvi.iImage = imageIndex;
        }
        
        if (subItemIndex == 0) {
            return ListView_InsertItem(hListView, &lvi) != -1;
        } else {
            return ListView_SetItem(hListView, &lvi) != FALSE;
        }
    }
    
    std::vector<int> UIUtils::GetListViewSelectedItems(HWND hListView) {
        std::vector<int> selectedItems;
        
        if (!hListView || !IsWindow(hListView)) {
            return selectedItems;
        }
        
        int itemCount = ListView_GetItemCount(hListView);
        for (int i = 0; i < itemCount; ++i) {
            if (ListView_GetItemState(hListView, i, LVIS_SELECTED) & LVIS_SELECTED) {
                selectedItems.push_back(i);
            }
        }
        
        return selectedItems;
    }
    
    bool UIUtils::SetStatusBarText(HWND hStatusBar, int partIndex, const String& text) {
        if (!hStatusBar || !IsWindow(hStatusBar)) {
            return false;
        }
        
        return SendMessage(hStatusBar, SB_SETTEXT, partIndex, (LPARAM)text.c_str()) != FALSE;
    }
    
    bool UIUtils::SetProgressBarValue(HWND hProgressBar, int value, int min, int max) {
        if (!hProgressBar || !IsWindow(hProgressBar)) {
            return false;
        }
        
        SendMessage(hProgressBar, PBM_SETRANGE, 0, MAKELPARAM(min, max));
        return SendMessage(hProgressBar, PBM_SETPOS, value, 0) != FALSE;
    }
    
    int UIUtils::ShowMessageBox(HWND hParent, const String& title, const String& message, UINT type) {
        return MessageBox(hParent, message.c_str(), title.c_str(), type);
    }
    
    bool UIUtils::ShowConfirmDialog(HWND hParent, const String& title, const String& message) {
        int result = ShowMessageBox(hParent, title, message, MB_YESNO | MB_ICONQUESTION);
        return result == IDYES;
    }
    
    void UIUtils::ShowErrorDialog(HWND hParent, const String& title, const String& message) {
        ShowMessageBox(hParent, title, message, MB_OK | MB_ICONERROR);
    }
    
    void UIUtils::ShowInfoDialog(HWND hParent, const String& title, const String& message) {
        ShowMessageBox(hParent, title, message, MB_OK | MB_ICONINFORMATION);
    }
    
    String UIUtils::GetControlText(HWND hControl) {
        if (!hControl || !IsWindow(hControl)) {
            return L"";
        }
        
        int length = GetWindowTextLength(hControl);
        if (length <= 0) {
            return L"";
        }
        
        std::vector<wchar_t> buffer(length + 1);
        GetWindowText(hControl, buffer.data(), length + 1);
        
        return String(buffer.data());
    }
    
    bool UIUtils::SetControlText(HWND hControl, const String& text) {
        if (!hControl || !IsWindow(hControl)) {
            return false;
        }
        
        return SetWindowText(hControl, text.c_str()) != FALSE;
    }
    
    bool UIUtils::EnableControl(HWND hControl, bool enable) {
        if (!hControl || !IsWindow(hControl)) {
            return false;
        }
        
        return EnableWindow(hControl, enable ? TRUE : FALSE) != FALSE;
    }
    
    bool UIUtils::ShowControl(HWND hControl, bool show) {
        if (!hControl || !IsWindow(hControl)) {
            return false;
        }
        
        return ShowWindow(hControl, show ? SW_SHOW : SW_HIDE) != FALSE;
    }
    
    RECT UIUtils::GetWindowRect(HWND hWnd, bool isClientArea) {
        RECT rect = {};
        
        if (hWnd && IsWindow(hWnd)) {
            if (isClientArea) {
                ::GetClientRect(hWnd, &rect);
            } else {
                ::GetWindowRect(hWnd, &rect);
            }
        }
        
        return rect;
    }
    
    bool UIUtils::ResizeWindow(HWND hWnd, int width, int height, bool repaint) {
        if (!hWnd || !IsWindow(hWnd)) {
            return false;
        }
        
        UINT flags = SWP_NOMOVE | SWP_NOZORDER;
        if (!repaint) {
            flags |= SWP_NOREDRAW;
        }
        
        return SetWindowPos(hWnd, nullptr, 0, 0, width, height, flags) != FALSE;
    }
    
    bool UIUtils::MoveWindow(HWND hWnd, int x, int y, bool repaint) {
        if (!hWnd || !IsWindow(hWnd)) {
            return false;
        }
        
        UINT flags = SWP_NOSIZE | SWP_NOZORDER;
        if (!repaint) {
            flags |= SWP_NOREDRAW;
        }
        
        return SetWindowPos(hWnd, nullptr, x, y, 0, 0, flags) != FALSE;
    }
    
    double UIUtils::GetDPIScale(HWND hWnd) {
        if (!hWnd || !IsWindow(hWnd)) {
            return 1.0;
        }
        
        HDC hdc = GetDC(hWnd);
        if (!hdc) {
            return 1.0;
        }
        
        int dpi = GetDeviceCaps(hdc, LOGPIXELSX);
        ReleaseDC(hWnd, hdc);
        
        return (double)dpi / 96.0; // 96 DPI是标准DPI
    }
    
    int UIUtils::ScaleForDPI(int size, HWND hWnd) {
        double scale = GetDPIScale(hWnd);
        return static_cast<int>(size * scale);
    }
    
    HICON UIUtils::LoadScaledIcon(HINSTANCE hInstance, int iconId, int width, int height) {
        return static_cast<HICON>(LoadImage(
            hInstance,
            MAKEINTRESOURCE(iconId),
            IMAGE_ICON,
            width,
            height,
            LR_DEFAULTCOLOR
        ));
    }
    
    bool UIUtils::SafeDestroyWindow(HWND& hWnd) {
        if (hWnd && IsWindow(hWnd)) {
            BOOL result = DestroyWindow(hWnd);
            hWnd = nullptr;
            return result != FALSE;
        }
        hWnd = nullptr;
        return true;
    }
    
    bool UIUtils::SafeDestroyMenu(HMENU& hMenu) {
        if (hMenu) {
            BOOL result = DestroyMenu(hMenu);
            hMenu = nullptr;
            return result != FALSE;
        }
        return true;
    }
    
    bool UIUtils::SafeDestroyImageList(HIMAGELIST& hImageList) {
        if (hImageList) {
            BOOL result = ImageList_Destroy(hImageList);
            hImageList = nullptr;
            return result != FALSE;
        }
        return true;
    }
    
    int UIUtils::GetSystemDPI() {
        HDC hdc = GetDC(nullptr);
        if (!hdc) {
            return 96; // 默认DPI
        }
        
        int dpi = GetDeviceCaps(hdc, LOGPIXELSX);
        ReleaseDC(nullptr, hdc);
        
        return dpi;
    }
    
} // namespace YG
