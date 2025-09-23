/**
 * @file StringUtils.cpp
 * @brief 字符串处理工具实现
 * @author YG Software
 * @version 1.0.1
 * @date 2025-01-21
 */

#include "utils/StringUtils.h"
#include <sstream>
#include <iomanip>
#include <cctype>
// 移除random和functional以减小体积

namespace YG {
    
    String StringUtils::Trim(const String& str) {
        return TrimLeft(TrimRight(str));
    }
    
    String StringUtils::TrimLeft(const String& str) {
        auto start = std::find_if(str.begin(), str.end(), [](wchar_t ch) {
            return !std::iswspace(ch);
        });
        return String(start, str.end());
    }
    
    String StringUtils::TrimRight(const String& str) {
        auto end = std::find_if(str.rbegin(), str.rend(), [](wchar_t ch) {
            return !std::iswspace(ch);
        });
        return String(str.begin(), end.base());
    }
    
    String StringUtils::ToUpper(const String& str) {
        String result = str;
        std::transform(result.begin(), result.end(), result.begin(), ::towupper);
        return result;
    }
    
    String StringUtils::ToLower(const String& str) {
        String result = str;
        std::transform(result.begin(), result.end(), result.begin(), ::towlower);
        return result;
    }
    
    StringVector StringUtils::Split(const String& str, const String& delimiter, size_t maxSplits) {
        StringVector result;
        
        if (str.empty() || delimiter.empty()) {
            result.push_back(str);
            return result;
        }
        
        size_t start = 0;
        size_t found = 0;
        size_t splits = 0;
        
        while ((found = str.find(delimiter, start)) != String::npos) {
            if (maxSplits > 0 && splits >= maxSplits) {
                break;
            }
            
            result.push_back(str.substr(start, found - start));
            start = found + delimiter.length();
            splits++;
        }
        
        result.push_back(str.substr(start));
        return result;
    }
    
    String StringUtils::Join(const StringVector& strings, const String& separator) {
        if (strings.empty()) {
            return L"";
        }
        
        std::wstringstream ss;
        for (size_t i = 0; i < strings.size(); ++i) {
            if (i > 0) {
                ss << separator;
            }
            ss << strings[i];
        }
        
        return ss.str();
    }
    
    String StringUtils::ReplaceAll(const String& str, const String& from, const String& to) {
        if (from.empty()) {
            return str;
        }
        
        String result = str;
        size_t pos = 0;
        
        while ((pos = result.find(from, pos)) != String::npos) {
            result.replace(pos, from.length(), to);
            pos += to.length();
        }
        
        return result;
    }
    
    bool StringUtils::StartsWith(const String& str, const String& prefix, bool ignoreCase) {
        if (prefix.length() > str.length()) {
            return false;
        }
        
        String strPart = str.substr(0, prefix.length());
        String prefixCopy = prefix;
        
        if (ignoreCase) {
            strPart = ToLower(strPart);
            prefixCopy = ToLower(prefixCopy);
        }
        
        return strPart == prefixCopy;
    }
    
    bool StringUtils::EndsWith(const String& str, const String& suffix, bool ignoreCase) {
        if (suffix.length() > str.length()) {
            return false;
        }
        
        String strPart = str.substr(str.length() - suffix.length());
        String suffixCopy = suffix;
        
        if (ignoreCase) {
            strPart = ToLower(strPart);
            suffixCopy = ToLower(suffixCopy);
        }
        
        return strPart == suffixCopy;
    }
    
    bool StringUtils::Contains(const String& str, const String& substring, bool ignoreCase) {
        String strCopy = str;
        String substringCopy = substring;
        
        if (ignoreCase) {
            strCopy = ToLower(strCopy);
            substringCopy = ToLower(substringCopy);
        }
        
        return strCopy.find(substringCopy) != String::npos;
    }
    
    String StringUtils::FormatFileSize(DWORD64 sizeInBytes, int precision) {
        const wchar_t* units[] = {L"字节", L"KB", L"MB", L"GB", L"TB"};
        const int unitCount = sizeof(units) / sizeof(units[0]);
        
        double size = static_cast<double>(sizeInBytes);
        int unitIndex = 0;
        
        while (size >= 1024.0 && unitIndex < unitCount - 1) {
            size /= 1024.0;
            unitIndex++;
        }
        
        std::wstringstream ss;
        ss << std::fixed << std::setprecision(precision) << size << L" " << units[unitIndex];
        return ss.str();
    }
    
    String StringUtils::FormatDuration(DWORD milliseconds) {
        if (milliseconds < 1000) {
            return std::to_wstring(milliseconds) + L"毫秒";
        }
        
        DWORD seconds = milliseconds / 1000;
        if (seconds < 60) {
            return std::to_wstring(seconds) + L"秒";
        }
        
        DWORD minutes = seconds / 60;
        seconds %= 60;
        
        if (minutes < 60) {
            return std::to_wstring(minutes) + L"分" + std::to_wstring(seconds) + L"秒";
        }
        
        DWORD hours = minutes / 60;
        minutes %= 60;
        
        return std::to_wstring(hours) + L"时" + std::to_wstring(minutes) + L"分" + std::to_wstring(seconds) + L"秒";
    }
    
    String StringUtils::FormatDateTime(const SYSTEMTIME& systemTime, const String& format) {
        std::wstringstream ss;
        
        // 简单的格式化实现
        if (format == L"yyyy-MM-dd HH:mm:ss") {
            ss << std::setfill(L'0') 
               << std::setw(4) << systemTime.wYear << L"-"
               << std::setw(2) << systemTime.wMonth << L"-"
               << std::setw(2) << systemTime.wDay << L" "
               << std::setw(2) << systemTime.wHour << L":"
               << std::setw(2) << systemTime.wMinute << L":"
               << std::setw(2) << systemTime.wSecond;
        } else if (format == L"yyyy-MM-dd") {
            ss << std::setfill(L'0')
               << std::setw(4) << systemTime.wYear << L"-"
               << std::setw(2) << systemTime.wMonth << L"-"
               << std::setw(2) << systemTime.wDay;
        } else {
            // 默认格式
            ss << std::setfill(L'0')
               << std::setw(4) << systemTime.wYear << L"/"
               << std::setw(2) << systemTime.wMonth << L"/"
               << std::setw(2) << systemTime.wDay << L" "
               << std::setw(2) << systemTime.wHour << L":"
               << std::setw(2) << systemTime.wMinute;
        }
        
        return ss.str();
    }
    
    String StringUtils::Truncate(const String& str, size_t maxLength, const String& ellipsis) {
        if (str.length() <= maxLength) {
            return str;
        }
        
        if (maxLength <= ellipsis.length()) {
            return ellipsis.substr(0, maxLength);
        }
        
        return str.substr(0, maxLength - ellipsis.length()) + ellipsis;
    }
    
    String StringUtils::Pad(const String& str, size_t length, wchar_t fillChar, bool leftAlign) {
        if (str.length() >= length) {
            return str;
        }
        
        size_t padLength = length - str.length();
        String padding(padLength, fillChar);
        
        return leftAlign ? (str + padding) : (padding + str);
    }
    
    int StringUtils::CompareIgnoreCase(const String& str1, const String& str2) {
        String lower1 = ToLower(str1);
        String lower2 = ToLower(str2);
        return lower1.compare(lower2);
    }
    
    bool StringUtils::IsNumeric(const String& str) {
        if (str.empty()) {
            return false;
        }
        
        size_t start = 0;
        if (str[0] == L'+' || str[0] == L'-') {
            start = 1;
        }
        
        bool hasDecimalPoint = false;
        for (size_t i = start; i < str.length(); ++i) {
            wchar_t ch = str[i];
            
            if (ch == L'.') {
                if (hasDecimalPoint) {
                    return false; // 多个小数点
                }
                hasDecimalPoint = true;
            } else if (!std::iswdigit(ch)) {
                return false;
            }
        }
        
        return start < str.length(); // 确保有数字字符
    }
    
    bool StringUtils::IsValidVersion(const String& version) {
        if (version.empty()) {
            return false;
        }
        
        StringVector parts = Split(version, L".");
        if (parts.empty() || parts.size() > 4) {
            return false;
        }
        
        for (const String& part : parts) {
            if (!IsNumeric(part) || part.empty()) {
                return false;
            }
        }
        
        return true;
    }
    
    int StringUtils::CompareVersions(const String& version1, const String& version2) {
        std::vector<int> v1 = ParseVersion(version1);
        std::vector<int> v2 = ParseVersion(version2);
        
        // 补齐到相同长度
        size_t maxLength = std::max(v1.size(), v2.size());
        v1.resize(maxLength, 0);
        v2.resize(maxLength, 0);
        
        for (size_t i = 0; i < maxLength; ++i) {
            if (v1[i] < v2[i]) return -1;
            if (v1[i] > v2[i]) return 1;
        }
        
        return 0;
    }
    
    String StringUtils::GenerateRandomString(size_t length, bool useAlphaNumeric) {
        const String chars = useAlphaNumeric ? 
            L"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789" :
            L"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789!@#$%^&*()";
        
        // 使用简单的随机数生成器
        srand(static_cast<unsigned int>(GetTickCount()));
        
        String result;
        result.reserve(length);
        
        for (size_t i = 0; i < length; ++i) {
            result += chars[rand() % chars.length()];
        }
        
        return result;
    }
    
    size_t StringUtils::CalculateHash(const String& str) {
        // 简单的哈希算法，避免使用std::hash
        size_t hash = 0;
        for (wchar_t ch : str) {
            hash = hash * 31 + ch;
        }
        return hash;
    }
    
    std::vector<int> StringUtils::ParseVersion(const String& version) {
        std::vector<int> result;
        StringVector parts = Split(version, L".");
        
        for (const String& part : parts) {
            try {
                result.push_back(std::stoi(part));
            } catch (...) {
                result.push_back(0);
            }
        }
        
        return result;
    }
    
} // namespace YG
