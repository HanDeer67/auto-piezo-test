#include "serieshelper.h"
#include "qlineseries.h"

seriesHelper::seriesHelper(QObject *parent)
    : QObject{parent}
{}


void seriesHelper::updateAxisRange(QLineSeries *series, QValueAxis *axisX)
{
    if (series->count() == 0) return;

    double minX = std::numeric_limits<double>::max();
    double maxX = std::numeric_limits<double>::lowest();

    for (auto p : series->points()) {
        minX = qMin(minX, p.x());
        maxX = qMax(maxX, p.x());
    }

    axisX->setRange(minX, maxX);
    axisX->applyNiceNumbers();
}
