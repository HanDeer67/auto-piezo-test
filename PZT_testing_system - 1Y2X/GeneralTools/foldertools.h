#ifndef FOLDERTOOLS_H
#define FOLDERTOOLS_H

#include <QObject>

class folderTools : public QObject
{
    Q_OBJECT
public:
    explicit folderTools(QObject *parent = nullptr);

public:
    bool createFolder(QString basePath,QString folderName);

signals:
};

#endif // FOLDERTOOLS_H
