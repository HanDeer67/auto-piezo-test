#ifndef TABITEM_H
#define TABITEM_H

#include <QWidget>
#include <QLabel>
#include <QVBoxLayout> // 垂直布局头文件
#include <QHBoxLayout> // 水平布局头文件
#include <QIcon>
#include <QPainter>
#include <QStyleOption>


/// *********************************主UI切换按钮***********************************
class TabItem : public QWidget {
    Q_OBJECT
public:
    explicit TabItem(const QIcon& icon, const QString& text, QWidget* parent = nullptr);

protected:
    void resizeEvent(QResizeEvent* event) override;

private:
    QLabel* m_iconLabel;   // 图像Label（上）
    QLabel* m_textLabel;   // 文字Label（下）
    QIcon m_icon;          // 原始图标
    QVBoxLayout* m_layout; // 垂直布局（图像和文字上下排列）
};


/// *********************************子切换按钮***********************************
class TabItemSub : public QWidget {
    Q_OBJECT
public:
    explicit TabItemSub(const QIcon& icon, const QString& text, QWidget* parent = nullptr);

protected:
    void resizeEvent(QResizeEvent* event) override;
//    QSize sizeHint() const override;

private:
    QLabel* m_iconLabel;   // 图像Label（上）
    QLabel* m_textLabel;   // 文字Label（下）
    QIcon m_icon;          // 原始图标
    QVBoxLayout* m_layout; // 垂直布局（图像和文字上下排列）
    QHBoxLayout* m_layout_h; // 垂直布局（图像和文字上下排列）
};




#endif // TABITEM_H
