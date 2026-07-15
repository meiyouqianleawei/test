#include "FilterDialog.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QFormLayout>
#include <QMessageBox>
#include <QDateTime>
#include <QLocale>
#include <QStringList>

#include <algorithm>
#include <cctype>
#include <sstream>

#include "filter/TypeFilter.h"
#include "filter/NameFilter.h"
#include "filter/PathFilter.h"
#include "filter/SizeFilter.h"
#include "filter/TimeFilter.h"

namespace DataBackup {

// FILETIME 起点(1601-01-01)与 Unix 起点(1970-01-01)之间的 100ns 数
static constexpr uint64_t FILETIME_EPOCH_DIFF_100NS = 116444736000000000ULL;

// ---------- FilterOptions ----------

std::vector<std::shared_ptr<IFileFilter>> FilterOptions::buildFilters() const {
    std::vector<std::shared_ptr<IFileFilter>> filters;

    if (typeEnabled && (!typeInclude.empty() || !typeExclude.empty())) {
        filters.push_back(std::make_shared<TypeFilter>(typeInclude, typeExclude));
    }

    if (nameEnabled && !namePattern.empty()) {
        filters.push_back(std::make_shared<NameFilter>(namePattern, nameUseRegex));
    }

    if (pathEnabled && (!pathInclude.empty() || !pathExclude.empty())) {
        filters.push_back(std::make_shared<PathFilter>(pathInclude, pathExclude));
    }

    if (sizeEnabled && (sizeMin > 0 || sizeMax > 0)) {
        filters.push_back(std::make_shared<SizeFilter>(sizeMin, sizeMax));
    }

    if (timeEnabled && (timeStart > 0 || timeEnd > 0)) {
        TimeFilter::TimeType tt = TimeFilter::TimeType::LAST_WRITE;
        if (timeType == 0) tt = TimeFilter::TimeType::CREATION;
        else if (timeType == 2) tt = TimeFilter::TimeType::LAST_ACCESS;
        filters.push_back(std::make_shared<TimeFilter>(tt, timeStart, timeEnd));
    }

    return filters;
}

std::string FilterOptions::summary() const {
    if (!anyEnabled()) return "未启用过滤";

    std::ostringstream os;
    os << (mode == FilterChain::Mode::AND ? "AND: " : "OR: ");
    bool first = true;
    auto sep = [&]() { if (!first) os << ", "; first = false; };

    if (typeEnabled) { sep(); os << "类型"; }
    if (nameEnabled) { sep(); os << "名称"; }
    if (pathEnabled) { sep(); os << "路径"; }
    if (sizeEnabled) { sep(); os << "大小"; }
    if (timeEnabled) { sep(); os << "时间"; }

    return os.str();
}

// ---------- FilterDialog ----------

FilterDialog::FilterDialog(const FilterOptions& initial, QWidget* parent)
    : QDialog(parent)
    , options_(initial)
{
    setWindowTitle("文件过滤设置");
    setMinimumWidth(520);
    setupUI();
    loadFromOptions(initial);
}

void FilterDialog::setupUI() {
    QVBoxLayout* root = new QVBoxLayout(this);

    // 1. 类型过滤
    typeGroup_ = new QGroupBox("按文件类型过滤(扩展名,不带点)", this);
    typeGroup_->setCheckable(true);
    {
        QFormLayout* form = new QFormLayout(typeGroup_);
        typeIncludeEdit_ = new QLineEdit(typeGroup_);
        typeIncludeEdit_->setPlaceholderText("例:txt, docx, pdf  (逗号或空格分隔)");
        typeExcludeEdit_ = new QLineEdit(typeGroup_);
        typeExcludeEdit_->setPlaceholderText("例:tmp, log, bak");
        form->addRow("包含类型:", typeIncludeEdit_);
        form->addRow("排除类型:", typeExcludeEdit_);
    }
    root->addWidget(typeGroup_);

    // 2. 名称过滤
    nameGroup_ = new QGroupBox("按文件名过滤", this);
    nameGroup_->setCheckable(true);
    {
        QFormLayout* form = new QFormLayout(nameGroup_);
        namePatternEdit_ = new QLineEdit(nameGroup_);
        namePatternEdit_->setPlaceholderText("通配符:*.log 或 image?.jpg");
        nameRegexCheck_ = new QCheckBox("使用正则表达式", nameGroup_);
        form->addRow("匹配模式:", namePatternEdit_);
        form->addRow("", nameRegexCheck_);
    }
    root->addWidget(nameGroup_);

    // 3. 路径过滤
    pathGroup_ = new QGroupBox("按路径过滤(前缀匹配,不区分大小写)", this);
    pathGroup_->setCheckable(true);
    {
        QFormLayout* form = new QFormLayout(pathGroup_);
        pathIncludeEdit_ = new QLineEdit(pathGroup_);
        pathIncludeEdit_->setPlaceholderText("例:D:/src, D:/docs  (逗号分隔)");
        pathExcludeEdit_ = new QLineEdit(pathGroup_);
        pathExcludeEdit_->setPlaceholderText("例:D:/temp, D:/cache");
        form->addRow("包含路径:", pathIncludeEdit_);
        form->addRow("排除路径:", pathExcludeEdit_);
    }
    root->addWidget(pathGroup_);

    // 4. 大小过滤
    sizeGroup_ = new QGroupBox("按大小过滤(留空或 0 表示不限)", this);
    sizeGroup_->setCheckable(true);
    {
        QFormLayout* form = new QFormLayout(sizeGroup_);
        sizeMinEdit_ = new QLineEdit(sizeGroup_);
        sizeMinEdit_->setPlaceholderText("最小值,例:1KB, 10MB, 500B");
        sizeMaxEdit_ = new QLineEdit(sizeGroup_);
        sizeMaxEdit_->setPlaceholderText("最大值,例:100MB, 2GB");
        form->addRow("最小大小:", sizeMinEdit_);
        form->addRow("最大大小:", sizeMaxEdit_);
    }
    root->addWidget(sizeGroup_);

    // 5. 时间过滤
    timeGroup_ = new QGroupBox("按时间过滤", this);
    timeGroup_->setCheckable(true);
    {
        QFormLayout* form = new QFormLayout(timeGroup_);

        timeTypeCombo_ = new QComboBox(timeGroup_);
        timeTypeCombo_->addItem("创建时间", 0);
        timeTypeCombo_->addItem("修改时间", 1);
        timeTypeCombo_->addItem("访问时间", 2);
        form->addRow("时间类型:", timeTypeCombo_);

        QHBoxLayout* startRow = new QHBoxLayout();
        timeStartCheck_ = new QCheckBox("起始", timeGroup_);
        timeStartEdit_ = new QDateTimeEdit(QDateTime::currentDateTime().addYears(-1), timeGroup_);
        timeStartEdit_->setCalendarPopup(true);
        timeStartEdit_->setDisplayFormat("yyyy-MM-dd HH:mm:ss");
        startRow->addWidget(timeStartCheck_);
        startRow->addWidget(timeStartEdit_, 1);
        form->addRow("起始时间:", startRow);

        QHBoxLayout* endRow = new QHBoxLayout();
        timeEndCheck_ = new QCheckBox("结束", timeGroup_);
        timeEndEdit_ = new QDateTimeEdit(QDateTime::currentDateTime(), timeGroup_);
        timeEndEdit_->setCalendarPopup(true);
        timeEndEdit_->setDisplayFormat("yyyy-MM-dd HH:mm:ss");
        endRow->addWidget(timeEndCheck_);
        endRow->addWidget(timeEndEdit_, 1);
        form->addRow("结束时间:", endRow);
    }
    root->addWidget(timeGroup_);

    // 组合模式
    QGroupBox* modeGroup = new QGroupBox("组合模式(多过滤器时)", this);
    QHBoxLayout* modeLayout = new QHBoxLayout(modeGroup);
    modeAndRadio_ = new QRadioButton("AND(全部匹配才保留)", modeGroup);
    modeOrRadio_ = new QRadioButton("OR(任一匹配即保留)", modeGroup);
    modeAndRadio_->setChecked(true);
    modeLayout->addWidget(modeAndRadio_);
    modeLayout->addWidget(modeOrRadio_);
    modeLayout->addStretch();
    root->addWidget(modeGroup);

    // 按钮
    QHBoxLayout* btnRow = new QHBoxLayout();
    resetButton_ = new QPushButton("清空", this);
    okButton_ = new QPushButton("确定", this);
    cancelButton_ = new QPushButton("取消", this);
    okButton_->setDefault(true);
    btnRow->addWidget(resetButton_);
    btnRow->addStretch();
    btnRow->addWidget(okButton_);
    btnRow->addWidget(cancelButton_);
    root->addLayout(btnRow);

    connect(okButton_, &QPushButton::clicked, this, &FilterDialog::onAccept);
    connect(cancelButton_, &QPushButton::clicked, this, &QDialog::reject);
    connect(resetButton_, &QPushButton::clicked, this, &FilterDialog::onReset);
}

void FilterDialog::loadFromOptions(const FilterOptions& opts) {
    // 类型
    typeGroup_->setChecked(opts.typeEnabled);
    typeIncludeEdit_->setText(joinCsv(opts.typeInclude));
    typeExcludeEdit_->setText(joinCsv(opts.typeExclude));

    // 名称
    nameGroup_->setChecked(opts.nameEnabled);
    namePatternEdit_->setText(QString::fromStdString(opts.namePattern));
    nameRegexCheck_->setChecked(opts.nameUseRegex);

    // 路径
    pathGroup_->setChecked(opts.pathEnabled);
    pathIncludeEdit_->setText(joinCsv(opts.pathInclude));
    pathExcludeEdit_->setText(joinCsv(opts.pathExclude));

    // 大小(把字节还原为人类可读)
    sizeGroup_->setChecked(opts.sizeEnabled);
    sizeMinEdit_->setText(opts.sizeMin ? QString::number(opts.sizeMin) + "B" : "");
    sizeMaxEdit_->setText(opts.sizeMax ? QString::number(opts.sizeMax) + "B" : "");

    // 时间
    timeGroup_->setChecked(opts.timeEnabled);
    int idx = timeTypeCombo_->findData(opts.timeType);
    if (idx >= 0) timeTypeCombo_->setCurrentIndex(idx);

    timeStartCheck_->setChecked(opts.timeStart > 0);
    if (opts.timeStart > 0) timeStartEdit_->setDateTime(fileTimeToQDateTime(opts.timeStart));

    timeEndCheck_->setChecked(opts.timeEnd > 0);
    if (opts.timeEnd > 0) timeEndEdit_->setDateTime(fileTimeToQDateTime(opts.timeEnd));

    // 模式
    if (opts.mode == FilterChain::Mode::OR) modeOrRadio_->setChecked(true);
    else modeAndRadio_->setChecked(true);
}

void FilterDialog::onAccept() {
    FilterOptions opts;

    opts.typeEnabled = typeGroup_->isChecked();
    opts.typeInclude = splitCsv(typeIncludeEdit_->text());
    opts.typeExclude = splitCsv(typeExcludeEdit_->text());

    opts.nameEnabled = nameGroup_->isChecked();
    opts.namePattern = namePatternEdit_->text().trimmed().toStdString();
    opts.nameUseRegex = nameRegexCheck_->isChecked();

    opts.pathEnabled = pathGroup_->isChecked();
    opts.pathInclude = splitCsv(pathIncludeEdit_->text());
    opts.pathExclude = splitCsv(pathExcludeEdit_->text());

    opts.sizeEnabled = sizeGroup_->isChecked();
    opts.sizeMin = parseSizeText(sizeMinEdit_->text());
    opts.sizeMax = parseSizeText(sizeMaxEdit_->text());

    opts.timeEnabled = timeGroup_->isChecked();
    opts.timeType = timeTypeCombo_->currentData().toInt();
    opts.timeStart = (opts.timeEnabled && timeStartCheck_->isChecked())
        ? qDateTimeToFileTime(timeStartEdit_->dateTime()) : 0;
    opts.timeEnd = (opts.timeEnabled && timeEndCheck_->isChecked())
        ? qDateTimeToFileTime(timeEndEdit_->dateTime()) : 0;

    opts.mode = modeOrRadio_->isChecked()
        ? FilterChain::Mode::OR : FilterChain::Mode::AND;

    // 有效性校验
    if (opts.nameEnabled && opts.namePattern.empty()) {
        QMessageBox::warning(this, "警告", "启用名称过滤时,匹配模式不能为空");
        return;
    }
    if (opts.sizeEnabled && opts.sizeMin > 0 && opts.sizeMax > 0 && opts.sizeMin > opts.sizeMax) {
        QMessageBox::warning(this, "警告", "最小大小不能大于最大大小");
        return;
    }
    if (opts.timeEnabled && opts.timeStart > 0 && opts.timeEnd > 0 && opts.timeStart > opts.timeEnd) {
        QMessageBox::warning(this, "警告", "起始时间不能晚于结束时间");
        return;
    }

    options_ = opts;
    accept();
}

void FilterDialog::onReset() {
    loadFromOptions(FilterOptions{});
}

// ---------- 辅助函数 ----------

std::vector<std::string> FilterDialog::splitCsv(const QString& text) {
    std::vector<std::string> result;
    QString t = text;
    t.replace(';', ',').replace('\n', ',').replace('\t', ',');
    // 空格也视为分隔符,但路径中的空格需保留:这里按行的直觉,只用 , 分隔
    const QStringList parts = t.split(',', Qt::SkipEmptyParts);
    for (const QString& p : parts) {
        QString s = p.trimmed();
        if (!s.isEmpty()) result.push_back(s.toStdString());
    }
    return result;
}

QString FilterDialog::joinCsv(const std::vector<std::string>& items) {
    QStringList list;
    for (const auto& s : items) list << QString::fromStdString(s);
    return list.join(", ");
}

uint64_t FilterDialog::parseSizeText(const QString& text) {
    QString t = text.trimmed().toUpper();
    if (t.isEmpty() || t == "0" || t == "0B") return 0;

    // 提取数字和单位
    QString numStr;
    QString unit;
    bool numDone = false;
    for (QChar c : t) {
        if (!numDone && (c.isDigit() || c == '.')) {
            numStr += c;
        } else if (!c.isSpace()) {
            numDone = true;
            unit += c;
        }
    }
    if (numStr.isEmpty()) return 0;

    bool ok = false;
    double v = numStr.toDouble(&ok);
    if (!ok || v < 0) return 0;

    if (unit == "KB" || unit == "K") v *= 1024.0;
    else if (unit == "MB" || unit == "M") v *= 1024.0 * 1024.0;
    else if (unit == "GB" || unit == "G") v *= 1024.0 * 1024.0 * 1024.0;
    else if (unit == "TB" || unit == "T") v *= 1024.0 * 1024.0 * 1024.0 * 1024.0;
    // 默认按字节

    if (v < 0) return 0;
    return static_cast<uint64_t>(v);
}

uint64_t FilterDialog::qDateTimeToFileTime(const QDateTime& dt) {
    // 转成 UTC 的 msecs since epoch 再换算为 100ns 起自 1601-01-01
    qint64 msecsSinceUnixEpoch = dt.toMSecsSinceEpoch();
    if (msecsSinceUnixEpoch < 0) return 0;
    uint64_t hundredNs = static_cast<uint64_t>(msecsSinceUnixEpoch) * 10000ULL;
    return hundredNs + FILETIME_EPOCH_DIFF_100NS;
}

QDateTime FilterDialog::fileTimeToQDateTime(uint64_t ft) {
    if (ft < FILETIME_EPOCH_DIFF_100NS) return QDateTime::currentDateTime();
    uint64_t hundredNs = ft - FILETIME_EPOCH_DIFF_100NS;
    qint64 msecs = static_cast<qint64>(hundredNs / 10000ULL);
    return QDateTime::fromMSecsSinceEpoch(msecs);
}

} // namespace DataBackup
