#ifndef MAINWIDGET_H
#define MAINWIDGET_H

#include <QWidget>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWidget;
}
QT_END_NAMESPACE

class MainWidget : public QWidget
{
    Q_OBJECT

public:
    MainWidget(QWidget *parent = nullptr);
    ~MainWidget();

private:
	void switch_page(int index);//切换主页面并同步标题与导航选中状态
	void set_sidebar_collapsed(bool collapsed);//展开或收起侧边导航

    Ui::MainWidget *ui;
	bool sidebar_collapsed = false;
};
#endif // MAINWIDGET_H
