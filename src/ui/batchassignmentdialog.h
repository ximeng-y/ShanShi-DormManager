#ifndef BATCHASSIGNMENTDIALOG_H
#define BATCHASSIGNMENTDIALOG_H

#include "core/school.h"

#include <QDialog>

QT_BEGIN_NAMESPACE
namespace Ui {
class BatchAssignmentDialog;
}
QT_END_NAMESPACE

class QShowEvent;

//批量住宿分配向导。负责选择补分/重排任务、展示固定预览并经school执行。
class BatchAssignmentDialog : public QDialog
{
	Q_OBJECT

public:
	explicit BatchAssignmentDialog(QWidget* parent = nullptr);
	~BatchAssignmentDialog();

signals:
	void student_navigation_requested(int student_id);//请求主窗口跳转到异常学生
	void dorm_navigation_requested(int building_id, int dorm_id);//请求主窗口跳转到异常宿舍

protected:
	void showEvent(QShowEvent* event) override;//首次显示时按当前屏幕可用区域收缩

private:
	bool is_reassignment() const;//当前是否选择全校重新安排
	quint32 current_seed(bool* valid = nullptr) const;//读取随机种子
	void refresh_task_summary();//刷新任务页当前数据概况
	void refresh_strategy_page();//按任务切换可用策略与说明
	void refresh_preview_page();//按当前预览刷新汇总、明细及确认状态
	void update_apply_state();//刷新确认执行按钮可用状态
	void regenerate_seed();//生成新的32位随机种子
	void generate_preview();//调用school生成固定预览
	void apply_preview();//确认并执行当前固定预览
	void copy_issues();//复制住宿异常信息
	void open_student_detail(int row, int column);//双击明细打开学生只读详情
	QString issue_details() const;//汇总当前预览异常信息

	Ui::BatchAssignmentDialog* ui;
	batch_assignment_preview assignment_preview;
	reassignment_preview all_students_preview;
	bool preview_ready = false;
	bool fitted_to_screen = false;
};

#endif // BATCHASSIGNMENTDIALOG_H
