# 数据备份软件 GUI 用户指南

## 概述

数据备份软件 GUI 是一个基于 Qt 6 开发的图形界面应用程序，提供完整的备份、还原、定时任务和实时备份功能。

## 系统要求

- **操作系统**：Windows 10 或更高版本
- **Qt 版本**：Qt 6.6.0 或更高版本（MSVC 2019 64-bit）
- **编译器**：Visual Studio 2022（MSVC）
- **CMake**：3.16 或更高版本

## 安装步骤

### 1. 安装 Qt 6

1. 下载 Qt 在线安装器：https://www.qt.io/download
2. 安装时选择以下组件：
   - Qt 6.6.0（或最新版本）
   - MSVC 2019 64-bit
   - Qt Creator（可选）

3. 安装路径建议：`C:\Qt\6.6.0\msvc2019_64`

### 2. 编译项目

打开命令提示符，进入项目根目录，执行：

```batch
# 方式 1：使用提供的脚本
build_gui.bat

# 方式 2：手动编译
cd build
cmake .. -G "Visual Studio 17 2022" -A x64 ^
    -DCMAKE_PREFIX_PATH="C:/Qt/6.6.0/msvc2019_64"
cmake --build . --config Release --target DataBackupGUI
```

### 3. 运行程序

```batch
# 方式 1：使用提供的脚本
run_gui.bat

# 方式 2：直接运行
build\src\ui\Release\DataBackupGUI.exe
```

## 功能说明

### 1. 备份功能（BackupTab）

**主要功能**：
- 选择源目录和目标路径
- 支持压缩（无压缩、Huffman、Zlib）
- 支持加密（XOR算法）
- 实时进度显示
- 可取消备份

**使用步骤**：
1. 点击"浏览..."按钮选择源目录
2. 点击"浏览..."按钮选择目标备份文件路径（建议扩展名为 `.pkg`）
3. 选择压缩方式（默认：无压缩）
4. （可选）勾选"启用加密"并输入密码
5. 点击"开始备份"按钮
6. 等待备份完成，进度条会实时显示进度

**注意事项**：
- 目标路径必须有写入权限
- 大文件备份可能需要较长时间
- 建议定期备份重要数据

### 2. 还原功能（RestoreTab）

**主要功能**：
- 从备份文件还原数据
- 支持解密还原
- 实时进度显示
- 可取消还原

**使用步骤**：
1. 点击"浏览..."按钮选择备份文件（`.pkg` 文件）
2. 点击"浏览..."按钮选择还原目录
3. （如果备份时加密）输入密码
4. 点击"开始还原"按钮
5. 等待还原完成

**注意事项**：
- 还原会覆盖目标目录中的同名文件
- 确保有足够的磁盘空间
- 如果忘记密码，无法解密还原

### 3. 定时任务（SchedulerTab）

**主要功能**：
- 创建定时备份任务
- 支持多种调度类型（一次性、每天、每周、每月、间隔）
- 暂停/恢复任务
- 手动执行任务
- 查看任务执行历史

**使用步骤**：
1. 点击"启动调度器"按钮启动后台调度服务
2. 点击"添加任务"按钮创建新任务
3. 在任务对话框中：
   - 输入任务名称
   - 选择调度类型（一次性/每天/每周/每月/间隔）
   - 设置执行时间
   - 配置备份参数（源目录、目标路径等）
   - 选择是否打包（定时备份可以打包）
4. 点击"确定"保存任务
5. 任务会按照计划自动执行

**注意事项**：
- 调度器必须保持运行才能执行定时任务
- 关闭 GUI 程序会停止调度器
- 任务配置会自动保存到文件

### 4. 实时备份（RealtimeTab）

**主要功能**：
- 实时监控文件变化
- 文件创建或修改时自动备份
- 支持加密备份
- 文件过滤器
- 统计信息显示

**使用步骤**：
1. 选择监控目录（源目录）
2. 选择备份目标目录
3. （可选）勾选"启用加密"并输入密码
4. （可选）设置文件过滤器（例如：`*.txt;*.doc`）
5. 点击"开始监控"按钮
6. 在监控目录中创建或修改文件，会自动备份

**与定时备份的区别**：
- **实时备份**：
  - 文件变化时立即备份
  - 不打包（直接复制文件）
  - 不压缩
  - 可加密
  - 保留目录结构

- **定时备份**：
  - 按计划执行备份
  - 可以打包（`.pkg` 文件）
  - 支持压缩
  - 可加密
  - 更灵活的调度方式

**注意事项**：
- 实时备份不会打包，适合快速备份
- 监控大量文件可能影响性能
- 建议只监控重要工作目录

## 常见问题

### Q1: 编译时提示找不到 Qt6？

**解决方案**：
1. 检查 Qt 安装路径是否正确
2. 修改 `build_gui.bat` 中的 `CMAKE_PREFIX_PATH` 路径
3. 确保安装了 MSVC 2019 64-bit 版本的 Qt

### Q2: 运行时提示缺少 Qt DLL？

**解决方案**：
1. 将 Qt 的 `bin` 目录添加到系统 PATH
2. 或者使用 `windeployqt` 工具部署 Qt 库：
   ```batch
   cd build\src\ui\Release
   C:\Qt\6.6.0\msvc2019_64\bin\windeployqt.exe DataBackupGUI.exe
   ```

### Q3: 备份速度很慢？

**可能原因**：
- 文件数量过多
- 压缩算法耗时
- 磁盘性能限制

**解决方案**：
- 对于大量小文件，建议使用"无压缩"
- 使用 Zlib 压缩可以设置压缩级别
- 备份到 SSD 可以提高速度

### Q4: 如何备份到网络路径？

**解决方案**：
- 在目标路径中直接输入网络路径（例如：`\\Server\Backup\backup.pkg`）
- 确保有网络路径的写入权限
- 网络备份速度可能较慢

## 技术架构

### 线程安全设计

- 所有耗时操作在后台线程执行（使用 `QThread`）
- 使用 Qt 信号槽机制（`Qt::QueuedConnection`）确保跨线程安全通信
- 避免阻塞主线程，保持 UI 响应

### 模块结构

```
src/ui/
├── MainWindow.h/cpp        # 主窗口（选项卡布局）
├── BackupTab.h/cpp         # 备份功能选项卡
├── RestoreTab.h/cpp        # 还原功能选项卡
├── SchedulerTab.h/cpp      # 定时任务选项卡
├── RealtimeTab.h/cpp       # 实时备份选项卡
├── BackupWorker.h/cpp      # 备份工作线程
├── RestoreWorker.h/cpp     # 还原工作线程
├── SchedulerWorker.h/cpp   # 定时调度器工作类
├── RealtimeWorker.h/cpp    # 实时备份监控工作类
└── main.cpp                # 应用程序入口
```

### 依赖关系

- **BackupTab** → BackupWorker → BackupEngine
- **RestoreTab** → RestoreWorker → RestoreEngine
- **SchedulerTab** → SchedulerWorker → BackupScheduler
- **RealtimeTab** → RealtimeWorker → RealtimeBackupMonitor

## 开发者说明

### 扩展功能

如需添加新功能，请遵循以下步骤：

1. 在相应的选项卡类中添加 UI 控件
2. 在 Worker 类中添加后台处理逻辑
3. 使用信号槽机制连接 UI 和 Worker

### 自定义样式

GUI 支持自定义 Qt 样式表（QSS）：

```cpp
// 在 main.cpp 中添加
app.setStyleSheet("QPushButton { background-color: #4CAF50; color: white; }");
```

### 国际化

GUI 使用中文界面，如需支持其他语言：

1. 使用 Qt 的国际化机制（`QTranslator`）
2. 提取可翻译字符串（`tr()` 函数）
3. 创建 `.ts` 翻译文件

## 许可证

本项目遵循 MIT 许可证。

## 联系方式

如有问题或建议，请提交 Issue 或 Pull Request。