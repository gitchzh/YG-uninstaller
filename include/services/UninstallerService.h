/**
 * @file UninstallerService.h
 * @brief 程序卸载服务
 * @author YG Software
 * @version 1.0.0
 * @date 2025-09-17
 */

#pragma once

#include "core/Common.h"
#include "core/Logger.h"
#include <functional>
#include <thread>
#include <atomic>
#include <queue>
#include <condition_variable>
#include <mutex>
#include <unordered_map>

namespace YG {
    
    /**
     * @brief 卸载模式枚举
     */
    enum class UninstallMode {
        Standard,    ///< 标准卸载
        Silent,      ///< 静默卸载
        Force,       ///< 强制卸载
        Deep         ///< 深度清理卸载
    };
    
    /**
     * @brief 卸载状态枚举
     */
    enum class UninstallStatus {
        NotStarted,     ///< 未开始
        Preparing,      ///< 准备中
        Running,        ///< 运行中
        CleaningUp,     ///< 清理中
        Completed,      ///< 已完成
        Failed,         ///< 失败
        Cancelled       ///< 已取消
    };
    
    /**
     * @brief 卸载任务结构
     */
    struct UninstallTask {
        String taskId;              ///< 任务ID
        ProgramInfo program;        ///< 程序信息
        UninstallMode mode;         ///< 卸载模式
        ProgressCallback progress;  ///< 进度回调
        CompletionCallback completion; ///< 完成回调
        UninstallStatus status;     ///< 任务状态
        String statusMessage;       ///< 状态消息
        DWORD startTime;           ///< 开始时间
        
        UninstallTask() : mode(UninstallMode::Standard), status(UninstallStatus::NotStarted), startTime(0) {}
    };
    
    /**
     * @brief 程序卸载服务类
     * 
     * 提供多种卸载模式和深度清理功能
     */
    class UninstallerService {
    public:
        /**
         * @brief 构造函数
         */
        UninstallerService();
        
        /**
         * @brief 析构函数
         */
        ~UninstallerService();
        
        YG_DISABLE_COPY_AND_ASSIGN(UninstallerService);
        
        /**
         * @brief 启动卸载服务
         * @return ErrorCode 操作结果
         */
        ErrorCode Start();
        
        /**
         * @brief 停止卸载服务
         * @return ErrorCode 操作结果
         */
        ErrorCode Stop();
        
        /**
         * @brief 提交卸载任务
         * @param program 程序信息
         * @param mode 卸载模式
         * @param progressCallback 进度回调
         * @param completionCallback 完成回调
         * @return String 任务ID
         */
        String SubmitUninstallTask(const ProgramInfo& program, 
                                 UninstallMode mode = UninstallMode::Standard,
                                 const ProgressCallback& progressCallback = nullptr,
                                 const CompletionCallback& completionCallback = nullptr);
        
        /**
         * @brief 同步卸载程序
         * @param program 程序信息
         * @param mode 卸载模式
         * @param progressCallback 进度回调
         * @return UninstallResult 卸载结果
         */
        UninstallResult UninstallSync(const ProgramInfo& program, 
                                    UninstallMode mode = UninstallMode::Standard,
                                    const ProgressCallback& progressCallback = nullptr);
        
        /**
         * @brief 取消卸载任务
         * @param taskId 任务ID
         * @return ErrorCode 操作结果
         */
        ErrorCode CancelTask(const String& taskId);
        
        /**
         * @brief 获取任务状态
         * @param taskId 任务ID
         * @param task 输出任务信息
         * @return ErrorCode 操作结果
         */
        ErrorCode GetTaskStatus(const String& taskId, UninstallTask& task);
        
        /**
         * @brief 获取所有活动任务
         * @param tasks 输出任务列表
         * @return ErrorCode 操作结果
         */
        ErrorCode GetActiveTasks(std::vector<UninstallTask>& tasks);
        
        /**
         * @brief 清理程序残留
         * @param program 程序信息
         * @param progressCallback 进度回调
         * @return UninstallResult 清理结果
         */
        UninstallResult CleanupRemnants(const ProgramInfo& program,
                                      const ProgressCallback& progressCallback = nullptr);
        
        /**
         * @brief 扫描程序残留
         * @param program 程序信息
         * @param remainingFiles 输出残留文件列表
         * @param remainingRegistries 输出残留注册表项列表
         * @return ErrorCode 操作结果
         */
        ErrorCode ScanRemnants(const ProgramInfo& program,
                             StringVector& remainingFiles,
                             StringVector& remainingRegistries);
        
        /**
         * @brief 强制终止程序进程
         * @param program 程序信息
         * @return ErrorCode 操作结果
         */
        ErrorCode ForceTerminateProcesses(const ProgramInfo& program);
        
        /**
         * @brief 备份程序数据
         * @param program 程序信息
         * @param backupPath 备份路径
         * @return ErrorCode 操作结果
         */
        ErrorCode BackupProgramData(const ProgramInfo& program, const String& backupPath);
        
        /**
         * @brief 恢复程序数据
         * @param backupPath 备份路径
         * @param restorePath 恢复路径
         * @return ErrorCode 操作结果
         */
        ErrorCode RestoreProgramData(const String& backupPath, const String& restorePath);
        
        /**
         * @brief 验证卸载结果
         * @param program 程序信息
         * @return bool 是否完全卸载
         */
        bool ValidateUninstall(const ProgramInfo& program);
        
        /**
         * @brief 设置最大并发任务数
         * @param maxConcurrent 最大并发数
         */
        void SetMaxConcurrentTasks(int maxConcurrent);
        
        /**
         * @brief 启用/禁用自动清理
         * @param enable 是否启用
         */
        void EnableAutoCleanup(bool enable);
        
        /**
         * @brief 启用/禁用备份功能
         * @param enable 是否启用
         */
        void EnableBackup(bool enable);
        
        /**
         * @brief 获取服务统计信息
         * @param totalTasks 总任务数
         * @param successfulTasks 成功任务数
         * @param failedTasks 失败任务数
         * @param averageTime 平均耗时
         */
        void GetStatistics(int& totalTasks, int& successfulTasks, 
                         int& failedTasks, double& averageTime);
        
    private:
        /**
         * @brief 执行标准卸载
         * @param task 卸载任务
         * @return UninstallResult 卸载结果
         */
        UninstallResult ExecuteStandardUninstall(UninstallTask& task);
        
        /**
         * @brief 执行静默卸载
         * @param task 卸载任务
         * @return UninstallResult 卸载结果
         */
        UninstallResult ExecuteSilentUninstall(UninstallTask& task);
        
        /**
         * @brief 执行强制卸载
         * @param task 卸载任务
         * @return UninstallResult 卸载结果
         */
        UninstallResult ExecuteForceUninstall(UninstallTask& task);
        
        /**
         * @brief 执行深度清理卸载
         * @param task 卸载任务
         * @return UninstallResult 卸载结果
         */
        UninstallResult ExecuteDeepUninstall(UninstallTask& task);
        
        /**
         * @brief 运行卸载程序
         * @param uninstallString 卸载命令
         * @param silent 是否静默
         * @param timeout 超时时间
         * @return ErrorCode 操作结果
         */
        ErrorCode RunUninstaller(const String& uninstallString, bool silent, DWORD timeout);
        
        /**
         * @brief 删除程序文件
         * @param installPath 安装路径
         * @param remainingFiles 输出残留文件
         * @return ErrorCode 操作结果
         */
        ErrorCode DeleteProgramFiles(const String& installPath, StringVector& remainingFiles);
        
        /**
         * @brief 清理注册表项
         * @param program 程序信息
         * @param remainingRegistries 输出残留注册表项
         * @return ErrorCode 操作结果
         */
        ErrorCode CleanupRegistryEntries(const ProgramInfo& program, StringVector& remainingRegistries);
        
        /**
         * @brief 清理开始菜单快捷方式
         * @param program 程序信息
         * @return ErrorCode 操作结果
         */
        ErrorCode CleanupStartMenuShortcuts(const ProgramInfo& program);
        
        /**
         * @brief 清理桌面快捷方式
         * @param program 程序信息
         * @return ErrorCode 操作结果
         */
        ErrorCode CleanupDesktopShortcuts(const ProgramInfo& program);
        
        /**
         * @brief 清理临时文件
         * @param program 程序信息
         * @return ErrorCode 操作结果
         */
        ErrorCode CleanupTempFiles(const ProgramInfo& program);
        
        /**
         * @brief 工作线程函数
         */
        void WorkerThread();
        
        /**
         * @brief 处理卸载任务
         * @param task 卸载任务
         */
        void ProcessUninstallTask(UninstallTask& task);
        
        /**
         * @brief 生成任务ID
         * @return String 任务ID
         */
        String GenerateTaskId();
        
        /**
         * @brief 更新任务状态
         * @param taskId 任务ID
         * @param status 新状态
         * @param message 状态消息
         */
        void UpdateTaskStatus(const String& taskId, UninstallStatus status, const String& message = L"");
        
    private:
        std::queue<UninstallTask> m_taskQueue;          ///< 任务队列
        std::vector<std::unique_ptr<std::thread>> m_workerThreads; ///< 工作线程池
        std::unordered_map<String, UninstallTask> m_activeTasks;   ///< 活动任务映射
        
        std::atomic<bool> m_running;                    ///< 是否正在运行
        std::atomic<bool> m_stopRequested;              ///< 是否请求停止
        std::atomic<int> m_nextTaskId;                  ///< 下一个任务ID
        
        int m_maxConcurrentTasks;                       ///< 最大并发任务数
        bool m_autoCleanup;                             ///< 自动清理
        bool m_backupEnabled;                           ///< 备份功能启用
        
        // 统计信息
        std::atomic<int> m_totalTasks;                  ///< 总任务数
        std::atomic<int> m_successfulTasks;             ///< 成功任务数
        std::atomic<int> m_failedTasks;                 ///< 失败任务数
        std::atomic<DWORD64> m_totalTime;               ///< 总耗时
        
        mutable std::mutex m_queueMutex;                ///< 队列锁
        mutable std::mutex m_tasksMutex;                ///< 任务锁
        std::condition_variable m_condition;            ///< 条件变量
        
        // 常量配置
        static constexpr DWORD DEFAULT_TIMEOUT = 300000;  ///< 默认超时时间(5分钟)
        static constexpr int DEFAULT_MAX_CONCURRENT = 2;   ///< 默认最大并发数
    };
    
} // namespace YG
