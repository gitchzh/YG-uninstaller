/**
 * @file RegistryHelper.cpp
 * @brief 注册表操作工具实现
 * @author gmrchzh@gmail.com
 * @version 1.0.1
 * @date 2025-09-17
 */

#include "utils/RegistryHelper.h"
#include "core/Logger.h"
#include <windows.h>

namespace YG {
    
    ErrorCode RegistryHelper::ReadString(HKEY hKey, const String& valueName, String& value) {
        wchar_t buffer[1024] = {0};
        DWORD bufferSize = sizeof(buffer);
        DWORD type;
        
        LONG result = RegQueryValueExW(hKey, valueName.c_str(), nullptr, &type, 
                                (LPBYTE)buffer, &bufferSize);
        
        if (result == ERROR_SUCCESS && (type == REG_SZ || type == REG_EXPAND_SZ)) {
            value = buffer;
            return ErrorCode::Success;
        }
        
        return ErrorCode::DataNotFound;
    }
    
    ErrorCode RegistryHelper::ReadDWord(HKEY hKey, const String& valueName, DWORD& value) {
        DWORD bufferSize = sizeof(DWORD);
        DWORD type;
        
        LONG result = RegQueryValueExW(hKey, valueName.c_str(), nullptr, &type, 
                                (LPBYTE)&value, &bufferSize);
        
        if (result == ERROR_SUCCESS && type == REG_DWORD) {
            return ErrorCode::Success;
        }
        
        return ErrorCode::DataNotFound;
    }
    
    ErrorCode RegistryHelper::OpenKey(HKEY hKeyParent, const String& subKey, 
                                    REGSAM samDesired, HKEY& hKey) {
        LONG result = RegOpenKeyExW(hKeyParent, subKey.c_str(), 0, samDesired, &hKey);
        return (result == ERROR_SUCCESS) ? ErrorCode::Success : ErrorCode::RegistryError;
    }
    
    ErrorCode RegistryHelper::CreateKey(HKEY hKeyParent, const String& subKey, 
                                      HKEY& hKey, bool* created) {
        DWORD disposition;
        LONG result = RegCreateKeyExW(hKeyParent, subKey.c_str(), 0, nullptr, 
                                    REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, 
                                    nullptr, &hKey, &disposition);
        
        if (created) {
            *created = (disposition == REG_CREATED_NEW_KEY);
        }
        
        return (result == ERROR_SUCCESS) ? ErrorCode::Success : ErrorCode::RegistryError;
    }
    
    ErrorCode RegistryHelper::DeleteKey(HKEY hKeyParent, const String& subKey, bool recursive) {
        if (recursive) {
            return RecursiveDeleteKey(hKeyParent, subKey);
        } else {
            LONG result = RegDeleteKeyW(hKeyParent, subKey.c_str());
            return (result == ERROR_SUCCESS) ? ErrorCode::Success : ErrorCode::RegistryError;
        }
    }
    
    bool RegistryHelper::KeyExists(HKEY hKeyParent, const String& subKey) {
        HKEY hKey;
        LONG result = RegOpenKeyExW(hKeyParent, subKey.c_str(), 0, KEY_READ, &hKey);
        if (result == ERROR_SUCCESS) {
            RegCloseKey(hKey);
            return true;
        }
        return false;
    }
    
    ErrorCode RegistryHelper::EnumerateSubKeys(HKEY hKey, StringVector& subKeys) {
        DWORD index = 0;
        wchar_t keyName[256];
        DWORD keyNameSize = sizeof(keyName) / sizeof(wchar_t);
        
        while (RegEnumKeyExW(hKey, index++, keyName, &keyNameSize, 
                           nullptr, nullptr, nullptr, nullptr) == ERROR_SUCCESS) {
            subKeys.push_back(keyName);
            keyNameSize = sizeof(keyName) / sizeof(wchar_t);
        }
        
        return ErrorCode::Success;
    }
    
    bool RegistryHelper::ValueExists(HKEY hKey, const String& valueName) {
        LONG result = RegQueryValueExW(hKey, valueName.c_str(), nullptr, nullptr, nullptr, nullptr);
        return (result == ERROR_SUCCESS);
    }
    
    ErrorCode RegistryHelper::EnumerateValues(HKEY hKey, StringVector& valueNames) {
        DWORD index = 0;
        wchar_t valueName[256];
        DWORD valueNameSize = sizeof(valueName) / sizeof(wchar_t);
        
        while (RegEnumValueW(hKey, index++, valueName, &valueNameSize, 
                           nullptr, nullptr, nullptr, nullptr) == ERROR_SUCCESS) {
            valueNames.push_back(valueName);
            valueNameSize = sizeof(valueName) / sizeof(wchar_t);
        }
        
        return ErrorCode::Success;
    }
    
    ErrorCode RegistryHelper::RecursiveDeleteKey(HKEY hKeyParent, const String& subKey) {
        // 手动实现递归删除，因为RegDeleteTreeW可能不可用
        HKEY hKey;
        LONG result = RegOpenKeyExW(hKeyParent, subKey.c_str(), 0, KEY_READ | KEY_WRITE, &hKey);
        if (result != ERROR_SUCCESS) {
            return ErrorCode::RegistryError;
        }
        
        // 枚举并删除所有子键
        StringVector subKeys;
        EnumerateSubKeys(hKey, subKeys);
        
        for (const auto& childKey : subKeys) {
            RecursiveDeleteKey(hKey, childKey);
        }
        
        RegCloseKey(hKey);
        
        // 删除空的键
        result = RegDeleteKeyW(hKeyParent, subKey.c_str());
        return (result == ERROR_SUCCESS) ? ErrorCode::Success : ErrorCode::RegistryError;
    }
    
    void RegistryHelper::CloseKey(HKEY hKey) {
        if (hKey && hKey != HKEY_CLASSES_ROOT && hKey != HKEY_CURRENT_USER && 
            hKey != HKEY_LOCAL_MACHINE && hKey != HKEY_USERS && 
            hKey != HKEY_CURRENT_CONFIG) {
            RegCloseKey(hKey);
        }
    }
    
    // 其他函数的简化实现
    ErrorCode RegistryHelper::WriteString(HKEY hKey, const String& valueName, const String& value) {
        LONG result = RegSetValueExW(hKey, valueName.c_str(), 0, REG_SZ, 
                                   (const BYTE*)value.c_str(), 
                                   (DWORD)(value.length() + 1) * sizeof(wchar_t));
        return (result == ERROR_SUCCESS) ? ErrorCode::Success : ErrorCode::RegistryError;
    }
    
    ErrorCode RegistryHelper::WriteDWord(HKEY hKey, const String& valueName, DWORD value) {
        LONG result = RegSetValueExW(hKey, valueName.c_str(), 0, REG_DWORD, 
                                   (const BYTE*)&value, sizeof(DWORD));
        return (result == ERROR_SUCCESS) ? ErrorCode::Success : ErrorCode::RegistryError;
    }
    
    ErrorCode RegistryHelper::DeleteValue(HKEY hKey, const String& valueName) {
        LONG result = RegDeleteValueW(hKey, valueName.c_str());
        return (result == ERROR_SUCCESS) ? ErrorCode::Success : ErrorCode::RegistryError;
    }
    
    // 占位符实现，避免链接错误
    ErrorCode RegistryHelper::ReadQWord(HKEY hKey, const String& valueName, DWORD64& value) {
        return ErrorCode::GeneralError;
    }
    
    ErrorCode RegistryHelper::ReadBinary(HKEY hKey, const String& valueName, std::vector<BYTE>& data) {
        return ErrorCode::GeneralError;
    }
    
    ErrorCode RegistryHelper::WriteQWord(HKEY hKey, const String& valueName, DWORD64 value) {
        return ErrorCode::GeneralError;
    }
    
    ErrorCode RegistryHelper::WriteBinary(HKEY hKey, const String& valueName, const std::vector<BYTE>& data) {
        return ErrorCode::GeneralError;
    }
    
    ErrorCode RegistryHelper::EnumerateSubKeysInfo(HKEY hKey, std::vector<RegistryKeyInfo>& keyInfos) {
        return ErrorCode::GeneralError;
    }
    
    ErrorCode RegistryHelper::GetKeyInfo(HKEY hKey, RegistryKeyInfo& keyInfo) {
        return ErrorCode::GeneralError;
    }
    
    ErrorCode RegistryHelper::ExportKey(HKEY hKey, const String& filePath) {
        return ErrorCode::GeneralError;
    }
    
    ErrorCode RegistryHelper::ImportFromFile(const String& filePath) {
        return ErrorCode::GeneralError;
    }
    
    ErrorCode RegistryHelper::BackupKey(HKEY hKeyParent, const String& subKey, const String& backupPath) {
        return ErrorCode::GeneralError;
    }
    
    ErrorCode RegistryHelper::SearchKeys(HKEY hKeyRoot, const String& searchPattern, 
                                       StringVector& foundKeys, int maxResults) {
        return ErrorCode::GeneralError;
    }
    
    ErrorCode RegistryHelper::SearchValues(HKEY hKeyRoot, const String& searchPattern, 
                                         StringVector& foundValues, int maxResults) {
        return ErrorCode::GeneralError;
    }
    
    ErrorCode RegistryHelper::CopyKey(HKEY hKeySrc, HKEY hKeyDest, bool recursive) {
        return ErrorCode::GeneralError;
    }
    
    String RegistryHelper::GetPredefinedKeyName(HKEY hKey) {
        return L"";
    }
    
    bool RegistryHelper::ParseRegistryPath(const String& fullPath, HKEY& rootKey, String& subKey) {
        return false;
    }
    
    String RegistryHelper::FormatRegistryPath(HKEY hKeyRoot, const String& subKey) {
        return L"";
    }
    
    bool RegistryHelper::HasRegistryAccess(HKEY hKeyParent, const String& subKey, REGSAM samDesired) {
        return false;
    }
    
    String RegistryHelper::ValueToString(const RegistryValueInfo& valueInfo) {
        return L"";
    }
    
    void RegistryHelper::RecursiveSearchKeys(HKEY hKey, const String& currentPath, 
                                           const String& searchPattern, StringVector& foundKeys, 
                                           int maxResults, int& currentCount) {
        // 占位符实现
    }
    
    void RegistryHelper::RecursiveSearchValues(HKEY hKey, const String& currentPath, 
                                             const String& searchPattern, StringVector& foundValues, 
                                             int maxResults, int& currentCount) {
        // 占位符实现
    }
    
    bool RegistryHelper::PatternMatch(const String& text, const String& pattern) {
        return false;
    }
    
} // namespace YG