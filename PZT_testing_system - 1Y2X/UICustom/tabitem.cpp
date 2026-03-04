#include "TabItem.h"


/// *********************************主UI切换按钮***********************************

TabItem::TabItem(const QIcon& icon, const QString& text, QWidget* parent)
    : QWidget(parent), m_icon(icon) {
    // 1. 垂直布局（核心：图像和文字上下排列）
    m_layout = new QVBoxLayout(this); // 垂直布局，确保上下排列
    m_layout->setContentsMargins(10, 10, 10, 10); // 内边距：上下15px，左右20px（避免贴边）
    m_layout->setSpacing(2); // 图像与文字的垂直间距（10px，避免拥挤）
    m_layout->setAlignment(Qt::AlignCenter); // 整体内容垂直居中

    // 2. 图像Label（填满宽度，保持比例）
    m_iconLabel = new QLabel(this);
    m_iconLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding); // 宽度填满，高度自适应
    m_iconLabel->setScaledContents(false); // 禁用自动拉伸（手动控制比例）
    m_iconLabel->setAlignment(Qt::AlignCenter); // 图像在Label内居中

    // 3. 文字Label（居中显示，清晰样式）
    m_textLabel = new QLabel(text, this);
    m_textLabel->setStyleSheet("font-size: 14px; color: #333; font-weight: 500;"); // 文字样式
    m_textLabel->setAlignment(Qt::AlignCenter); // 文字水平居中
    m_textLabel->setFixedHeight(15); // 文字高度固定（避免换行时被压缩）

    // 4. 添加控件到布局（图像上，文字下）
    m_layout->addWidget(m_iconLabel); // 图像Label（上）
    m_layout->addWidget(m_textLabel); // 文字Label（下）

    // 5. 设置Item固定高度（足够容纳图像+文字+间距，避免拥挤）
    setFixedHeight(100); // 垂直排列需要更大高度（根据图标大小调整，如120px）

//    // 初始计算图像尺寸（填满宽度，保持比例）
    resizeEvent(nullptr);
}

// 尺寸变化时更新图像（填满宽度，保持比例）
void TabItem::resizeEvent(QResizeEvent* event) {
    QWidget::resizeEvent(event);
    if (m_iconLabel && !m_icon.isNull()) {
        QSize labelSize = m_iconLabel->size(); // 图像Label的可用尺寸（Item宽度 - 左右内边距）
        QSize iconSize = m_icon.actualSize(QSize(1000, 1000)); // 获取原始图标尺寸（避免缩放失真）

        if (iconSize.isEmpty()) return;

        // 计算缩放比例：填满Label宽度（垂直排列时优先保证宽度填满）
        qreal scale = qreal(labelSize.width()) / iconSize.width();
        QSize scaledSize = iconSize * scale * 0.5;

        // 若高度超过Label，按高度缩放（避免图像溢出）
        if (scaledSize.height() > labelSize.height()) {
            scale = qreal(labelSize.height()) / iconSize.height();
            scaledSize = iconSize * scale;
        }

        // 设置缩放后的图像（保持比例，填满宽度）
        m_iconLabel->setPixmap(m_icon.pixmap(scaledSize));
    }
}


/// *********************************子切换按钮***********************************
TabItemSub::TabItemSub(const QIcon& icon, const QString& text, QWidget* parent)
    : QWidget(parent), m_icon(icon) {
    // 设置Item固定高度（关键：控制“不太高”，如40px，根据需求调整）

    // 1. 垂直布局（核心：图像和文字上下排列）
    m_layout_h = new QHBoxLayout(this); // 水平布局，确保左右排列
//    m_layout_h->setContentsMargins(10, 10, 10, 10); // 内边距：上下15px，左右20px（避免贴边）
//    m_layout_h->setSpacing(2); // 图像与文字的水平间距（10px，避免拥挤）
    m_layout_h->setAlignment(Qt::AlignLeft); // 整体内容水平居中

    // 2. 图像Label（填满宽度，保持比例）
    m_iconLabel = new QLabel(this);
    m_iconLabel->setFixedWidth(30); // 图标固定宽度（列表项中图标不宜过宽）
    m_iconLabel->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding); // 宽度固定，高度占满Item（垂直居中）
//    m_iconLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding); // 宽度填满，高度自适应
//    m_iconLabel->setScaledContents(false); // 禁用自动拉伸（手动控制比例）
    m_iconLabel->setAlignment(Qt::AlignCenter); // 图像在Label内垂直居中

    // 3. 文字Label（居中显示，清晰样式）
    m_textLabel = new QLabel(text, this);
//    m_textLabel->setStyleSheet("font-size: 14px; color: #333; font-weight: 400;"); // 文字样式
    m_textLabel->setStyleSheet(R"(
        font-size: 14px;    /* 文字大小 */
        color: #333;        /* 文字颜色（深灰色）*/
        font-weight: normal; /* 字体粗细 normal=400, bold=700 */
    )");
//    m_textLabel->setAlignment(Qt::AlignCenter); // 文字垂直居中
    m_textLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed); // 宽度扩展（占满剩余空间），高度固定
    m_textLabel->setAlignment(Qt::AlignVCenter | Qt::AlignLeft); // 垂直居中、水平左对齐（列表项文字习惯）
//    m_textLabel->setFixedHeight(15); // 文字高度固定（避免换行时被压缩）

    // 4. 添加控件到布局（图像上，文字下）
    m_layout_h->addWidget(m_iconLabel); // 图像Label（左）
    m_layout_h->addWidget(m_textLabel); // 文字Label（右）

    // 5. 设置Item固定高度（足够容纳图像+文字+间距，避免拥挤）
//    setFixedHeight(50); // 垂直排列需要更大高度（根据图标大小调整，如120px）
    // 设置Item固定尺寸（宽280，高60，根据需求调整）
//    setFixedSize(280, 20);
//    // 初始计算图像尺寸（填满宽度，保持比例）
    resizeEvent(nullptr);
}

// 尺寸变化时更新图像（填满宽度，保持比例）
void TabItemSub::resizeEvent(QResizeEvent* event) {
    QWidget::resizeEvent(event);
    if (m_iconLabel && !m_icon.isNull()) {
        QSize labelSize = m_iconLabel->size(); // 图像Label的可用尺寸（Item宽度 - 左右内边距）
        QSize iconSize = m_icon.actualSize(QSize(1000, 1000)); // 获取原始图标尺寸（避免缩放失真）

        if (iconSize.isEmpty()) return;

        // 计算缩放比例：填满Label宽度（垂直排列时优先保证宽度填满）
        qreal scale = qreal(labelSize.width()) / iconSize.width();
        QSize scaledSize = iconSize * scale * 0.5;

        // 若高度超过Label，按高度缩放（避免图像溢出）
        if (scaledSize.height() > labelSize.height()) {
            scale = qreal(labelSize.height()) / iconSize.height();
            scaledSize = iconSize * scale;
        }

        // 设置缩放后的图像（保持比例，填满宽度）
        m_iconLabel->setPixmap(m_icon.pixmap(scaledSize));
    }
}


