#include <QObject>
#include <QUdpSocket>
#include <QTimer>
#include <QRandomGenerator>

class UDPSimulator : public QObject
{
    Q_OBJECT

public:
    explicit UDPSimulator(QObject* parent = nullptr);


public:
    void start();

    double gaussian(double mean, double stddev);
private slots:
    void sendRandomData();

private:
    QUdpSocket* socket;
    QTimer* timer;
};
