#ifndef OVERVIEWPAGE_H
#define OVERVIEWPAGE_H

#include <QWidget>

QT_BEGIN_NAMESPACE
namespace Ui {
class OverviewPage;
}
QT_END_NAMESPACE

class QShowEvent;

//数据概览页面。负责展示学校协调层提供的只读统计结果。
class OverviewPage : public QWidget
{
	Q_OBJECT

public:
	explicit OverviewPage(QWidget* parent = nullptr);
	~OverviewPage();
	void refresh_data();//刷新概览统计与楼栋容量数据

protected:
	void showEvent(QShowEvent* event) override;//页面显示时刷新只读数据

private:
	void open_sample_data_dialog();//打开样例参数弹窗并在成功后刷新概览
	void refresh_summary();//刷新学生、床位与入住情况统计
	void refresh_building_capacity();//刷新楼栋容量表格

	Ui::OverviewPage* ui;
};

#endif // OVERVIEWPAGE_H
