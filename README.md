# 数据备份软件

Windows 环境下的数据备份软件，使用 C++17 + CMake + Google Test，采用策略模式和面向对象设计。

## 项目状态

### ✅ 第一阶段：基础架构（已完成）
- 项目结构、CMake 配置
- Google Test 集成（Gitee 镜像）
- FileInfo 实体类（支持硬链接/软链接）
- 策略接口定义（压缩、加密、过滤器、打包）

### ✅ 第二阶段：文件处理核心（已完成）
- **FileScanner 文件扫描器**（9 个测试通过）
  - ✅ 硬链接识别（使用 `GetFileInformationByHandle` API）
  - ✅ 软链接处理（使用 `FILE_FLAG_OPEN_REPARSE_POINT`）
  - ✅ 广度优先遍历（避免递归栈溢出）
  - ✅ 扫描统计信息

- **MetadataManager 元数据管理**（12 个测试通过）
  - ✅ Windows 元数据提取（创建时间、修改时间、文件属性）
  - ✅ 元数据设置与还原
  - ✅ 批量处理
  - ✅ 序列化（JSON 格式）
  - ✅ 持久化到文件

- **Packager 文件打包器**（14 个测试通过）
  - ✅ 文件打包（多个文件打包为一个连续文件）
  - ✅ 文件解包（还原文件和目录结构）
  - ✅ 元数据保存与还原
  - ✅ 打包格式验证（Magic Number "DPKG"）
  - ✅ 完整性验证
  - ✅ 支持大文件（>1GB）
  - ✅ 支持目录结构（basePath 参数）
  - ✅ 进度回调支持

- **文件过滤器链**（37 个测试通过）
  - ✅ **PathFilter** - 路径过滤（包含/排除特定路径前缀）
  - ✅ **TypeFilter** - 类型过滤（按文件扩展名过滤）
  - ✅ **NameFilter** - 文件名过滤（支持通配符 `*` 和 `?` 匹配）
  - ✅ **TimeFilter** - 时间过滤（创建/修改/访问时间范围）
  - ✅ **SizeFilter** - 大小过滤（文件大小范围，支持 KB/MB/GB）
  - ✅ **FilterChain** - 过滤器链组合（AND/OR 模式）

### ✅ 第三阶段：压缩与加密（已完成）
- **压缩模块**（23 个测试通过）
  - ✅ **ICompressionStrategy** - 压缩策略接口（Strategy Pattern）
  - ✅ **HuffmanCompression** - Huffman 压缩实现（自实现，无外部依赖）
    - ✅ 基于字节频率的最优编码
    - ✅ 空数据处理
    - ✅ 单字节数据支持（边界情况）
    - ✅ 大文件压缩（>1MB）
    - ✅ 重复数据压缩（高压缩率）
    - ✅ 随机数据压缩
    - ✅ 二进制数据支持
    - ✅ 无损压缩，完美还原原始数据
    - ✅ 文本压缩率：30-50%
  - ✅ **ZlibCompression** - Zlib 压缩实现（可选，需安装zlib库）
    - ✅ 支持压缩级别 0-9（无压缩到最高压缩）
    - ✅ 空数据处理
    - ✅ 大文件压缩（>1MB）
    - ✅ 重复数据压缩（高压缩率）
    - ✅ 随机数据压缩
    - ✅ 错误数据检测（无效压缩数据拒绝）
    - ✅ 使用本地编译的 Zlib 静态库（zs.lib）

- **加密模块**（12 个测试通过）
  - ✅ **IEncryptionStrategy** - 加密策略接口（Strategy Pattern）
  - ✅ **XOREncryption** - XOR 加密实现（自实现，无外部依赖）
    - ✅ 对称加密，使用密码进行加密/解密
    - ✅ 空数据处理
    - ✅ 空密码检测
    - ✅ 简单文本加密解密
    - ✅ 二进制数据加密解密
    - ✅ 大文件加密解密（>1MB）
    - ✅ 不同密码产生不同密文
    - ✅ 错误密码检测
    - ✅ 重复数据加密
    - ✅ 长密码加密解密
    - ✅ 中文字符加密解密

### ✅ 第四阶段：备份引擎（已完成）
- **备份引擎**（13 个测试通过）
  - ✅ **BackupEngine** - 备份引擎核心类
    - ✅ 完整备份流程（扫描→过滤→压缩→加密→打包→验证）
    - ✅ 集成所有模块（扫描、过滤、压缩、加密、打包）
    - ✅ 进度管理和回调
    - ✅ 取消备份功能
    - ✅ 错误处理和日志
  - ✅ **RestoreEngine** - 还原引擎核心类
    - ✅ 完整还原流程（解包→解密→解压→还原）
    - ✅ 备份验证
    - ✅ 备份信息查询
    - ✅ 进度管理
  - ✅ **BackupConfig** - 备份配置类
    - ✅ 路径配置
    - ✅ 压缩算法选择（none/huffman/zlib）
    - ✅ 加密算法选择（none/xor）
    - ✅ 过滤器配置
    - ✅ 其他选项（保留目录结构、验证、硬链接处理）
  - ✅ **BackupTypes** - 备份类型定义
    - ✅ BackupPhase - 备份阶段枚举
    - ✅ BackupProgress - 实时进度信息
    - ✅ BackupResult - 备份结果
    - ✅ RestoreResult - 还原结果

### ✅ 第五阶段：定时调度器（已完成）
- **定时调度器**（15 个测试通过）
  - ✅ **BackupScheduler** - 备份调度器核心类
    - ✅ 完整的任务管理（添加、删除、暂停、恢复）
    - ✅ 多种定时类型（一次性、每天、每周、每月、间隔）
    - ✅ 任务执行和监控
    - ✅ 任务回调机制
    - ✅ 配置持久化（保存/加载任务）
  - ✅ **ScheduleTypes** - 定时任务类型定义
    - ✅ ScheduleType - 任务类型枚举
    - ✅ TaskStatus - 任务状态枚举
    - ✅ ScheduleConfig - 任务配置
    - ✅ TaskInfo - 任务信息
    - ✅ TaskExecutionResult - 任务执行结果
  - ✅ 功能特性
    - ✅ 支持一次性任务（指定时间执行一次）
    - ✅ 支持周期性任务（每天、每周、每月）
    - ✅ 支持间隔任务（每隔N小时/分钟）
    - ✅ 任务执行历史记录
    - ✅ 失败重试机制
    - ✅ 多线程调度
    - ✅ 线程安全设计（分段加锁，避免死锁）

### ✅ 第六阶段：实时备份监控（已完成）
- **实时备份监控**（14 个测试通过）
  - ✅ **RealtimeBackupMonitor** - 实时备份监控器核心类
    - ✅ Windows API监控（ReadDirectoryChangesW）
    - ✅ 实时文件变化检测（创建、修改、删除、重命名）
    - ✅ 自动备份触发
    - ✅ 可选加密备份（XOR加密）
    - ✅ 文件过滤功能
    - ✅ 暂停/恢复监控
    - ✅ 手动备份单个文件
  - ✅ **RealtimeBackupTypes** - 实时备份类型定义
    - ✅ RealtimeBackupConfig - 实时备份配置
    - ✅ FileChangeType - 文件变化类型枚举
    - ✅ RealtimeBackupStatus - 监控状态枚举
    - ✅ FileChangeEvent - 文件变化事件
    - ✅ RealtimeBackupStats - 统计信息
  - ✅ 功能特性
    - ✅ 使用Windows原生API监控文件变化
    - ✅ 实时备份，不打包文件
    - ✅ 保留目录结构
    - ✅ 支持中文路径（UTF-8编码）
    - ✅ 元数据保存（.meta文件）
    - ✅ 线程安全设计

### 测试统计
- **总计：177 个测试通过**（无zlib）或 **187 个测试通过**（有zlib）
- test_framework_basic: 8 个测试
- test_filescanner: 9 个测试
- test_metadatamanager: 12 个测试
- test_packager: 14 个测试
- test_filters: 37 个测试
- test_huffman_compression: 13 个测试
- test_xor_encryption: 12 个测试
- test_backup_engine: 13 个测试
- test_backup_scheduler: 17 个测试（含死锁测试）
- test_realtime_backup: 14 个测试  ✅ 新增
- test_zlib_compression: 10 个测试（可选，需安装zlib）

## 构建与测试

### 环境要求
- Windows 10/11
- Visual Studio 2022（MSVC 编译器）
- CMake 3.15+
- Git
- （可选）Zlib 库 - 用于ZlibCompression压缩

### 构建步骤

#### 方式1：基本构建（仅Huffman压缩，无需外部依赖）

```powershell
# 克隆仓库
git clone <repository-url>
cd DataBackupSoftware

# 构建项目（仅Huffman压缩）
.\build_project.bat

# 运行所有测试（102个测试）
.\run_all_tests.bat
```

#### 方式2：完整构建（Huffman + Zlib压缩）

```powershell
# 1. 安装zlib（手动下载或使用vcpkg）
# 详见：docs/zlib_installation.md

# 2. 配置zlib路径（在CMakeLists.txt中修改）
set(ZLIB_ROOT "D:/path/to/zlib" CACHE PATH "Zlib root directory")
set(ZLIB_INCLUDE_DIR "${ZLIB_ROOT}" CACHE PATH "Zlib include directory")
set(ZLIB_LIBRARY "${ZLIB_ROOT}/build/Release/zs.lib" CACHE FILEPATH "Zlib static library")

# 3. 构建项目
.\build_project.bat

# 4. 运行所有测试（112个测试）
.\run_all_tests.bat
```

### 单独运行测试

```powershell
# 基础框架测试
.\build\tests\Debug\test_framework_basic.exe

# 文件扫描测试
.\build\tests\Debug\test_filescanner.exe

# 元数据管理测试
.\build\tests\Debug\test_metadatamanager.exe

# 文件打包测试
.\build\tests\Debug\test_packager.exe

# 文件过滤器测试
.\build\tests\Debug\test_filters.exe

# Huffman压缩测试（总是可用）
.\build\tests\Debug\test_huffman_compression.exe

# XOR加密测试（总是可用）
.\build\tests\Debug\test_xor_encryption.exe

# 备份引擎测试（总是可用）
.\build\tests\Debug\test_backup_engine.exe

# 备份调度器测试（总是可用）
.\build\tests\Debug\test_backup_scheduler.exe

# Zlib压缩测试（需要安装zlib）
.\build\tests\Debug\test_zlib_compression.exe
```

### 集成测试（可选）

```powershell
# 完整备份还原流程测试
.\build\tests\Debug\test_integration.exe

# 真实目录测试（交互式）
.\build\tests\Debug\test_real_directory.exe
```

## 项目结构

```
DataBackupSoftware/
├── src/
│   ├── core/           # FileScanner, MetadataManager, FileInfo
│   ├── compression/    # 压缩策略接口和实现
│   │   ├── ICompressionStrategy.h      # 压缩策略接口
│   │   ├── HuffmanCompression.h/cpp    # Huffman压缩（✅ 已实现）
│   │   └── ZlibCompression.h/cpp       # Zlib压缩（✅ 已实现，可选）
│   ├── encryption/     # 加密策略接口和实现
│   │   ├── IEncryptionStrategy.h       # 加密策略接口
│   │   └── XOREncryption.h/cpp         # XOR加密（✅ 已实现）
│   ├── filter/         # 文件过滤器链（PathFilter, TypeFilter, NameFilter, TimeFilter, SizeFilter）
│   ├── pack/           # Packager 文件打包器
│   ├── engine/         # 备份引擎和还原引擎
│   │   ├── BackupEngine.h/cpp          # 备份引擎（✅ 已实现）
│   │   ├── RestoreEngine.h/cpp         # 还原引擎（✅ 已实现）
│   │   ├── BackupConfig.h              # 备份配置（✅ 已实现）
│   │   └── BackupTypes.h               # 备份类型定义（✅ 已实现）
│   ├── scheduler/      # 定时调度器
│   │   ├── BackupScheduler.h/cpp       # 备份调度器（✅ 已实现）
│   │   ├── ScheduleTypes.h             # 定时任务类型定义（✅ 已实现）
│   │   └── IScheduler.h                # 调度器接口（✅ 已实现）
│   └── ui/             # Qt 界面（待实现）
├── tests/              # Google Test 测试
├── thirdparty/         # 第三方库（Zlib 等，可选）
├── build/              # 构建输出目录
├── docs/               # 设计文档
├── CMakeLists.txt      # CMake 配置
└── README.md           # 项目说明
```

## 下一步开发计划

### 第六阶段：图形界面
- 🔲 Qt GUI 开发
- 🔲 可视化备份进度
- 🔲 备份文件浏览器
- 🔲 定时任务管理界面

### 第七阶段：高级功能（可选）
- 🔲 实时文件监控
- 🔲 增量备份支持
- 🔲 备份计划管理
- 🔲 网络备份支持

## 算法对比

### 压缩算法

| 算法 | 状态 | 压缩率 | 速度 | 依赖 |
|------|------|--------|------|------|
| Huffman | ✅ 已实现 | 30-50%（文本） | 快 | 无 |
| Zlib | ✅ 已实现 | 40-70% | 很快 | zlib库（可选） |

### 加密算法

| 加密算法 | 状态 | 安全性 | 速度 | 依赖 |
|---------|------|--------|------|------|
| XOR | ✅ 已实现 | 低 | 很快 | 无 |
| AES | 🔲 待实现 | 高 | 中等 | Crypto++（可选） |

## 技术特点

- ✅ **测试驱动开发（TDD）**：每个模块都有完整的单元测试
- ✅ **策略模式**：压缩、加密、过滤器采用策略模式解耦
- ✅ **Windows API**：使用 Windows-specific APIs 处理元数据和链接文件
- ✅ **异常处理**：完善的异常处理机制
- ✅ **内存安全**：使用智能指针管理对象生命周期
- ✅ **模块化设计**：各模块独立，易于维护和扩展

## 开发团队

软件开发实验课程项目

## 许可证

MIT License