#include "udpsimulator.h"

UDPSimulator::UDPSimulator(QObject *parent)
    : QObject{parent}
{

    socket = new QUdpSocket(this);
    timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &UDPSimulator::sendRandomData);
}

void UDPSimulator::start()
{
    timer->start(250);   // 每250ms发送一次
}

// void UDPSimulator::sendRandomData() {
//     int value = QRandomGenerator::global()->bounded(0, 100); // 生成 0-99
//     QByteArray data = QByteArray::number(value);
//     socket->writeDatagram(data, QHostAddress("127.0.0.1"), 25565);
//     qDebug()<<"数据发送完成";
// }

void UDPSimulator::sendRandomData() {

    QByteArray packet;

    // 包头
    packet.append(char(0xFE));

    // 固定单位
    QByteArray unitBytes = "MM  ";
    unitBytes.resize(4);
    packet.append(unitBytes);

    /// ---- 模拟真实振动 + 漂移 ----
    // static double t = 0;

    // // 模拟漂移趋势，例如零点随时间慢慢变化
    // double drift = sin(t * 0.01) * 0.05;

    // // 高斯噪声（传感器随机波动）
    // double xValue = gaussian(3.25 + drift, 0.02);
    // double yValue = gaussian(2.85 + drift, 0.02);

    // t += 1; // 控制波动趋势速度

    /// -----模拟线性增长-----
    static double t = 0;

    double slope = 1;   // 每个周期增长量（mm）
    double baseX = 3.25;
    double baseY = 2.85;

    double trend = slope * t;

    double xValue = gaussian(baseX + trend, 0.02);
    double yValue = gaussian(baseY + trend, 0.02);

    t += 1;


    // 加入数据
    packet.append(reinterpret_cast<char*>(&xValue), sizeof(double));
    packet.append(reinterpret_cast<char*>(&yValue), sizeof(double));

    // 包尾
    packet.append(char(0xFF));

    socket->writeDatagram(packet, QHostAddress("127.0.0.1"), 25565);

    qDebug() << "模拟准直仪数据  X:" << xValue << " Y:" << yValue;
}



double UDPSimulator::gaussian(double mean, double stddev)
{
    static bool hasSpare = false;
    static double spare;

    if(hasSpare)
    {
        hasSpare = false;
        return spare * stddev + mean;
    }

    hasSpare = true;

    double u, v, s;
    do {
        u = QRandomGenerator::global()->generateDouble() * 2.0 - 1.0;
        v = QRandomGenerator::global()->generateDouble() * 2.0 - 1.0;
        s = u*u + v*v;
    } while(s == 0.0 || s >= 1.0);

    s = std::sqrt(-2.0 * log(s) / s);
    spare = v * s;
    return mean + stddev * (u * s);
}
