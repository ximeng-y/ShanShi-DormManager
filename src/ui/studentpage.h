#ifndef STUDENTPAGE_H
#define STUDENTPAGE_H

#include <QWidget>

QT_BEGIN_NAMESPACE
namespace Ui {
class StudentPage;
}
QT_END_NAMESPACE

class QShowEvent;

//学生管理页面。负责学生目录、资料摘要与学生相关业务入口。
class StudentPage : public QWidget
{
	Q_OBJECT

public:
	explicit StudentPage(QWidget* parent = nullptr);
	~StudentPage();
	void refresh_data();//刷新学生目录与筛选结果

signals:
	void accommodation_requested(int student_id, bool assigned);//请求住宿任务页预填当前学生

protected:
	void showEvent(QShowEvent* event) override;//页面显示时刷新学生目录

private:
	void rebuild_class_filter();//根据当前学生目录刷新班级筛选项
	void apply_filters();//按搜索、班级和入住状态刷新表格
	void reset_filters();//阻断控件信号后重置筛选并单次刷新
	void show_student_summary(int student_id);//在详情面板显示学生摘要
	void clear_student_summary();//清除失效选择并恢复详情空状态
	void set_detail_panel_visible(bool visible);//展开或收起详情面板并保存状态
	void start_edit_student();//在详情面板进入基础资料编辑状态
	void cancel_edit_student();//退出编辑并恢复学生摘要
	void save_student_changes();//校验并保存姓名、班级与年级
	void set_student_directory_enabled(bool enabled);//编辑期间锁定目录与筛选，防止静默丢失输入
	void clear_edit_validation();//清除编辑字段错误状态

	Ui::StudentPage* ui;
	int selected_student_id = 0;
	bool filter_refresh_in_progress = false;
};

#endif // STUDENTPAGE_H
