#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <visa.h>
#include "Structs.h"
#include "udphelper.h"
#include<QThread>
#include <QtCharts>
#include <udpsimulator.h>
#include "serieshelper.h"
#include "datatransfer.h"
#include "foldertools.h"
#include "csvmodule.h"
#include "dataanalysis.h"


QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

public:

    ViSession defaultRM, instr;
    ViStatus status;
    ViUInt32 retCount;
    char buffer[256] = {0};

    void testStart();

    void controlAFG1062();

    void RAMP_OUTPUT(SignalConfig inputConfig);
    void SINE_OUTPUT(SignalConfig inputConfig);
    void DC_OUTPUT(SignalConfig inputConfig, bool isToSwitch);
    void RAMP_OUTPUT_DIRECT(bool ch1Orch2);
    void SINE_OUTPUT_DIRECT(bool ch1Orch2);
    void DC_OUTPUT_DIRECT(bool ch1Orch2);


    void addToListCH1(bool addOrInsert);
    void fastAdd();
    void delFromListCH1();
    void clearListCH1();
    void startRun();
    QVector<SignalConfig> SignalList;

    QString offsetStartMain;

    UDPHelper *udpHelper;
    QThread *t1;

    QVector<QPointF>bufferPointX; // 准直仪数据缓存
    QVector<QPointF>bufferPointY; // 准直仪数据缓存
    QValueAxis *axisX;
    QValueAxis *axisY;
    QValueAxis *axisX_L;
    QValueAxis *axisY_L;
    QTimer* chartUpdateTimer;

    void updateChart();
    void updateAxisAutoRange(QLineSeries *series, QValueAxis *axis);
    void saveToCsv(double time, QString csvPath,QString saveToCsv);

    void clearContainer();
    QString timeStr;

    // 测试对象文件夹创建
    QString basePath = QCoreApplication::applicationDirPath(); // exe 所在目录
    QString filePath;

    folderTools *folderHelper;

    QString voltTitle;
    QString voltOffset;
    bool goOrBack = true;
    QString testObjectName;

    int cloneNo = 0;

    QFile *csvFile = nullptr;
    QTextStream *csvStream = nullptr;
    QTimer *csvTimer = nullptr;

    bool keepRun = true;
    void startCsvLogging(QString csvPath, QString csvName);
    void stopCsvLogging();
    CsvModule *csvHelper;
    dataAnalysis *dataAnalysisHelper;

    // QVector<double> stdXList;
    // QVector<double> stdYList;
    QVector<double> aveXSingleList;
    QVector<double> aveYSingleList;
    QVector<QVector<double>> aveXList;
    QVector<QVector<double>> aveYList;
    QVector<double> aveXListAveUp;
    // QVector<double> aveXListAveUpTest;
    // QVector<double> aveXListAveUpTestAll;
    QVector<double> aveXListAveDown;
    QVector<double> aveYListAveUp;
    QVector<double> aveYListAveDown;
    QVector<double> aveX_P;
    QVector<double> aveY_P;

    // double voltStart; // 这两个参数是用于在绘制线性度曲线时设置坐标轴的
    // int levelNumAxisX;
    QChart *chartLinearity_X;
    QChart *chartLinearity_Y;
    QChartView *chartViewLinearity_X;
    QChartView *chartViewLinearity_Y;
    QLayout *layoutLinearity_X;
    QLayout *layoutLinearity_Y;

    void dataProcessing(int repeatNumNow, int ListNum);
    void correctData(); // 当csv保存有问题时手动修正csv文件然后重新计算结果
public slots:
    void readCsvAndReloadX(QString csvPath, QString csvName, int i);
    void readCsvAndReloadY(QString csvPath, QString csvName, int i);

private:
    Ui::MainWindow *ui;
    void initAFG1062();
    void initSignalListTable();

    seriesHelper *seriesHelper1;
    DataTransfer *dataTransfer;

private:
    QChart *chartX;
    QChart *chartY;
    QLineSeries *seriesX;
    QLineSeries *seriesY;
    QLineSeries *seriesX_L;
    QLineSeries *seriesY_L;

    qint64 counter = 0;
    QValueAxis *axisTimeX;
    QValueAxis *axisTimeY;
    QValueAxis *axisVolt_X;
    QValueAxis *axisVolt_Y;

    UDPSimulator *sim;
    void saveToCsv();
};
#endif // MAINWINDOW_H
