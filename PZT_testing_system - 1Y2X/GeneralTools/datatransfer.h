#ifndef DATATRANSFER_H
#define DATATRANSFER_H

#include <QObject>
#include <QByteArray>

class DataTransfer : public QObject
{
    Q_OBJECT

signals:
    // void byteDataSignal(QByteArray data);  // 声明信号

public:
    explicit DataTransfer(QObject *parent = nullptr);
    QByteArray string2Bytearray(QString stringData);
    QString Bytearray2string(QByteArray byteArrrayData);

private:
    // 私有成员...
};

#endif // DATATRANSFER_H
