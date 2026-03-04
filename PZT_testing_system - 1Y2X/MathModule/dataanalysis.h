#ifndef DATAANALYSIS_H
#define DATAANALYSIS_H

#include <QObject>

class dataAnalysis : public QObject
{
    Q_OBJECT
public:
    explicit dataAnalysis(QObject *parent = nullptr);

public:
    double  computeStd(QVector<double> inputVector);
    double  computeAve(QVector<double> inputVector);
    QVector<double>  computeOLS(QVector<double> Xlist, QVector<double> Ylist);// 最小二乘计算

signals:
};

#endif // DATAANALYSIS_H
