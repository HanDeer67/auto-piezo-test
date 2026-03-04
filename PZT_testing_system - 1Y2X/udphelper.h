#ifndef UDPHELPER_H
#define UDPHELPER_H

#include <QObject>
#include <QUdpSocket>

class UDPHelper : public QObject
{
    Q_OBJECT
public:
    explicit UDPHelper(QObject *parent = nullptr);

public:
    bool isRec = false;

private:
    QUdpSocket *socket;


public slots:
    void startRec();
    void stopRec();

private slots:
    void onReadyRead();  // 直接在这里处理数据

signals:
    void newDataReceived(QString unit, double x, double y);
    void dataRec(QByteArray dataRec);
};




#endif // UDPHELPER_H
