#include <QApplication>
#include "MainWindow.h"

int main(int argc, char* argv[]) {
    // 创建应用程序
    QApplication app(argc, argv);

    // 设置应用程序信息
    app.setApplicationName("数据备份软件");
    app.setApplicationVersion("1.0.0");
    app.setOrganizationName("DataBackup");

    // 创建并显示主窗口
    DataBackup::MainWindow mainWindow;
    mainWindow.setWindowTitle("数据备份软件");
    mainWindow.resize(900, 600);
    mainWindow.show();

    // 运行应用程序
    return app.exec();
}