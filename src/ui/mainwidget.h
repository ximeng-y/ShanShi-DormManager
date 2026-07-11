#ifndef MAINWIDGET_H
#define MAINWIDGET_H

#include <QWidget>

class QCloseEvent;
class QResizeEvent;

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

protected:
	void closeEvent(QCloseEvent* event) override;//关闭前保存窗口与导航状态
	void resizeEvent(QResizeEvent* event) override;//窄窗口自动收起侧边导航

private:
	void switch_page(int index);//切换主页面并同步标题与导航选中状态
	void set_sidebar_collapsed(bool collapsed);//展开或收起侧边导航
	void restore_window_state();//恢复窗口尺寸、位置与导航状态
	void save_window_state() const;//保存窗口尺寸、位置与导航状态

    Ui::MainWidget *ui;
	bool sidebar_collapsed = false;
};
#endif // MAINWIDGET_H
