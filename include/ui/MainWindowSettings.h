/**
 * @file MainWindowSettings.h
 * @brief 主窗口设置管理模块头文件
 * 
 * 该文件定义了YG Uninstaller应用程序的设置管理功能，包括：
 * - 用户设置的存储和管理
 * - 设置对话框的显示和交互
 * - 设置的导入导出功能
 * - 设置数据的验证和应用
 * 
 * @details
 * 该模块采用单一职责原则，专门负责应用程序的所有设置相关功能。
 * 通过分离设置管理逻辑，提高了代码的可维护性和可测试性。
 * 
 * @author YG Software Team
 * @email gmrchzh@gmail.com
 * @version 2.0.0
 * @date 2025-01-21
 * @copyright Copyright (c) 2025 YG Software. All rights reserved.
 * 
 * @see MainWindow.h 主窗口类定义
 * @see Config.h 配置管理类
 * @see Logger.h 日志管理类
 */

#pragma once

#include "core/Common.h"
#include <windows.h>

namespace YG {

    // 前向声明
    class MainWindow;

    /**
     * @brief 应用程序设置数据结构
     * 
     * 该结构体定义了应用程序的所有可配置选项，按照功能类别进行分组。
     * 每个成员都有合理的默认值，确保应用程序在首次运行时能够正常工作。
     * 
     * @details
     * 设置项分类：
     * - 启动行为：控制应用程序启动时的行为
     * - 用户交互：控制用户操作时的确认和反馈
     * - 高级功能：控制高级清理和系统集成功能
     * - 性能调优：控制多线程、缓存和日志级别
     * 
     * @note 修改默认值时需要考虑向后兼容性
     */
    struct SettingsData {
        // ==================== 启动行为设置 ====================
        
        /** @brief 启动时自动扫描已安装程序 (默认: true) */
        bool autoScanOnStartup = true;
        
        /** @brief 显示隐藏的系统组件 (默认: false) */
        bool showHiddenPrograms = false;
        
        /** @brief 启动时最小化到系统托盘 (默认: false) */
        bool startMinimized = false;
        
        /** @brief 关闭窗口时最小化到系统托盘而不是退出 (默认: false) */
        bool closeToTray = false;
        
        // ==================== 用户交互设置 ====================
        
        /** @brief 卸载前显示确认对话框 (默认: true) */
        bool confirmUninstall = true;
        
        /** @brief 卸载完成后自动刷新程序列表 (默认: true) */
        bool autoRefreshAfterUninstall = true;
        
        /** @brief 保留卸载操作日志 (默认: true) */
        bool keepUninstallLogs = true;
        
        /** @brief 在扫描时包含系统组件 (默认: false) */
        bool includeSystemComponents = false;
        
        // ==================== 高级功能设置 ====================
        
        /** @brief 启用深度清理模式 (默认: false) */
        bool enableDeepClean = false;
        
        /** @brief 清理注册表残留项 (默认: false) */
        bool cleanRegistry = false;
        
        /** @brief 清理程序文件夹残留 (默认: false) */
        bool cleanFolders = false;
        
        /** @brief 清理开始菜单快捷方式 (默认: false) */
        bool cleanStartMenu = false;
        
        /** @brief 创建系统还原点 (默认: false) */
        bool createRestorePoint = false;
        
        /** @brief 备份注册表 (默认: false) */
        bool backupRegistry = false;
        
        /** @brief 验证程序数字签名 (默认: false) */
        bool verifySignature = false;
        
        /** @brief 启用多线程处理 (默认: true) */
        bool enableMultiThread = true;
        
        /** @brief 启用详细日志记录 (默认: false) */
        bool verboseLogging = false;
        
        // ==================== 性能调优设置 ====================
        
        /** @brief 工作线程数量 (默认: 4, 范围: 1-16) */
        int threadCount = 4;
        
        /** @brief 缓存大小(MB) (默认: 100, 范围: 10-1000) */
        int cacheSize = 100;
        
        /** @brief 日志记录级别 (默认: 2, 0=错误, 1=警告, 2=信息, 3=调试) */
        int logLevel = 2;
    };

    /**
     * @brief 主窗口设置管理类
     * 
     * 该类负责管理YG Uninstaller应用程序的所有用户设置功能，采用单一职责原则
     * 将设置相关的逻辑从主窗口类中分离出来，提高代码的可维护性和可测试性。
     * 
     * @details
     * 主要功能包括：
     * - 设置对话框的创建、显示和管理
     * - 设置数据的加载、保存和持久化
     * - 设置的导入导出功能
     * - 设置控件的初始化和数据绑定
     * - 设置验证和应用逻辑
     * 
     * @design_patterns
     * - 单一职责原则：专门负责设置管理
     * - 依赖注入：通过构造函数注入主窗口依赖
     * - 封装：隐藏设置管理的内部实现细节
     * 
     * @thread_safety
     * 该类不是线程安全的，所有操作都应在主UI线程中执行。
     * 
     * @example
     * ```cpp
     * // 创建设置管理器
     * auto settingsManager = std::make_unique<MainWindowSettings>(mainWindow);
     * 
     * // 显示设置对话框
     * settingsManager->ShowSettingsDialog();
     * 
     * // 获取当前设置
     * const auto& settings = settingsManager->GetSettings();
     * ```
     */
    class MainWindowSettings {
    public:
        /**
         * @brief 构造函数
         * 
         * 初始化设置管理器，建立与主窗口的关联关系。
         * 
         * @param mainWindow 主窗口实例的指针，用于回调通知和状态更新
         * 
         * @pre mainWindow 不能为 nullptr
         * @post 设置管理器已初始化，可以正常使用
         * 
         * @throws std::invalid_argument 如果 mainWindow 为 nullptr
         */
        explicit MainWindowSettings(MainWindow* mainWindow);
        
        /**
         * @brief 析构函数
         * 
         * 清理设置管理器资源，确保所有设置已保存。
         * 使用默认析构函数，因为该类管理的是简单的值类型数据。
         */
        ~MainWindowSettings() = default;
        
        // ==================== 对话框显示接口 ====================
        
        /**
         * @brief 显示主设置对话框
         * 
         * 显示包含所有设置选项的主设置对话框，用户可以通过该对话框
         * 修改应用程序的各种配置选项。
         * 
         * @details
         * 该对话框采用选项卡式设计，将设置按功能分类显示：
         * - 常规设置：基本的行为和显示选项
         * - 高级设置：高级功能和系统集成选项
         * - 性能设置：多线程、缓存和日志配置
         * 
         * @post 如果用户确认修改，设置将被保存并应用
         * @note 该方法是模态对话框，会阻塞调用线程直到对话框关闭
         */
        void ShowSettingsDialog();
        
        /**
         * @brief 显示常规设置对话框
         * 
         * 显示包含基本设置选项的对话框，如启动行为、用户交互等。
         * 
         * @post 如果用户确认修改，常规设置将被保存并应用
         */
        void ShowGeneralSettingsDialog();
        
        /**
         * @brief 显示高级设置对话框
         * 
         * 显示包含高级功能选项的对话框，如深度清理、系统集成等。
         * 
         * @warning 高级设置可能影响系统稳定性，建议高级用户使用
         * @post 如果用户确认修改，高级设置将被保存并应用
         */
        void ShowAdvancedSettingsDialog();
        
        /**
         * @brief 显示选项卡式设置对话框
         * 
         * 显示包含多个选项卡的设置对话框，提供更丰富的用户界面。
         * 
         * @details
         * 该对话框支持：
         * - 动态选项卡切换
         * - 实时设置预览
         * - 设置验证和错误提示
         * 
         * @post 如果用户确认修改，所有设置将被保存并应用
         */
        void ShowTabbedSettingsDialog();
        
        // ==================== 设置数据管理接口 ====================
        
        /**
         * @brief 从配置文件加载设置
         * 
         * 从持久化存储中读取用户设置，如果配置文件不存在或损坏，
         * 将使用默认设置值。
         * 
         * @details
         * 加载过程包括：
         * - 验证配置文件的有效性
         * - 解析设置数据
         * - 验证设置值的合法性
         * - 应用默认值（对于缺失或无效的设置项）
         * 
         * @post 设置数据已从存储中加载并更新到内存
         * @note 该方法通常在应用程序启动时调用
         */
        void LoadSettings();
        
        /**
         * @brief 保存设置到配置文件
         * 
         * 将当前的设置数据持久化到配置文件中，确保用户设置
         * 在应用程序重启后仍然有效。
         * 
         * @details
         * 保存过程包括：
         * - 验证设置数据的完整性
         * - 序列化设置数据
         * - 写入配置文件
         * - 创建备份文件（防止数据丢失）
         * 
         * @post 设置数据已保存到持久化存储
         * @throws std::runtime_error 如果保存过程中发生错误
         */
        void SaveSettings();
        
        /**
         * @brief 应用当前设置
         * 
         * 将内存中的设置数据应用到应用程序的各个模块，
         * 使设置更改立即生效。
         * 
         * @details
         * 应用过程包括：
         * - 通知相关模块设置已更改
         * - 更新UI状态
         * - 重新配置系统集成功能
         * - 调整性能参数
         * 
         * @post 所有相关模块已应用新的设置
         */
        void ApplySettings();
        
        /**
         * @brief 导出设置到文件
         * 
         * 将当前设置导出到用户指定的文件中，支持设置备份和
         * 在多台计算机间同步设置。
         * 
         * @details
         * 导出功能包括：
         * - 显示文件选择对话框
         * - 验证导出路径的有效性
         * - 序列化设置数据
         * - 创建可读的配置文件
         * 
         * @post 如果用户确认，设置已导出到指定文件
         * @throws std::runtime_error 如果导出过程中发生错误
         */
        void ExportSettings();
        
        /**
         * @brief 从文件导入设置
         * 
         * 从用户选择的配置文件中导入设置，支持从备份文件
         * 恢复设置或在多台计算机间同步设置。
         * 
         * @details
         * 导入功能包括：
         * - 显示文件选择对话框
         * - 验证导入文件的有效性
         * - 解析设置数据
         * - 验证设置值的合法性
         * - 应用导入的设置
         * 
         * @return bool 导入是否成功
         * @retval true 设置导入成功
         * @retval false 导入失败（用户取消或文件错误）
         * 
         * @post 如果成功，设置已从文件导入并应用
         */
        bool ImportSettings();
        
        // ==================== 设置数据访问接口 ====================
        
        /**
         * @brief 获取当前设置数据
         * 
         * 返回当前内存中设置数据的常量引用，用于读取设置值。
         * 
         * @return const SettingsData& 设置数据的常量引用
         * 
         * @note 返回的是引用，修改返回的数据会影响内部状态
         * @see SetSettings() 用于修改设置数据
         */
        const SettingsData& GetSettings() const { return m_settings; }
        
        /**
         * @brief 设置当前设置数据
         * 
         * 用新的设置数据替换当前设置，通常用于批量更新设置。
         * 
         * @param settings 新的设置数据
         * 
         * @post 内部设置数据已更新为新值
         * @note 调用此方法后建议调用 ApplySettings() 使更改生效
         * @see GetSettings() 用于获取当前设置数据
         */
        void SetSettings(const SettingsData& settings) { m_settings = settings; }
        
        /**
         * @brief 重置为默认设置
         * 
         * 将所有设置项重置为默认值，用于恢复出厂设置。
         * 
         * @details
         * 重置过程包括：
         * - 将所有设置项设为默认值
         * - 清除用户自定义配置
         * - 应用默认设置
         * 
         * @post 所有设置已重置为默认值
         * @warning 此操作不可撤销，会丢失所有用户自定义设置
         */
        void ResetToDefaultSettings();
        
        /**
         * @brief 获取当前用户名
         * 
         * 获取当前登录用户的用户名，用于设置导出时的文件命名。
         * 
         * @return String 当前用户的用户名
         * 
         * @details
         * 用户名获取方式：
         * - 优先使用环境变量 USERNAME
         * - 如果环境变量不可用，使用 GetUserName API
         * - 如果都失败，返回 "Unknown"
         * 
         * @post 返回有效的用户名字符串
         */
        String GetCurrentUserName() const;
        
        // ==================== 对话框过程函数 ====================
        
        /**
         * @brief 常规设置对话框消息处理过程
         * 
         * 处理常规设置对话框的所有Windows消息，包括控件初始化、
         * 用户输入验证和设置应用等。
         * 
         * @param hDlg 对话框窗口句柄
         * @param message Windows消息类型
         * @param wParam 消息参数1
         * @param lParam 消息参数2
         * 
         * @return INT_PTR 消息处理结果
         * @retval TRUE 消息已处理
         * @retval FALSE 消息未处理，调用默认处理
         * 
         * @details
         * 支持的消息类型：
         * - WM_INITDIALOG: 初始化对话框控件
         * - WM_COMMAND: 处理按钮点击和控件事件
         * - WM_CLOSE: 处理对话框关闭
         * 
         * @note 这是一个静态函数，通过GetWindowLongPtr获取this指针
         */
        static INT_PTR CALLBACK GeneralSettingsDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
        
        /**
         * @brief 高级设置对话框消息处理过程
         * 
         * 处理高级设置对话框的所有Windows消息，专门用于管理
         * 高级功能相关的设置选项。
         * 
         * @param hDlg 对话框窗口句柄
         * @param message Windows消息类型
         * @param wParam 消息参数1
         * @param lParam 消息参数2
         * 
         * @return INT_PTR 消息处理结果
         * @retval TRUE 消息已处理
         * @retval FALSE 消息未处理，调用默认处理
         * 
         * @warning 高级设置可能影响系统稳定性，需要额外的用户确认
         */
        static INT_PTR CALLBACK AdvancedSettingsDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
        
        /**
         * @brief 选项卡式设置对话框消息处理过程
         * 
         * 处理多选项卡设置对话框的所有Windows消息，支持动态
         * 选项卡切换和复杂设置的交互式配置。
         * 
         * @param hDlg 对话框窗口句柄
         * @param message Windows消息类型
         * @param wParam 消息参数1
         * @param lParam 消息参数2
         * 
         * @return INT_PTR 消息处理结果
         * @retval TRUE 消息已处理
         * @retval FALSE 消息未处理，调用默认处理
         * 
         * @details
         * 特殊处理的消息：
         * - TCN_SELCHANGE: 选项卡切换事件
         * - WM_SIZE: 对话框大小调整事件
         * - WM_NOTIFY: 选项卡控件通知消息
         */
        static INT_PTR CALLBACK TabbedSettingsDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
        
    private:
        // ==================== 内部辅助函数 ====================
        
        /**
         * @brief 从对话框控件加载设置到成员变量
         * 
         * 读取对话框中所有控件的当前值，并将这些值更新到
         * 内部设置数据结构中。
         * 
         * @param hDlg 对话框窗口句柄
         * 
         * @details
         * 加载过程包括：
         * - 读取复选框的选中状态
         * - 读取编辑框的文本内容
         * - 读取下拉列表的选择项
         * - 验证输入数据的有效性
         * - 更新内部设置数据结构
         * 
         * @post 内部设置数据已与对话框控件同步
         * @throws std::invalid_argument 如果控件数据无效
         */
        void LoadSettingsFromDialog(HWND hDlg);
        
        /**
         * @brief 从成员变量保存设置到对话框控件
         * 
         * 将内部设置数据同步到对话框的所有控件中，
         * 确保界面显示与当前设置一致。
         * 
         * @param hDlg 对话框窗口句柄
         * 
         * @details
         * 同步过程包括：
         * - 设置复选框的选中状态
         * - 设置编辑框的文本内容
         * - 设置下拉列表的选择项
         * - 更新控件的状态和可用性
         * 
         * @post 对话框控件已与内部设置数据同步
         */
        void SaveSettingsToDialog(HWND hDlg);
        
        /**
         * @brief 初始化设置对话框控件
         * 
         * 设置对话框控件的初始状态，包括设置默认值、
         * 配置控件样式和绑定事件处理程序。
         * 
         * @param hDlg 对话框窗口句柄
         * 
         * @details
         * 初始化过程包括：
         * - 设置控件的默认值
         * - 配置控件的样式和属性
         * - 设置控件的工具提示
         * - 绑定事件处理程序
         * - 设置控件的初始可用性状态
         * 
         * @post 所有对话框控件已正确初始化
         */
        void InitializeSettingsControls(HWND hDlg);
        
        /**
         * @brief 显示指定选项卡的设置内容
         * 
         * 根据选项卡索引显示对应的设置选项，支持动态
         * 选项卡内容的加载和切换。
         * 
         * @param hDlg 对话框窗口句柄
         * @param tabIndex 选项卡索引 (0=常规, 1=高级, 2=性能)
         * 
         * @details
         * 选项卡内容：
         * - 0: 常规设置（启动行为、用户交互）
         * - 1: 高级设置（深度清理、系统集成）
         * - 2: 性能设置（多线程、缓存、日志）
         * 
         * @post 指定选项卡的内容已正确显示
         * @throws std::out_of_range 如果选项卡索引无效
         */
        void ShowSettingsTabContent(HWND hDlg, int tabIndex);
        
        /**
         * @brief 保存选项卡式设置对话框的设置
         * 
         * 从选项卡式对话框中收集所有设置项的值，
         * 并保存到内部设置数据结构中。
         * 
         * @param hDlg 对话框窗口句柄
         * 
         * @details
         * 保存过程包括：
         * - 遍历所有选项卡
         * - 读取每个选项卡的控件值
         * - 验证设置数据的有效性
         * - 更新内部设置数据结构
         * 
         * @post 所有选项卡的设置已保存到内部数据结构
         * @throws std::invalid_argument 如果设置数据无效
         */
        void SaveTabbedSettings(HWND hDlg);
        
        /**
         * @brief 加载选项卡式设置对话框的设置
         * 
         * 将内部设置数据加载到选项卡式对话框的各个
         * 选项卡中，确保界面显示正确。
         * 
         * @param hDlg 对话框窗口句柄
         * 
         * @details
         * 加载过程包括：
         * - 遍历所有选项卡
         * - 将设置数据同步到各个选项卡的控件
         * - 设置控件的初始状态
         * - 更新控件的可用性
         * 
         * @post 所有选项卡的控件已正确加载设置数据
         */
        void LoadTabbedSettings(HWND hDlg);
        
        /**
         * @brief 调整选项卡式设置对话框的大小
         * 
         * 当对话框大小改变时，调整所有选项卡内容的大小
         * 和位置，确保界面布局正确。
         * 
         * @param hDlg 对话框窗口句柄
         * @param width 新的对话框宽度
         * @param height 新的对话框高度
         * 
         * @details
         * 调整过程包括：
         * - 计算新的控件位置和大小
         * - 调整选项卡控件的大小
         * - 重新布局各个选项卡的内容
         * - 更新滚动条（如果需要）
         * 
         * @post 对话框布局已适应新的尺寸
         */
        void ResizeTabbedSettingsDialog(HWND hDlg, int width, int height);
        
    private:
        // ==================== 成员变量 ====================
        
        /** @brief 主窗口实例指针，用于回调通知和状态更新 */
        MainWindow* m_mainWindow;
        
        /** @brief 当前设置数据，存储所有用户配置选项 */
        SettingsData m_settings;
        
        // ==================== 禁用操作 ====================
        
        /**
         * @brief 禁用拷贝构造函数
         * 
         * 该类管理主窗口的依赖关系，拷贝可能导致意外的行为，
         * 因此禁用拷贝构造。
         */
        MainWindowSettings(const MainWindowSettings&) = delete;
        
        /**
         * @brief 禁用赋值操作符
         * 
         * 该类管理主窗口的依赖关系，赋值可能导致意外的行为，
         * 因此禁用赋值操作。
         */
        MainWindowSettings& operator=(const MainWindowSettings&) = delete;
    };

} // namespace YG
