#ifndef CSVMODULE_H
#define CSVMODULE_H

#include <QObject>
#include <QPointF>  // 或者 #include <QtGui/QPointF>

class CsvModule : public QObject
{
    Q_OBJECT
public:
    explicit CsvModule(QObject *parent = nullptr);

public:

    QVector<double> readCsvToVector(const QString &filePath, int colNo);
    void writeToCsv(QVector<QPointF> pointXY);

signals:
};

#endif // CSVMODULE_H
