#include "ImageGraphicsView.h"
#include <QGraphicsView>
#include <QGraphicsPixmapItem>
#include <QColor>
#include <cmath>


ImageGraphicsView::ImageGraphicsView(QWidget *parent)
    : QGraphicsView(parent),isDragging(false)
{
//    setDragMode(QGraphicsView::ScrollHandDrag);
    setDragMode(QGraphicsView::NoDrag);
    setCursor(Qt::ArrowCursor);

    setMouseTracking(true); // 启用鼠标追踪，用于实时捕获当前光标位置

    setTransformationAnchor(QGraphicsView::AnchorUnderMouse);  // 缩放时以鼠标为中心
}

void ImageGraphicsView::wheelEvent(QWheelEvent *event)
{
    if (event->angleDelta().y() > 0)
        scale(scaleFactor, scaleFactor);       // 放大
    else
        scale(1.0 / scaleFactor, 1.0 / scaleFactor); // 缩小
}

/*// 实现其他事件处理函数    完全禁用自动场景范围调整
void ImageGraphicsView::wheelEvent(QWheelEvent *event) {
    const double scaleFactor = 1.15;

    if (event->angleDelta().y() > 0) {
        scale(scaleFactor, scaleFactor);  // 放大
    } else {
        scale(1.0 / scaleFactor, 1.0 / scaleFactor);  // 缩小
    }
    // 缩放之后，调整场景的范围
    // adjustSceneRect();
    // 添加以下代码强制扩展场景范围   // 完全禁用自动调整，保持固定的大场景范围
    QRectF currentRect = sceneRect();
    setSceneRect(currentRect.adjusted(-2000, -2000, 2000, 2000));
} */

void ImageGraphicsView::mouseMoveEvent(QMouseEvent *event) {
    QGraphicsView::mouseMoveEvent(event);
    // 注意：event 是一个指向 QMouseEvent 对象的指针，它包含与鼠标事件相关的信息（比如鼠标点击、移动、释放等）
    // QPointF viewPos = event->pos(); //获取当前视图坐标
    // 获取当前视图坐标,并将视图坐标转为场景坐标
    scenePos = mapToScene(event->pos());
    //发射鼠标坐标更新的信号，随信号附带坐标
    emit mousePositionChanged(static_cast<int>(scenePos.x()),static_cast<int>(scenePos.y()),scenePos);

    // 实时显示当前像素的灰度值
    QPointF clickedPos = mapToScene(event->pos());  // 场景坐标
    QPointF itemPos = pixmapItem->mapFromScene(clickedPos); // 转换为pixmapItem内的坐标

    if (pixmapItem->contains(itemPos)) {
        QImage tempImg = pixmapItem->pixmap().toImage();

        // floor 避免浮点取到邻近像素
        QPoint pixelPos(floor(itemPos.x()), floor(itemPos.y()));

        // 限制边界，避免越界崩溃
        pixelPos.setX(qBound(0, pixelPos.x(), tempImg.width() - 1));
        pixelPos.setY(qBound(0, pixelPos.y(), tempImg.height() - 1));

        int grayValue = qGray(tempImg.pixel(pixelPos));
        emit mouseClicked(grayValue, clickedPos); // 注意这里传的是 scene 坐标
    }
}

void ImageGraphicsView::resizeEvent(QResizeEvent *event) {
    QGraphicsView::resizeEvent(event);  // 调用父类的事件处理

    // 获取当前窗口的大小
    QSize viewportSize = viewport()->size();
    // qDebug()<<viewportSize;

    // 当前计算方式为窗口大小的800%，建议改为更大的倍数
    qreal sceneWidth = viewportSize.width() * 8;  // 从 2 改为 8
    qreal sceneHeight = viewportSize.height() * 8; // 从 2 改为 8

    // 设置场景范围比窗口大 50%
    // setSceneRect(-sceneWidth / 4, -sceneHeight / 4, sceneWidth, sceneHeight);
    setSceneRect(-sceneWidth / 2, -sceneHeight / 2, sceneWidth, sceneHeight);
}

void ImageGraphicsView::adjustSceneRect() {
    qreal scenePadding = 500;  // 设置适当的边距，确保有足够的拖动空间

    // 调整场景范围
    QRectF newSceneRect = scene()->itemsBoundingRect().adjusted(-scenePadding, -scenePadding, scenePadding, scenePadding);
    setSceneRect(newSceneRect);
}


void ImageGraphicsView::mousePressEvent(QMouseEvent *event) {

    if (event->button() == Qt::LeftButton) {
        setDragMode(QGraphicsView::ScrollHandDrag);
        dragStartPos = event->pos();
        setCursor(Qt::ClosedHandCursor);
    }
    QGraphicsView::mousePressEvent(event); /// 注意，这行代码必须放在后面，不能放在前面，如果防在前面
    /// 无法及时获取状态，比如点击左键，按理说进入可以自由拖拽的状态，但是因为这行代码在前面，无法及时获悉。
 /*   // 添加鼠标点击事件：点击图像时显示当前像素的灰度值
    QPointF clickedPos = mapToScene(event->pos());  // 场景坐标
    QPointF itemPos = pixmapItem->mapFromScene(clickedPos); // 转换为pixmapItem内的坐标

    if (pixmapItem->contains(itemPos)) {
        QImage tempImg = pixmapItem->pixmap().toImage();

        // floor 避免浮点取到邻近像素
        QPoint pixelPos(floor(itemPos.x()), floor(itemPos.y()));

        // 限制边界，避免越界崩溃
        pixelPos.setX(qBound(0, pixelPos.x(), tempImg.width() - 1));
        pixelPos.setY(qBound(0, pixelPos.y(), tempImg.height() - 1));

        int grayValue = qGray(tempImg.pixel(pixelPos));
        emit mouseClicked(grayValue, clickedPos); // 注意这里传的是 scene 坐标
    }
*/

}


void ImageGraphicsView::mouseReleaseEvent(QMouseEvent *event) {

    if (event->button() == Qt::LeftButton) {
        setDragMode(QGraphicsView::NoDrag); // 👆 松开后恢复箭头
        setCursor(Qt::ArrowCursor);
    }
    QGraphicsView::mouseReleaseEvent(event);
}
