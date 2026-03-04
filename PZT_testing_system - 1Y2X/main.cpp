#include "mainwindow.h"

#include <QApplication>
#include <QFile>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    qRegisterMetaType<SignalConfig>("SignalConfig");

    QString qssFilePath = ":/QssFiles/mainQss.qss";

    qDebug() << "QSS File Path:" << qssFilePath;
    QApplication::setStyle("Fusion"); // 启用更接近win11的主题
    QFile qssFile(qssFilePath);
    if (qssFile.open(QFile::ReadOnly)) { // 只需要打开一次
        qApp->setStyleSheet(qssFile.readAll());
        qssFile.close();
    } else {
        qDebug() << "Failed to open QSS file:" << qssFilePath;
    }

    MainWindow w;
    w.show();
    return a.exec();
}
