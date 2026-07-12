#ifndef DORMRESOURCEPAGE_H
#define DORMRESOURCEPAGE_H

#include <QWidget>

QT_BEGIN_NAMESPACE
namespace Ui {
class DormResourcePage;
}
QT_END_NAMESPACE

class QShowEvent;
class QModelIndex;
class bedtablemodel;

//宿舍资源页面。以楼栋、宿舍、床位三级结构展示住宿空间。
class DormResourcePage : public QWidget
{
	Q_OBJECT

public:
	explicit DormResourcePage(QWidget* parent = nullptr);
	~DormResourcePage();
	void refresh_data();//刷新楼栋与宿舍目录
	void select_building(int building_id);//定位指定楼栋并清除宿舍搜索条件

protected:
	void showEvent(QShowEvent* event) override;//页面显示时刷新资源目录

private:
	void refresh_building_list();//刷新楼栋目录并恢复选择
	void refresh_dorm_list();//按当前楼栋和搜索条件刷新宿舍目录
	void clear_dorm_detail();//清除失效宿舍选择
	void show_dorm_detail(int dorm_id);//显示宿舍摘要与床位住客
	void update_bed_action_state(const QModelIndex& index);//按选中床位更新情境操作栏
	void assign_selected_bed();//为当前空床办理指定床位入住
	void remove_selected_occupant();//为当前住客办理退宿
	void begin_within_dorm_bed_change();//进入同宿舍目标床位选择状态
	void move_selected_occupant_to_other_dorm();//为当前住客选择并执行跨宿舍调宿
	void cancel_pending_bed_action();//取消床位目标选择状态
	void complete_within_dorm_bed_change(int target_bed_id, int target_student_id);//按目标床状态执行移动或住客交换

	Ui::DormResourcePage* ui;
	bedtablemodel* bed_model = nullptr;
	int selected_building_id = 0;
	int selected_dorm_id = 0;
	int selected_bed_id = 0;
	int selected_student_id = 0;
	int pending_move_student_id = 0;
	int pending_source_building_id = 0;
	int pending_source_dorm_id = 0;
	int pending_source_bed_id = 0;
};

#endif // DORMRESOURCEPAGE_H
