#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QButtonGroup>

#include <visa.h>
#include <QMessageBox>
#include <QThread>
#include <QTimer>
#include <QThread>
#include <cmath>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    qDebug() << "Main thread ID:" << QThread::currentThreadId();

    /// **********************************↓界面初始化↓****************************
    setWindowTitle("压电性能测试系统");

    QIcon iconApp(":/Icon/peach_princess_super_mario_bros_icon_232944 -mini.png");
    setWindowIcon(iconApp);

    // 创建状态栏 显示作者、版本、日期等信息
    QStatusBar *statusBar = new QStatusBar(this);
    setStatusBar(statusBar);
    // 创建标签并设置信息
    QLabel *infoLabel = new QLabel(this);
    infoLabel->setText("上海国科航星量子科技有限公司 | XuXiaohan | Version 1.0.3 | 2026.01.06");
    // 设置字体颜色
    infoLabel->setStyleSheet("color: gray;");

    /*infoLabel->setFont(QFont("Times New Roman", 18));
    infoLabel->setFont(QFont("Microsoft YaHei", 14)); // Windows
    QFont font;
    font.setFamily("Microsoft YaHei");
    font.setPointSize(14);
    infoLabel->setFont(font);
    statusBar->setMinimumHeight(40);
    infoLabel->setMinimumHeight(30);*/

    // 将标签添加到状态栏
    statusBar->addPermanentWidget(infoLabel);
    /// **********************************↑界面初始化↑****************************

    // 文件夹管理对象
    folderHelper = new folderTools(this);
    // 开启列表交替行颜色
    ui->tableWidget_ch1->setAlternatingRowColors(true);
    ui->tableWidget_results->setAlternatingRowColors(true);

    // combobox
    ui->comboBox_Freq->addItem("Hz", 1);
    ui->comboBox_Freq->addItem("kHz", 1000);
    ui->comboBox_Freq->addItem("MHz", 1000000);
    ui->comboBox_Freq->addItem("mHz", 0.001);

    ui->comboBox_Ampl->addItem("Vpp",1);
    ui->comboBox_Ampl->addItem("mVpp",0.001);

    ui->comboBox_Offset->addItem("Vpp",1);
    ui->comboBox_Offset->addItem("mVpp",0.001);

    // 绑定起始偏置电压
    connect(ui->lineEdit_start,&QLineEdit::textChanged,this,[=](const QString &text){
        // 阻止ui->lineEdit_Offset触发信号
        QSignalBlocker blocker(ui->lineEdit_Offset);
        ui->lineEdit_Offset->setText(text);
    });
    connect(ui->lineEdit_Offset,&QLineEdit::textChanged,this,[=](const QString &text){
        // 阻止ui->lineEdit_start触发信号
        QSignalBlocker blocker(ui->lineEdit_start);
        ui->lineEdit_start->setText(text);
    });


    QButtonGroup *group = new QButtonGroup(this);
    group->addButton(ui->pushButton_ch1_Sine);
    group->addButton(ui->pushButton_ch1_Ramp);
    group->addButton(ui->pushButton_ch1_Dc);
    group->setExclusive(true);

    initAFG1062();
    initSignalListTable();

    // 终止按钮
    connect(ui->pushButton_end,&QPushButton::clicked,this,[=](){
        keepRun = false;
    });
    // 重载按钮
    connect(ui->pushButton_reload,&QPushButton::clicked,this,[=](){
        initAFG1062();
    });

    // connect(ui->pushButton,&QPushButton::clicked,this,&MainWindow::controlAFG1062);
    connect(ui->pushButton_ch1_open,&QPushButton::clicked,this,[=](){
        if(ui->pushButton_ch1_Sine->isChecked()){
            SINE_OUTPUT_DIRECT(true);
            ui->textEdit_status->append("CH1立即执行正弦波输出");
        }
        else if(ui->pushButton_ch1_Ramp->isChecked()){
            RAMP_OUTPUT_DIRECT(true);
            ui->textEdit_status->append("CH1立即执行锯齿波输出");
        }
        else if(ui->pushButton_ch1_Dc->isChecked()){
            DC_OUTPUT_DIRECT(true);
            ui->textEdit_status->append("CH1立即执行DC直流输出");
        }
        else{
            return;
        }
        // viPrintf(instr, "OUTP1 ON\n");
    });
    connect(ui->pushButton_ch1_close,&QPushButton::clicked,this,[=](){
        viPrintf(instr, "OUTP1 OFF\n");
    });

    // 添加信号变换序列按钮
    connect(ui->pushButton_ch1_add,&QPushButton::clicked,this,[=](){
        addToListCH1(true);
    });

    // 插入信号变换序列按钮
    connect(ui->pushButton_ch1_insert,&QPushButton::clicked,this,[=](){
        addToListCH1(false);
    });

    // 删除一行按钮
    connect(ui->pushButton_ch1_del,&QPushButton::clicked,this,&MainWindow::delFromListCH1);
    // 清空按钮
    connect(ui->pushButton_ch1_clear,&QPushButton::clicked,this,&MainWindow::clearListCH1);

    // 开始控制信号发生器按照信号列表序列执行
    connect(ui->pushButton_start,&QPushButton::clicked,this,&MainWindow::startRun);

    // 快捷添加按钮
    connect(ui->pushButton_fastAdd,&QPushButton::clicked,this,&MainWindow::fastAdd);

    // 修正按钮
    connect(ui->pushButton_correction,&QPushButton::clicked,this,&MainWindow::correctData);


    /// *********************************自准直仪************************************
    // 创建线程
    t1 = new QThread();
    udpHelper = new UDPHelper();
    udpHelper->moveToThread(t1);
    connect(t1,&QThread::started,udpHelper,&UDPHelper::startRec);
    connect(t1, &QThread::finished, udpHelper, &UDPHelper::deleteLater);
    t1->start();

    // 实时显示数据曲线
    seriesX = new QLineSeries();
    seriesX->setName("X");

    seriesY = new QLineSeries();
    seriesY->setName("Y");

    // 创建图表
    chartX = new QChart();
    chartX->legend()->setVisible(false);
    chartX->addSeries(seriesX); // ☆
    chartX->setMargins(QMargins(0, 0, 0, 0)); // 去除图表边距
    chartX->setBackgroundRoundness(0); // 去除背景圆角（可选，避免圆角导致的空白）

    chartY = new QChart();
    chartY->legend()->setVisible(false);
    chartY->addSeries(seriesY);// ☆
    chartY->setMargins(QMargins(0, 0, 0, 0)); // 去除图表边距
    chartY->setBackgroundRoundness(0); // 去除背景圆角（可选，避免圆角导致的空白）

    // 创建坐标轴
    axisTimeX = new QValueAxis();
    axisTimeX->setRange(0, 100);
    // axisTimeX->setTickCount(10);
    axisTimeX->setTitleText("X");

    axisTimeY = new QValueAxis();
    axisTimeY->setRange(0, 100);
    // axisTimeX->setTickCount(10);
    axisTimeY->setTitleText("Y");

    seriesHelper1 = new seriesHelper(this);

    axisX = new QValueAxis();
    seriesHelper1->updateAxisRange(seriesX,axisX);// ☆
    axisX->setRange(-10, 10);
    axisX->setTitleText("θ");

    axisY = new QValueAxis();
    seriesHelper1->updateAxisRange(seriesY,axisY);// ☆
    axisY->setRange(-10, 10);
    axisY->setTitleText("θ");

    // 添加坐标轴到图表
    chartX->addAxis(axisTimeX, Qt::AlignBottom);
    chartX->addAxis(axisX, Qt::AlignLeft);

    chartY->addAxis(axisTimeY, Qt::AlignBottom);
    chartY->addAxis(axisY, Qt::AlignLeft);

    // 绑定曲线到坐标轴 (Qt6 新写法)
    seriesX->attachAxis(axisTimeX);
    seriesX->attachAxis(axisX);
    seriesY->attachAxis(axisTimeY);
    seriesY->attachAxis(axisY);

    // 显示到 UI
    QChartView *viewX = new QChartView(chartX);
    viewX->setRenderHint(QPainter::Antialiasing); // 抗锯齿（可选，但建议保留）
    viewX->setContentsMargins(0, 0, 0, 0); // 去除视图内边距
    QChartView *viewY = new QChartView(chartY);
    viewY->setRenderHint(QPainter::Antialiasing);
    viewY->setContentsMargins(0, 0, 0, 0); // 去除视图内边距

    // 如果你有布局用 layout→addWidget(view)，否则作为主 UI：
    // setCentralWidget(view);
    ui->verticalLayout_chart->addWidget(viewX);
    ui->verticalLayout_chart->addWidget(viewY);


    /// ☆☆☆☆☆☆☆☆☆☆☆显示线性度结果☆☆☆☆☆☆☆☆☆☆☆
    // 实时显示数据曲线
    seriesX_L = new QLineSeries();
    seriesX_L->setName("X_L");
    seriesY_L = new QLineSeries();
    seriesY_L->setName("Y_L");

    // 步骤1：创建图表和坐标轴
    // --------------------------
    chartLinearity_X = new QChart();
    chartLinearity_X->setTitle("最小二乘拟合直线");  // 图表标题
    chartLinearity_X->legend()->hide();  // 隐藏图例（可选）
    chartLinearity_X->addSeries(seriesX_L);
    chartLinearity_X->setMargins(QMargins(0, 0, 0, 0)); // 去除图表边距
    chartLinearity_X->setBackgroundRoundness(0);

    chartLinearity_Y = new QChart();
    chartLinearity_Y->setTitle("最小二乘拟合直线");  // 图表标题
    chartLinearity_Y->legend()->hide();  // 隐藏图例（可选）
    chartLinearity_Y->addSeries(seriesY_L);
    chartLinearity_Y->setMargins(QMargins(0, 0, 0, 0)); // 去除图表边距
    chartLinearity_Y->setBackgroundRoundness(0);

    // X轴
    axisVolt_X = new QValueAxis();
    axisVolt_X->setRange(0,100);
    axisVolt_X->setTitleText("U/V");
    axisVolt_Y = new QValueAxis();
    axisVolt_Y->setRange(0,100);
    axisVolt_Y->setTitleText("U/V");
    // Y轴
    axisX_L = new QValueAxis();
    axisY_L = new QValueAxis();
    axisX_L->setTitleText("P/μm");  // X轴标题（根据你的图修改）
    axisY_L->setTitleText("P/μm");  // Y轴标题（根据你的图修改）
    seriesHelper1->updateAxisRange(seriesX_L,axisX_L);
    seriesHelper1->updateAxisRange(seriesY_L,axisY_L);
    axisX_L->setRange(-10,10);
    axisY_L->setRange(-10,10);

    chartLinearity_X->addAxis(axisVolt_X, Qt::AlignBottom);
    chartLinearity_X->addAxis(axisX_L, Qt::AlignLeft);
    chartLinearity_Y->addAxis(axisVolt_Y, Qt::AlignBottom);
    chartLinearity_Y->addAxis(axisY_L, Qt::AlignLeft);

    // 绑定曲线到坐标轴 (Qt6 新写法)
    seriesX_L->attachAxis(axisVolt_X);
    seriesX_L->attachAxis(axisX_L);
    seriesY_L->attachAxis(axisVolt_Y);
    seriesY_L->attachAxis(axisY_L);

    chartViewLinearity_X = new QChartView(chartLinearity_X);
    chartViewLinearity_X->setRenderHint(QPainter::Antialiasing);  // 抗锯齿，使线条更平滑
    chartViewLinearity_Y = new QChartView(chartLinearity_Y);
    chartViewLinearity_Y->setRenderHint(QPainter::Antialiasing);  // 抗锯齿，使线条更平滑

    // 3. 将图表视图添加到 widget_linearity
    layoutLinearity_X = ui->verticalLayout_X;
    layoutLinearity_X->addWidget(chartViewLinearity_X);
    layoutLinearity_Y = ui->verticalLayout_Y;
    layoutLinearity_Y->addWidget(chartViewLinearity_Y);

    // 4. 可选：调整 widget_linearity 的大小策略
    chartViewLinearity_X->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    chartViewLinearity_Y->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    ui->widget_linearity_X->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    ui->widget_linearity_Y->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    /// ☆☆☆☆☆☆☆☆☆☆☆显示线性度结果☆☆☆☆☆☆☆☆☆☆☆

    dataTransfer = new DataTransfer(this);
    // 限制ui->textEdit_Rec最大行数
    ui->textEdit_Rec->document()->setMaximumBlockCount(50);
    connect(udpHelper, &UDPHelper::dataRec,this,[=](QByteArray dataRec){
        // ui->textEdit_Rec->append(QString::fromUtf8(dataRec));
        ui->textEdit_Rec->append(dataTransfer->Bytearray2string(dataRec));
    });

    chartUpdateTimer = new QTimer(this);
    chartUpdateTimer->setInterval(50); // 50ms刷新一次图，相当于20FPS
    connect(chartUpdateTimer, &QTimer::timeout, this, &MainWindow::updateChart);
    chartUpdateTimer->start();

    connect(udpHelper, &UDPHelper::newDataReceived, this,[=](QString unit, double x, double y){
        if (!csvStream) return; // 若当前阶段未打开CSV，则不写
        QString timestamp = QDateTime::currentDateTime().toString("hh:mm:ss.zzz");
        *csvStream << timestamp << "," << x << "," << y << "\n";
        csvStream->flush();
    });

    connect(udpHelper, &UDPHelper::newDataReceived, this,[=](QString unit, double x, double y){
        bufferPointX.append(QPointF(counter++,x));
        bufferPointY.append(QPointF(counter,y));

        qDebug() << "Unit:" << unit << " X:" << x << " Y:" << y;
        ui->lineEdit_x->setText(QString::number(x));
        ui->lineEdit_y->setText(QString::number(y));
        ui->lineEdit_unit->setText(unit);

        // TODO 更新 UI，例如 table/chart 等
        counter++; // 数据计数
        // seriesX->append(counter, x);
        // seriesY->append(counter, y);

        if (counter > 100)
        {
            // 获取横坐标轴（Qt6 推荐方式）
            auto axesX = chartX->axes(Qt::Horizontal);
            if (!axesX.isEmpty()) {
                QValueAxis* ax = qobject_cast<QValueAxis*>(axesX.first());
                if (ax) {
                    ax->setRange(counter - 100, counter);
                }
            }
            auto axesY = chartY->axes(Qt::Horizontal);
            if (!axesY.isEmpty()) {
                QValueAxis* ay = qobject_cast<QValueAxis*>(axesY.first());
                if (ay) {
                    ay->setRange(counter - 100, counter);
                }
            }
        }

        // 限制数据点数量，否则长时间运行会越来越卡
        if (seriesX->count() > 200) {
            // ui->textEdit_status->append("X总数据超过200，开始删除旧数据");
            seriesX->removePoints(0, seriesX->count() - 200);
        }
        if (seriesY->count() > 200) {
            // ui->textEdit_status->append("Y总数据超过200，开始删除旧数据");
            seriesY->removePoints(0, seriesY->count() - 200);
        }

    });

    sim = new UDPSimulator(this);
    // sim->start(); // 用于模拟准直仪给上位机发送数据

    //
    csvHelper = new CsvModule(this);
    dataAnalysisHelper = new dataAnalysis(this);
}

MainWindow::~MainWindow()
{
    keepRun = false;
    if (t1->isRunning())
    {
        t1->quit();  // 请求退出
        t1->wait();  // 阻塞等待线程结束（确保安全）
    }
    // === 关闭资源 ===
    viPrintf(instr, "OUTP1 OFF\n");
    viPrintf(instr, "OUTP2 OFF\n");
    viClose(instr);
    viClose(defaultRM);

//     // 注意：这里必须先退出线程再关闭资源：
// 主线程: viClose(instr)
//              ↓
//         t1线程: 正在 / 即将 调用 viPrintf(instr, ...)
//              ↓
// 💥 instr 已被关闭 → 野指针 / 无效句柄 → 崩溃



    delete t1;  // 会触发 finished → deleteLater → udpHelper 自然销毁
    // udpHelper->isRec = false;
    delete ui;
}

void MainWindow::testStart()
{

}

void MainWindow::RAMP_OUTPUT(SignalConfig inputConfig)
{
    QString SYMM =  inputConfig.Symm;
    QString FREQ = QString::number(inputConfig.Freq.toDouble() * inputConfig.FreqMultiTimes);
    QString freqUnit = inputConfig.FreqUnit;
    QString VOLT = QString::number(inputConfig.Ampl.toDouble() * inputConfig.AmplMultiTimes);
    QString voltUnit = inputConfig.AmplUnit;
    QString OFFSET = QString::number(inputConfig.Offset.toDouble() * inputConfig.OffsetMultiTimes);
    QString offsetUnit = inputConfig.OffsetUnit;
    QString channel;
    QString startPHA = inputConfig.startPha;
    if(inputConfig.Channel == "CH1"){
        channel = "CH1";
        // === 设置输出为三角波 ===
        viPrintf(instr, "SOUR1:FUNC RAMP\n"); // 波形选择
        viPrintf(instr, "SOUR1:FREQ %s\n",FREQ.toUtf8().data()); // 频率
        viPrintf(instr, "SOUR1:VOLT %s\n",VOLT.toUtf8().data()); // 电压
        viPrintf(instr, "SOUR1:VOLT:OFFS %s\n", OFFSET.toUtf8().data());

        viPrintf(instr, "SOUR1:PHASe:ADJust %sDEG\n", startPHA.toUtf8().data());
        viPrintf(instr, "SOUR1:RAMP:SYMM %s\n",SYMM.toUtf8().data()); // 对称性

        // 添加详细的调试信息
        qDebug() << "=== 调试信息 ===";
        qDebug() << "SYMM 值:" << SYMM <<"长度:" << SYMM.length();
        qDebug() << "startPHA 值:" << startPHA << "长度:" << startPHA.length();
        qDebug() << "发送的 SYMM 命令:" << QString("SOUR1:RAMP:SYMM %1").arg(SYMM);
        qDebug() << "发送的 PHAS 命令:" << QString("SOUR1:PHAS %1").arg(startPHA);
    }
    else{
        channel = "CH2";
        // === 设置输出为三角波 ===
        viPrintf(instr, "SOUR2:FUNC RAMP\n"); // 波形选择
        viPrintf(instr, "SOUR2:FREQ %s\n",FREQ.toUtf8().data()); // 频率
        viPrintf(instr, "SOUR2:VOLT %s\n",VOLT.toUtf8().data()); // 电压
        viPrintf(instr, "SOUR2:VOLT:OFFS %s\n", OFFSET.toUtf8().data());

        viPrintf(instr, "SOUR2:PHASe:ADJust %sDEG\n", startPHA.toUtf8().data());
        viPrintf(instr, "SOUR2:RAMP:SYMM %s\n",SYMM.toUtf8().data()); // 对称性  暂未上线
    }

    qDebug() << "通道"<<channel<<"输出已启动：三角波" <<FREQ<< freqUnit<<VOLT<< voltUnit<<"偏置" <<OFFSET<< offsetUnit;
    QString currentStatus = "通道"+channel+"输出已启动——>三角波：" +"频率："+FREQ+ freqUnit+"  "+ "幅值："+VOLT+ voltUnit+"  "+"偏置：" +OFFSET+ offsetUnit+"\n";
    ui->textEdit_status->append(currentStatus);
}

void MainWindow::SINE_OUTPUT(SignalConfig inputConfig)
{
    QString SYMM =  inputConfig.Symm;
    QString FREQ = QString::number(inputConfig.Freq.toDouble() * inputConfig.FreqMultiTimes);
    QString freqUnit = inputConfig.FreqUnit;
    QString VOLT = QString::number(inputConfig.Ampl.toDouble() * inputConfig.AmplMultiTimes);
    QString voltUnit = inputConfig.AmplUnit;
    QString OFFSET = QString::number(inputConfig.Offset.toDouble() * inputConfig.OffsetMultiTimes);
    QString offsetUnit = inputConfig.OffsetUnit;
    QString channel;
    QString startPHA = inputConfig.startPha;
    if(inputConfig.Channel == "CH1"){
        channel = "CH1";
        // === 设置输出为正弦波 ===
        viPrintf(instr, "SOUR1:FUNC SIN\n"); // 波形选择
        viPrintf(instr, "SOUR1:FREQ %s\n",FREQ.toUtf8().data()); // 频率
        viPrintf(instr, "SOUR1:VOLT %s\n",VOLT.toUtf8().data()); // 电压
        viPrintf(instr, "SOUR1:VOLT:OFFS %s\n", OFFSET.toUtf8().data());

        viPrintf(instr, "SOUR1:PHASe:ADJust %sDEG\n", startPHA.toUtf8().data());
        // viPrintf(instr, "SOUR1:PULSe:DCYCle %s\n",SYMM.toUtf8().data()); // 对称性
    }
    else{
        channel = "CH2";
        // === 设置输出为正弦波 ===
        viPrintf(instr, "SOUR2:FUNC SIN\n"); // 波形选择
        viPrintf(instr, "SOUR2:FREQ %s\n",FREQ.toUtf8().data()); // 频率
        viPrintf(instr, "SOUR2:VOLT %s\n",VOLT.toUtf8().data()); // 电压
        viPrintf(instr, "SOUR2:PHAS %s\n", startPHA.toUtf8().data());
        viPrintf(instr, "SOUR2:VOLT:OFFS %s\n", OFFSET.toUtf8().data());

        viPrintf(instr, "SOUR2:PHASe:ADJust %sDEG\n", startPHA.toUtf8().data());
        // viPrintf(instr, "SOUR2:PULSe:DCYCle %s\n",SYMM.toUtf8().data()); // 对称性
    }

    qDebug() << "通道"<<channel<<"输出已启动：正弦波" <<FREQ<< freqUnit<<VOLT<< voltUnit<<"偏置" <<OFFSET<< offsetUnit;
    QString currentStatus = "通道"+channel+"输出已启动——>正弦波：" +"频率："+FREQ+ freqUnit+"  "+ "幅值："+VOLT+ voltUnit+"  "+"偏置：" +OFFSET+ offsetUnit+"\n";
    ui->textEdit_status->append(currentStatus);
}

void MainWindow::DC_OUTPUT(SignalConfig inputConfig, bool isToSwitch)
{
    // 获取偏置电压
    QString OFFSET = inputConfig.Offset;
    qDebug()<<"OFFSET"<<OFFSET;
    // 获取偏置电压单位
    QString offsetUnit = inputConfig.OffsetUnit;
    // 获得偏置电压放大倍数（起始就是单位）
    double offsetMultiTimes = inputConfig.OffsetMultiTimes;
    qDebug()<<"offsetMultiTimes"<<offsetMultiTimes;
    OFFSET = QString::number(OFFSET.toDouble() * offsetMultiTimes);
    qDebug()<<"OFFSET"<<OFFSET;
    qDebug()<<"signal"<<inputConfig.signalType;
    // 获取通道
    QString channel;
    if(isToSwitch){
        channel = "CH2";
    }
    else{
        channel = inputConfig.Channel;
    }

    if(channel == "CH1"){
        qDebug()<<"CH1，正在输出直流";
        // channel = "CH1";
        // === 设置输出为直流 ===
        viPrintf(instr, "SOUR1:FUNC DC\n");
        viPrintf(instr, "SOUR1:VOLT:OFFSET %s\n",OFFSET.toUtf8().data()); // 偏置
    }
    else{
        qDebug()<<"CH2，正在输出直流";
        // channel = "CH2";
        // === 设置输出为直流 ===
        viPrintf(instr, "SOUR2:FUNC DC\n");
        viPrintf(instr, "SOUR2:VOLT:OFFS %s\n",OFFSET.toUtf8().data()); // 偏置
    }

    qDebug() << "通道"<<channel<<"输出已启动：DC，偏置 "<<OFFSET<< offsetUnit;
    QString currentStatus = "通道"+channel+"输出已启动：DC，偏置 "+OFFSET+ offsetUnit;
    ui->textEdit_status->append(currentStatus);
}

// 锯齿波
void MainWindow::RAMP_OUTPUT_DIRECT(bool ch1Orch2)
{
    // 获取配置参数
    QString SYMM;
    QString FREQ;
    QString freqUnit;
    QString VOLT;
    QString OFFSET;
    QString channel;
    QString voltUnit;
    QString offsetUnit;


    if(ch1Orch2){
        SYMM =  ui->lineEdit_Symmetry->text();
        FREQ = QString::number(ui->lineEdit_Freq->text().toDouble() * ui->comboBox_Freq->currentData().toDouble());
        // FREQ = ui->lineEdit_Freq->text();
        freqUnit = ui->comboBox_Freq->currentText();
        VOLT = QString::number(ui->lineEdit_Ampl->text().toDouble() * ui->comboBox_Ampl->currentData().toDouble());
        // VOLT = ui->lineEdit_Ampl->text();
        voltUnit = ui->comboBox_Ampl->currentText();
        OFFSET = QString::number(ui->lineEdit_OffsetSingle->text().toDouble() * ui->comboBox_Offset->currentData().toDouble());
        // OFFSET = ui->lineEdit_Offset->text();
        offsetUnit = ui->comboBox_Offset->currentText();
        channel = "CH1";
        // === 设置输出为三角波 ===
        viPrintf(instr, "SOUR1:FUNC RAMP\n"); // 波形选择
        viPrintf(instr, "SOUR1:FREQ %s\n",FREQ.toUtf8().data()); // 频率
        viPrintf(instr, "SOUR1:VOLT %s\n",VOLT.toUtf8().data()); // 电压
        viPrintf(instr, "SOUR1:VOLT:OFFS %s\n", OFFSET.toUtf8().data());
        viPrintf(instr, "SOUR1:RAMP:SYMM %s\n",SYMM.toUtf8().data()); // 对称性
    }


    qDebug() << "通道"<<channel<<"输出已启动：三角波" <<FREQ<< "Hz"<<VOLT<< voltUnit<<"偏置" <<OFFSET<< offsetUnit;
    QString currentStatus = "通道"+channel+"输出已启动——>三角波：" +"频率："+FREQ+ "Hz"+"  "+ "幅值："+VOLT+ voltUnit+"  "+"偏置：" +OFFSET+ offsetUnit+"\n";
    ui->textEdit_status->append(currentStatus);

}

// 正弦波
void MainWindow::SINE_OUTPUT_DIRECT(bool ch1Orch2)
{
    // 获取配置参数
    QString SYMM;
    QString FREQ;
    QString freqUnit;
    QString VOLT;
    QString OFFSET;
    QString channel;
    QString voltUnit;
    QString offsetUnit;

    if(ch1Orch2){
        SYMM =  ui->lineEdit_Symmetry->text();
        FREQ = QString::number(ui->lineEdit_Freq->text().toDouble() * ui->comboBox_Freq->currentData().toDouble());;
        freqUnit = ui->comboBox_Freq->currentText();
        VOLT = QString::number(ui->lineEdit_Ampl->text().toDouble() * ui->comboBox_Ampl->currentData().toDouble());
        voltUnit = ui->comboBox_Ampl->currentText();
        OFFSET = QString::number(ui->lineEdit_OffsetSingle->text().toDouble() * ui->comboBox_Offset->currentData().toDouble());
        offsetUnit = ui->comboBox_Offset->currentText();
        channel = "CH1";
        // === 设置输出为正弦波 ===
        viPrintf(instr, "SOUR1:FUNC SIN\n"); // 波形选择
        viPrintf(instr, "SOUR1:FREQ %s\n",FREQ.toUtf8().data()); // 频率
        viPrintf(instr, "SOUR1:VOLT %s\n",VOLT.toUtf8().data()); // 电压
        viPrintf(instr, "SOUR1:VOLT:OFFSET %s\n",OFFSET.toUtf8().data()); // 偏置
        viPrintf(instr, "SOUR1:SIN:SYMM %s\n",SYMM.toUtf8().data()); // 对称性
    }


    qDebug() << "通道"<<channel<<"输出已启动：正弦波" <<FREQ<< "Hz"<<VOLT<< voltUnit<<"偏置" <<OFFSET<< offsetUnit;
    QString currentStatus = "通道"+channel+"输出已启动——>正弦波：" +"频率："+FREQ+ "Hz"+"  "+ "幅值："+VOLT+ voltUnit+"  "+"偏置：" +OFFSET+ offsetUnit+"\n";
    ui->textEdit_status->append(currentStatus);

}

// 直流
void MainWindow::DC_OUTPUT_DIRECT(bool ch1Orch2)
{
    QString channel;
    QString OFFSET;
    QString offsetUnit;
    // 获取通道
    if(ch1Orch2){
        channel = "CH1";
        // 获取配置参数
        OFFSET = QString::number(ui->lineEdit_OffsetSingle->text().toDouble() * ui->comboBox_Offset->currentData().toDouble());
        qDebug()<<"OFFSET"<<OFFSET;
        // === 设置输出为直流 ===
        viPrintf(instr, "SOUR1:FUNC DC\n");
        viPrintf(instr, "SOUR1:VOLT:OFFSET %s\n",OFFSET.toUtf8().data()); // 偏置
        offsetUnit  = ui->comboBox_Offset->currentText();
        viPrintf(instr, "OUTP1 ON\n");
    }
    else{
        channel = "CH2";
        // 获取配置参数
        OFFSET = QString::number(ui->lineEdit_OffsetSingle->text().toDouble() * ui->comboBox_Offset->currentData().toDouble());
        // === 设置输出为直流 ===
        viPrintf(instr, "SOUR2:FUNC DC\n");
        viPrintf(instr, "SOUR2:VOLT:OFFSET %s\n",OFFSET.toUtf8().data()); // 偏置
        offsetUnit  = ui->comboBox_Offset->currentText();
        viPrintf(instr, "OUTP2 ON\n");
    }

    qDebug() << "通道"<<channel<<"输出已启动：DC直流，偏置 "<<OFFSET<< offsetUnit;
    QString currentStatus = "通道"+channel+"输出已启动：DC直流，偏置 "+OFFSET+ offsetUnit;
    ui->textEdit_status->append(currentStatus);

}

void MainWindow::addToListCH1(bool addOrInsert)
{
    int rowIndex = 0;
    if(addOrInsert){
        // 获取当前表格的总行数
        int rowCount = ui->tableWidget_ch1->rowCount();
        rowIndex = rowCount;
        ui->tableWidget_ch1->insertRow(rowIndex);
    }
    else{
        // 获取当前选中的行索引，如果没有选中，则弹窗警告，提示需要选中一行
        int insertNewRow = ui->tableWidget_ch1->currentRow(); // 获取当前选中行索引
        if (insertNewRow == -1) {
            QMessageBox::warning(this, "提示", "请先选中一行指令再执行该操作");
            return;
        }
        rowIndex = insertNewRow;
        ui->tableWidget_ch1->insertRow(rowIndex);
    }

    // 获取通道
    // QString channel = "CH1";
    // 获取时间戳
    QString timeDuration = ui->lineEdit_timeDelay->text();
    QString timeWait = ui->lineEdit_timeWait->text();
    // 获取起始相位
    QString startPhase = ui->lineEdit_StartPhase->text();
    // 获取频率
    QString freq = ui->lineEdit_Freq->text();
    // 获取频率单位
    QString freqUnit = ui->comboBox_Freq->currentText();
    // 获取频率倍率
    double freqMultiTimes = ui->comboBox_Freq->currentData().toDouble();
    // 获取电压
    QString ampl = ui->lineEdit_Ampl->text();
    // 获取电压单位
    QString amplUnit = ui->comboBox_Ampl->currentText();
    // 获取电压倍率
    double amplMultiTimes = ui->comboBox_Ampl->currentData().toDouble();
    // 起始相位
    QString startPha = ui->lineEdit_StartPhase->text();

    // 获取偏置
    QString offset = ui->lineEdit_Offset->text();
    // 获取偏置单位
    QString offsetUnit = ui->comboBox_Offset->currentText();
    // 获取偏置电压倍率
    double offsetMultiTimes = ui->comboBox_Offset->currentData().toDouble();
    // 获取对称性
    QString symm = ui->lineEdit_Symmetry->text();
    // 获取阶级数量
    QString levelNum = ui->lineEdit_levelNum->text();

    if(ui->pushButton_ch1_Dc->isChecked()){
        QTableWidgetItem* timeItem = new QTableWidgetItem(timeDuration);
        ui->tableWidget_ch1->setItem(rowIndex, 0, timeItem);
        QTableWidgetItem* signalItem = new QTableWidgetItem("DC");
        ui->tableWidget_ch1->setItem(rowIndex, 1, signalItem);
        QTableWidgetItem* startPhaseItem = new QTableWidgetItem("-");
        ui->tableWidget_ch1->setItem(rowIndex, 2, startPhaseItem);
        QTableWidgetItem* freqItem = new QTableWidgetItem("-");
        ui->tableWidget_ch1->setItem(rowIndex, 3, freqItem);
        QTableWidgetItem* amplItem = new QTableWidgetItem("-");
        ui->tableWidget_ch1->setItem(rowIndex, 4, amplItem);
        QTableWidgetItem* offsetItem = new QTableWidgetItem(offset + offsetUnit);
        ui->tableWidget_ch1->setItem(rowIndex, 5, offsetItem);
        QTableWidgetItem* symmItem = new QTableWidgetItem("-");
        ui->tableWidget_ch1->setItem(rowIndex, 6, symmItem);
        QTableWidgetItem* symmItemX = new QTableWidgetItem("");
        ui->tableWidget_ch1->setItem(rowIndex, 7, symmItemX);
        QTableWidgetItem* symmItemY = new QTableWidgetItem("");
        ui->tableWidget_ch1->setItem(rowIndex, 8, symmItemY);


        SignalConfig tempSignalConfig;
        tempSignalConfig.Channel = "CH1";
        tempSignalConfig.signalType = "DC";
        tempSignalConfig.Ampl = ampl;
        tempSignalConfig.AmplUnit = amplUnit;
        tempSignalConfig.AmplMultiTimes  = amplMultiTimes;
        tempSignalConfig.Freq = freq;
        tempSignalConfig.FreqUnit = freqUnit;
        tempSignalConfig.FreqMultiTimes = freqMultiTimes;
        tempSignalConfig.Offset = offset;
        tempSignalConfig.OffsetUnit = offsetUnit;
        tempSignalConfig.OffsetMultiTimes = offsetMultiTimes;
        tempSignalConfig.Symm = symm;
        tempSignalConfig.timeDuration = timeDuration;
        tempSignalConfig.timeWait = timeWait;
        tempSignalConfig.startPha = startPha;

        tempSignalConfig.goOrBackSign = goOrBack;
        tempSignalConfig.voltTitle = voltTitle;
        tempSignalConfig.cloneNo = QString::number(cloneNo);
        tempSignalConfig.testObjectName = testObjectName;
        tempSignalConfig.levelNum = levelNum;
        tempSignalConfig.voltStart = ui->lineEdit_start->text();
        tempSignalConfig.step = ui->lineEdit_stepSize->text();

        if(addOrInsert){
            SignalList.append(tempSignalConfig);
        }
        else{
            SignalList.insert(rowIndex,tempSignalConfig);
        }
        int num = SignalList.length();
        for(int i = 0 ; i < num ; i ++){
            qDebug()<<SignalList.at(i).signalType;
        }

    }
    else if(ui->pushButton_ch1_Sine->isChecked()){
        QTableWidgetItem* timeItem = new QTableWidgetItem(timeDuration);
        ui->tableWidget_ch1->setItem(rowIndex, 0, timeItem);
        QTableWidgetItem* signalItem = new QTableWidgetItem("SINE");
        ui->tableWidget_ch1->setItem(rowIndex, 1, signalItem);
        QTableWidgetItem* startPhaseItem = new QTableWidgetItem(startPhase);
        ui->tableWidget_ch1->setItem(rowIndex, 2, startPhaseItem);
        QTableWidgetItem* freqItem = new QTableWidgetItem(freq + freqUnit);
        ui->tableWidget_ch1->setItem(rowIndex, 3, freqItem);
        QTableWidgetItem* amplItem = new QTableWidgetItem(ampl + amplUnit);
        ui->tableWidget_ch1->setItem(rowIndex, 4, amplItem);
        QTableWidgetItem* offsetItem = new QTableWidgetItem(offset + offsetUnit);
        ui->tableWidget_ch1->setItem(rowIndex, 5, offsetItem);
        QTableWidgetItem* symmItem = new QTableWidgetItem(symm+"%");
        ui->tableWidget_ch1->setItem(rowIndex, 6, symmItem);
        SignalConfig tempSignalConfig;

        tempSignalConfig.Channel = "CH1";
        tempSignalConfig.signalType = "SINE";
        tempSignalConfig.Ampl = ampl;
        tempSignalConfig.AmplUnit = amplUnit;
        tempSignalConfig.AmplMultiTimes  = amplMultiTimes;
        tempSignalConfig.Freq = freq;
        tempSignalConfig.FreqUnit = freqUnit;
        tempSignalConfig.FreqMultiTimes = freqMultiTimes;
        tempSignalConfig.Offset = offset;
        tempSignalConfig.OffsetUnit = offsetUnit;
        tempSignalConfig.OffsetMultiTimes = offsetMultiTimes;
        tempSignalConfig.Symm = symm;
        tempSignalConfig.timeDuration = timeDuration;
        tempSignalConfig.startPha = startPha;

        if(addOrInsert){
            SignalList.append(tempSignalConfig);
        }
        else{
            SignalList.insert(rowIndex,tempSignalConfig);
        }
        int num = SignalList.length();
        for(int i = 0 ; i < num ; i ++){
            qDebug()<<SignalList.at(i).signalType;
        }
    }
    else if(ui->pushButton_ch1_Ramp->isChecked()){
        QTableWidgetItem* timeItem = new QTableWidgetItem(timeDuration);
        ui->tableWidget_ch1->setItem(rowIndex, 0, timeItem);
        QTableWidgetItem* signalItem = new QTableWidgetItem("RAMP");
        ui->tableWidget_ch1->setItem(rowIndex, 1, signalItem);
        QTableWidgetItem* startPhaseItem = new QTableWidgetItem(startPhase);
        ui->tableWidget_ch1->setItem(rowIndex, 2, startPhaseItem);
        QTableWidgetItem* freqItem = new QTableWidgetItem(freq + freqUnit);
        ui->tableWidget_ch1->setItem(rowIndex, 3, freqItem);
        QTableWidgetItem* amplItem = new QTableWidgetItem(ampl + amplUnit);
        ui->tableWidget_ch1->setItem(rowIndex, 4, amplItem);
        QTableWidgetItem* offsetItem = new QTableWidgetItem(offset + offsetUnit);
        ui->tableWidget_ch1->setItem(rowIndex, 5, offsetItem);
        QTableWidgetItem* symmItem = new QTableWidgetItem(symm+"%");
        ui->tableWidget_ch1->setItem(rowIndex, 6, symmItem);

        SignalConfig tempSignalConfig;
        tempSignalConfig.Channel = "CH1";
        tempSignalConfig.signalType = "RAMP";
        tempSignalConfig.Ampl = ampl;
        tempSignalConfig.AmplUnit = amplUnit;
        tempSignalConfig.AmplMultiTimes  = amplMultiTimes;
        tempSignalConfig.Freq = freq;
        tempSignalConfig.FreqUnit = freqUnit;
        tempSignalConfig.FreqMultiTimes = freqMultiTimes;
        tempSignalConfig.Offset = offset;
        tempSignalConfig.OffsetUnit = offsetUnit;
        tempSignalConfig.OffsetMultiTimes = offsetMultiTimes;
        tempSignalConfig.Symm = symm;
        tempSignalConfig.timeDuration = timeDuration;
        tempSignalConfig.startPha = startPha;

        if(addOrInsert){
            SignalList.append(tempSignalConfig);
        }
        else{
            SignalList.insert(rowIndex,tempSignalConfig);
        }
        int num = SignalList.length();
        for(int i = 0 ; i < num ; i ++){
            qDebug()<<SignalList.at(i).signalType;
        }
    }
}

void MainWindow::fastAdd()
{
    clearListCH1();
    testObjectName = ui->lineEdit_testObject->text();
    // 设置电压title 例如0.6、0.7 ...
    voltTitle= ui->lineEdit_stepSize->text();
    // 创建文件夹，文件夹1需要包含产品型号（mario-1） “开始”按钮中已完成
    // 创建文件夹，文件夹2需要包含电压title（0.6） 每一行信号执行时都要寻找这个文件夹并保存在其中
    // csv文件命名需要包含偏置电压（1.8）、往复标志（true）、序次编号


    // 获取起始偏置
    offsetStartMain =  ui->lineEdit_start->text();
    double offsetStart = ui->lineEdit_start->text().toDouble();
    // 获取步进
    double offsetStep = ui->lineEdit_stepSize->text().toDouble();
    // 获取层级数量
    int stepNum = ui->lineEdit_levelNum->text().toInt();
    // 获取重复次数
    int repeatNum = ui->lineEdit_repeat->text().toInt();
    for(int j = 0 ; j < repeatNum ; j++){
        // 设置序次编号 例如1、2 ...
        cloneNo  = j + 1;
        offsetStart = offsetStartMain.toDouble();
        for(int i = 0 ; i < stepNum ; i++){
            goOrBack = true;
            ui->lineEdit_Offset->setText(QString::number(offsetStart));
            voltOffset = QString::number(offsetStart);
            addToListCH1(true);

            offsetStart+=offsetStep;
        }
        // 往复测试
        if(ui->checkBox_back->isChecked()){
            offsetStart -= offsetStep;            for(int i = 0 ; i < stepNum ; i++){
                goOrBack = false;
                if (qAbs(offsetStart) < 1e-10) {  // 认为足够接近0
                    offsetStart = 0.0;
                }
                ui->lineEdit_Offset->setText(QString::number(offsetStart));
                voltOffset = QString::number(offsetStart);
                addToListCH1(true);
                offsetStart -= offsetStep;
                qDebug()<<"offsetStart"<<offsetStart;
            }
        }
    }
    ui->lineEdit_start->setText(offsetStartMain);
    QVector<QString> tempQStringList;
    for(int i = 0 ; i < stepNum ; i++){
        tempQStringList.append(QString::number(i*offsetStep));
    }
    ui->tableWidget_results->setRowCount(stepNum);
    ui->tableWidget_results->setVerticalHeaderLabels(tempQStringList
    );
    ui->tableWidget_results->verticalHeader()->setSectionResizeMode(QHeaderView::Stretch);


    ui->tableWidget_results->setColumnCount(12);
    ui->tableWidget_results->setHorizontalHeaderLabels({
        "Px↑",
        "Px↓",
        "Px",
        "Rx↑",
        "Rx↓",
        "Rx",
        "Py↑",
        "Py↓",
        "Py",
        "Ry↑",
        "Ry↓",
        "Ry"
    });
    ui->tableWidget_results->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    // 允许用户拖动列分隔线调整列宽
    ui->tableWidget_results->horizontalHeader()->setSectionsMovable(true);
}

void MainWindow::delFromListCH1()
{
    int curInxdex = ui->tableWidget_ch1->currentRow();
    if(curInxdex == -1){
        QMessageBox::warning(this, "提示", "请先选中一行指令再执行该操作");
        return;
    }
    ui->tableWidget_ch1->removeRow(curInxdex); // 删除该行
    // 同时还要删除docNodeList中对应的元素
    SignalList.removeAt(curInxdex);
}



void MainWindow::clearListCH1()
{
    ui->tableWidget_ch1->setRowCount(0); // 删除所有行
    initSignalListTable();
    SignalList.clear();

    ui->tableWidget_results->setRowCount(0); // 删除所有行
    // initSignalListTable();
}


void MainWindow::startRun()
{
    // 为了防止软件进行下一轮测试时收到上轮测试数据的影响，在每次点击运行时要先清空全部变量缓存

    clearContainer();

    // 检测列表tableWidget_ch1是否为空
    if (ui->tableWidget_ch1->rowCount() == 0) {
        return;
    }

    keepRun = true;
    // 根据测试对象创建文件夹
    // 获取当前系统时间
    timeStr = QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss_zzz");
    testObjectName = testObjectName + "-" +timeStr;
    bool  createSucc= folderHelper->createFolder(basePath,testObjectName);
    if(!createSucc){
        QMessageBox::critical(this, "错误", "无法创建文件夹: " + testObjectName);
        return;
    }

    viPrintf(instr, "OUTP1 ON\n"); // 一开始就打开示波器，是否有意义？


    // 获取重复次数
    int repeatNumNow = SignalList.last().cloneNo.toInt();
    qDebug()<<"重复次数"<<repeatNumNow;// 6
    // 计算组别个数
    int ListNum = SignalList.size() / repeatNumNow;
    qDebug()<<"信号总长度"<<SignalList.size();// 11*6*2=132
    qDebug()<<"组别个数"<<ListNum;// 22，指的是上升11个和下降11个

    if (SignalList.size() % repeatNumNow != 0) {
        // 不能整除，报错
        ui->textEdit_status->append( "Error: SignalList.size() is not divisible by repeatNumNow!");
        return;
    }

    /// ※※※※※※※※※※※※※※※上半场（测x）CH1轮询变化，CH2持续为4V※※※※※※※※※※※※※※※※※※※※※※※
    // 从SignalList中按顺序控制信号发生器切换信号
    for(int i = 0 ; i < SignalList.length(); i++){
        if(!keepRun){
            return;
        }
        // 获取信号类型
        QString signalType = SignalList.at(i).signalType;
        // 获取持续时间
        double  timeDuration= SignalList.at(i).timeDuration.toDouble();
        double  timeWait= SignalList.at(i).timeWait.toDouble();
        QString csvName;
        QString savePath;
        if(signalType == "DC"){
            // 上半场（测x）CH1轮询变化，CH2持续为4V
            DC_OUTPUT(SignalList.at(i),false);

            ui->textEdit_status->append("CH1当前正在执行升降直流输出，CH2维持稳定直流输出");
            // 设置CH2输出偏置为4V的直流
            DC_OUTPUT_DIRECT(false);

            // 保存（csv）并分析准直仪数据
            // 获取偏置电压
            QString offsetVolt  = SignalList.at(i).Offset;
            // 获取电压title
            QString voltTitle = SignalList.at(i).voltTitle;
            // 获取往复标志
            QString goOrBack;
            if(SignalList.at(i).goOrBackSign){
                goOrBack = "up";
            }
            else{
                goOrBack = "down";
            }
            // 获取测试对象名称
            QString testObjectName = SignalList.at(i).testObjectName + "-" +timeStr;
            // 获取重复次序
            QString cloneNo = SignalList.at(i).cloneNo;
            // 根据上述信号特征创建属性文件夹和csv文件
            // 1. 属性文件夹
            bool createSucc = folderHelper->createFolder(basePath +"/"+ testObjectName , voltTitle);
            if(!createSucc){
                QMessageBox::critical(this, "错误", "无法创建文件夹: " + voltTitle);
                return;
            }
            bool createSucc1 = folderHelper->createFolder(basePath +"/"+ testObjectName +"/"+ voltTitle, "2");
            if(!createSucc1){
                QMessageBox::critical(this, "错误", "无法创建文件夹: 2");
                return;
            }
            csvName = offsetVolt +"-"+ goOrBack  +"-"+ cloneNo;
            savePath = basePath +"/"+ testObjectName + "/" + voltTitle+ "/" + "2";
            // saveToCsv(timeDuration, savePath, csvName);
            // ⭐ 开启采集
            // // 使用QTimer实现非阻塞延时
            // double timeWait = ui->lineEdit_timeWait->text().toDouble();
            // double timeDelay = ui->lineEdit_timeDelay->text().toDouble();
            if(timeWait>=timeDuration){
                timeWait = timeDuration;
            }
            QEventLoop loop;
            QTimer::singleShot(static_cast<int>(timeWait * 1000), &loop, &QEventLoop::quit);
            loop.exec();
            startCsvLogging(savePath, csvName);
        }
        else if(signalType == "SINE"){
            SINE_OUTPUT(SignalList.at(i));
            ui->textEdit_status->append("当前正在执行正弦波输出");
        }
        else if(signalType == "RAMP"){
            RAMP_OUTPUT(SignalList.at(i));
            ui->textEdit_status->append("当前正在执行锯齿波输出");
        }

        // 使用QTimer实现非阻塞延时
        QEventLoop loop;
        QTimer::singleShot(static_cast<int>((timeDuration - timeWait) * 1000), &loop, &QEventLoop::quit);
        loop.exec();

        // 当前执行的信号所对应的信号列表的行高亮
        // ⭐ 结束采集
        stopCsvLogging();

        readCsvAndReloadY(savePath,csvName,i);

        // // 寻找名为csvName的文件，读取其数据并计算X和Y数据的标准差
        // QVector<double> csvDataY = csvHelper->readCsvToVector(savePath + "/" + csvName + ".csv", 2);
        // // QVector<double> csvDataY = csvHelper->readCsvToVector(savePath + "/" + csvName + ".csv", 1);

        // /** 开始计算标准差
        // // double stdX = dataAnalysisHelper->computeStd(csvDataX);
        // // double stdY = dataAnalysisHelper->computeStd(csvDataY);
        // // stdXList.append(stdX);
        // // stdYList.append(stdY);

        // // // 往表格填充标准差数据
        // // QTableWidgetItem * ItemX = new QTableWidgetItem(QString::number(stdX));
        // // ui->tableWidget_ch1->setItem(i,7,ItemX);
        // // QTableWidgetItem * ItemY = new QTableWidgetItem(QString::number(stdY));
        // // ui->tableWidget_ch1->setItem(i,8,ItemY);
        // */

        // // 开始计算均值
        // double aveY = dataAnalysisHelper->computeAve(csvDataY);
        // // double aveX = dataAnalysisHelper->computeAve(csvDataX);
        // aveYSingleList.append(aveY); // 均值Y列表
        // // aveXSingleList.append(aveX); // 均值X列表

        // // 往表格填充均值数据
        // QTableWidgetItem * ItemY = new QTableWidgetItem(QString::number(aveY));
        // ui->tableWidget_ch1->setItem(i,8,ItemY);
        // // QTableWidgetItem * ItemY = new QTableWidgetItem(QString::number(aveY));
        // // ui->tableWidget_ch1->setItem(i,8,ItemY);
    }

    /// ※※※※※※※※※※下半场（测y）CH2轮询变化，CH1持续为4V※※※※※※※※※※※※※※※※※※※※※
    // 从SignalList中按顺序控制信号发生器切换信号
    for(int i = 0 ; i < SignalList.length(); i++){
        if(!keepRun){
            return;
        }
        // 获取信号类型
        QString signalType = SignalList.at(i).signalType;
        // 获取持续时间
        double  timeDuration= SignalList.at(i).timeDuration.toDouble();
        double  timeWait= SignalList.at(i).timeWait.toDouble();
        QString csvName;
        QString savePath;
        QString cloneNo1;
        if(signalType == "DC"){
            // 上半场CH1轮询变化，CH2持续为4V
            DC_OUTPUT(SignalList.at(i),true);

            // 下半场CH2轮询变化，CH1持续为4V
            DC_OUTPUT_DIRECT(true);

            ui->textEdit_status->append("CH2当前正在执行升降直流输出，CH1维持稳定直流");
            // 保存（csv）并分析准直仪数据
            // 获取偏置电压
            QString offsetVolt  = SignalList.at(i).Offset;
            // 获取电压title
            QString voltTitle = SignalList.at(i).voltTitle;
            // 获取往复标志
            QString goOrBack;
            if(SignalList.at(i).goOrBackSign){
                goOrBack = "up";
            }
            else{
                goOrBack = "down";
            }
            // 获取测试对象名称
            QString testObjectName = SignalList.at(i).testObjectName+ "-" +timeStr;
            // 获取重复次序
            cloneNo1 = SignalList.at(i).cloneNo;
            qDebug()<<"下半场cloneNo1"<<cloneNo1;
            // 根据上述信号特征创建属性文件夹和csv文件
            // 1. 属性文件夹
            bool createSucc = folderHelper->createFolder(basePath +"/"+ testObjectName , voltTitle);
            if(!createSucc){
                QMessageBox::critical(this, "错误", "无法创建文件夹: " + voltTitle);
                return;
            }
            bool createSucc1 = folderHelper->createFolder(basePath +"/"+ testObjectName +"/"+ voltTitle, "1");
            if(!createSucc1){
                QMessageBox::critical(this, "错误", "无法创建文件夹: 1");
                return;
            }
            csvName = offsetVolt +"-"+ goOrBack  +"-"+ cloneNo1;
            savePath = basePath +"/"+ testObjectName + "/" + voltTitle+ "/" + "1";
            // saveToCsv(timeDuration, savePath, csvName);
            // ⭐ 开启采集
            // // 使用QTimer实现非阻塞延时
            // double timeWait = ui->lineEdit_timeWait->text().toDouble();
            // double timeDelay = ui->lineEdit_timeDelay->text().toDouble();
            if(timeWait>=timeDuration){
                timeWait = timeDuration;
            }
            QEventLoop loop;
            QTimer::singleShot(static_cast<int>(timeWait * 1000), &loop, &QEventLoop::quit);
            loop.exec();
            startCsvLogging(savePath, csvName);
        }
        else if(signalType == "SINE"){
            SINE_OUTPUT(SignalList.at(i));
            ui->textEdit_status->append("当前正在执行正弦波输出");
        }
        else if(signalType == "RAMP"){
            RAMP_OUTPUT(SignalList.at(i));
            ui->textEdit_status->append("当前正在执行锯齿波输出");
        }

        // 使用QTimer实现非阻塞延时
        QEventLoop loop;
        QTimer::singleShot(static_cast<int>((timeDuration - timeWait) * 1000), &loop, &QEventLoop::quit);
        loop.exec();

        // 当前执行的信号所对应的信号列表的行高亮
        // ⭐ 结束采集
        stopCsvLogging();

        readCsvAndReloadX(savePath,csvName,i);

        // // // 寻找名为csvName的文件，读取其数据并计算X和Y数据的标准差
        // // QVector<double> csvDataY = csvHelper->readCsvToVector(savePath + "/" + csvName + ".csv", 2);
        // QVector<double> csvDataX = csvHelper->readCsvToVector(savePath + "/" + csvName +".csv", 1);

        // /** 开始计算标准差
        // // double stdX = dataAnalysisHelper->computeStd(csvDataX);
        // // double stdY = dataAnalysisHelper->computeStd(csvDataY);
        // // stdXList.append(stdX);
        // // stdYList.append(stdY);

        // // // 往表格填充标准差数据
        // // QTableWidgetItem * ItemX = new QTableWidgetItem(QString::number(stdX));
        // // ui->tableWidget_ch1->setItem(i,7,ItemX);
        // // QTableWidgetItem * ItemY = new QTableWidgetItem(QString::number(stdY));
        // // ui->tableWidget_ch1->setItem(i,8,ItemY);
        // */

        // // 开始计算均值
        // // double aveX = dataAnalysisHelper->computeAve(csvDataX);
        // double aveX = dataAnalysisHelper->computeAve(csvDataX);
        // // aveXSingleList.append(aveX); // 均值X列表
        // aveXSingleList.append(aveX); // 均值Y列表
        // qDebug()
        //     << "X append:"
        //     << "volt=" << voltTitle
        //     << "upDown=" << goOrBack
        //     << "clone=" << cloneNo1
        //     << "value=" << aveX;

        // // 往表格填充均值数据
        // // QTableWidgetItem * ItemX = new QTableWidgetItem(QString::number(aveX));
        // // ui->tableWidget_ch1->setItem(i,7,ItemX);
        // QTableWidgetItem * ItemX = new QTableWidgetItem(QString::number(aveX));
        // ui->tableWidget_ch1->setItem(i,7,ItemX);
    }


    dataProcessing(repeatNumNow,ListNum);
    // /// ※※※※※※※※※※※※※※全部数据的标准差都计算完成后开始找全局最大最小值※※※※※※※※※※※
    // // int maxIndex = std::distance(std::begin(stdXList), std::max_element(stdXList.begin(), stdXList.end()));
    // // int minIndex = std::distance(std::begin(stdXList), std::min_element(stdXList.begin(), stdXList.end()));
    // QVector<double> tempSingleListX;
    // QVector<double> tempSingleListY;
    // qDebug()<<"aveXSingleList.size()"<<aveXSingleList.size(); // 132

    // for(int i = 0 ; i < ListNum; i++){ // ListNum : 组别个数22
    //     qDebug()<<"repeatNumNow"<<repeatNumNow;
    //     for(int j = 0 ; j < repeatNumNow ; j++){
    //         tempSingleListX.append(aveXSingleList.at(i+j * ListNum));   // 某个电压点位所有重复测试值，如第一次0↑、第二次0↑...第六次0↑，第一次0.6↑、第二次0.6↑...第六次0.6↑
    //         tempSingleListY.append(aveYSingleList.at(i+j * ListNum));   // 某个电压点位所有重复测试值
    //     }
    //     aveXList.append(tempSingleListX); // {(0↑)[3.0,3.1,2.9...],(0.6↑)[3.3,2.8,3.0],....(0↓)[3.2,2.8,2.9],(0.6↓)[3.2,2.8,2.9]...} size：22
    //     tempSingleListX.clear();
    //     aveYList.append(tempSingleListY);
    //     tempSingleListY.clear();
    // }

    // // 在aveYList中，0是0上升（元素格式为重复次数）、1是0下降、2是0.6上升、3是0.6下降......

    // for(int k = 0 ; k < aveXList.size()/2 ; k ++){
    //     // aveXListAveUpTest.append(dataAnalysisHelper->computeAve(aveXList.at(2*k))); // {(0↑均值)3.0,(0.6↑均值)3.2,(1.2↑均值)3.3...} size：11
    //     // aveYListAveUp.append(dataAnalysisHelper->computeAve(aveYList.at(2*k)));
    //     aveXListAveUp.append(dataAnalysisHelper->computeAve(aveXList.at(k))); // {(0↑均值)3.0,(0.6↑均值)3.2,(1.2↑均值)3.3...} size：11
    //     // aveXListAveUpTestAll.append(dataAnalysisHelper->computeAve(aveXList.at(k + aveXList.size()/2)));
    //     aveYListAveUp.append(dataAnalysisHelper->computeAve(aveYList.at(k)));
    // }

    // // // 输出aveXListAveUpTest、aveXListAveUp、aveXListAveUpTestAll
    // // for (int i = 0; i < aveXListAveUpTest.size(); ++i) {
    // //     qDebug() << "2k***"<<aveXListAveUpTest[i];
    // // }
    // // for (int i = 0; i < aveXListAveUp.size(); ++i) {
    // //     qDebug() << "k前***"<<aveXListAveUp[i];
    // // }
    // // for (int i = 0; i < aveXListAveUpTestAll.size(); ++i) {
    // //     qDebug() << "k后***"<<aveXListAveUpTestAll[i];
    // // }

    // for(int k = 0 ; k < aveXList.size()/2 ; k ++){
    //     // aveXListAveDown.append(dataAnalysisHelper->computeAve(aveXList.at(2*k + 1))); // {(0↓均值)3.0,(0.6↓均值)3.1,(1.2↓均值)3.2...} size：11
    //     // aveYListAveDown.append(dataAnalysisHelper->computeAve(aveYList.at(2*k + 1)));
    //     aveXListAveDown.append(dataAnalysisHelper->computeAve(aveXList.at(k + aveXList.size()/2))); // {(0↓均值)3.0,(0.6↓均值)3.1,(1.2↓均值)3.2...} size：11
    //     aveYListAveDown.append(dataAnalysisHelper->computeAve(aveYList.at(k + aveYList.size()/2)));
    // }

    // for(int t = 0 ; t < aveXListAveUp.size(); t++){
    //     // P↑
    //     QTableWidgetItem * ItemXup = new QTableWidgetItem(QString::number((aveXListAveUp.at(t))));
    //     ui->tableWidget_results->setItem(t,0,ItemXup);
    //     QTableWidgetItem * ItemYup = new QTableWidgetItem(QString::number((aveYListAveUp.at(t))));
    //     ui->tableWidget_results->setItem(t,0 + 6,ItemYup);

    //     // P↓
    //     // aveXList.at(2*k)
    //     // → 上升段本来就是 0 → 6 的顺序
    //     //  aveXList.at(2*k + 1)
    //     // → 下降段本来就是 6 → 0 的顺序
    //     // ⭐️⭐️⭐️⭐️⭐️所以这里下降段需要反过来填表才对⭐️⭐️⭐️⭐️⭐️
    //     QTableWidgetItem * ItemXdown = new QTableWidgetItem(QString::number((aveXListAveDown.at(aveXListAveDown.size() - 1 - t))));
    //     ui->tableWidget_results->setItem(t,1,ItemXdown);
    //     QTableWidgetItem * ItemYdown = new QTableWidgetItem(QString::number((aveYListAveDown.at(aveYListAveDown.size() - 1 - t))));
    //     ui->tableWidget_results->setItem(t,1 + 6,ItemYdown);

    //     // P
    //     QTableWidgetItem * ItemX_P = new QTableWidgetItem(QString::number((aveXListAveUp.at(t) + (aveXListAveDown.at(aveXListAveDown.size() - 1 - t)))/2));
    //     ui->tableWidget_results->setItem(t,2,ItemX_P);
    //     QTableWidgetItem * ItemY_P = new QTableWidgetItem(QString::number((aveYListAveUp.at(t) + (aveYListAveDown.at(aveYListAveDown.size() - 1 - t)))/2));
    //     ui->tableWidget_results->setItem(t,2 + 6,ItemY_P);

    //     aveX_P.append((aveXListAveUp.at(t) + (aveXListAveDown.at(aveXListAveDown.size() - 1 - t)))/2);
    //     aveY_P.append((aveYListAveUp.at(t) + (aveYListAveDown.at(aveYListAveDown.size() - 1 - t)))/2);
    // }

    // /// 开始计算R↑、R↓和R
    // QVector<QVector<double>> tempUpPList_X;
    // QVector<QVector<double>> tempdownPList_X;
    // QVector<QVector<double>> tempUpPList_Y;
    // QVector<QVector<double>> tempdownPList_Y;

    // for(int i = 0 ; i <aveXList.size()/2 ; i++){
    //     tempUpPList_X.append(aveXList.at(i)); // X上升均值列表
    //     tempdownPList_X.append(aveXList.at(aveXList.size()-1-i));// X下降均值列表
    //     tempUpPList_Y.append(aveYList.at(i));// Y上升均值列表
    //     tempdownPList_Y.append(aveYList.at(aveYList.size()-1-i));// Y下降均值列表
    // }

    // qDebug()<<"tempUpPList_X.size"<<tempUpPList_X.size();
    // // QVector<QVector<double>> tempUpRList_X;
    // // QVector<QVector<double>> tempdownRList_X;
    // // QVector<QVector<double>> tempUpRList_Y;
    // // QVector<QVector<double>> tempdownRList_Y;

    // // UP
    // QVector<double> tempR_UP_X_List;
    // QVector<double> tempR_UP_Y_List;
    // qDebug()<<"aveX_P.size"<<aveX_P.size();
    // qDebug()<<"tempUpPList_X.at(0).size()"<<tempUpPList_X.at(0).size();
    // for(int i =0 ; i < tempUpPList_X.size();i++){
    //     double tempR_UP_X_Add = 0.0;
    //     double tempR_UP_Y_Add = 0.0;
    //     for(int j = 1 ; j <tempUpPList_X.at(0).size(); j++){
    //         tempR_UP_X_Add += std::pow(tempUpPList_X.at(i).at(j) - aveX_P.at(i),2);
    //         tempR_UP_Y_Add += std::pow(tempUpPList_Y.at(i).at(j) - aveY_P.at(i),2);
    //         qDebug()<<"tempR_UP_X_Add"<<tempR_UP_X_Add;
    //         qDebug()<<"tempR_UP_Y_Add"<<tempR_UP_Y_Add;
    //     }
    //     double tempR_X = std::sqrt(tempR_UP_X_Add / (tempUpPList_X.at(0).size()-2));
    //     double tempR_Y = std::sqrt(tempR_UP_Y_Add / (tempUpPList_Y.at(0).size()-2));
    //     // 计算R⬆️
    //     tempR_UP_X_List.append(tempR_X);
    //     tempR_UP_Y_List.append(tempR_Y);
    // }
    // // DOWN
    // QVector<double> tempR_DOWN_X_List;
    // QVector<double> tempR_DOWN_Y_List;
    // for(int i =0 ; i < tempdownPList_X.size();i++){
    //     double tempR_DOWN_X_Add = 0.0;
    //     double tempR_DOWN_Y_Add = 0.0;
    //     for(int j = 1 ; j <tempdownPList_X.at(0).size(); j++){
    //         tempR_DOWN_X_Add += std::pow(tempdownPList_X.at(i).at(j) - aveX_P.at(i),2);
    //         tempR_DOWN_Y_Add += std::pow(tempdownPList_Y.at(i).at(j) - aveY_P.at(i),2);
    //     }
    //     double tempR_X = std::sqrt(tempR_DOWN_X_Add / (tempdownPList_X.at(0).size()-2));
    //     double tempR_Y = std::sqrt(tempR_DOWN_Y_Add / (tempdownPList_X.at(0).size()-2));
    //     // 计算R⬇️
    //     tempR_DOWN_X_List.append(tempR_X);
    //     tempR_DOWN_Y_List.append(tempR_Y);
    // }
    // // 将计算的R值写进列表
    // qDebug()<<"tempR_DOWN_X_List.size()"<<tempR_DOWN_X_List.size();
    // for(int i = 0 ; i <tempR_DOWN_X_List.size(); i ++){
    //     QTableWidgetItem * ItemXupR = new QTableWidgetItem(QString::number((tempR_UP_X_List.at(i))));
    //     ui->tableWidget_results->setItem(i,3,ItemXupR);
    //     QTableWidgetItem * ItemXdownR = new QTableWidgetItem(QString::number((tempR_DOWN_X_List.at(i))));
    //     ui->tableWidget_results->setItem(i,4,ItemXdownR);

    //     QTableWidgetItem * ItemYupR = new QTableWidgetItem(QString::number((tempR_UP_Y_List.at(i))));
    //     ui->tableWidget_results->setItem(i,9,ItemYupR);
    //     QTableWidgetItem * ItemYdownR = new QTableWidgetItem(QString::number((tempR_DOWN_Y_List.at(i))));
    //     ui->tableWidget_results->setItem(i,10,ItemYdownR);
    // }

    // // 分别取出X和Y的R的MAX,填入R结果
    // auto max_X_up = std::max_element(tempR_UP_X_List.begin(),tempR_UP_X_List.end());
    // auto max_X_down = std::max_element(tempR_DOWN_X_List.begin(),tempR_DOWN_X_List.end());
    // double max_X_up_value   = *max_X_up;
    // double max_X_down_value = *max_X_down;
    // double maxValue_X = std::max(max_X_up_value, max_X_down_value);;


    // auto max_Y_up = std::max_element(tempR_UP_Y_List.begin(),tempR_UP_Y_List.end());
    // auto max_Y_down = std::max_element(tempR_DOWN_Y_List.begin(),tempR_DOWN_Y_List.end());
    // double max_Y_up_value   = *max_Y_up;
    // double max_Y_down_value = *max_Y_down;
    // double maxValue_Y = std::max(max_Y_up_value, max_Y_down_value);

    // QTableWidgetItem * ItemMaxX = new QTableWidgetItem(QString::number(maxValue_X));
    // ItemMaxX->setForeground(QColor(255, 0, 0));
    // ui->tableWidget_results->setItem(0,5,ItemMaxX);
    // QTableWidgetItem * ItemMaxY = new QTableWidgetItem(QString::number(maxValue_Y));
    // ItemMaxY->setForeground(QColor(255, 0, 0));
    // ui->tableWidget_results->setItem(0,11,ItemMaxY);

    // /// *************************绘制线性度曲线***********************************
    // // 获取x轴数组（电压阶级）
    // // 获取阶级数
    // int levelNumAxisX = SignalList.last().levelNum.toInt(); // 11
    // // 获取起始电压
    // double voltStart = SignalList.last().voltStart.toDouble();
    // // 获取步进
    // double stepAdd = SignalList.last().step.toDouble();
    // QVector<double> AxisVoltList; // 存储电压阶级，放在x轴
    // for(int i = 0 ; i < levelNumAxisX; i++){
    //     AxisVoltList.append(voltStart + i * stepAdd);
    // }
    // // 获取y轴数组（P↑） aveXListAveUp

    // // --------------------------
    // // 步骤2：绘制散点图（原始数据）
    // // --------------------------
    // QScatterSeries *scatterSeries = new QScatterSeries();
    // QScatterSeries *scatterSeries2 = new QScatterSeries();
    // scatterSeries->setName("原始数据");
    // scatterSeries->setMarkerSize(8);  // 散点大小
    // scatterSeries->setColor(Qt::blue);  // 散点颜色
    // scatterSeries2->setName("原始数据");
    // scatterSeries2->setMarkerSize(8);  // 散点大小
    // scatterSeries2->setColor(Qt::blue);  // 散点颜色

    // // 添加数据到散点系列
    // QVector<double> Xlist;
    // QVector<double> Ylist;
    // Xlist = AxisVoltList;
    // Ylist = aveXListAveUp;
    // for (int i = 0; i < Xlist.size(); ++i) {
    //     scatterSeries->append(Xlist[i], Ylist[i]);
    // }
    // // 1. 清除旧数据曲线，但保持 chartView 不变
    // chartLinearity_X->removeAllSeries();
    // chartLinearity_X->addAxis(axisVolt_X, Qt::AlignBottom);
    // chartLinearity_X->addAxis(axisX_L, Qt::AlignLeft);
    // chartLinearity_X->addSeries(scatterSeries);
    // scatterSeries->attachAxis(axisX_L);
    // // --------------------------
    // // 步骤3：计算最小二乘拟合直线（k和b）
    // // --------------------------
    // double k = dataAnalysisHelper->computeOLS(Xlist,Ylist).at(0);
    // double b = dataAnalysisHelper->computeOLS(Xlist,Ylist).at(1);
    // double deltaL = dataAnalysisHelper->computeOLS(Xlist,Ylist).at(3);
    // ui->lineEdit_linearity_X->setText(QString::number(deltaL));

    // // 步骤4：绘制拟合直线
    // // --------------------------
    // QLineSeries *lineSeries = new QLineSeries();
    // lineSeries->setName("最小二乘拟合直线");
    // lineSeries->setPen(QPen(QColorConstants::Svg::skyblue, 2, Qt::SolidLine));  // 线宽2px

    // // 生成直线数据（取X轴范围的起点和终点）
    // double xMin = *std::min_element(Xlist.begin(), Xlist.end());  // X最小值
    // double xMax = *std::max_element(Xlist.begin(), Xlist.end());  // X最大值
    // // double xMargin = (xMax - xMin) * 0.1; // 5%边距
    // // axisVolt_X->setRange(xMin - xMargin, xMax + xMargin);
    // axisVolt_X->setRange(xMin, xMax);

    // double yMin = std::min(*std::min_element(Ylist.begin(), Ylist.end()), k * xMin + b);
    // double yMax = std::max(*std::max_element(Ylist.begin(), Ylist.end()), k * xMax + b);
    // // double yMargin = (yMax - yMin) * 0.1; // 5%边距
    // // axisX_L->setRange(yMin - yMargin, yMax + yMargin);
    // axisX_L->setRange(yMin, yMax);

    // lineSeries->append(xMin, k * xMin + b);  // 直线起点
    // lineSeries->append(xMax, k * xMax + b);  // 直线终点

    // chartLinearity_X->addSeries(lineSeries);
    // lineSeries->attachAxis(axisVolt_X);
    // lineSeries->attachAxis(axisX_L);

    // chartViewLinearity_X->update();

    // // 添加数据到散点系列
    // QVector<double> Xlist2;
    // QVector<double> Ylist2;
    // Xlist2 = AxisVoltList;
    // Ylist2 = aveYListAveUp;
    // for (int i = 0; i < Xlist2.size(); ++i) {
    //     scatterSeries2->append(Xlist2[i], Ylist2[i]);
    // }
    // // 1. 清除旧数据曲线，但保持 chartView 不变
    // chartLinearity_Y->removeAllSeries();
    // chartLinearity_Y->addAxis(axisVolt_Y, Qt::AlignBottom);
    // chartLinearity_Y->addAxis(axisY_L, Qt::AlignLeft);
    // chartLinearity_Y->addSeries(scatterSeries2);
    // scatterSeries2->attachAxis(axisY_L);

    // // --------------------------
    // // 步骤3：计算最小二乘拟合直线（k和b）
    // // --------------------------
    // double k2 = dataAnalysisHelper->computeOLS(Xlist2,Ylist2).at(0);
    // double b2 = dataAnalysisHelper->computeOLS(Xlist2,Ylist2).at(1);
    // double deltaL2 = dataAnalysisHelper->computeOLS(Xlist2,Ylist2).at(3);
    // ui->lineEdit_linearity_Y->setText(QString::number(deltaL2));

    // // 步骤4：绘制拟合直线
    // // --------------------------
    // QLineSeries *lineSeries2 = new QLineSeries();
    // lineSeries2->setName("最小二乘拟合直线");
    // lineSeries2->setPen(QPen(QColorConstants::Svg::skyblue, 2, Qt::SolidLine));  // 线宽2px

    // // 生成直线数据（取X轴范围的起点和终点）
    // double xMin2 = *std::min_element(Xlist2.begin(), Xlist2.end());  // X最小值
    // double xMax2 = *std::max_element(Xlist2.begin(), Xlist2.end());  // X最大值
    // // double xMargin = (xMax - xMin) * 0.1; // 5%边距
    // // axisVolt_X->setRange(xMin - xMargin, xMax + xMargin);
    // axisVolt_Y->setRange(xMin2, xMax2);

    // double yMin2 = std::min(*std::min_element(Ylist2.begin(), Ylist2.end()), k2 * xMin2 + b2);
    // double yMax2 = std::max(*std::max_element(Ylist2.begin(), Ylist2.end()), k2 * xMax2 + b2);
    // // double yMargin = (yMax - yMin) * 0.1; // 5%边距
    // // axisX_L->setRange(yMin - yMargin, yMax + yMargin);
    // axisY_L->setRange(yMin2, yMax2);

    // lineSeries2->append(xMin2, k2 * xMin2 + b2);  // 直线起点
    // lineSeries2->append(xMax2, k2 * xMax2 + b2);  // 直线终点

    // chartLinearity_Y->addSeries(lineSeries2);
    // lineSeries2->attachAxis(axisVolt_Y);
    // lineSeries2->attachAxis(axisY_L);

    // chartViewLinearity_Y->update();


}

void MainWindow::initAFG1062()
{

    // === 打开资源管理器 ===
    status = viOpenDefaultRM(&defaultRM);
    if (status < VI_SUCCESS) {
        qDebug() << "无法打开 VISA 资源管理器";
        return;
    }

    // === 打开 AFG1062 设备 ===
    // 注意修改为设备 VISA 地址
    QString Id = ui->lineEdit_Id->text();
    QString visaAddr = "USB0::0x0699::0x0353::" + Id + "::INSTR";
    // QString visaAddr = "USB0::0x0699::0x0353::2042614::INSTR";
    // QString visaAddr = "USB0::0x0699::0x0353::2310325::INSTR";
    status = viOpen(defaultRM, (ViRsrc)visaAddr.toStdString().c_str(), VI_NULL, VI_NULL, &instr);
    if (status < VI_SUCCESS) {
        qDebug() << "无法连接到 AFG1062";
        ui->textEdit_status->append("无法连接到 AFG1062");
        viClose(defaultRM);
        return;
    }

    // === 查询设备 IDN ===
    viPrintf(instr, "*IDN?\n");
    viRead(instr, (ViBuf)buffer, sizeof(buffer), &retCount);
    qDebug() << "已连接设备:" << buffer;
}

void MainWindow::initSignalListTable()
{
    ui->tableWidget_ch1->setColumnCount(9);
    ui->tableWidget_ch1->setHorizontalHeaderLabels({
        "时间",
        "类型",
        "起始相位",
        "频率",
        "电压",
        "偏置",
        "对称性",
        "stdX",
        "stdY"
    });

    ui->tableWidget_ch1->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    // 允许用户拖动列分隔线调整列宽
    ui->tableWidget_ch1->horizontalHeader()->setSectionsMovable(true);
}

void MainWindow::updateChart(){

    if (bufferPointX.isEmpty() || bufferPointY.isEmpty()) return;

    // 从缓冲区拿数据
    while (!bufferPointX.isEmpty()) {
        QPointF p = bufferPointX.takeFirst();
        seriesX->append(counter, p.y());
    }
    while (!bufferPointY.isEmpty()) {
        QPointF p = bufferPointY.takeFirst();
        seriesY->append(counter, p.y());
    }

    // 滚动 time 轴范围
    if (seriesX->count() > 100) {
        axisTimeX->setRange(counter - 100, counter);
    }
    if (seriesY->count() > 100) {
        axisTimeY->setRange(counter - 100, counter);
    }

    // 限制数据点数量，否则长时间运行会越来越卡
    if (seriesX->count() > 200) {
        // ui->textEdit_status->append("X总数据超过200，开始删除旧数据");
        seriesX->removePoints(0, seriesX->count() - 200);
    }
    if (seriesY->count() > 200) {
        // ui->textEdit_status->append("Y总数据超过200，开始删除旧数据");
        seriesY->removePoints(0, seriesY->count() - 200);
    }

    // 自动调整y轴坐标范围

    updateAxisAutoRange(seriesX, axisX);
    updateAxisAutoRange(seriesY, axisY);

    // 强制只更新图，不 repaint 整个窗口
    chartX->plotArea();
    chartY->plotArea();
}

// ------------ 自动调整 Y 轴范围 --------------
void MainWindow::updateAxisAutoRange(QLineSeries* series, QValueAxis* axis){
    if(series->count() == 0) return;

    int total = series->count();
    int startIndex = qMax(0, total - 200); // 只看最后200点

    double minY = series->at(startIndex).y();
    double maxY = minY;

    for (int i = startIndex; i < total; i++) {
        double val = series->at(i).y();
        if (val < minY) minY = val;
        if (val > maxY) maxY = val;
    }

    // 给范围增加10% padding，避免贴边
    double padding = (maxY - minY) * 0.1;
    if (padding < 0.00001) padding = 0.01; // 避免全部值一样导致 padding=0

    axis->setRange(minY - padding, maxY + padding);
}

void MainWindow::saveToCsv(double time,QString csvPath, QString csvName)
{
    // 在time时间内将来自准直仪的数据全部保存到csv文件中,文件路径csvPath，文件名csvName
    // 检查csvPath是否存在，不存在就报错
    qDebug()<<"csvPath"<<csvPath;
    qDebug()<<"csvName"<<csvName;
    // 创建一个名为csvName的文件
    QDir dir(csvPath);
    if(!dir.exists()){
        QMessageBox::warning(this,"错误","找不到csv存储路径！");
        return;
    }
    // 确保文件名以.csv结尾
    if (!csvName.endsWith(".csv", Qt::CaseInsensitive)) {
        csvName += ".csv";
    }

    // 构建完整文件路径
    QString fullPath = csvPath + QDir::separator() + csvName;

    // 检查文件是否已存在
    if (QFile::exists(fullPath)) {
        qDebug() << "File already exists, skipping creation";
        return;
    }

    // 创建并打开文件
    QFile file(fullPath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qDebug() << "Error: Could not create file";
        return;
    }

    // 可以在这里写入CSV文件头或其他初始内容
    QTextStream out(&file);
    out << "Timestamp,Data1,Data2,Data3\n"; // 示例CSV头

    file.close();

    qDebug() << "CSV file created successfully at:" << fullPath;
    /**


    */

}

void MainWindow::clearContainer()
{
    aveXList.clear();
    aveYList.clear();
    aveXSingleList.clear();
    aveYSingleList.clear();
    aveXListAveUp.clear();
    aveYListAveUp.clear();
    aveXListAveDown.clear();
    aveYListAveDown.clear();
    aveX_P.clear();
    aveY_P.clear();
};

void MainWindow::startCsvLogging(QString csvPath, QString csvName)
{
    // 确保 .csv 后缀
    if (!csvName.endsWith(".csv"))
        csvName += ".csv";

    // 路径检查
    QDir dir(csvPath);
    if(!dir.exists()){
        QMessageBox::warning(this,"错误","找不到csv存储路径！");
        return;
    }

    // 完整路径
    QString fullPath = csvPath + "/" + csvName;

    // 如果已经存在则追加写
    // bool appendMode = QFile::exists(fullPath);
    bool appendMode = false;

    csvFile = new QFile(fullPath);
    if (!csvFile->open(appendMode ? QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text
                                  : QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this,"错误","无法打开CSV写入!");
        return;
    }

    csvStream = new QTextStream(csvFile);


}

void MainWindow::stopCsvLogging()
{
    if(csvTimer) csvTimer->stop();

    if(csvFile){
        csvFile->close();
        delete csvFile;
        csvFile = nullptr;
    }

    if(csvStream){
        delete csvStream;
        csvStream = nullptr;
    }
}

void MainWindow::readCsvAndReloadX(QString savePath, QString csvName, int i){
    // 寻找名为csvName的文件，读取其数据并计算X和Y数据的标准差
    QVector<double> csvDataX = csvHelper->readCsvToVector(savePath + "/" + csvName +".csv", 1);

    /** 开始计算标准差
        // double stdX = dataAnalysisHelper->computeStd(csvDataX);
        // double stdY = dataAnalysisHelper->computeStd(csvDataY);
        // stdXList.append(stdX);
        // stdYList.append(stdY);

        // // 往表格填充标准差数据
        // QTableWidgetItem * ItemX = new QTableWidgetItem(QString::number(stdX));
        // ui->tableWidget_ch1->setItem(i,7,ItemX);
        // QTableWidgetItem * ItemY = new QTableWidgetItem(QString::number(stdY));
        // ui->tableWidget_ch1->setItem(i,8,ItemY);
        */

    // 开始计算均值
    // double aveX = dataAnalysisHelper->computeAve(csvDataX);
    double aveX = dataAnalysisHelper->computeAve(csvDataX);
    // aveXSingleList.append(aveX); // 均值X列表
    aveXSingleList.append(aveX); // 均值Y列表
    qDebug()
        << "X append:"
        << "volt=" << voltTitle
        << "upDown=" << goOrBack
        // << "clone=" << cloneNo1
        << "value=" << aveX;

    // 往表格填充均值数据
    QTableWidgetItem * ItemX = new QTableWidgetItem(QString::number(aveX));
    ui->tableWidget_ch1->setItem(i,7,ItemX);
}

void MainWindow::readCsvAndReloadY(QString savePath, QString csvName, int i){

    // 寻找名为csvName的文件，读取其数据并计算X和Y数据的标准差
    QVector<double> csvDataY = csvHelper->readCsvToVector(savePath + "/" + csvName + ".csv", 2);

    /** 开始计算标准差
        // double stdX = dataAnalysisHelper->computeStd(csvDataX);
        // double stdY = dataAnalysisHelper->computeStd(csvDataY);
        // stdXList.append(stdX);
        // stdYList.append(stdY);

        // // 往表格填充标准差数据
        // QTableWidgetItem * ItemX = new QTableWidgetItem(QString::number(stdX));
        // ui->tableWidget_ch1->setItem(i,7,ItemX);
        // QTableWidgetItem * ItemY = new QTableWidgetItem(QString::number(stdY));
        // ui->tableWidget_ch1->setItem(i,8,ItemY);
        */

    // 开始计算均值
    double aveY = dataAnalysisHelper->computeAve(csvDataY);
    // double aveX = dataAnalysisHelper->computeAve(csvDataX);
    aveYSingleList.append(aveY); // 均值Y列表
    // aveXSingleList.append(aveX); // 均值X列表

    // 往表格填充均值数据
    QTableWidgetItem * ItemY = new QTableWidgetItem(QString::number(aveY));
    ui->tableWidget_ch1->setItem(i,8,ItemY);

}

void MainWindow::dataProcessing(int repeatNumNow, int ListNum){
    /// ※※※※※※※※※※※※※※全部数据的标准差都计算完成后开始找全局最大最小值※※※※※※※※※※※
    // int maxIndex = std::distance(std::begin(stdXList), std::max_element(stdXList.begin(), stdXList.end()));
    // int minIndex = std::distance(std::begin(stdXList), std::min_element(stdXList.begin(), stdXList.end()));
    QVector<double> tempSingleListX;
    QVector<double> tempSingleListY;
    qDebug()<<"aveXSingleList.size()"<<aveXSingleList.size(); // 132

    for(int i = 0 ; i < ListNum; i++){ // ListNum : 组别个数22
        qDebug()<<"repeatNumNow"<<repeatNumNow;
        for(int j = 0 ; j < repeatNumNow ; j++){
            tempSingleListX.append(aveXSingleList.at(i+j * ListNum));   // 某个电压点位所有重复测试值，如第一次0↑、第二次0↑...第六次0↑，第一次0.6↑、第二次0.6↑...第六次0.6↑
            tempSingleListY.append(aveYSingleList.at(i+j * ListNum));   // 某个电压点位所有重复测试值
        }
        aveXList.append(tempSingleListX); // {(0↑)[3.0,3.1,2.9...],(0.6↑)[3.3,2.8,3.0],....(0↓)[3.2,2.8,2.9],(0.6↓)[3.2,2.8,2.9]...} size：22
        tempSingleListX.clear();
        aveYList.append(tempSingleListY);
        tempSingleListY.clear();
    }

    // 在aveYList中，0是0上升（元素格式为重复次数）、1是0下降、2是0.6上升、3是0.6下降......

    for(int k = 0 ; k < aveXList.size()/2 ; k ++){
        // aveXListAveUpTest.append(dataAnalysisHelper->computeAve(aveXList.at(2*k))); // {(0↑均值)3.0,(0.6↑均值)3.2,(1.2↑均值)3.3...} size：11
        // aveYListAveUp.append(dataAnalysisHelper->computeAve(aveYList.at(2*k)));
        aveXListAveUp.append(dataAnalysisHelper->computeAve(aveXList.at(k))); // {(0↑均值)3.0,(0.6↑均值)3.2,(1.2↑均值)3.3...} size：11
        // aveXListAveUpTestAll.append(dataAnalysisHelper->computeAve(aveXList.at(k + aveXList.size()/2)));
        aveYListAveUp.append(dataAnalysisHelper->computeAve(aveYList.at(k)));
    }

    // // 输出aveXListAveUpTest、aveXListAveUp、aveXListAveUpTestAll
    // for (int i = 0; i < aveXListAveUpTest.size(); ++i) {
    //     qDebug() << "2k***"<<aveXListAveUpTest[i];
    // }
    // for (int i = 0; i < aveXListAveUp.size(); ++i) {
    //     qDebug() << "k前***"<<aveXListAveUp[i];
    // }
    // for (int i = 0; i < aveXListAveUpTestAll.size(); ++i) {
    //     qDebug() << "k后***"<<aveXListAveUpTestAll[i];
    // }

    for(int k = 0 ; k < aveXList.size()/2 ; k ++){
        // aveXListAveDown.append(dataAnalysisHelper->computeAve(aveXList.at(2*k + 1))); // {(0↓均值)3.0,(0.6↓均值)3.1,(1.2↓均值)3.2...} size：11
        // aveYListAveDown.append(dataAnalysisHelper->computeAve(aveYList.at(2*k + 1)));
        aveXListAveDown.append(dataAnalysisHelper->computeAve(aveXList.at(k + aveXList.size()/2))); // {(0↓均值)3.0,(0.6↓均值)3.1,(1.2↓均值)3.2...} size：11
        aveYListAveDown.append(dataAnalysisHelper->computeAve(aveYList.at(k + aveYList.size()/2)));
    }

    for(int t = 0 ; t < aveXListAveUp.size(); t++){
        // P↑
        QTableWidgetItem * ItemXup = new QTableWidgetItem(QString::number((aveXListAveUp.at(t))));
        ui->tableWidget_results->setItem(t,0,ItemXup);
        QTableWidgetItem * ItemYup = new QTableWidgetItem(QString::number((aveYListAveUp.at(t))));
        ui->tableWidget_results->setItem(t,0 + 6,ItemYup);

        // P↓
        // aveXList.at(2*k)
        // → 上升段本来就是 0 → 6 的顺序
        //  aveXList.at(2*k + 1)
        // → 下降段本来就是 6 → 0 的顺序
        // ⭐️⭐️⭐️⭐️⭐️所以这里下降段需要反过来填表才对⭐️⭐️⭐️⭐️⭐️
        QTableWidgetItem * ItemXdown = new QTableWidgetItem(QString::number((aveXListAveDown.at(aveXListAveDown.size() - 1 - t))));
        ui->tableWidget_results->setItem(t,1,ItemXdown);
        QTableWidgetItem * ItemYdown = new QTableWidgetItem(QString::number((aveYListAveDown.at(aveYListAveDown.size() - 1 - t))));
        ui->tableWidget_results->setItem(t,1 + 6,ItemYdown);

        // P
        QTableWidgetItem * ItemX_P = new QTableWidgetItem(QString::number((aveXListAveUp.at(t) + (aveXListAveDown.at(aveXListAveDown.size() - 1 - t)))/2));
        ui->tableWidget_results->setItem(t,2,ItemX_P);
        QTableWidgetItem * ItemY_P = new QTableWidgetItem(QString::number((aveYListAveUp.at(t) + (aveYListAveDown.at(aveYListAveDown.size() - 1 - t)))/2));
        ui->tableWidget_results->setItem(t,2 + 6,ItemY_P);

        aveX_P.append((aveXListAveUp.at(t) + (aveXListAveDown.at(aveXListAveDown.size() - 1 - t)))/2);
        aveY_P.append((aveYListAveUp.at(t) + (aveYListAveDown.at(aveYListAveDown.size() - 1 - t)))/2);
    }

    /// 开始计算R↑、R↓和R
    QVector<QVector<double>> tempUpPList_X;
    QVector<QVector<double>> tempdownPList_X;
    QVector<QVector<double>> tempUpPList_Y;
    QVector<QVector<double>> tempdownPList_Y;

    for(int i = 0 ; i <aveXList.size()/2 ; i++){
        tempUpPList_X.append(aveXList.at(i)); // X上升均值列表
        tempdownPList_X.append(aveXList.at(aveXList.size()-1-i));// X下降均值列表
        tempUpPList_Y.append(aveYList.at(i));// Y上升均值列表
        tempdownPList_Y.append(aveYList.at(aveYList.size()-1-i));// Y下降均值列表
    }

    qDebug()<<"tempUpPList_X.size"<<tempUpPList_X.size();
    // QVector<QVector<double>> tempUpRList_X;
    // QVector<QVector<double>> tempdownRList_X;
    // QVector<QVector<double>> tempUpRList_Y;
    // QVector<QVector<double>> tempdownRList_Y;

    // UP
    QVector<double> tempR_UP_X_List;
    QVector<double> tempR_UP_Y_List;
    qDebug()<<"aveX_P.size"<<aveX_P.size();
    qDebug()<<"tempUpPList_X.at(0).size()"<<tempUpPList_X.at(0).size();
    for(int i =0 ; i < tempUpPList_X.size();i++){
        double tempR_UP_X_Add = 0.0;
        double tempR_UP_Y_Add = 0.0;
        for(int j = 1 ; j <tempUpPList_X.at(0).size(); j++){
            tempR_UP_X_Add += std::pow(tempUpPList_X.at(i).at(j) - aveX_P.at(i),2);
            tempR_UP_Y_Add += std::pow(tempUpPList_Y.at(i).at(j) - aveY_P.at(i),2);
            qDebug()<<"tempR_UP_X_Add"<<tempR_UP_X_Add;
            qDebug()<<"tempR_UP_Y_Add"<<tempR_UP_Y_Add;
        }
        double tempR_X = std::sqrt(tempR_UP_X_Add / (tempUpPList_X.at(0).size()-2));
        double tempR_Y = std::sqrt(tempR_UP_Y_Add / (tempUpPList_Y.at(0).size()-2));
        // 计算R⬆️
        tempR_UP_X_List.append(tempR_X);
        tempR_UP_Y_List.append(tempR_Y);
    }
    // DOWN
    QVector<double> tempR_DOWN_X_List;
    QVector<double> tempR_DOWN_Y_List;
    for(int i =0 ; i < tempdownPList_X.size();i++){
        double tempR_DOWN_X_Add = 0.0;
        double tempR_DOWN_Y_Add = 0.0;
        for(int j = 1 ; j <tempdownPList_X.at(0).size(); j++){
            tempR_DOWN_X_Add += std::pow(tempdownPList_X.at(i).at(j) - aveX_P.at(i),2);
            tempR_DOWN_Y_Add += std::pow(tempdownPList_Y.at(i).at(j) - aveY_P.at(i),2);
        }
        double tempR_X = std::sqrt(tempR_DOWN_X_Add / (tempdownPList_X.at(0).size()-2));
        double tempR_Y = std::sqrt(tempR_DOWN_Y_Add / (tempdownPList_X.at(0).size()-2));
        // 计算R⬇️
        tempR_DOWN_X_List.append(tempR_X);
        tempR_DOWN_Y_List.append(tempR_Y);
    }
    // 将计算的R值写进列表
    qDebug()<<"tempR_DOWN_X_List.size()"<<tempR_DOWN_X_List.size();
    for(int i = 0 ; i <tempR_DOWN_X_List.size(); i ++){
        QTableWidgetItem * ItemXupR = new QTableWidgetItem(QString::number((tempR_UP_X_List.at(i))));
        ui->tableWidget_results->setItem(i,3,ItemXupR);
        QTableWidgetItem * ItemXdownR = new QTableWidgetItem(QString::number((tempR_DOWN_X_List.at(i))));
        ui->tableWidget_results->setItem(i,4,ItemXdownR);

        QTableWidgetItem * ItemYupR = new QTableWidgetItem(QString::number((tempR_UP_Y_List.at(i))));
        ui->tableWidget_results->setItem(i,9,ItemYupR);
        QTableWidgetItem * ItemYdownR = new QTableWidgetItem(QString::number((tempR_DOWN_Y_List.at(i))));
        ui->tableWidget_results->setItem(i,10,ItemYdownR);
    }

    // 分别取出X和Y的R的MAX,填入R结果
    auto max_X_up = std::max_element(tempR_UP_X_List.begin(),tempR_UP_X_List.end());
    auto max_X_down = std::max_element(tempR_DOWN_X_List.begin(),tempR_DOWN_X_List.end());
    double max_X_up_value   = *max_X_up;
    double max_X_down_value = *max_X_down;
    double maxValue_X = std::max(max_X_up_value, max_X_down_value);;


    auto max_Y_up = std::max_element(tempR_UP_Y_List.begin(),tempR_UP_Y_List.end());
    auto max_Y_down = std::max_element(tempR_DOWN_Y_List.begin(),tempR_DOWN_Y_List.end());
    double max_Y_up_value   = *max_Y_up;
    double max_Y_down_value = *max_Y_down;
    double maxValue_Y = std::max(max_Y_up_value, max_Y_down_value);

    QTableWidgetItem * ItemMaxX = new QTableWidgetItem(QString::number(maxValue_X));
    ItemMaxX->setForeground(QColor(255, 0, 0));
    ui->tableWidget_results->setItem(0,5,ItemMaxX);
    QTableWidgetItem * ItemMaxY = new QTableWidgetItem(QString::number(maxValue_Y));
    ItemMaxY->setForeground(QColor(255, 0, 0));
    ui->tableWidget_results->setItem(0,11,ItemMaxY);

    /// *************************绘制线性度曲线***********************************
    // 获取x轴数组（电压阶级）
    // 获取阶级数
    int levelNumAxisX = SignalList.last().levelNum.toInt(); // 11
    // 获取起始电压
    double voltStart = SignalList.last().voltStart.toDouble();
    // 获取步进
    double stepAdd = SignalList.last().step.toDouble();
    QVector<double> AxisVoltList; // 存储电压阶级，放在x轴
    for(int i = 0 ; i < levelNumAxisX; i++){
        AxisVoltList.append(voltStart + i * stepAdd);
    }
    // 获取y轴数组（P↑） aveXListAveUp

    // --------------------------
    // 步骤2：绘制散点图（原始数据）
    // --------------------------
    QScatterSeries *scatterSeries = new QScatterSeries();
    QScatterSeries *scatterSeries2 = new QScatterSeries();
    scatterSeries->setName("原始数据");
    scatterSeries->setMarkerSize(8);  // 散点大小
    scatterSeries->setColor(Qt::blue);  // 散点颜色
    scatterSeries2->setName("原始数据");
    scatterSeries2->setMarkerSize(8);  // 散点大小
    scatterSeries2->setColor(Qt::blue);  // 散点颜色

    // 添加数据到散点系列
    QVector<double> Xlist;
    QVector<double> Ylist;
    Xlist = AxisVoltList;
    Ylist = aveXListAveUp;
    for (int i = 0; i < Xlist.size(); ++i) {
        scatterSeries->append(Xlist[i], Ylist[i]);
    }
    // 1. 清除旧数据曲线，但保持 chartView 不变
    chartLinearity_X->removeAllSeries();
    chartLinearity_X->addAxis(axisVolt_X, Qt::AlignBottom);
    chartLinearity_X->addAxis(axisX_L, Qt::AlignLeft);
    chartLinearity_X->addSeries(scatterSeries);
    scatterSeries->attachAxis(axisX_L);
    // --------------------------
    // 步骤3：计算最小二乘拟合直线（k和b）
    // --------------------------
    double k = dataAnalysisHelper->computeOLS(Xlist,Ylist).at(0);
    double b = dataAnalysisHelper->computeOLS(Xlist,Ylist).at(1);
    double deltaL = dataAnalysisHelper->computeOLS(Xlist,Ylist).at(3);
    ui->lineEdit_linearity_X->setText(QString::number(deltaL));

    // 步骤4：绘制拟合直线
    // --------------------------
    QLineSeries *lineSeries = new QLineSeries();
    lineSeries->setName("最小二乘拟合直线");
    lineSeries->setPen(QPen(QColorConstants::Svg::skyblue, 2, Qt::SolidLine));  // 线宽2px

    // 生成直线数据（取X轴范围的起点和终点）
    double xMin = *std::min_element(Xlist.begin(), Xlist.end());  // X最小值
    double xMax = *std::max_element(Xlist.begin(), Xlist.end());  // X最大值
    // double xMargin = (xMax - xMin) * 0.1; // 5%边距
    // axisVolt_X->setRange(xMin - xMargin, xMax + xMargin);
    axisVolt_X->setRange(xMin, xMax);

    double yMin = std::min(*std::min_element(Ylist.begin(), Ylist.end()), k * xMin + b);
    double yMax = std::max(*std::max_element(Ylist.begin(), Ylist.end()), k * xMax + b);
    // double yMargin = (yMax - yMin) * 0.1; // 5%边距
    // axisX_L->setRange(yMin - yMargin, yMax + yMargin);
    axisX_L->setRange(yMin, yMax);

    lineSeries->append(xMin, k * xMin + b);  // 直线起点
    lineSeries->append(xMax, k * xMax + b);  // 直线终点

    chartLinearity_X->addSeries(lineSeries);
    lineSeries->attachAxis(axisVolt_X);
    lineSeries->attachAxis(axisX_L);

    chartViewLinearity_X->update();

    // 添加数据到散点系列
    QVector<double> Xlist2;
    QVector<double> Ylist2;
    Xlist2 = AxisVoltList;
    Ylist2 = aveYListAveUp;
    for (int i = 0; i < Xlist2.size(); ++i) {
        scatterSeries2->append(Xlist2[i], Ylist2[i]);
    }
    // 1. 清除旧数据曲线，但保持 chartView 不变
    chartLinearity_Y->removeAllSeries();
    chartLinearity_Y->addAxis(axisVolt_Y, Qt::AlignBottom);
    chartLinearity_Y->addAxis(axisY_L, Qt::AlignLeft);
    chartLinearity_Y->addSeries(scatterSeries2);
    scatterSeries2->attachAxis(axisY_L);

    // --------------------------
    // 步骤3：计算最小二乘拟合直线（k和b）
    // --------------------------
    double k2 = dataAnalysisHelper->computeOLS(Xlist2,Ylist2).at(0);
    double b2 = dataAnalysisHelper->computeOLS(Xlist2,Ylist2).at(1);
    double deltaL2 = dataAnalysisHelper->computeOLS(Xlist2,Ylist2).at(3);
    ui->lineEdit_linearity_Y->setText(QString::number(deltaL2));

    // 步骤4：绘制拟合直线
    // --------------------------
    QLineSeries *lineSeries2 = new QLineSeries();
    lineSeries2->setName("最小二乘拟合直线");
    lineSeries2->setPen(QPen(QColorConstants::Svg::skyblue, 2, Qt::SolidLine));  // 线宽2px

    // 生成直线数据（取X轴范围的起点和终点）
    double xMin2 = *std::min_element(Xlist2.begin(), Xlist2.end());  // X最小值
    double xMax2 = *std::max_element(Xlist2.begin(), Xlist2.end());  // X最大值
    // double xMargin = (xMax - xMin) * 0.1; // 5%边距
    // axisVolt_X->setRange(xMin - xMargin, xMax + xMargin);
    axisVolt_Y->setRange(xMin2, xMax2);

    double yMin2 = std::min(*std::min_element(Ylist2.begin(), Ylist2.end()), k2 * xMin2 + b2);
    double yMax2 = std::max(*std::max_element(Ylist2.begin(), Ylist2.end()), k2 * xMax2 + b2);
    // double yMargin = (yMax - yMin) * 0.1; // 5%边距
    // axisX_L->setRange(yMin - yMargin, yMax + yMargin);
    axisY_L->setRange(yMin2, yMax2);

    lineSeries2->append(xMin2, k2 * xMin2 + b2);  // 直线起点
    lineSeries2->append(xMax2, k2 * xMax2 + b2);  // 直线终点

    chartLinearity_Y->addSeries(lineSeries2);
    lineSeries2->attachAxis(axisVolt_Y);
    lineSeries2->attachAxis(axisY_L);

    chartViewLinearity_Y->update();
}

void MainWindow::correctData()
{
    // 为了防止软件进行下一轮测试时收到上轮测试数据的影响，在每次点击运行时要先清空全部变量缓存

    clearContainer();

    // 检测列表tableWidget_ch1是否为空
    if (ui->tableWidget_ch1->rowCount() == 0) {
        return;
    }

    // 获取重复次数
    int repeatNumNow = SignalList.last().cloneNo.toInt();
    qDebug()<<"重复次数"<<repeatNumNow;// 6
    // 计算组别个数
    int ListNum = SignalList.size() / repeatNumNow;
    qDebug()<<"信号总长度"<<SignalList.size();// 11*6*2=132
    qDebug()<<"组别个数"<<ListNum;// 22，指的是上升11个和下降11个

    if (SignalList.size() % repeatNumNow != 0) {
        // 不能整除，报错
        ui->textEdit_status->append( "Error: SignalList.size() is not divisible by repeatNumNow!");
        return;
    }

    // 获取修正文件夹
    QString corrDirName = ui->lineEdit_corrObject->text();

    for(int i = 0 ; i < SignalList.length(); i++){

        // 获取偏置电压
        QString offsetVolt  = SignalList.at(i).Offset;
        QString goOrBack;
        if(SignalList.at(i).goOrBackSign){
            goOrBack = "up";
        }
        else{
            goOrBack = "down";
        }
        // 获取重复次序
        QString cloneNo = SignalList.at(i).cloneNo;
        // 获取电压title
        QString voltTitle = SignalList.at(i).voltTitle;

        // 重新加载csv文件X
        QString csvNameX = offsetVolt +"-"+ goOrBack  +"-"+ cloneNo;
        QString  savePathX = basePath +"/"+ corrDirName + "/" + voltTitle+ "/" + "1";
        readCsvAndReloadX(savePathX, csvNameX, i);

        // 重新加载csv文件Y
        QString csvNameY = offsetVolt +"-"+ goOrBack  +"-"+ cloneNo;
        QString  savePathY = basePath +"/"+ corrDirName + "/" + voltTitle+ "/" + "2";
        readCsvAndReloadY(savePathY, csvNameY, i);
    }


    // 重新进行数据处理
    dataProcessing(repeatNumNow, ListNum);

}
