# YG Uninstaller

一个高效、轻量级的Windows程序卸载工具，类似于Geek Uninstaller。

## 特性

- 🚀 **高性能**: 使用C++开发，启动速度快，内存占用低
- 🔍 **深度扫描**: 彻底清理程序残留文件和注册表项
- 💪 **强制卸载**: 支持顽固程序的强制卸载
- 📦 **轻量级**: 编译后程序体积小于2MB

## 快速开始

### 系统要求
- Python 3.6+
- Windows 7+ 
- Visual Studio 2019+ 或 MinGW-w64

### 一键构建运行
```bash
python build.py
```

这一个命令就能完成：
- 自动检测编译器
- 编译整个项目
- 打包可执行文件
- 启动程序

### 其他命令
```bash
python build.py --run     # 仅运行程序
python build.py --clean   # 清理构建文件
python build.py --help    # 显示帮助
```

## 项目架构

```
YGUninstaller/
├── src/                    # 源代码
│   ├── core/              # 核心模块
│   ├── services/          # 服务层
│   ├── ui/                # 用户界面
│   └── utils/             # 工具类
├── include/               # 头文件
├── resources/             # 资源文件
└── build.py              # 构建脚本
```

## 许可证

MIT License