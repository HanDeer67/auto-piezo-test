#include "dataanalysis.h"
#include <QDebug>

dataAnalysis::dataAnalysis(QObject *parent)
    : QObject{parent}
{}

// 计算标准差
double  dataAnalysis::computeStd(QVector<double> inputVector)
{
    if (inputVector.size() < 2) return 0.0;

    double sum = 0;
    for (double value : inputVector) sum += value;
    double mean = sum / inputVector.size();

    double variance = 0;
    for (double value : inputVector)
        variance += qPow(value - mean, 2);

    variance /= (inputVector.size());  // 统计标准差 (sample std)

    return qSqrt(variance);
}

double dataAnalysis::computeAve(QVector<double> inputVector)
{
    double tempAdd = 0.0;
    for(int i = 0 ; i < inputVector.size(); i++){
        tempAdd +=inputVector.at(i);
    }
    return tempAdd / inputVector.size();
}

QVector<double> dataAnalysis::computeOLS(QVector<double> Xlist, QVector<double> Ylist)
{
    QVector<double> kb;
    int n = Xlist.size();
    if (n < 2) {
        qDebug() << "数据点数量不足（至少需要2个点）";
        kb.append(0);
        kb.append(0);
        return kb;
    }

    // 计算中间变量：sumX, sumY, sumXY, sumX2
    double sumX = 0, sumY = 0, sumXY = 0, sumX2 = 0;
    for (int i = 0; i < n; ++i) {
        double x = Xlist[i];
        double y = Ylist[i];
        sumX += x;
        sumY += y;
        sumXY += x * y;
        sumX2 += x * x;
    }

    // 计算斜率k和截距b
    double k = (n * sumXY - sumX * sumY) / (n * sumX2 - sumX * sumX);  // 斜率
    // double b = (sumY - k * sumX) / n;  // 截距
    double meanX = sumX / n;
    double meanY = sumY / n;
    double b = meanY - k * meanX;
    qDebug() << "拟合直线：y = " << k << "x + " << b;
    kb.append(k);
    kb.append(b);


    // 计算最大残差ΔPｍａｘ
    double maxDistance = 0.0;
    int maxIndex = -1;  // 最远点索引
    // 计算 y 方向最大偏差 ΔPmax
    double maxResidualY = 0.0;
    int maxIndexY = -1;

    for (int i = 0; i < Xlist.size(); ++i) {
        double x = Xlist[i];
        double y = Ylist[i];
        // 垂直斜线的偏差
        double distance = std::abs(k*x - y + b) / std::sqrt(k*k + 1);
        // y方向偏差（符合线性度定义）
        double residualY = std::abs(y - (k * x + b));

        if (distance > maxDistance) {
            maxDistance = distance;
            maxIndex = i;
        }
        if (residualY > maxResidualY) {
            maxResidualY = residualY;
            maxIndexY = i;
        }
    }

    qDebug() << "偏离拟合直线最远的点索引:" << maxIndex
             << " x:" << Xlist[maxIndex]
             << " y:" << Ylist[maxIndex]
             << " 最大垂直距离:" << maxDistance;

    qDebug() << "偏离拟合直线最远的点索引:" << maxIndexY
             << " x:" << Xlist[maxIndexY]
             << " y:" << Ylist[maxIndexY]
             << " 最大距离Y方向:" << maxResidualY;

    // 计算线性度δＬ＝ΔPｍａｘ／T× １００％
    double yMin = *std::min_element(Ylist.begin(), Ylist.end());  // X最小值
    double yMax = *std::max_element(Ylist.begin(), Ylist.end());  // X最大值
    double L = yMax - yMin;
    double deltaL = maxResidualY / L;
    kb.append(maxResidualY);
    kb.append(deltaL);
    return kb;
}
