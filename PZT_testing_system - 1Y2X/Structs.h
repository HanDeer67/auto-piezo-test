#ifndef STRUCTS_H
#define STRUCTS_H
#include <QString>



struct SignalConfig{
    // 信号通道
    QString Channel;

    // 时间戳
    QString timeDuration;
    QString timeWait;

    // 信号类型
    QString signalType;

    // 频率
    QString Freq;
    // 频率单位
    QString FreqUnit;
    double FreqMultiTimes = 1;

    // 幅值
    QString Ampl;
    // 幅值单位
    QString AmplUnit;
    double AmplMultiTimes = 1;

    // 起始相位
    QString startPha;

    // 偏置电压
    QString Offset;
    // 偏置电压单位
    QString OffsetUnit;
    double OffsetMultiTimes = 1;

    // 对称性
    QString Symm; // 0-100

    // 阶级数量
    QString levelNum;
    // 起始电压
    QString voltStart;
    // 步进
    QString step;
    // 重复次序
    QString cloneNo;
    // 往复标志
    bool goOrBackSign = true;
    // 电压title
    QString voltTitle;
    // 测试对象
    QString testObjectName;
};



#endif // STRUCTS_H
