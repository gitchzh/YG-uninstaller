/**
 * @file DirectProgramScanner.h
 * @brief 直接使用Windows API扫描已安装程序
 * @author gmrchzh@gmail.com
 * @version 1.0.0
 * @date 2025-09-17
 */

#pragma once

#include "core/Common.h"
#include <vector>

namespace YG {

/**
 * @brief 直接从注册表获取已安装程序列表
 * @param programs 输出的程序列表
 * @return 找到的程序数量
 */
int GetInstalledProgramsDirect(std::vector<ProgramInfo>& programs);

} // namespace YG
