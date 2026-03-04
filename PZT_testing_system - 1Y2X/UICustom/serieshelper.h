#ifndef SERIESHELPER_H
#define SERIESHELPER_H

#include <QObject>
#include <QLineSeries>
#include <QValueAxis>

class seriesHelper : public QObject
{
    Q_OBJECT
public:
    explicit seriesHelper(QObject *parent = nullptr);

    void updateAxisRange(QLineSeries *series, QValueAxis *axisX);

signals:
};

#endif // SERIESHELPER_H
