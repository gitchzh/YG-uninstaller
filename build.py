#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
YG Uninstaller 构建脚本
一键构建运行，不依赖CMake

使用方法:
  python build.py                 # 默认：构建并运行程序
  python build.py --run           # 仅运行程序
  python build.py --clean         # 清理构建文件
  python build.py --build-only    # 仅构建项目
"""

import os
import sys
import subprocess
import shutil
import argparse
from pathlib import Path
import time

class SimpleBuilder:
    """简化构建器类"""
    
    def __init__(self):
        self.project_root = Path(__file__).parent.absolute()
        self.build_dir = self.project_root / "build_simple"
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
        ]
        
        # 编译器配置
        self.compiler = None
        self.compile_flags = []
        self.link_flags = []
        self.latest_exe_path = None
        
    def log(self, message: str, level: str = "INFO"):
        """输出日志"""
        timestamp = time.strftime("%H:%M:%S")
        colors = {
            "INFO": "\033[92m",    # 绿色
            "WARN": "\033[93m",    # 黄色  
            "ERROR": "\033[91m",   # 红色
            "RESET": "\033[0m"     # 重置
        }
        
        color = colors.get(level, colors["INFO"])
        reset = colors["RESET"]
        
        print(f"{color}[{timestamp}] [{level}] {message}{reset}")
        
    def detect_compiler(self):
        """检测可用的编译器"""
        self.log("检测编译器...")
        
        # 检查MSVC (cl.exe)
        try:
            result = subprocess.run(["where", "cl"], capture_output=True, text=True)
            if result.returncode == 0:
                self.compiler = "msvc"
                self.compile_flags = [
                    "/std:c++17",           # C++17标准
                    "/EHsc",                # 异常处理
                    "/W4",                  # 警告级别
                    "/O2",                  # 优化
                    "/DUNICODE",            # Unicode支持
                    "/D_UNICODE",
                    "/I" + str(self.include_dir),  # 包含目录
                ]
                self.link_flags = [
                    "advapi32.lib", "shell32.lib", "user32.lib", 
                    "kernel32.lib", "ole32.lib", "oleaut32.lib",
                    "uuid.lib", "comctl32.lib", "gdi32.lib"
                ]
                self.log("✓ 找到 MSVC 编译器")
                return True
        except Exception:
            pass
            
        # 检查GCC (g++.exe)
        try:
            result = subprocess.run(["where", "g++"], capture_output=True, text=True)
            if result.returncode == 0:
                self.compiler = "gcc"
                self.compile_flags = [
                    "-std=c++14",           # C++14标准（兼容性更好）
                    "-Wall", "-Wextra",     # 警告
                    "-O2",                  # 优化
                    "-DUNICODE",            # Unicode支持
                    "-D_UNICODE",
                    "-I" + str(self.include_dir),  # 包含目录
                    "-municode",            # 支持Unicode入口点
                ]
                self.link_flags = [
                    "-ladvapi32", "-lshell32", "-luser32",
                    "-lkernel32", "-lole32", "-loleaut32", 
                    "-luuid", "-lcomctl32", "-lgdi32",
                    "-mwindows"             # Windows子系统
                ]
                self.log("✓ 找到 GCC 编译器")
                return True
        except Exception:
            pass
            
        self.log("✗ 未找到可用的编译器", "ERROR")
        self.log("请安装 Visual Studio 或 MinGW-w64", "ERROR")
        return False
        
    def clean(self):
        """清理构建文件"""
        self.log("清理构建文件...")
        
        # 删除构建目录
        if self.build_dir.exists():
            shutil.rmtree(self.build_dir)
            self.log(f"已删除: {self.build_dir}")
            
        # 删除输出目录
        if self.output_dir.exists():
            shutil.rmtree(self.output_dir)
            self.log(f"已删除: {self.output_dir}")
            
        # 删除临时文件
        for pattern in ["*.obj", "*.o", "*.exe"]:
            for file in self.project_root.glob(pattern):
                file.unlink()
                self.log(f"已删除: {file}")
                
        self.log("✓ 清理完成")
        
    def build(self):
        """构建项目"""
        self.log("开始构建项目...")
        
        # 先尝试关闭正在运行的程序
        try:
            subprocess.run(["taskkill", "/f", "/im", "YGUninstaller.exe"], 
                          capture_output=True, check=False)
        except:
            pass
        
        # 检测编译器
        if not self.detect_compiler():
            return False
            
        # 创建构建目录
        self.build_dir.mkdir(parents=True, exist_ok=True)
        self.output_dir.mkdir(parents=True, exist_ok=True)
        
        # 构建编译命令
        if self.compiler == "msvc":
            cmd = ["cl"] + self.compile_flags
            
            # 添加源文件
            for src_file in self.source_files:
                if Path(src_file).exists():
                    cmd.append(str(src_file))
                else:
                    self.log(f"警告：源文件不存在 {src_file}", "WARN")
            
            # 输出文件
            exe_path = self.output_dir / "YGUninstaller.exe"
            cmd.extend(["/Fe:" + str(exe_path)])
            
            # 链接库
            cmd.extend(self.link_flags)
            
        elif self.compiler == "gcc":
            cmd = ["g++"] + self.compile_flags
            
            # 添加源文件
            for src_file in self.source_files:
                if Path(src_file).exists():
                    cmd.append(str(src_file))
                else:
                    self.log(f"警告：源文件不存在 {src_file}", "WARN")
            
            # 输出文件（添加时间戳避免文件占用问题）
            import time
            timestamp = str(int(time.time()))
            exe_path = self.output_dir / f"YGUninstaller_{timestamp}.exe"
            cmd.extend(["-o", str(exe_path)])
            
            # 记录最新的可执行文件路径
            self.latest_exe_path = exe_path
            
            # 链接库
            cmd.extend(self.link_flags)
        
        # 执行编译
        self.log(f"编译命令: {' '.join(cmd)}")
        
        try:
            result = subprocess.run(cmd, cwd=self.project_root, 
                                  capture_output=True, text=True,
                                  encoding='utf-8', errors='replace')
            
            if result.returncode == 0:
                self.log("✓ 编译成功")
                if result.stdout and result.stdout.strip():
                    self.log(f"编译输出:\n{result.stdout}")
                return True
            else:
                self.log("✗ 编译失败", "ERROR")
                if result.stderr:
                    self.log(f"编译错误:\n{result.stderr}", "ERROR")
                if result.stdout:
                    self.log(f"编译输出:\n{result.stdout}", "ERROR")
                return False
                
        except Exception as e:
            self.log(f"✗ 编译过程发生错误: {e}", "ERROR")
            return False
            
    def run_program(self):
        """运行构建好的程序"""
        self.log("准备运行程序...")
        
        exe_path = None
        
        # 优先使用最新构建的可执行文件
        if hasattr(self, 'latest_exe_path') and self.latest_exe_path and self.latest_exe_path.exists():
            exe_path = self.latest_exe_path
        else:
            # 查找最新的YGUninstaller程序
            if self.output_dir.exists():
                exe_files = list(self.output_dir.glob("YGUninstaller*.exe"))
                if exe_files:
                    # 按修改时间排序，获取最新的
                    exe_files.sort(key=lambda x: x.stat().st_mtime, reverse=True)
                    exe_path = exe_files[0]
        
        if not exe_path or not exe_path.exists():
            self.log("✗ 未找到可执行文件，请先构建项目", "ERROR")
            return False
            
        self.log(f"✓ 找到可执行文件: {exe_path}")
        
        try:
            self.log("🚀 启动程序...")
            # 在Windows上以管理员权限启动程序
            if os.name == 'nt':  # Windows
                try:
                    # 尝试以管理员权限启动
                    subprocess.Popen([
                        "powershell", "-Command", 
                        f"Start-Process -FilePath '{exe_path}' -Verb RunAs"
                    ], cwd=exe_path.parent)
                except:
                    # 如果失败，尝试普通启动
                    subprocess.Popen([str(exe_path)], cwd=exe_path.parent)
            else:
                subprocess.Popen([str(exe_path)], cwd=exe_path.parent)
            
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
            

def show_welcome():
    """显示欢迎信息"""
    print("=" * 50)
    print("🚀 YG Uninstaller 构建脚本")
    print("=" * 50)
    print()
    print("默认操作：构建并运行程序")
    print()
    print("常用命令：")
    print("  python build.py                # 构建并运行")
    print("  python build.py --run          # 仅运行程序")
    print("  python build.py --clean        # 清理构建")
    print("  python build.py --help         # 显示帮助")
    print()

def main():
    """主函数"""
    # 如果没有参数，显示欢迎信息
    if len(sys.argv) == 1:
        show_welcome()
    
    parser = argparse.ArgumentParser(description="YG Uninstaller 构建脚本")
    parser.add_argument("--clean", action="store_true",
                       help="清理构建文件")
    parser.add_argument("--build-only", action="store_true",
                       help="仅构建项目")
    parser.add_argument("--run", "-r", action="store_true",
                       help="仅运行程序（不构建）")
    
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
