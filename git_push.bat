@echo off
echo 正在推送YG Uninstaller项目到GitHub...

echo 1. 初始化git仓库...
git init .

echo 2. 设置用户信息...
git config user.name "gitchzh"
git config user.email "gmrchzh@gmail.com"

echo 3. 添加所有文件...
git add .

echo 4. 提交更改...
git commit -m "YG Uninstaller v1.0.0-Beta - 完整功能版本"

echo 5. 添加远程仓库...
git remote add origin git@github.com:gitchzh/YG-uninstaller.git

echo 6. 设置主分支...
git branch -M main

echo 7. 推送到远程仓库...
git push -u origin main

echo 8. 检查推送状态...
git status
git remote -v

echo 推送完成！
pause
