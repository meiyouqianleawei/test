#include <gtest/gtest.h>
#include "../src/filter/FilterChain.h"
#include "../src/filter/PathFilter.h"
#include "../src/filter/TypeFilter.h"
#include "../src/filter/NameFilter.h"
#include "../src/filter/TimeFilter.h"
#include "../src/filter/SizeFilter.h"
#include "../src/core/FileInfo.h"
#include <filesystem>

/**
 * @brief 文件过滤器测试套件
 *
 * 测试所有过滤器的功能：
 * - PathFilter：路径过滤
 * - TypeFilter：类型过滤
 * - NameFilter：文件名过滤
 * - TimeFilter：时间过滤
 * - SizeFilter：大小过滤
 * - FilterChain：过滤器链组合
 */
class FilterTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 创建测试文件信息
        testFile_.path = "C:\\Users\\test\\Documents\\file.txt";
        testFile_.size = 1024 * 1024;  // 1MB
        testFile_.creationTime = 132456789012345678;
        testFile_.lastWriteTime = 132456789012345678;
        testFile_.lastAccessTime = 132456789012345678;
    }

    FileInfo testFile_;
};

// ==================== PathFilter 测试 ====================

TEST_F(FilterTest, PathFilter_IncludePath) {
    PathFilter filter({"C:\\Users\\test"}, {});
    EXPECT_TRUE(filter.accept(testFile_));
}

TEST_F(FilterTest, PathFilter_ExcludePath) {
    PathFilter filter({}, {"C:\\Users\\test\\Documents"});
    EXPECT_FALSE(filter.accept(testFile_));
}

TEST_F(FilterTest, PathFilter_MixedPaths) {
    PathFilter filter({"C:\\Users\\test"}, {"C:\\Users\\test\\Documents"});
    EXPECT_FALSE(filter.accept(testFile_));  // 排除优先级更高
}

TEST_F(FilterTest, PathFilter_NoMatch) {
    PathFilter filter({"C:\\Other"}, {});
    EXPECT_FALSE(filter.accept(testFile_));
}

TEST_F(FilterTest, PathFilter_EmptyFilter) {
    PathFilter filter({}, {});
    EXPECT_TRUE(filter.accept(testFile_));  // 空过滤器接受所有
}

// ==================== TypeFilter 测试 ====================

TEST_F(FilterTest, TypeFilter_IncludeType) {
    TypeFilter filter({"txt", "pdf"}, {});
    EXPECT_TRUE(filter.accept(testFile_));
}

TEST_F(FilterTest, TypeFilter_ExcludeType) {
    TypeFilter filter({}, {"txt", "doc"});
    EXPECT_FALSE(filter.accept(testFile_));
}

TEST_F(FilterTest, TypeFilter_MixedTypes) {
    TypeFilter filter({"txt", "pdf"}, {"txt"});
    EXPECT_FALSE(filter.accept(testFile_));  // 排除优先级更高
}

TEST_F(FilterTest, TypeFilter_NoExtension) {
    FileInfo noExtFile;
    noExtFile.path = "C:\\Users\\test\\file";
    TypeFilter filter({"txt"}, {});
    EXPECT_FALSE(filter.accept(noExtFile));
}

TEST_F(FilterTest, TypeFilter_EmptyFilter) {
    TypeFilter filter({}, {});
    EXPECT_TRUE(filter.accept(testFile_));  // 空过滤器接受所有
}

TEST_F(FilterTest, TypeFilter_Directories) {
    FileInfo dir;
    dir.path = "C:\\Users\\test\\Documents";
    dir.isDirectory = true;

    TypeFilter filter({"txt"}, {});
    EXPECT_TRUE(filter.accept(dir));  // 目录不受类型过滤器影响
}

// ==================== NameFilter 测试 ====================

TEST_F(FilterTest, NameFilter_WildcardAsterisk) {
    NameFilter filter("*.txt");
    EXPECT_TRUE(filter.accept(testFile_));
}

TEST_F(FilterTest, NameFilter_WildcardQuestionMark) {
    NameFilter filter("file?.txt");
    FileInfo file1;
    file1.path = "C:\\Users\\test\\Documents\\file1.txt";
    EXPECT_TRUE(filter.accept(file1));  // file1.txt 匹配 file?.txt (9个字符)

    // file.txt 不应该匹配 file?.txt（长度不同）
    FileInfo fileNoNum;
    fileNoNum.path = "C:\\Users\\test\\Documents\\file.txt";
    EXPECT_FALSE(filter.accept(fileNoNum));  // file.txt 不匹配 file?.txt
}

TEST_F(FilterTest, NameFilter_ExactMatch) {
    NameFilter filter("file.txt");
    EXPECT_TRUE(filter.accept(testFile_));
}

TEST_F(FilterTest, NameFilter_NoMatch) {
    NameFilter filter("*.pdf");
    EXPECT_FALSE(filter.accept(testFile_));
}

TEST_F(FilterTest, NameFilter_ComplexWildcard) {
    NameFilter filter("f*.t?t");
    EXPECT_TRUE(filter.accept(testFile_));
}

TEST_F(FilterTest, NameFilter_CaseInsensitive) {
    NameFilter filter("*.TXT");
    EXPECT_TRUE(filter.accept(testFile_));  // 不区分大小写
}

// ==================== TimeFilter 测试 ====================

TEST_F(FilterTest, TimeFilter_InRange) {
    TimeFilter filter(TimeFilter::TimeType::CREATION,
                      testFile_.creationTime - 1000000,
                      testFile_.creationTime + 1000000);
    EXPECT_TRUE(filter.accept(testFile_));
}

TEST_F(FilterTest, TimeFilter_BeforeRange) {
    TimeFilter filter(TimeFilter::TimeType::CREATION,
                      testFile_.creationTime + 1000000,
                      testFile_.creationTime + 2000000);
    EXPECT_FALSE(filter.accept(testFile_));
}

TEST_F(FilterTest, TimeFilter_AfterRange) {
    TimeFilter filter(TimeFilter::TimeType::CREATION,
                      testFile_.creationTime - 2000000,
                      testFile_.creationTime - 1000000);
    EXPECT_FALSE(filter.accept(testFile_));
}

TEST_F(FilterTest, TimeFilter_NoLimit) {
    TimeFilter filter(TimeFilter::TimeType::CREATION, 0, 0);
    EXPECT_TRUE(filter.accept(testFile_));  // 无限制，接受所有
}

TEST_F(FilterTest, TimeFilter_LastWrite) {
    TimeFilter filter(TimeFilter::TimeType::LAST_WRITE,
                      testFile_.lastWriteTime - 1000000,
                      testFile_.lastWriteTime + 1000000);
    EXPECT_TRUE(filter.accept(testFile_));
}

TEST_F(FilterTest, TimeFilter_LastAccess) {
    TimeFilter filter(TimeFilter::TimeType::LAST_ACCESS,
                      testFile_.lastAccessTime - 1000000,
                      testFile_.lastAccessTime + 1000000);
    EXPECT_TRUE(filter.accept(testFile_));
}

// ==================== SizeFilter 测试 ====================

TEST_F(FilterTest, SizeFilter_InRange) {
    SizeFilter filter(512 * 1024, 2 * 1024 * 1024);  // 512KB - 2MB
    EXPECT_TRUE(filter.accept(testFile_));
}

TEST_F(FilterTest, SizeFilter_TooSmall) {
    SizeFilter filter(2 * 1024 * 1024, 10 * 1024 * 1024);  // 2MB - 10MB
    EXPECT_FALSE(filter.accept(testFile_));
}

TEST_F(FilterTest, SizeFilter_TooLarge) {
    SizeFilter filter(0, 512 * 1024);  // 0 - 512KB
    EXPECT_FALSE(filter.accept(testFile_));
}

TEST_F(FilterTest, SizeFilter_NoLimit) {
    SizeFilter filter(0, 0);
    EXPECT_TRUE(filter.accept(testFile_));  // 无限制，接受所有
}

TEST_F(FilterTest, SizeFilter_MinOnly) {
    SizeFilter filter(512 * 1024, 0);  // 最小512KB
    EXPECT_TRUE(filter.accept(testFile_));
}

TEST_F(FilterTest, SizeFilter_MaxOnly) {
    SizeFilter filter(0, 2 * 1024 * 1024);  // 最大2MB
    EXPECT_TRUE(filter.accept(testFile_));
}

TEST_F(FilterTest, SizeFilter_FromReadable) {
    SizeFilter filter = SizeFilter::fromReadable("512KB", "2MB");
    EXPECT_TRUE(filter.accept(testFile_));
}

TEST_F(FilterTest, SizeFilter_Directories) {
    FileInfo dir;
    dir.path = "C:\\Users\\test\\Documents";
    dir.isDirectory = true;

    SizeFilter filter(1024, 1024 * 1024);
    EXPECT_TRUE(filter.accept(dir));  // 目录不受大小过滤器影响
}

// ==================== FilterChain 测试 ====================

TEST_F(FilterTest, FilterChain_ANDMode) {
    auto pathFilter = std::make_shared<PathFilter>(PathFilter({"C:\\Users\\test"}, {}));
    auto typeFilter = std::make_shared<TypeFilter>(TypeFilter({"txt"}, {}));
    auto sizeFilter = std::make_shared<SizeFilter>(SizeFilter(512 * 1024, 2 * 1024 * 1024));

    FilterChain chain(FilterChain::Mode::AND);
    chain.addFilter(pathFilter);
    chain.addFilter(typeFilter);
    chain.addFilter(sizeFilter);

    EXPECT_TRUE(chain.accept(testFile_));  // 所有条件都满足

    // 测试不满足条件的情况
    FileInfo pdfFile;
    pdfFile.path = "C:\\Users\\test\\Documents\\file.pdf";
    pdfFile.size = 1024 * 1024;
    EXPECT_FALSE(chain.accept(pdfFile));  // 类型不匹配
}

TEST_F(FilterTest, FilterChain_ORMode) {
    auto pathFilter = std::make_shared<PathFilter>(PathFilter({"C:\\Other"}, {}));
    auto typeFilter = std::make_shared<TypeFilter>(TypeFilter({"txt"}, {}));

    FilterChain chain(FilterChain::Mode::OR);
    chain.addFilter(pathFilter);
    chain.addFilter(typeFilter);

    EXPECT_TRUE(chain.accept(testFile_));  // 类型匹配即可
}

TEST_F(FilterTest, FilterChain_EmptyChain) {
    FilterChain chain(FilterChain::Mode::AND);
    EXPECT_TRUE(chain.accept(testFile_));  // 空链接受所有文件
}

TEST_F(FilterTest, FilterChain_MixedFilters) {
    // 测试复杂组合：路径包含 + 类型排除 + 大小限制
    auto pathFilter = std::make_shared<PathFilter>(PathFilter({"C:\\Users\\test"}, {}));
    auto typeFilter = std::make_shared<TypeFilter>(TypeFilter({}, {"tmp", "bak"}));
    auto sizeFilter = std::make_shared<SizeFilter>(SizeFilter(0, 10 * 1024 * 1024));

    FilterChain chain(FilterChain::Mode::AND);
    chain.addFilter(pathFilter);
    chain.addFilter(typeFilter);
    chain.addFilter(sizeFilter);

    EXPECT_TRUE(chain.accept(testFile_));

    // 测试排除类型
    FileInfo tmpFile;
    tmpFile.path = "C:\\Users\\test\\file.tmp";
    tmpFile.size = 1024;
    EXPECT_FALSE(chain.accept(tmpFile));  // 类型被排除
}

TEST_F(FilterTest, FilterChain_GetFilterCount) {
    FilterChain chain(FilterChain::Mode::AND);
    EXPECT_EQ(chain.getFilterCount(), 0);

    chain.addFilter(std::make_shared<PathFilter>(PathFilter({}, {})));
    EXPECT_EQ(chain.getFilterCount(), 1);

    chain.addFilter(std::make_shared<TypeFilter>(TypeFilter({}, {})));
    EXPECT_EQ(chain.getFilterCount(), 2);

    chain.clearFilters();
    EXPECT_EQ(chain.getFilterCount(), 0);
}

// ==================== 综合测试 ====================

TEST_F(FilterTest, RealWorldScenario) {
    // 模拟真实场景：备份用户文档，排除临时文件和大文件

    // 创建过滤器链
    auto pathFilter = std::make_shared<PathFilter>(
        PathFilter({"C:\\Users\\test\\Documents"}, {"C:\\Users\\test\\Documents\\Temp"})
    );

    auto typeFilter = std::make_shared<TypeFilter>(
        TypeFilter({"txt", "pdf", "doc", "docx"}, {"tmp", "bak"})
    );

    auto sizeFilter = std::make_shared<SizeFilter>(
        SizeFilter(0, 100 * 1024 * 1024)  // 最大100MB
    );

    FilterChain chain(FilterChain::Mode::AND);
    chain.addFilter(pathFilter);
    chain.addFilter(typeFilter);
    chain.addFilter(sizeFilter);

    // 测试应接受的文件
    FileInfo validFile;
    validFile.path = "C:\\Users\\test\\Documents\\work.docx";
    validFile.size = 1024 * 1024;
    EXPECT_TRUE(chain.accept(validFile));

    // 测试应排除的文件（在Temp目录）
    FileInfo tempFile;
    tempFile.path = "C:\\Users\\test\\Documents\\Temp\\cache.tmp";
    tempFile.size = 1024;
    EXPECT_FALSE(chain.accept(tempFile));

    // 测试应排除的文件（类型不匹配）
    FileInfo exeFile;
    exeFile.path = "C:\\Users\\test\\Documents\\program.exe";
    exeFile.size = 1024 * 1024;
    EXPECT_FALSE(chain.accept(exeFile));

    // 测试应排除的文件（大小超限）
    FileInfo largeFile;
    largeFile.path = "C:\\Users\\test\\Documents\\large.pdf";
    largeFile.size = 200 * 1024 * 1024;  // 200MB
    EXPECT_FALSE(chain.accept(largeFile));
}