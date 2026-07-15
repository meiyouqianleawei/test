# 数据备份软件 DataBackup

Windows 平台下的桌面级数据备份工具，基于 C++17 + Qt 6 + CMake 构建。
支持普通备份、还原、定时备份和文件夹级实时备份四种工作模式，全部以图形界面驱动。

- 后端：C++17 · 面向对象与策略模式 · Windows 原生 API
- 前端：Qt 6 Widgets（多标签页）
- 构建：CMake · MSVC 2022
- 测试：Google Test

---

## 功能一览

| 模式 | 说明 | 输出形态 |
|------|------|----------|
| 普通备份 | 一次性把源目录打包成单文件，支持压缩+加密+文件过滤 | `.pkg` 打包文件 |
| 还原 | 从 `.pkg` 或实时备份目录还原到指定目录 | 目录 |
| 定时备份 | 按时间表自动执行备份任务 | `.pkg` 打包文件 |
| 实时备份 | 监听源目录变化，把改动同步到目标目录（文件夹镜像，不打包） | 镜像目录 |

四种模式可**并行使用**：同一个源目录可以同时被普通/定时/实时监控。

---

## 主要特性

### 备份 · 还原
- **打包格式** `DPKG v3`：头部（Magic + 版本 + 文件数 + 总大小 + 加密标志 + 压缩编码）+ 索引区 + 数据区
- **压缩** 三选一：无压缩 / Huffman（自实现） / Zlib（可选依赖）
- **加密** 可选 XOR，写入前"先压缩后加密"，附带密码验证块（密码错误立即识别，避免解密整包）
- **文件过滤器链** 五种可组合过滤：路径 / 类型 / 名称（通配符或正则） / 大小 / 时间；AND / OR 两种组合模式
- **元数据保留**：创建时间、修改时间、访问时间、文件属性、目录结构
- **备份后自校验**：默认打开
- **重复备份到同一 `.pkg`** 自动覆盖，含只读位处理
- **目标位于源目录内** 的场景自动排除，避免自反循环

### 定时备份
- 任务类型：**一次性 / 每天 / 每周 / 每月 / 间隔**（时/分/秒）
- 一次性任务执行后自动禁用，周期任务自动计算下次时间
- 任务级：暂停 / 恢复 / 立即执行 / 删除
- 调度器：启动 / 停止（停止后不再触发自动执行，但"立即执行"仍可用）
- 失败重试（`maxRetries` 可配置），运行状态防重入
- 任务列表每 2 秒刷新一次，实时显示"下次执行时间"

### 实时备份
- **触发源**：Windows `ReadDirectoryChangesW` 异步通知
- **镜像行为**：创建/修改 → 复制到目标；删除/重命名 → 目标同步删除或重命名（可关闭）
- **启动时全量同步** 可选：先把源目录已有内容复制到目标，再进入监听
- **子目录监控** 可选
- **加密写入**（XOR，同名覆盖）
- **扩展名过滤**：支持 `txt`、`.txt`、`*.txt` 三种写法
- **UTF-8 路径**：从 Qt 到 Windows API 全链路走宽字符，中文路径正常
- **自反循环保护**：目标目录不允许等于或位于源目录内部
- **`.meta` 元数据**：每个备份文件旁附带同名 `.meta`，还原时可复原时间戳/属性

### 还原
- **两种来源**：
  - 从 `.pkg` 打包文件还原（`RestoreEngine` + `Packager`）
  - 从实时备份目录还原（遍历目录、按需 XOR 解密、按 `.meta` 恢复元数据）
- 密码错误提示、进度回调、取消支持

---

## 界面

四个标签页，进入即用：

- **备份**：源目录 + 目标 `.pkg` + 压缩 + 加密 + 过滤器 + 进度条
- **还原**：来源类型（打包文件 / 实时备份目录）+ 目标目录 + 密码
- **定时任务**：任务表 + 类型（一次性/每天/每周/每月/间隔）+ 时间参数 + 压缩加密过滤 + 启停调度器
- **实时备份**：监控目录 + 目标目录 + 加密 + 扩展名过滤 + 三个行为开关（子目录 / 启动全量同步 / 同步删除）+ 统计面板

---

## 快速开始

### 环境
- Windows 10 / 11
- Visual Studio 2022（MSVC）
- CMake 3.16+
- Qt 6（默认路径 `D:/Qt/6.11.1/msvc2022_64`，可在 `build_gui.bat` 中修改）
- Zlib（可选）

### 构建 GUI

```powershell
.\build_gui.bat
.\run_gui.bat
```

产物：`build\src\ui\Release\DataBackupGUI.exe`

### 构建并运行所有测试

```powershell
.\build_project.bat
.\run_all_tests.bat
```

### 清理

```powershell
.\clean_project.bat
```

---

## 项目结构

```
软件开发实验/
├── src/
│   ├── core/            FileScanner · MetadataManager · FileInfo
│   ├── compression/     ICompressionStrategy · Huffman · Zlib
│   ├── encryption/      IEncryptionStrategy · XOREncryption
│   ├── filter/          FilterChain · Path/Type/Name/Time/Size Filter
│   ├── pack/            Packager（DPKG v3 打包/解包）
│   ├── engine/          BackupEngine · RestoreEngine · BackupConfig
│   ├── scheduler/       BackupScheduler · IScheduler · ScheduleTypes
│   ├── realtime/        RealtimeBackupMonitor · IRealtimeBackupMonitor
│   └── ui/              MainWindow + 四个 Tab + 各自 Worker + FilterDialog
├── tests/               13 个 Google Test 测试文件
├── thirdparty/          Zlib 静态库（可选）
├── build_gui.bat        构建 GUI
├── run_gui.bat          运行 GUI
├── build_project.bat    构建全部（含测试）
├── run_all_tests.bat    跑全部测试
├── clean_project.bat    清理 build/
└── CMakeLists.txt
```

---

## 模块与测试

| 模块 | 测试文件 | 说明 |
|------|----------|------|
| 基础框架 | `test_framework_basic.cpp` | GTest + 项目基础 |
| 文件扫描 | `test_filescanner.cpp` | 硬链接/软链接/BFS |
| 元数据 | `test_metadatamanager.cpp` | Windows API 时间/属性 |
| 打包器 | `test_packager.cpp` | DPKG 打包/解包/完整性 |
| 过滤器 | `test_filters.cpp` | 5 种过滤器 + FilterChain |
| Huffman | `test_huffman_compression.cpp` | 无外部依赖压缩 |
| Zlib | `test_zlib_compression.cpp` | 可选，依赖 zlib |
| XOR 加密 | `test_xor_encryption.cpp` | 对称加密 |
| 备份引擎 | `test_backup_engine.cpp` | 端到端备份流程 |
| 调度器 | `test_backup_scheduler.cpp` | 定时任务 + 死锁场景 |
| 实时备份 | `test_realtime_backup.cpp` | 监控 + 同步 |
| 集成 | `test_integration.cpp` | 备份→还原完整链路 |
| 真实目录 | `test_real_directory.cpp` | 交互式，选真实目录 |

单独运行示例：

```powershell
.\build\tests\Release\test_backup_engine.exe
.\build\tests\Release\test_realtime_backup.exe
```

---

## 关键设计决策

- **策略模式**：压缩、加密、过滤器全部通过接口注入 `BackupConfig`，运行时任意组合。
- **线程模型**：
  - 备份/还原：`QThread` 子类（`BackupWorker` / `RestoreWorker`），信号槽回主线程更新 UI。
  - 定时器：独立 `std::thread` 循环（`BackupScheduler`），Qt 侧用 `Qt::QueuedConnection` 转发结果。
  - 实时监控：独立 `std::thread` + `ReadDirectoryChangesW` 异步 IO + 停止事件。
- **加锁策略**：调度器 / 监控器都采用"抓取任务信息后立即释放锁，长耗时操作在锁外执行"的模式，避免阻塞。
- **路径编码**：Qt 侧统一 UTF-8，Windows API / `std::filesystem` 边界处显式转 `wchar_t`，中文路径全链路可用。
- **DPKG 格式向前兼容**：V1（无加密无压缩）→ V2（加密）→ V3（加密+压缩），解包时按版本号选择头部大小。

---

## 常见问题

**Q：可以同时对同一目录做实时+定时备份吗？**
可以。实时备份把源目录持续镜像到目标目录 A，定时备份把源目录打包成 `.pkg` 存到位置 B，两者互不影响。

**Q：实时备份得到的加密文件怎么打开？**
在"还原"页选"从实时备份目录还原（文件夹）"，选目标目录、输入原密码即可解密还原到新目录。

**Q：一次性任务执行完还会再触发吗？**
不会。执行后被自动禁用，`nextExecutionTime` 置 0。

**Q：停止调度器后已有任务丢了吗？**
任务保留，只是不再按时触发。手动"立即执行"仍然可用。

**Q：Zlib 可以不装吗？**
可以。默认走 Huffman。装了 Zlib 后压缩下拉框会自动出现该选项。

---

## 许可证

MIT License
