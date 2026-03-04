#ifndef LISTWIDGETHELPER_H
#define LISTWIDGETHELPER_H

#include <QObject>
#include <QListWidget>
#include <QStackedWidget>
#include <QLabel>
#include <QVBoxLayout>

class ListWidgetHelper : public QObject
{
    Q_OBJECT
public:
    explicit ListWidgetHelper(QObject *parent = nullptr);

    void addTabItem(QListWidget *listWidget, const QIcon &icon, const QString &text, bool setCurrentToNew);
    void addStackedPage(QStackedWidget *stackedWidget, const QString &pageName);

signals:
    void updateUiCurComShowSignal(QString text);

public slots:
};

#endif // LISTWIDGETHELPER_H
