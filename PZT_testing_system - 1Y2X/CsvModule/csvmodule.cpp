#include "csvmodule.h"
#include <QFile>
#include <QTextStream>
#include <QDir>
#include <QDirIterator>


CsvModule::CsvModule(QObject *parent)
    : QObject{parent}
{

}

// 读取 CSV 文件，假设每行一个数据
QVector<double> CsvModule::readCsvToVector(const QString& filePath, int colNo)
{
    QVector<double> data;
    QFile file(filePath);

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)){
        qDebug()<<"!file.open(QIODevice::ReadOnly | QIODevice::Text)";
        return data;
    }

    QTextStream in(&file);

    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        if (line.isEmpty()) continue;

        // 按逗号分割数据
        QStringList parts = line.split(",");

        // 确认是否存在该列
        if (colNo >= 0 && colNo < parts.size()) {
            bool ok;
            double value = parts[colNo].toDouble(&ok);
            if (ok)
                data.append(value);
        }
    }

    qDebug()<<"data.size"<<data.size();
    return data;

}



void CsvModule::writeToCsv(QVector<QPointF> pointXY)
{

}


