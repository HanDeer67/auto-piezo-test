#ifndef IMAGEGRAPHICSVIEW_H
#define IMAGEGRAPHICSVIEW_H

#include <QGraphicsView>
#include <QWheelEvent>

class ImageGraphicsView : public QGraphicsView
{
    Q_OBJECT
public:
    explicit ImageGraphicsView(QWidget *parent = nullptr);

    QGraphicsPixmapItem* pixmapItem;  //这个指针只是用来引用 mainwindow.cpp 中的 pixmapItem，而不是创建一个新的实例。
    void setPixmapItem(QGraphicsPixmapItem* item) {
        pixmapItem = item;
    }

protected:
    void wheelEvent(QWheelEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void adjustSceneRect();


private:
    double scaleFactor = 1.15; // 缩放因子

    bool isDragging;  // 记录是否处于拖动状态
    QPoint dragStartPos;  // 记录拖动的起始位置
    QPointF scenePos;

signals:
    void mousePositionChanged(int x,int y,QPointF scenePos); // 鼠标光标位置变化时发送的信号
    void mouseClicked(int pixelValue,QPointF scenePos); // 鼠标点击时发送的信号
};

#endif // IMAGEGRAPHICSVIEW_H
