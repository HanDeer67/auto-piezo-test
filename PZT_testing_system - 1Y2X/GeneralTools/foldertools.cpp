#include "foldertools.h"
#include <QDir>
#include <QMessageBox>

folderTools::folderTools(QObject *parent)
    : QObject{parent}
{}

bool folderTools::createFolder(QString basePath, QString folderName)
{
    QString filePath = basePath + "/" + folderName;
    // 创建文件夹
    QDir saveFolder(filePath); // 这个代码并不会覆盖已经存在的文件夹
    if(!saveFolder.exists()){
        if(!saveFolder.mkpath(".")){
            return false;
        }
        else
            return true;
    }
    return true;
}

