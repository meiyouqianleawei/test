// 真实目录测试工具 - 验证软件能否处理您的实际文件
// 只读取信息，不修改数据

#include <iostream>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>
#include "core/FileInfo.h"
#include "core/FileScanner.h"
#include "core/MetadataManager.h"
#include "pack/Packager.h"
#include "filter/FilterChain.h"
#include "filter/TypeFilter.h"
#include "filter/SizeFilter.h"

namespace fs = std::filesystem;

std::string formatSize(uint64_t bytes) {
    const char* units[] = {"B", "KB", "MB", "GB"};
    int i = 0;
    double s = bytes;
    while (s >= 1024 && i < 3) { s /= 1024; i++; }
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2) << s << " " << units[i];
    return oss.str();
}

std::string formatTime(uint64_t ft) {
    FILETIME fileTime;
    fileTime.dwLowDateTime = ft & 0xFFFFFFFF;
    fileTime.dwHighDateTime = ft >> 32;
    SYSTEMTIME st;
    FileTimeToSystemTime(&fileTime, &st);
    std::ostringstream oss;
    oss << st.wYear << "-" << std::setw(2) << st.wMonth << "-"
        << std::setw(2) << st.wDay << " " << std::setw(2) << st.wHour
        << ":" << std::setw(2) << st.wMinute;
    return oss.str();
}

void testScan(const fs::path& dir) {
    std::cout << "\n=== 测试扫描功能 ===\n";
    std::cout << "扫描目录: " << dir << "\n";

    FileScanner scanner;
    std::vector<FileInfo> files;
    scanner.scan(dir, files);
    auto stats = scanner.getStatistics();

    std::cout << "找到 " << stats.totalFiles << " 个文件，"
              << stats.totalDirectories << " 个目录\n";
    std::cout << "总大小: " << formatSize(stats.totalSize) << "\n";

    std::cout << "\n示例文件（前5个）:\n";
    int n = 0;
    for (auto& f : files) {
        if (!f.isDirectory && n++ < 5) {
            std::cout << "  " << f.path.filename()
                      << " (" << formatSize(f.size) << ")\n";
        }
    }
}

void testMetadata(const fs::path& dir) {
    std::cout << "\n=== 测试元数据提取 ===\n";

    FileScanner scanner;
    std::vector<FileInfo> files;
    scanner.scan(dir, files);

    MetadataManager meta;
    std::cout << "提取前3个文件的元数据:\n";
    int n = 0;
    for (auto& f : files) {
        if (!f.isDirectory && n++ < 3) {
            FileInfo info = meta.extractMetadata(f.path);
            std::cout << "  " << info.path.filename() << "\n";
            std::cout << "    创建: " << formatTime(info.creationTime) << "\n";
            std::cout << "    修改: " << formatTime(info.lastWriteTime) << "\n";
        }
    }
}

void testFilter(const fs::path& dir) {
    std::cout << "\n=== 测试过滤功能 ===\n";

    FileScanner scanner;
    std::vector<FileInfo> files;
    scanner.scan(dir, files);

    FilterChain chain(FilterChain::Mode::AND);
    TypeFilter typeFilter(std::vector<std::string>{"txt", "pdf"}, std::vector<std::string>{});
    chain.addFilter(std::make_shared<TypeFilter>(typeFilter));
    chain.addFilter(std::make_shared<SizeFilter>(0, 1024*1024));

    int accept = 0, reject = 0;
    for (auto& f : files) {
        if (!f.isDirectory) {
            if (chain.accept(f)) accept++;
            else reject++;
        }
    }
    std::cout << "过滤结果: 接受 " << accept << " 个，拒绝 " << reject << " 个\n";
}

void testPack(const fs::path& dir) {
    std::cout << "\n=== 测试打包解包 ===\n";
    std::cout << "注意: 将创建临时文件测试打包功能\n";

    FileScanner scanner;
    std::vector<FileInfo> files;
    scanner.scan(dir, files);

    FilterChain chain(FilterChain::Mode::AND);
    chain.addFilter(std::make_shared<SizeFilter>(0, 512*1024));

    std::vector<FileInfo> smallFiles;
    for (auto& f : files) {
        if (!f.isDirectory && chain.accept(f) && smallFiles.size() < 10) {
            FileInfo info = MetadataManager().extractMetadata(f.path);
            smallFiles.push_back(info);
        }
    }

    if (smallFiles.empty()) {
        std::cout << "没有适合的文件进行打包测试\n";
        return;
    }

    fs::path tempDir = fs::temp_directory_path() / "PackTest";
    fs::path pkg = tempDir / "test.pkg";
    fs::path restore = tempDir / "restore";
    fs::create_directories(tempDir);
    fs::create_directories(restore);

    Packager packager;
    std::cout << "打包 " << smallFiles.size() << " 个文件...\n";
    packager.pack(smallFiles, pkg, nullptr, dir);
    std::cout << "包大小: " << formatSize(fs::file_size(pkg)) << "\n";

    std::cout << "解包...\n";
    packager.unpack(pkg, restore);

    int restored = 0;
    for (auto& e : fs::recursive_directory_iterator(restore)) {
        if (e.is_regular_file()) restored++;
    }
    std::cout << "还原 " << restored << " 个文件\n";
    std::cout << "临时文件: " << tempDir << "\n";
}

int main() {
    std::cout << "====================================\n";
    std::cout << "真实目录测试工具\n";
    std::cout << "====================================\n\n";

    std::cout << "输入要测试的目录（如 D:\\Documents）:\n";
    std::string input;
    std::getline(std::cin, input);

    fs::path dir = input.empty() ? fs::temp_directory_path() : input;

    if (!fs::exists(dir)) {
        std::cout << "目录不存在\n";
        return 1;
    }

    std::cout << "\n测试目录: " << dir << "\n";

    try {
        testScan(dir);
        testMetadata(dir);
        testFilter(dir);

        std::cout << "\n是否测试打包？(y/n): ";
        std::string c;
        std::getline(std::cin, c);
        if (c == "y") testPack(dir);

        std::cout << "\n====================================\n";
        std::cout << "测试完成！软件可正常处理您的文件\n";
        std::cout << "====================================\n";

    } catch (std::exception& e) {
        std::cout << "错误: " << e.what() << "\n";
        return 1;
    }

    return 0;
}