#include "udphelper.h"
#include <QDebug>
#include <QTimer>
#include <QEventLoop>
#include <QThread>

UDPHelper::UDPHelper(QObject *parent)
    : QObject{parent}
{

}


void UDPHelper::onReadyRead(){
    qDebug()<<"onReadyRead()";
    while (socket->hasPendingDatagrams()) {
        qDebug()<<"socket->hasPendingDatagrams()";
        QByteArray datagram;
        datagram.resize(socket->pendingDatagramSize());
        QHostAddress sender;
        quint16 senderPort;

        socket->readDatagram(datagram.data(), datagram.size(), &sender, &senderPort);

        emit dataRec(datagram.data());

        // 解析协议
        if (datagram.size() == 22) {
            const unsigned char *raw = reinterpret_cast<const unsigned char*>(datagram.data());

            if(raw[0] == 0xFE && raw[21] == 0xFF) {
                QByteArray unitBytes = datagram.mid(1, 4);
                QString unit = QString(unitBytes);

                double xValue = 0, yValue = 0;
                memcpy(&xValue, datagram.constData() + 5, 8);
                memcpy(&yValue, datagram.constData() + 13, 8);

                qDebug() << "Received - Unit:" << unit << "X:" << xValue << "Y:" << yValue;
                emit newDataReceived(unit, xValue, yValue);
            }
        }

    }
}

void UDPHelper::stopRec()
{
    socket->close();
}

void UDPHelper::startRec()
{
    // 输出当前线程地址
    qDebug() << "UDP thread ID:" << QThread::currentThreadId();
    socket = new QUdpSocket(this);
    // 绑定端口（25565根据你设备协议）
    if(socket->bind(QHostAddress("127.0.0.1"), 25565))
        qDebug() << "UDP bind success!";
    else
        qDebug() << "UDP bind FAILED!";

    connect(socket, &QUdpSocket::readyRead, this, &UDPHelper::onReadyRead);

    // while(isRec){

    //     // 开始接收UDP数据
    //     if (socket->hasPendingDatagrams())
    //     {
    //         QByteArray datagram;
    //         datagram.resize(socket->pendingDatagramSize());
    //         socket->readDatagram(datagram.data(), datagram.size());

    //         // ---- 解析协议 ----
    //         if (datagram.size() == 22)
    //         {
    //             const unsigned char *raw = reinterpret_cast<const unsigned char*>(datagram.data());

    //             if(raw[0] == 0xFE && raw[21] == 0xFF)
    //             {
    //                 QByteArray unitBytes = datagram.mid(1, 4);
    //                 QString unit = QString(unitBytes);

    //                 double xValue = 0, yValue = 0;
    //                 memcpy(&xValue, datagram.constData() + 5, 8);
    //                 memcpy(&yValue, datagram.constData() + 13, 8);

    //                 emit newDataReceived(unit, xValue, yValue);
    //             }
    //         }
    //     }

    //     // // 测试用
    //     // qDebug()<<"startRec()执行中";
    //     // QEventLoop loop;
    //     // QTimer::singleShot(static_cast<int>(1 * 1000), &loop, &QEventLoop::quit);
    //     // loop.exec();
    // }
}
