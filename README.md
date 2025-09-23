# YG Uninstaller

[![Version](https://img.shields.io/badge/version-1.0.1-blue.svg)](https://github.com/gitchzh/YG-uninstaller)
[![License](https://img.shields.io/badge/license-MIT-green.svg)](LICENSE.txt)
[![Platform](https://img.shields.io/badge/platform-Windows-lightgrey.svg)](https://www.microsoft.com/windows)
[![Language](https://img.shields.io/badge/language-C%2B%2B17-blue.svg)](https://isocpp.org/)

一个高效、轻量级的Windows程序卸载工具，采用现代化架构设计，提供专业的程序管理和系统清理功能。

## ✨ 主要特性

### 🚀 核心功能
- **智能程序扫描**: 自动检测系统中已安装的程序，包括传统程序和Windows Store应用
- **智能深度清理**: 卸载完成后自动扫描残留文件、注册表、快捷方式等，确保系统彻底清洁
- **实时进度显示**: 状态栏实时显示清理进度和发现的残留项数量
- **可视化清理界面**: 专业的清理对话框，支持选择性删除残留项
- **批量操作**: 支持批量卸载多个程序，提高操作效率

### 🎯 高级特性
- **数字签名验证**: 验证程序的可信度，确保安全性
- **系统还原点**: 卸载前自动创建系统还原点，提供安全保障
- **多线程处理**: 利用多核CPU提升扫描和卸载性能
- **详细日志**: 记录所有操作过程，便于问题诊断

### 🎨 用户界面
- **现代化UI**: 采用Windows原生控件，提供流畅的用户体验
- **智能搜索**: 支持程序名称模糊搜索，快速定位目标程序
- **可点击属性对话框**: 现代化的程序属性窗口，支持直接点击安装路径和官网链接
- **美化清理窗口**: 与系统对话框一致的清理界面，支持复选框选择和详细信息显示
- **分类显示**: 按程序类型、大小、安装时间等维度分类显示
- **系统托盘**: 支持最小化到系统托盘，方便后台运行

## 🏗️ 技术架构

### 模块化设计
项目采用模块化架构，将功能按职责分离：

```
YG Uninstaller/
├── Core/                    # 核心功能模块
│   ├── Config              # 配置管理
│   ├── Logger              # 日志系统
│   ├── ErrorHandler        # 错误处理
│   └── ResidualItem        # 残留项数据结构
├── UI/                     # 用户界面模块
│   ├── MainWindow          # 主窗口
│   ├── MainWindowSettings  # 设置管理
│   ├── MainWindowLogs      # 日志管理
│   ├── MainWindowTray      # 系统托盘
│   ├── CleanupDialog       # 清理对话框
│   └── ResourceManager     # 资源管理
├── Services/               # 服务层
│   ├── ProgramDetector     # 程序检测
│   ├── UninstallerService  # 卸载服务
│   ├── DirectProgramScanner # 直接程序扫描
│   ├── ResidualScanner     # 残留扫描
│   └── ProgramCache        # 程序缓存
└── Utils/                  # 工具类
    ├── RegistryHelper      # 注册表操作
    ├── StringUtils         # 字符串处理
    ├── UIUtils             # UI工具
    └── InputValidator      # 输入验证
```

### 设计模式
- **单一职责原则**: 每个模块专注于特定功能
- **依赖注入**: 通过构造函数注入依赖，提高可测试性
- **观察者模式**: 事件驱动的UI更新机制
- **工厂模式**: 统一的对象创建接口

## 📋 系统要求

### 最低要求
- **操作系统**: Windows 7 SP1 或更高版本
- **架构**: x86 (32位) 或 x64 (64位)
- **内存**: 512 MB RAM
- **磁盘空间**: 50 MB 可用空间

### 推荐配置
- **操作系统**: Windows 10 或 Windows 11
- **架构**: x64 (64位)
- **内存**: 2 GB RAM 或更多
- **磁盘空间**: 100 MB 可用空间

## 🚀 快速开始

### 下载安装
1. 访问 [Releases页面](https://github.com/gitchzh/YG-uninstaller/releases)
2. 下载最新版本的安装包
3. 运行安装程序，按照向导完成安装

### 首次使用
1. 启动YG Uninstaller
2. 程序将自动扫描系统中已安装的程序
3. 浏览程序列表，选择要卸载的程序
4. 点击"卸载"按钮开始卸载过程

## 🛠️ 开发指南

### 环境准备
- **编译器**: GCC 10.0+（MinGW-w64，建议 Ninja 生成器）
- **依赖库**: Windows SDK/Win32 API（随 MinGW-w64 提供）
- **C++标准**: C++17

### 使用 CMake + MinGW/GCC 编译（推荐）
```bash
# 克隆仓库
git clone https://github.com/gitchzh/YG-uninstaller.git
cd yguninstaller

# 安装并准备 MinGW-w64 (建议 posix 线程, seh 异常, x86_64)
# 确保 gcc/g++/windres 在 PATH 中

# 方式一：使用 CMake Presets（自动生成 x86 与 x64，推荐）
cmake --preset mingw-x64
cmake --build --preset build-x64

cmake --preset mingw-x86
cmake --build --preset build-x86

# 产物位置：
#   output/x64/YGUninstaller.exe
#   output/x86/YGUninstaller.exe

# 方式二：手动配置（示例：x64）
cmake -S . -B build-mingw-x64 -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER=x86_64-w64-mingw32-gcc -DCMAKE_CXX_COMPILER=x86_64-w64-mingw32-g++ -DOUTPUT_SUBDIR=x64 -DSIZE_OPTIMIZED=ON
cmake --build build-mingw-x64 --config Release

# 手动配置（示例：x86）
cmake -S . -B build-mingw-x86 -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER=i686-w64-mingw32-gcc -DCMAKE_CXX_COMPILER=i686-w64-mingw32-g++ -DOUTPUT_SUBDIR=x86 -DSIZE_OPTIMIZED=ON
cmake --build build-mingw-x86 --config Release

# 生成的可执行文件位于 output/x64 与 output/x86
```

### 体积优化说明
- 默认启用 `-Os -ffunction-sections -fdata-sections -Wl,--gc-sections -s`。
- LTO 默认关闭（`-DENABLE_LTO=ON` 可开启）。开启后个别文件（如 `src/services/ProgramDetector.cpp`）会被自动禁用 LTO 以规避已知 GCC 15 的内部错误。
- 若系统安装了 `upx`，构建后会自动执行 `upx --best --lzma` 压缩可执行文件。
- 如需关闭体积优化（对比大小/调试方便）：在配置时加 `-DSIZE_OPTIMIZED=OFF`。

## 源码/资源文件增减与 CMake 修改指南

本项目的 `CMakeLists.txt` 已启用自动发现源码：
- 已配置 `file(GLOB_RECURSE ... CONFIGURE_DEPENDS)` 收集源码目录：
  - `src/core/*.cpp`
  - `src/services/*.cpp`
  - `src/ui/*.cpp`
  - `src/utils/*.cpp`
- 因此，以上目录中新建或删除 `.cpp` 文件时，通常不需要手动修改 CMake。Ninja/构建系统会在需要时自动触发重新配置；若使用的生成器未自动感知，请手动执行一次配置命令，例如：
  - 预设方式：`cmake --preset mingw-x64`（或对应 x86 预设）
  - 手动方式：`cmake -S . -B build-... <你的参数>`

需要手动修改 CMake 的场景与操作：
- 新增资源文件（.rc/.ico/.manifest 等）
  - 默认仅编译 `resources/app.rc` 与 `resources/version.rc`。若你新增了新的独立 `.rc` 文件，请在 `CMakeLists.txt` 的 `RESOURCE_FILES` 中增加该文件路径。
  - 若只是给 `app.rc` 引入新资源（如位图/图标/菜单）并通过 `app.rc` 包含，通常无需修改 CMake。
- 新增公共头文件目录
  - 头文件位于 `include/` 下已整体加入，通常无需改动。
  - 如需额外包含目录（不在 `include/` 或 `resources/` 下），请在 `CMakeLists.txt` 中的 `include_directories(...)` 或 `target_include_directories(...)` 增加路径。
- 新增第三方库链接
  - 当前已链接常用 Win32 库（`version, comctl32, comdlg32, uxtheme, shell32, shlwapi, user32, gdi32, advapi32, ole32, oleaut32, uuid, ws2_32`）。
  - 若引入新的系统库或第三方静态/动态库，请在 `target_link_libraries(YGUninstaller PRIVATE ...)` 补充相应库名，同时确保库与头文件在可搜索路径中（必要时添加 `target_link_directories`/`target_include_directories`）。
- 更改可执行文件输出位置或区分架构目录
  - 已通过 `OUTPUT_SUBDIR` 控制输出子目录（如 `x64` / `x86`）。若需要新的布局，可在配置时传参 `-DOUTPUT_SUBDIR=<your_subdir>` 或在 `CMakePresets.json` 对应预设的 `cacheVariables` 中调整。
- LTO 与体积优化策略
  - 默认不开启 LTO（`ENABLE_LTO=OFF`）。若需开启：在配置时增加 `-DENABLE_LTO=ON`。若个别源触发编译器问题，可在 `CMakeLists.txt` 使用 `set_source_files_properties(<file>.cpp PROPERTIES INTERPROCEDURAL_OPTIMIZATION FALSE)` 针对性关闭。

小贴士：
- 如构建后提示找不到新增的源文件，多数是生成器未自动感知变更，重新运行一次 `cmake -S . -B <build_dir>` 即可。
- 若引入新的 `.rc` 文件但未参与编译，请确认它已添加到 `RESOURCE_FILES` 列表，或改为被 `app.rc` 包含。

<!-- 取消 Python 构建脚本说明：已统一使用 CMake + MinGW 构建 -->

### 代码规范
项目遵循以下代码规范：
- **命名约定**: 使用匈牙利命名法
- **注释标准**: 使用Doxygen格式的文档注释
- **错误处理**: 统一的异常处理机制，使用std::optional进行安全错误处理
- **内存管理**: 使用智能指针管理资源
- **现代C++**: 采用C++17特性，包括结构化绑定、string_view、optional等

## 📖 用户手册

### 基本操作

#### 扫描程序
- 程序启动时自动扫描
- 手动刷新：点击"刷新"按钮或按F5
- 搜索程序：在搜索框中输入程序名称

#### 卸载程序
1. 选择要卸载的程序
2. 点击"卸载"按钮
3. 确认卸载操作
4. 等待卸载完成

#### 程序管理
- **查看属性**: 右键点击程序，选择"属性"
- **打开位置**: 右键点击程序，选择"打开安装位置"
- **批量操作**: 使用Ctrl/Shift键选择多个程序

### 高级功能

#### 设置配置
通过"工具" → "设置"菜单访问设置对话框：

**常规设置**
- 启动行为：自动扫描、显示隐藏程序
- 用户交互：确认对话框、自动刷新
- 系统集成：系统托盘、开始菜单

**高级设置**
- 深度清理：清理注册表、文件夹、快捷方式
- 安全选项：创建还原点、备份注册表
- 验证选项：数字签名验证

**性能设置**
- 多线程：工作线程数量
- 缓存：内存缓存大小
- 日志：日志级别和详细程度

#### 导入导出设置
- **导出设置**: 备份当前配置到文件
- **导入设置**: 从文件恢复配置
- **重置设置**: 恢复为默认配置

## 🔧 故障排除

### 常见问题

#### 程序无法启动
1. 检查系统要求是否满足
2. 以管理员身份运行程序
3. 检查防病毒软件是否阻止程序运行

#### 扫描失败
1. 确保有足够的系统权限
2. 检查注册表访问权限
3. 重启程序后重试

#### 卸载失败
1. 确保程序未被其他进程占用
2. 尝试以管理员身份运行
3. 使用"强制卸载"功能

### 日志分析
程序会生成详细的日志文件，位于：
```
%APPDATA%\YG Uninstaller\logs\yguninstaller.log
```

日志级别说明：
- **ERROR**: 严重错误，需要立即处理
- **WARNING**: 警告信息，可能影响功能
- **INFO**: 一般信息，记录正常操作
- **DEBUG**: 调试信息，用于问题诊断

## 🤝 贡献指南

我们欢迎社区贡献！请遵循以下步骤：

### 报告问题
1. 检查现有问题列表
2. 提供详细的错误描述
3. 包含系统信息和日志文件

### 提交代码
1. Fork 项目仓库
2. 创建功能分支
3. 提交代码变更
4. 创建 Pull Request

### 代码审查
所有提交的代码都会经过审查，确保：
- 代码质量和规范性
- 功能正确性和完整性
- 性能影响评估
- 文档更新

## 📄 许可证

本项目采用 [MIT许可证](LICENSE.txt) 开源协议。

## 🙏 致谢

感谢所有为项目做出贡献的开发者和用户！

### 特别感谢
- Windows API 文档和示例
- 开源社区的支持和建议
- 测试用户的反馈和报告

## 📞 联系我们

- **项目主页**: [GitHub Repository](https://github.com/gitchzh/YG-uninstaller)
- **问题报告**: [Issues](https://github.com/gitchzh/YG-uninstaller/issues)
- **功能请求**: [Feature Requests](https://github.com/gitchzh/YG-uninstaller/issues/new?template=feature_request.md)
- **邮箱**: gmrchzh@gmail.com

---

**© 2025 YG Software. All rights reserved.**