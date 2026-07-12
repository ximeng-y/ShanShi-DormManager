#ifndef MAINWIDGET_H
#define MAINWIDGET_H

#include <QWidget>

class QCloseEvent;
class QResizeEvent;
class QShowEvent;
class QEvent;

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
	void enter_read_only_mode();//运行中发生严重持久化错误时立即禁用全部写入口

protected:
	void closeEvent(QCloseEvent* event) override;//关闭前保存窗口与导航状态
	void resizeEvent(QResizeEvent* event) override;//窄窗口自动收起侧边导航
	void showEvent(QShowEvent* event) override;//首次显示后处理持久化启动提示
	bool eventFilter(QObject* watched, QEvent* event) override;//只读模式阻止写控件被页面刷新重新启用

private:
	void switch_page(int index);//切换主页面并同步标题与导航选中状态
	void set_sidebar_collapsed(bool collapsed);//展开或收起侧边导航
	void restore_window_state();//恢复窗口尺寸、位置与导航状态
	void save_window_state() const;//保存窗口尺寸、位置与导航状态
	void show_persistence_startup_notice();//显示目录降级、备份恢复、冲突或只读提示
	void refresh_all_pages();//加载或选择数据后刷新全部页面
	void lock_read_only_controls();//禁用全部写入口并保留查询、详情和跳转

    Ui::MainWidget *ui;
	bool sidebar_collapsed = false;
	bool persistence_notice_shown = false;
};
#endif // MAINWIDGET_H
