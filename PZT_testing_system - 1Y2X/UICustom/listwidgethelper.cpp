#include "listwidgethelper.h"
#include "tabitem.h"


ListWidgetHelper::ListWidgetHelper(QObject *parent) : QObject(parent)
{

}

// ---------------------- 工具函数：添加自定义标签项 ----------------------
void ListWidgetHelper::addTabItem(QListWidget* listWidget, const QIcon& icon, const QString& text, bool setCurrentToNew) {
    // 1. 创建QListWidgetItem（作为容器）
    QListWidgetItem* item = new QListWidgetItem(listWidget);   // 为listWidget创建一个新的项目，也就是新的一行
    item->setSizeHint(QSize(listWidget->width(), 50)); // 项大小（宽度自适应listWidget，高度50px）

    // 2. 创建自定义TabItem（图标+文字）
    TabItemSub* tabWidget = new TabItemSub(icon, text, listWidget); // 为上面创建的新的项目填充内容，包括样式、文字等

    // 3. 将TabItem设置为item的 widget
    listWidget->setItemWidget(item, tabWidget);
    if(setCurrentToNew) listWidget->setCurrentRow(0); // 界面初始化默认索引0
    else listWidget->setCurrentRow(listWidget->count() - 1); // 选中最新添加的
//    emit updateUiCurComShowSignal(text);
    // 存储标签名到 UserRole
    item->setData(Qt::UserRole, text);     // 保存该item的名字，以便于后文根据这个名字执行相应的操作，比如设置checkbox的状态
}

/// ---------------------- 工具函数：为stackedWidget添加页面 ----------------------
void ListWidgetHelper::addStackedPage(QStackedWidget* stackedWidget, const QString& pageName) {
    // 1. 创建页面 widget（可替换为实际业务控件，比如串口设置界面）
    QWidget* page = new QWidget(stackedWidget);
    page->setStyleSheet("background-color: white;"); // 页面背景色（可选）

    // 2. 添加测试内容（替换为实际控件，比如你的串口名下拉、打开串口按钮）
    QLabel* label = new QLabel(pageName, page);
    label->setStyleSheet("font-size: 20px; color: #666;");
    QVBoxLayout* layout = new QVBoxLayout(page);
    layout->addWidget(label, 0, Qt::AlignCenter); // 测试内容居中

    // 3. 将页面添加到stackedWidget
    stackedWidget->addWidget(page);
}
