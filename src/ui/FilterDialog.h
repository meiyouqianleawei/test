#pragma once

#include <QDialog>
#include <QCheckBox>
#include <QComboBox>
#include <QLineEdit>
#include <QSpinBox>
#include <QDateTimeEdit>
#include <QGroupBox>
#include <QLabel>
#include <QPushButton>
#include <QRadioButton>
#include <vector>
#include <memory>
#include <string>

#include "filter/IFileFilter.h"
#include "filter/FilterChain.h"

namespace DataBackup {

/**
 * 文件过滤配置
 *
 * 保存 5 种过滤器的启用状态与参数,可序列化为
 * std::vector<std::shared_ptr<IFileFilter>>供 BackupConfig 使用。
 */
struct FilterOptions {
    // 组合模式
    FilterChain::Mode mode = FilterChain::Mode::AND;

    // 1. 类型过滤器
    bool typeEnabled = false;
    std::vector<std::string> typeInclude;   // 包含扩展名(不带点)
    std::vector<std::string> typeExclude;   // 排除扩展名(不带点)

    // 2. 名称过滤器(通配符,如 *.log)
    bool nameEnabled = false;
    std::string namePattern;
    bool nameUseRegex = false;

    // 3. 路径过滤器
    bool pathEnabled = false;
    std::vector<std::string> pathInclude;
    std::vector<std::string> pathExclude;

    // 4. 大小过滤器(字节,0 表示不限)
    bool sizeEnabled = false;
    uint64_t sizeMin = 0;
    uint64_t sizeMax = 0;

    // 5. 时间过滤器
    bool timeEnabled = false;
    int timeType = 1;         // 0=创建,1=修改,2=访问
    uint64_t timeStart = 0;   // FILETIME(100ns since 1601-01-01),0 表示不限
    uint64_t timeEnd = 0;

    // 是否启用了任一过滤器
    bool anyEnabled() const {
        return typeEnabled || nameEnabled || pathEnabled || sizeEnabled || timeEnabled;
    }

    // 构建可传入 BackupConfig 的过滤器列表
    std::vector<std::shared_ptr<IFileFilter>> buildFilters() const;

    // 供 UI 显示的一行摘要
    std::string summary() const;
};

/**
 * 文件过滤配置对话框
 *
 * 五种过滤器:类型/名称/路径/大小/时间 + AND/OR 组合模式。
 */
class FilterDialog : public QDialog {
    Q_OBJECT

public:
    explicit FilterDialog(const FilterOptions& initial, QWidget* parent = nullptr);
    ~FilterDialog() override = default;

    FilterOptions options() const { return options_; }

private slots:
    void onAccept();
    void onReset();

private:
    void setupUI();
    void loadFromOptions(const FilterOptions& opts);

    // 解析输入辅助
    static std::vector<std::string> splitCsv(const QString& text);
    static QString joinCsv(const std::vector<std::string>& items);
    static uint64_t parseSizeText(const QString& text);
    static uint64_t qDateTimeToFileTime(const QDateTime& dt);
    static QDateTime fileTimeToQDateTime(uint64_t ft);

    FilterOptions options_;

    // 类型
    QGroupBox* typeGroup_;
    QLineEdit* typeIncludeEdit_;
    QLineEdit* typeExcludeEdit_;

    // 名称
    QGroupBox* nameGroup_;
    QLineEdit* namePatternEdit_;
    QCheckBox* nameRegexCheck_;

    // 路径
    QGroupBox* pathGroup_;
    QLineEdit* pathIncludeEdit_;
    QLineEdit* pathExcludeEdit_;

    // 大小(输入"1MB"这样)
    QGroupBox* sizeGroup_;
    QLineEdit* sizeMinEdit_;
    QLineEdit* sizeMaxEdit_;

    // 时间
    QGroupBox* timeGroup_;
    QComboBox* timeTypeCombo_;
    QCheckBox* timeStartCheck_;
    QDateTimeEdit* timeStartEdit_;
    QCheckBox* timeEndCheck_;
    QDateTimeEdit* timeEndEdit_;

    // 组合模式
    QRadioButton* modeAndRadio_;
    QRadioButton* modeOrRadio_;

    // 底部按钮
    QPushButton* okButton_;
    QPushButton* cancelButton_;
    QPushButton* resetButton_;
};

} // namespace DataBackup
