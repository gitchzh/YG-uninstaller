/**
 * @file InputValidator.cpp
 * @brief 输入验证工具实现
 * @author YG Software
 * @version 1.0.1
 * @date 2025-01-21
 */

#include "utils/InputValidator.h"
#include "core/Logger.h"
#include <algorithm>
#include <sstream>
#include <cctype>

namespace YG {
    
    // 危险路径模式
    const std::vector<String> InputValidator::s_dangerousPathPatterns = {
        L"../", L"..\\", L"./", L".\\",
        L"//", L"\\\\", L"http://", L"https://", L"ftp://",
        L"\\\\?\\", L"\\\\.\\", L"CON", L"PRN", L"AUX", L"NUL",
        L"COM1", L"COM2", L"COM3", L"COM4", L"COM5", L"COM6", L"COM7", L"COM8", L"COM9",
        L"LPT1", L"LPT2", L"LPT3", L"LPT4", L"LPT5", L"LPT6", L"LPT7", L"LPT8", L"LPT9"
    };
    
    // 危险字符
    const std::vector<wchar_t> InputValidator::s_dangerousCharacters = {
        L'<', L'>', L'|', L'"', L'*', L'?', L'\0', L'\r', L'\n', L'\t'
    };
    
    // Windows保留文件名
    const std::vector<String> InputValidator::s_reservedFileNames = {
        L"CON", L"PRN", L"AUX", L"NUL",
        L"COM1", L"COM2", L"COM3", L"COM4", L"COM5", L"COM6", L"COM7", L"COM8", L"COM9",
        L"LPT1", L"LPT2", L"LPT3", L"LPT4", L"LPT5", L"LPT6", L"LPT7", L"LPT8", L"LPT9"
    };
    
    ErrorContext InputValidator::ValidatePath(const String& path, bool allowRelative) {
        if (path.empty()) {
            return YG_DETAILED_ERROR(DetailedErrorCode::RequiredParameterMissing, L"路径不能为空");
        }
        
        // 检查路径长度
        if (path.length() > MAX_PATH) {
            return YG_DETAILED_ERROR(DetailedErrorCode::PathTooLong, 
                L"路径长度超过限制 (" + std::to_wstring(path.length()) + L"/" + std::to_wstring(MAX_PATH) + L")");
        }
        
        // 检查危险模式
        if (ContainsDangerousPathPatterns(path)) {
            return YG_DETAILED_ERROR(DetailedErrorCode::InvalidParameter, L"路径包含危险字符或模式");
        }
        
        // 检查是否为相对路径
        if (!allowRelative && (path[0] != L'\\' && (path.length() < 3 || path[1] != L':'))) {
            return YG_DETAILED_ERROR(DetailedErrorCode::InvalidParameter, L"不允许相对路径");
        }
        
        // 检查每个字符是否有效
        for (wchar_t ch : path) {
            if (!IsValidPathCharacter(ch)) {
                return YG_DETAILED_ERROR(DetailedErrorCode::InvalidParameter, 
                    L"路径包含无效字符: '" + String(1, ch) + L"'");
            }
        }
        
        return ErrorContext(DetailedErrorCode::Success);
    }
    
    ErrorContext InputValidator::ValidateFileName(const String& fileName) {
        if (fileName.empty()) {
            return YG_DETAILED_ERROR(DetailedErrorCode::RequiredParameterMissing, L"文件名不能为空");
        }
        
        // 检查长度
        if (fileName.length() > 255) {
            return YG_DETAILED_ERROR(DetailedErrorCode::ParameterTooLong, L"文件名过长");
        }
        
        // 检查保留名称
        String upperFileName = fileName;
        std::transform(upperFileName.begin(), upperFileName.end(), upperFileName.begin(), ::towupper);
        
        for (const String& reserved : s_reservedFileNames) {
            if (upperFileName == reserved || upperFileName.find(reserved + L".") == 0) {
                return YG_DETAILED_ERROR(DetailedErrorCode::InvalidFileName, 
                    L"文件名使用了Windows保留名称: " + reserved);
            }
        }
        
        // 检查每个字符
        for (wchar_t ch : fileName) {
            if (!IsValidFileNameCharacter(ch)) {
                return YG_DETAILED_ERROR(DetailedErrorCode::InvalidFileName, 
                    L"文件名包含无效字符: '" + String(1, ch) + L"'");
            }
        }
        
        // 检查是否以点或空格结尾
        if (fileName.back() == L'.' || fileName.back() == L' ') {
            return YG_DETAILED_ERROR(DetailedErrorCode::InvalidFileName, L"文件名不能以点或空格结尾");
        }
        
        return ErrorContext(DetailedErrorCode::Success);
    }
    
    ErrorContext InputValidator::ValidateRegistryKeyPath(const String& keyPath) {
        if (keyPath.empty()) {
            return YG_DETAILED_ERROR(DetailedErrorCode::RequiredParameterMissing, L"注册表键路径不能为空");
        }
        
        // 检查长度限制
        if (keyPath.length() > 255) {
            return YG_DETAILED_ERROR(DetailedErrorCode::ParameterTooLong, L"注册表键路径过长");
        }
        
        // 检查是否以有效的根键开始
        const std::vector<String> validRoots = {
            L"HKEY_CLASSES_ROOT", L"HKCR",
            L"HKEY_CURRENT_USER", L"HKCU",
            L"HKEY_LOCAL_MACHINE", L"HKLM",
            L"HKEY_USERS", L"HKU",
            L"HKEY_CURRENT_CONFIG", L"HKCC"
        };
        
        bool validRoot = false;
        for (const String& root : validRoots) {
            if (keyPath.find(root) == 0) {
                validRoot = true;
                break;
            }
        }
        
        if (!validRoot) {
            return YG_DETAILED_ERROR(DetailedErrorCode::RegistryKeyNotFound, L"无效的注册表根键");
        }
        
        // 检查危险字符
        if (ContainsControlCharacters(keyPath)) {
            return YG_DETAILED_ERROR(DetailedErrorCode::InvalidParameter, L"注册表键路径包含控制字符");
        }
        
        return ErrorContext(DetailedErrorCode::Success);
    }
    
    ErrorContext InputValidator::ValidateUninstallString(const String& uninstallString) {
        if (uninstallString.empty()) {
            return YG_DETAILED_ERROR(DetailedErrorCode::UninstallStringNotFound, L"卸载字符串为空");
        }
        
        // 检查长度
        if (uninstallString.length() > 2048) {
            return YG_DETAILED_ERROR(DetailedErrorCode::UninstallStringInvalid, L"卸载字符串过长");
        }
        
        // 检查是否包含可执行文件
        if (uninstallString.find(L".exe") == String::npos && 
            uninstallString.find(L".msi") == String::npos &&
            uninstallString.find(L".bat") == String::npos &&
            uninstallString.find(L".cmd") == String::npos) {
            return YG_DETAILED_ERROR(DetailedErrorCode::UninstallStringInvalid, 
                L"卸载字符串不包含有效的可执行文件类型");
        }
        
        // 检查危险模式
        const std::vector<String> dangerousPatterns = {
            L"format ", L"del /", L"rmdir /", L"rd /", L"deltree ",
            L"shutdown ", L"reboot ", L"restart ", L"taskkill /",
            L"net user ", L"net share ", L"reg delete ", L"regedit "
        };
        
        String lowerUninstallString = uninstallString;
        std::transform(lowerUninstallString.begin(), lowerUninstallString.end(), 
                      lowerUninstallString.begin(), ::towlower);
        
        for (const String& pattern : dangerousPatterns) {
            if (lowerUninstallString.find(pattern) != String::npos) {
                return YG_DETAILED_ERROR(DetailedErrorCode::UninstallStringInvalid, 
                    L"卸载字符串包含潜在危险命令: " + pattern);
            }
        }
        
        return ErrorContext(DetailedErrorCode::Success);
    }
    
    ErrorContext InputValidator::ValidateConfigValue(const String& key, const String& value) {
        if (key.empty()) {
            return YG_DETAILED_ERROR(DetailedErrorCode::RequiredParameterMissing, L"配置键不能为空");
        }
        
        // 检查键名长度
        if (key.length() > 100) {
            return YG_DETAILED_ERROR(DetailedErrorCode::ParameterTooLong, L"配置键名过长");
        }
        
        // 检查值长度
        if (value.length() > 1024) {
            return YG_DETAILED_ERROR(DetailedErrorCode::ParameterTooLong, L"配置值过长");
        }
        
        // 检查键名是否包含特殊字符
        for (wchar_t ch : key) {
            if (!std::isalnum(ch) && ch != L'_' && ch != L'-' && ch != L'.') {
                return YG_DETAILED_ERROR(DetailedErrorCode::InvalidParameter, 
                    L"配置键名包含无效字符: '" + String(1, ch) + L"'");
            }
        }
        
        // 检查值是否包含控制字符
        if (ContainsControlCharacters(value)) {
            return YG_DETAILED_ERROR(DetailedErrorCode::ConfigValueInvalid, L"配置值包含控制字符");
        }
        
        return ErrorContext(DetailedErrorCode::Success);
    }
    
    ErrorContext InputValidator::ValidateIntRange(int value, int minValue, int maxValue, 
                                                const String& paramName) {
        if (value < minValue || value > maxValue) {
            return YG_DETAILED_ERROR(DetailedErrorCode::ParameterOutOfRange, 
                paramName + L"超出有效范围 [" + std::to_wstring(minValue) + 
                L", " + std::to_wstring(maxValue) + L"]，当前值: " + std::to_wstring(value));
        }
        
        return ErrorContext(DetailedErrorCode::Success);
    }
    
    ErrorContext InputValidator::ValidateStringLength(const String& str, size_t minLength, 
                                                    size_t maxLength, const String& paramName) {
        if (str.length() < minLength) {
            return YG_DETAILED_ERROR(DetailedErrorCode::ParameterTooShort, 
                paramName + L"长度不足，最小长度: " + std::to_wstring(minLength) + 
                L"，当前长度: " + std::to_wstring(str.length()));
        }
        
        if (str.length() > maxLength) {
            return YG_DETAILED_ERROR(DetailedErrorCode::ParameterTooLong, 
                paramName + L"长度超限，最大长度: " + std::to_wstring(maxLength) + 
                L"，当前长度: " + std::to_wstring(str.length()));
        }
        
        return ErrorContext(DetailedErrorCode::Success);
    }
    
    ErrorContext InputValidator::ValidateNotEmpty(const String& str, const String& paramName) {
        if (str.empty()) {
            return YG_DETAILED_ERROR(DetailedErrorCode::RequiredParameterMissing, 
                paramName + L"不能为空");
        }
        
        // 检查是否仅包含空白字符
        bool allWhitespace = true;
        for (wchar_t ch : str) {
            if (!std::iswspace(ch)) {
                allWhitespace = false;
                break;
            }
        }
        
        if (allWhitespace) {
            return YG_DETAILED_ERROR(DetailedErrorCode::RequiredParameterMissing, 
                paramName + L"不能仅包含空白字符");
        }
        
        return ErrorContext(DetailedErrorCode::Success);
    }
    
    ErrorContext InputValidator::ValidateSafeString(const String& str, const String& paramName) {
        // 检查危险字符
        for (wchar_t ch : s_dangerousCharacters) {
            if (str.find(ch) != String::npos) {
                return YG_DETAILED_ERROR(DetailedErrorCode::InvalidParameter, 
                    paramName + L"包含危险字符: '" + String(1, ch) + L"'");
            }
        }
        
        // 检查控制字符
        if (ContainsControlCharacters(str)) {
            return YG_DETAILED_ERROR(DetailedErrorCode::InvalidParameter, 
                paramName + L"包含控制字符");
        }
        
        return ErrorContext(DetailedErrorCode::Success);
    }
    
    ErrorContext InputValidator::ValidateVersionString(const String& version) {
        if (version.empty()) {
            return YG_DETAILED_ERROR(DetailedErrorCode::RequiredParameterMissing, L"版本号不能为空");
        }
        
        // 简单的版本号格式验证（数字.数字.数字.数字）
        std::wregex versionRegex(L"^\\d+(\\.\\d+)*$");
        if (!std::regex_match(version, versionRegex)) {
            return YG_DETAILED_ERROR(DetailedErrorCode::ParameterFormatInvalid, 
                L"版本号格式无效，应为数字.数字格式");
        }
        
        return ErrorContext(DetailedErrorCode::Success);
    }
    
    ErrorContext InputValidator::ValidateURL(const String& url) {
        if (url.empty()) {
            return YG_DETAILED_ERROR(DetailedErrorCode::RequiredParameterMissing, L"URL不能为空");
        }
        
        // 简单的URL格式验证
        if (url.find(L"http://") != 0 && url.find(L"https://") != 0 && url.find(L"ftp://") != 0) {
            return YG_DETAILED_ERROR(DetailedErrorCode::ParameterFormatInvalid, 
                L"URL必须以http://、https://或ftp://开头");
        }
        
        // 检查长度
        if (url.length() > 2048) {
            return YG_DETAILED_ERROR(DetailedErrorCode::ParameterTooLong, L"URL过长");
        }
        
        return ErrorContext(DetailedErrorCode::Success);
    }
    
    ErrorContext InputValidator::ValidateEmail(const String& email) {
        if (email.empty()) {
            return YG_DETAILED_ERROR(DetailedErrorCode::RequiredParameterMissing, L"邮箱地址不能为空");
        }
        
        // 简单的邮箱格式验证
        std::wregex emailRegex(L"^[\\w\\.-]+@[\\w\\.-]+\\.[\\w]+$");
        if (!std::regex_match(email, emailRegex)) {
            return YG_DETAILED_ERROR(DetailedErrorCode::ParameterFormatInvalid, L"邮箱地址格式无效");
        }
        
        return ErrorContext(DetailedErrorCode::Success);
    }
    
    String InputValidator::SanitizeString(const String& input) {
        String result = input;
        
        // 移除危险字符
        for (wchar_t ch : s_dangerousCharacters) {
            result.erase(std::remove(result.begin(), result.end(), ch), result.end());
        }
        
        // 移除控制字符
        result.erase(std::remove_if(result.begin(), result.end(), 
            [](wchar_t ch) { return ch < 32 && ch != L'\t' && ch != L'\n' && ch != L'\r'; }), 
            result.end());
        
        return result;
    }
    
    String InputValidator::EscapeSQLString(const String& input) {
        String result = input;
        
        // 转义单引号
        size_t pos = 0;
        while ((pos = result.find(L'\'', pos)) != String::npos) {
            result.replace(pos, 1, L"''");
            pos += 2;
        }
        
        return result;
    }
    
    String InputValidator::EscapeRegexString(const String& input) {
        String result = input;
        
        const std::vector<wchar_t> regexChars = {
            L'.', L'^', L'$', L'*', L'+', L'?', L'(', L')', L'[', L']', L'{', L'}', L'|', L'\\'
        };
        
        for (wchar_t ch : regexChars) {
            size_t pos = 0;
            String replacement = L"\\" + String(1, ch);
            while ((pos = result.find(ch, pos)) != String::npos) {
                result.replace(pos, 1, replacement);
                pos += 2;
            }
        }
        
        return result;
    }
    
    ErrorContext InputValidator::ValidateProgramInfo(const ProgramInfo& programInfo) {
        // 验证程序名称
        ErrorContext nameResult = ValidateNotEmpty(programInfo.name, L"程序名称");
        if (nameResult.code != DetailedErrorCode::Success) {
            return nameResult;
        }
        
        ErrorContext nameValidResult = ValidateSafeString(programInfo.name, L"程序名称");
        if (nameValidResult.code != DetailedErrorCode::Success) {
            return nameValidResult;
        }
        
        // 验证安装路径（如果提供）
        if (!programInfo.installLocation.empty()) {
            ErrorContext pathResult = ValidatePath(programInfo.installLocation, false);
            if (pathResult.code != DetailedErrorCode::Success) {
                return pathResult;
            }
        }
        
        // 验证卸载字符串（如果提供）
        if (!programInfo.uninstallString.empty()) {
            ErrorContext uninstallResult = ValidateUninstallString(programInfo.uninstallString);
            if (uninstallResult.code != DetailedErrorCode::Success) {
                return uninstallResult;
            }
        }
        
        // 验证版本号（如果提供）
        if (!programInfo.version.empty()) {
            ErrorContext versionResult = ValidateVersionString(programInfo.version);
            if (versionResult.code != DetailedErrorCode::Success) {
                return versionResult;
            }
        }
        
        return ErrorContext(DetailedErrorCode::Success);
    }
    
    bool InputValidator::ContainsDangerousPathPatterns(const String& path) {
        String lowerPath = path;
        std::transform(lowerPath.begin(), lowerPath.end(), lowerPath.begin(), ::towlower);
        
        for (const String& pattern : s_dangerousPathPatterns) {
            String lowerPattern = pattern;
            std::transform(lowerPattern.begin(), lowerPattern.end(), lowerPattern.begin(), ::towlower);
            if (lowerPath.find(lowerPattern) != String::npos) {
                return true;
            }
        }
        
        return false;
    }
    
    bool InputValidator::ContainsControlCharacters(const String& str) {
        for (wchar_t ch : str) {
            if (ch < 32 && ch != L'\t' && ch != L'\n' && ch != L'\r') {
                return true;
            }
        }
        return false;
    }
    
    bool InputValidator::IsValidPathCharacter(wchar_t ch) {
        // Windows路径中的无效字符
        const std::vector<wchar_t> invalidChars = {L'<', L'>', L'|', L'"', L'*', L'?'};
        
        return std::find(invalidChars.begin(), invalidChars.end(), ch) == invalidChars.end() &&
               ch >= 32; // 排除控制字符
    }
    
    bool InputValidator::IsValidFileNameCharacter(wchar_t ch) {
        // Windows文件名中的无效字符
        const std::vector<wchar_t> invalidChars = {L'<', L'>', L':', L'"', L'|', L'?', L'*', L'/', L'\\'};
        
        return std::find(invalidChars.begin(), invalidChars.end(), ch) == invalidChars.end() &&
               ch >= 32; // 排除控制字符
    }
    
} // namespace YG
