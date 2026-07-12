#ifndef BATCHCLEARDIALOG_H
#define BATCHCLEARDIALOG_H

#include "core/school.h"

#include <QDialog>

QT_BEGIN_NAMESPACE
namespace Ui {
class BatchClearDialog;
}
QT_END_NAMESPACE

class QShowEvent;

//批量清退向导。负责选择清退范围、展示固定预览并经 school 统一执行。
class BatchClearDialog : public QDialog
{
	Q_OBJECT

public:
	explicit BatchClearDialog(QWidget* parent = nullptr);
	~BatchClearDialog();

	void set_initial_dorm(int building_id, int dorm_id);//从宿舍资源页进入时预选当前宿舍

protected:
	void showEvent(QShowEvent* event) override;//首次显示时按当前屏幕可用区域收缩

private:
	clear_scope selected_scope() const;//返回当前选择的清退范围
	bool reset_gender() const;//返回是否解除宿舍性别锁
	void populate_buildings();//刷新楼栋下拉框
	void populate_dorms();//根据楼栋刷新宿舍下拉框
	void refresh_scope_controls();//刷新范围控件、概况和按钮状态
	void refresh_summary();//刷新当前范围概况
	void generate_preview();//生成并展示固定清退预览
	void show_preview();//填充学生、宿舍和异常预览表格
	void update_execute_state();//按预览、范围和风险勾选更新执行按钮
	void return_to_selection();//返回范围选择并使旧预览失效
	void apply_preview();//确认并执行固定预览
	void copy_issues();//复制住宿异常信息
	QString gender_text(int gender) const;//转换宿舍性别锁文本
	QString scope_text() const;//转换当前清退范围文本

	Ui::BatchClearDialog* ui;
	batch_clear_preview current_preview;
	bool preview_ready = false;
	bool fitted_to_screen = false;
};

#endif // BATCHCLEARDIALOG_H
