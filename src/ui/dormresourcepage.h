#ifndef DORMRESOURCEPAGE_H
#define DORMRESOURCEPAGE_H

#include <QWidget>

QT_BEGIN_NAMESPACE
namespace Ui {
class DormResourcePage;
}
QT_END_NAMESPACE

class QShowEvent;

//宿舍资源页面。以楼栋、宿舍、床位三级结构展示住宿空间。
class DormResourcePage : public QWidget
{
	Q_OBJECT

public:
	explicit DormResourcePage(QWidget* parent = nullptr);
	~DormResourcePage();
	void refresh_data();//刷新楼栋与宿舍目录

protected:
	void showEvent(QShowEvent* event) override;//页面显示时刷新资源目录

private:
	void refresh_building_list();//刷新楼栋目录并恢复选择
	void refresh_dorm_list();//按当前楼栋和搜索条件刷新宿舍目录
	void clear_dorm_detail();//清除失效宿舍选择
	void show_dorm_detail(int dorm_id);//显示宿舍摘要与床位住客
	void clear_bed_grid();//清除动态床位卡片

	Ui::DormResourcePage* ui;
	int selected_building_id = 0;
	int selected_dorm_id = 0;
};

#endif // DORMRESOURCEPAGE_H
