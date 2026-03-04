#include "datatransfer.h"

DataTransfer::DataTransfer(QObject *parent)
    : QObject(parent)
{
}

QByteArray DataTransfer::string2Bytearray(QString stringData)
{
    // 移除字符串中的空格
    QString hexStringData = stringData.remove(' ');
    // 将十六进制字符串转换为字节数组
    QByteArray byteArrayData = QByteArray::fromHex(hexStringData.toLatin1());
    // 发送信号
    // emit byteDataSignal(byteArrayData);
    return byteArrayData;
}

QString DataTransfer::Bytearray2string(QByteArray byteArrrayData){
    // 转换为十六进制字符串（带空格分隔）
    QString hexString;
    for(int i=0; i<byteArrrayData.size(); ++i) {
        hexString += QString("%1 ")
        .arg(static_cast<quint8>(byteArrrayData[i]), 2, 16, QLatin1Char('0'))
            .toUpper();
    }
    hexString = hexString.trimmed();  // 去掉末尾多余空格
    return hexString;
}

