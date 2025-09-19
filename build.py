#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
YG Uninstaller 构建脚本
一键构建运行

使用方法:
  python build.py                 # 默认：构建并运行程序
  python build.py --run           # 仅运行程序
  python build.py --clean         # 清理构建文件
  python build.py --build-only    # 仅构建项目
"""

import os
import sys
import subprocess
import argparse
from pathlib import Path
import time
import shutil

class SimpleBuilder:
    """简化构建器类"""
    
    def __init__(self):
        self.project_root = Path(__file__).parent.absolute()
        self.output_dir = self.project_root / "output"
        self.src_dir = self.project_root / "src"
        self.include_dir = self.project_root / "include"
        
        # 源文件列表
        self.source_files = [
            "src/main.cpp",
            "src/core/Common.cpp", 
            "src/core/Config.cpp",
            "src/core/Logger.cpp",
            "src/core/ErrorHandler.cpp",
            "src/ui/MainWindow.cpp",
            "src/services/ProgramDetector.cpp",
            "src/services/UninstallerService.cpp",
            "src/services/DirectProgramScanner.cpp",
            "src/utils/RegistryHelper.cpp",
        ]
        
        # 资源文件
        self.resource_files = [
            "resources/app.rc"
        ]
        
    def log(self, message: str, level: str = "INFO"):
        """输出日志"""
        timestamp = time.strftime("%H:%M:%S")
        print(f"[{timestamp}] [{level}] {message}")
    
    def kill_running_processes(self):
        """强制终止正在运行的程序进程"""
        killed_any = False
        
        # 方法1: 使用taskkill命令终止具体的exe文件（静默模式）
        exe_files = ["YGUninstaller.exe", "YGUninstaller_*.exe"]
        for exe_pattern in exe_files:
            try:
                cmd = ["taskkill", "/f", "/im", exe_pattern, "/t"]
                result = subprocess.run(cmd, capture_output=True, text=True)
                if result.returncode == 0:
                    self.log(f"✓ taskkill终止: {exe_pattern}")
                    killed_any = True
            except Exception:
                pass  # 静默处理
        
        # 方法2: 使用PowerShell通配符终止所有相关进程
        try:
            cmd = ["powershell", "-Command", """
            $processes = Get-Process YGUninstaller* -ErrorAction SilentlyContinue
            if ($processes) {
                $processes | ForEach-Object { 
                    Write-Host "终止进程: $($_.Name) (PID: $($_.Id))"
                    Stop-Process -Id $_.Id -Force -ErrorAction SilentlyContinue 
                }
                Write-Host "进程终止完成"
            }
            """]
            result = subprocess.run(cmd, capture_output=True, text=True, timeout=10)
            if "进程终止完成" in result.stdout:
                self.log("✓ PowerShell通配符终止完成")
                killed_any = True
        except Exception as e:
            self.log(f"PowerShell通配符终止失败: {e}", "DEBUG")
        
        # 方法3: 使用wmic命令终止进程（备用方法）
        try:
            cmd = ["wmic", "process", "where", "name like '%YGUninstaller%'", "delete"]
            result = subprocess.run(cmd, capture_output=True, text=True, timeout=10)
            if result.returncode == 0 and "deleted successfully" in result.stdout:
                self.log("✓ wmic终止进程完成")
                killed_any = True
        except Exception as e:
            self.log(f"wmic终止失败: {e}", "DEBUG")
        
        if killed_any:
            self.log("✓ 进程终止操作完成")
            # 等待进程完全释放文件句柄
            time.sleep(3)
        else:
            self.log("没有发现需要终止的进程", "INFO")
        
    def clean(self):
        """清理构建文件"""
        self.log("清理构建文件...")
        
        # 删除输出目录下的所有可执行文件
        if self.output_dir.exists():
            for exe_file in self.output_dir.glob("*.exe"):
                # 尝试多次删除，使用更强力的方法
                deleted = False
                for attempt in range(5):  # 增加到5次尝试
                    try:
                        exe_file.unlink()
                        self.log(f"✓ 已删除旧程序: {exe_file.name}")
                        deleted = True
                        break
                    except Exception as e:
                        if attempt < 4:  # 前4次尝试失败时等待并重新终止进程
                            self.log(f"删除失败 (尝试 {attempt + 1}/5): {exe_file.name}", "DEBUG")
                            # 再次尝试终止可能的进程
                            try:
                                subprocess.run([
                                    "powershell", "-Command", 
                                    f"Get-Process | Where-Object {{$_.Path -eq '{exe_file.absolute()}'}} | Stop-Process -Force -ErrorAction SilentlyContinue"
                                ], capture_output=True, text=True, timeout=5)
                            except:
                                pass
                            time.sleep(1)
                        else:  # 最后一次尝试失败
                            self.log(f"✗ 无法删除 {exe_file.name}: {e}", "WARN")
                            self.log("  提示：文件可能被系统或安全软件保护", "WARN")
                
                # 如果删除失败，尝试重命名文件（为编译让路）
                if not deleted:
                    try:
                        # 先删除可能存在的旧备份文件
                        backup_name = exe_file.with_suffix('.exe.old')
                        if backup_name.exists():
                            backup_name.unlink()
                        
                        # 重命名当前文件
                        exe_file.rename(backup_name)
                        self.log(f"✓ 已重命名文件: {exe_file.name} -> {backup_name.name}", "INFO")
                    except Exception as e:
                        self.log(f"✗ 重命名也失败: {e}", "WARN")
                        # 最后尝试：生成带时间戳的备份文件名
                        try:
                            import datetime
                            timestamp = datetime.datetime.now().strftime("%H%M%S")
                            backup_name = exe_file.with_suffix(f'.exe.bak{timestamp}')
                            exe_file.rename(backup_name)
                            self.log(f"✓ 已重命名为时间戳文件: {backup_name.name}", "INFO")
                        except Exception as e2:
                            self.log(f"✗ 时间戳重命名也失败: {e2}", "ERROR")
        
        # 删除资源编译产生的对象文件
        if self.output_dir.exists():
            for obj_file in self.output_dir.glob("*.o"):
                try:
                    obj_file.unlink()
                    self.log(f"✓ 已删除对象文件: {obj_file.name}")
                except Exception as e:
                    self.log(f"✗ 无法删除 {obj_file.name}: {e}", "WARN")
        
        # 删除项目根目录下的临时文件
        for pattern in ["*.obj", "*.o", "*.tmp"]:
            for file in self.project_root.glob(pattern):
                try:
                    file.unlink()
                    self.log(f"✓ 已删除临时文件: {file.name}")
                except Exception as e:
                    self.log(f"✗ 无法删除 {file.name}: {e}", "WARN")
        
        # 删除可能的编译缓存目录
        cache_dirs = [".cache", "build", "CMakeFiles"]
        for cache_dir in cache_dirs:
            cache_path = self.project_root / cache_dir
            if cache_path.exists() and cache_path.is_dir():
                try:
                    shutil.rmtree(cache_path)
                    self.log(f"✓ 已删除缓存目录: {cache_dir}")
                except Exception as e:
                    self.log(f"✗ 无法删除缓存目录 {cache_dir}: {e}", "WARN")
                
        self.log("✓ 清理完成")
        
    def build(self):
        """构建项目"""
        self.log("开始构建项目...")
        
        # 构建前首先强制终止所有相关进程
        self.log("检查并终止正在运行的程序进程...")
        self.kill_running_processes()
        
        # 构建前自动清理旧文件
        self.clean()
        
        # 创建输出目录
        self.output_dir.mkdir(parents=True, exist_ok=True)
        
        # 先编译资源文件
        resource_objects = []
        for rc_file in self.resource_files:
            if Path(rc_file).exists():
                rc_output = self.output_dir / (Path(rc_file).stem + ".o")
                # 使用windres编译资源文件
                windres_cmd = ["windres", "-i", str(rc_file), "-o", str(rc_output), 
                              "--input-format=rc", "--output-format=coff"]
                try:
                    result = subprocess.run(windres_cmd, cwd=self.project_root, 
                                          capture_output=True, text=True, encoding='utf-8', errors='replace')
                    if result.returncode == 0:
                        resource_objects.append(str(rc_output))
                        self.log(f"✓ 资源文件编译成功: {rc_file}")
                    else:
                        self.log(f"✗ 资源文件编译失败: {rc_file}", "WARN")
                        self.log(f"错误信息: {result.stderr}", "WARN")
                except Exception as e:
                    self.log(f"✗ 资源编译错误: {e}", "WARN")
        
        # 构建编译命令
        cmd = [
            "g++", "-std=c++11", "-Wall", "-g", "-O0", 
            "-DUNICODE", "-D_UNICODE", "-finput-charset=utf-8", 
            "-fexec-charset=utf-8", f"-I{self.include_dir}", "-municode"
        ]
        
        # 添加源文件
        for src_file in self.source_files:
            if Path(src_file).exists():
                cmd.append(str(src_file))
            else:
                self.log(f"警告：源文件不存在 {src_file}", "WARN")
        
        # 添加编译好的资源对象文件
        cmd.extend(resource_objects)
        
        # 输出文件
        exe_path = self.output_dir / "YGUninstaller.exe"
        cmd.extend(["-o", str(exe_path)])
        
        # 链接库
        cmd.extend([
            "-ladvapi32", "-lshell32", "-luser32", "-lkernel32", 
            "-lole32", "-loleaut32", "-luuid", "-lcomctl32", 
            "-lgdi32", "-lversion", "-mwindows"
        ])
        
        # 执行编译
        self.log(f"编译命令: {' '.join(cmd)}")
        
        try:
            result = subprocess.run(cmd, cwd=self.project_root, 
                                  capture_output=True, text=True, encoding='utf-8', errors='replace')
            
            if result.returncode == 0:
                self.log("✓ 编译成功")
                return True
            else:
                # 如果编译失败且是权限问题，尝试再次终止进程
                if "Permission denied" in result.stderr or "拒绝访问" in result.stderr:
                    self.log("检测到权限问题，尝试强制终止进程...", "WARN")
                    self.kill_running_processes()
                    
                    # 再次尝试编译
                    self.log("重新尝试编译...")
                    result2 = subprocess.run(cmd, cwd=self.project_root, 
                                           capture_output=True, text=True, encoding='utf-8', errors='replace')
                    
                    if result2.returncode == 0:
                        self.log("✓ 重新编译成功")
                        return True
                    else:
                        self.log("✗ 重新编译仍然失败", "ERROR")
                        if result2.stderr:
                            self.log(f"编译错误:\n{result2.stderr}", "ERROR")
                        return False
                else:
                    self.log("✗ 编译失败", "ERROR")
                    if result.stderr:
                        self.log(f"编译错误:\n{result.stderr}", "ERROR")
                    return False
                
        except Exception as e:
            self.log(f"✗ 编译过程发生错误: {e}", "ERROR")
            return False
            
    def run_program(self):
        """运行构建好的程序"""
        self.log("准备运行程序...")
        
        exe_path = self.output_dir / "YGUninstaller.exe"
        
        if not exe_path.exists():
            self.log("✗ 未找到可执行文件，请先构建项目", "ERROR")
            return False
            
        self.log(f"✓ 找到可执行文件: {exe_path}")
        
        try:
            self.log("🚀 启动程序...")
            # 以管理员权限启动程序
            subprocess.Popen([
                "powershell", "-Command", 
                f"Start-Process -FilePath '{exe_path}' -Verb RunAs"
            ], cwd=exe_path.parent)
            
            self.log("✓ 程序已启动")
            return True
        except Exception as e:
            self.log(f"✗ 启动程序失败: {e}", "ERROR")
            return False
            
    def build_and_run(self):
        """构建并运行程序"""
        self.log("开始构建并运行流程...")
        start_time = time.time()
        
        try:
            # 构建
            if not self.build():
                return False
                
            # 运行
            if not self.run_program():
                return False
                
            elapsed = time.time() - start_time
            self.log(f"✓ 构建并运行流程完成，总耗时: {elapsed:.2f} 秒")
            return True
            
        except Exception as e:
            self.log(f"✗ 构建运行过程中发生错误: {e}", "ERROR")
            return False

def main():
    """主函数"""
    parser = argparse.ArgumentParser(description="YG Uninstaller 构建脚本")
    parser.add_argument("--clean", action="store_true", help="清理构建文件")
    parser.add_argument("--build-only", action="store_true", help="仅构建项目")
    parser.add_argument("--run", "-r", action="store_true", help="仅运行程序（不构建）")
    
    args = parser.parse_args()
    
    builder = SimpleBuilder()
    
    try:
        if args.clean:
            builder.clean()
            return 0
        elif args.build_only:
            success = builder.build()
        elif args.run:
            success = builder.run_program()
        else:
            # 默认：构建并运行
            success = builder.build_and_run()
            
        return 0 if success else 1
        
    except KeyboardInterrupt:
        builder.log("构建被用户中断", "WARN")
        return 130
    except Exception as e:
        builder.log(f"构建脚本发生未处理的错误: {e}", "ERROR")
        return 1

if __name__ == "__main__":
    sys.exit(main())
