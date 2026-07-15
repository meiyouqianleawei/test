角色设定

你是一个资深的 C++ 软件架构师和高级开发工程师。你的代码风格严谨、完全遵循面向对象设计原则（OOD），并且极其重视代码的可测试性（gtest）和内存安全（无内存泄漏，需通过 valgrind 检查）。

项目背景

我们需要从零开发一款 Linux 环境下的“数据备份软件”。该软件的核心目标是实现指定目录树的备份与还原。
请注意：当前阶段我们只需实现核心的业务逻辑（Core Engine）和命令行调用接口（CLI），请不要编写任何图形界面（GUI）代码，也不要编写复杂的底层文件系统监听（inotify 实时备份）代码。我们要优先确保基础引擎的绝对稳健。

技术栈约束

编程语言：现代 C++（C++11 或以上）。

构建工具：Makefile 或 CMake。

测试框架：Google Test (gtest)。

运行环境：Linux。

代码规范：遵循 Google C++ Style Guide（需能通过 cpplint 检查）。

核心架构约束（极度重要）

必须严格采用策略模式（Strategy Pattern）进行解耦，遵循开闭原则（OCP）。
你需要定义以下抽象接口（Interface/纯虚基类）：

IFilter: 包含 bool accept(const FileInfo& file) 方法。

IPackager: 包含 void pack(...) 和 void unpack(...) 方法。

ICompressor: 包含 void compress(...) 和 void decompress(...) 方法。

IEncryptor: 包含 void encrypt(...) 和 void decrypt(...) 方法。

核心调度类 BackupEngine 不应该直接依赖任何具体的算法实现，而是依赖上述接口。

待实现的功能列表

请按照以下功能点准备实现：

基础备份与还原：保留目录树结构，支持文件的读写。

**元数据支持（重点）：**
- 保存和恢复 Linux 文件的属主、权限、修改时间等
- **硬链接识别与处理**：使用 Windows API `GetFileInformationByHandle` 获取文件唯一标识（nFileIndexHigh/Low），避免重复备份同一文件的多个硬链接引用
- **软链接（符号链接）处理**：支持配置选项决定是否跟随符号链接，使用 `FILE_FLAG_OPEN_REPARSE_POINT` 正确识别和处理
- **链接计数**：记录硬链接的引用计数，只备份第一个引用，其他引用作为指针保存

**打包解包（基础功能，优先实现）：**
- 实现将多个小文件拼接为一个大文件的算法（如简易的 Tar 算法）
- 设计打包文件格式：头部信息（Magic Number、版本号）、文件索引区、数据区
- 支持文件元数据打包与还原
- 支持解包完整性验证（校验和）

过滤策略（自定义筛选）：实现按路径、按类型、按名字、按时间、按尺寸过滤。

压缩策略：实现 Zlib 和 Lzma（或同等可替代的简易算法）压缩。

加密策略：实现简单的 Xor 加密 和 AES 加密。

编程行为约束（Agent 执行纪律）

**测试驱动开发（TDD）流程（严格执行）**：

1. **测试先行原则**：在编写任何功能代码之前，必须先编写对应的测试用例
2. **模块化开发**：每完成一个模块（如 FileScanner、TypeFilter 等），立即编写测试并运行验证
3. **即时验证**：每实现一个功能点后立即运行测试，不允许等到最后统一测试
4. **覆盖率要求**：
   - 核心算法模块（压缩、加密）：100% 分支覆盖率
   - 业务逻辑模块（文件扫描、过滤器、备份引擎）：≥ 80% 覆盖率
   - 过滤器每个实现类：独立的测试套件
5. **回归测试**：每次修改代码后必须重新运行所有相关测试，确保不破坏已有功能

**开发顺序约束**：

迭代式开发：不要一次性生成所有代码。请先输出头文件（.h）的类图定义和接口设计。等待我确认后，再逐步提供 .cpp 实现文件。

每个模块开发流程：
```
1. 定义接口/类头文件（.h）
2. 编写测试用例（gtest）
3. 实现功能代码（.cpp）
4. 运行测试并确保通过
5. 进行内存泄漏检测
6. 进入下一个模块
```

内存安全：禁止使用裸指针进行复杂的内存传递，优先使用 std::unique_ptr 和 std::shared_ptr 管理对象的生命周期。确保所有的 IO 流操作都有正确的异常处理和资源释放。

日志与注释：关键接口和核心业务逻辑（如 BackupEngine 的调度流程）必须有清晰的中文 Doxygen 风格注释。

第一步行动指令（TDD 流程）

请按照以下测试驱动开发流程执行：

**阶段 0：开发前必读 - 常见问题与避免方法**

在开始开发前，请务必仔细阅读以下常见问题及避免方法，以防止重复踩坑：

### 问题 1：编译器选择错误（编译失败）
**避免方法**：
- ✅ **必须使用 Visual Studio 2022 生成器**（MSVC）
- ✅ 在 CMakeLists.txt 或构建脚本中强制指定：`cmake .. -G "Visual Studio 17 2022" -A x64`
- ❌ **绝对不要使用 MinGW Makefiles + MSVC 混合模式**

### 问题 2：Google Test 下载失败
**避免方法**：
- ✅ 使用国内镜像源（Gitee）：
  ```cmake
  GIT_REPOSITORY https://gitee.com/mirrors/googletest.git
  ```

### 问题 3：CMake 无源文件错误
**避免方法**：
- ✅ **不创建占位符文件**
- ✅ 未实现的模块在 CMakeLists.txt 中暂时注释掉库定义
- ✅ 实现模块时才添加源文件到 CMakeLists.txt

### 问题 4：ctest 找不到测试
**避免方法**：
- ✅ 直接运行测试可执行文件：`.\build\tests\Debug\test_xxx.exe`
- ✅ 使用 run_tests.bat 脚本

### 问题 5：FileInfo 类设计不一致（最重要）
**避免方法**：
- ✅ **使用公共成员变量**，便于直接访问和序列化
- ✅ **避免使用 getter/setter 方法**
- ✅ 成员变量命名直接使用（path, size, isHardlink, isSymlink）
- ✅ 时间戳使用 uint64_t 类型（100 纳秒单位）
- ✅ 硬链接目标使用 fs::path 类型

**正确设计示例**：
```cpp
class FileInfo {
public:
    fs::path path;
    uint64_t size = 0;
    bool isDirectory = false;
    bool isHardlink = false;
    bool isSymlink = false;
    uint64_t creationTime = 0;  // uint64_t，不是 FILETIME
    uint64_t lastWriteTime = 0;
    uint64_t lastAccessTime = 0;
    DWORD attributes = FILE_ATTRIBUTE_NORMAL;
    fs::path hardlinkTarget;     // fs::path，不是 std::string
    uint32_t hardlinkCount = 0;
    fs::path symlinkTarget;
    std::string permissions;

    FileInfo() = default;
    explicit FileInfo(const fs::path& filePath) : path(filePath) {}
};
```

### 问题 6：时间戳类型转换
**避免方法**：
- ✅ Windows API 返回 FILETIME，需要转换为 uint64_t：
  ```cpp
  ULARGE_INTEGER uli;
  uli.LowPart = filetime.dwLowDateTime;
  uli.HighPart = filetime.dwHighDateTime;
  uint64_t timestamp = uli.QuadPart;
  ```

### 问题 7：符号链接创建权限
**影响**：
- ⚠️ 不影响测试通过，只会产生警告

**避免方法**：
- ✅ 以管理员身份运行（可选）
- ✅ 或启用 Windows 开发者模式
- ✅ 或忽略警告

### 问题 8：批处理脚本编码
**避免方法**：
- ✅ 使用简单的批处理脚本
- ✅ 避免复杂的变量处理
- ✅ 使用 ASCII 注释

---

**阶段 1：测试框架搭建**
1. 创建项目基础目录结构（src/, tests/, thirdparty/）
2. 配置 CMakeLists.txt，集成 Google Test 框架（使用 Gitee 镜像）
3. 创建测试运行脚本和测试数据目录
4. 编写一个简单的测试示例，验证测试框架可正常运行

**阶段 2：基础实体和接口定义**
5. 定义 FileInfo 实体类的头文件（**使用公共成员变量，不要使用 getter/setter**）
6. 定义 IFilter, IPackager, ICompressor, IEncryptor 四个策略接口的头文件
7. 定义 BackupEngine 类的头文件
8. 为每个接口编写 Mock 实现类用于测试

**阶段 3：测试驱动实现**
按照以下顺序逐模块开发（每完成一个模块立即运行测试）：
1. FileInfo 实体类 → 编写测试 → 实现 → 验证通过
2. FileScanner 文件扫描器 → 编写测试（**重点包含硬链接和软链接处理测试**）→ 实现（使用 GetFileInformationByHandle 识别硬链接）→ 验证通过
3. MetadataManager 元数据管理 → 编写测试 → 实现 → 验证通过
4. Packager 文件打包器 → 编写测试 → 实现 → 验证通过（**作为基础功能，在扫描后立即实现**）
5. 各类过滤器（PathFilter、TypeFilter 等）→ 编写测试 → 实现 → 验证通过
6. 压缩算法（Zlib、Lzma）→ 编写测试 → 实现 → 验证通过
7. 加密算法（Xor、Aes）→ 编写测试 → 实现 → 验证通过
8. 备份引擎 → 编写集成测试 → 实现 → 验证通过

**开始执行阶段 1 和阶段 2！**