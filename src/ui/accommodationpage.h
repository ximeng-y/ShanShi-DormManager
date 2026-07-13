#ifndef ACCOMMODATIONPAGE_H
#define ACCOMMODATIONPAGE_H

#include <QList>
#include <QHash>
#include <QWidget>

QT_BEGIN_NAMESPACE
namespace Ui {
class AccommodationPage;
}
QT_END_NAMESPACE

class QShowEvent;
class QComboBox;
class QEvent;
class student;

//住宿安排页面。按任务组织入住、退宿、调宿换床与学生互换流程。
class AccommodationPage : public QWidget
{
	Q_OBJECT

public:
	explicit AccommodationPage(QWidget* parent = nullptr);
	~AccommodationPage();
	void refresh_data();//刷新四类住宿任务的学生与目标预览
	void prepare_student_task(int student_id, bool assigned);//从学生详情进入并预填入住或调宿任务

protected:
	void showEvent(QShowEvent* event) override;//页面显示时刷新住宿任务预览
	bool eventFilter(QObject* watched, QEvent* event) override;//输入法合成期间延后学生搜索，避免弹层重入

private:
	void refresh_student_selectors();//按各任务适用的入住状态刷新可搜索学生下拉框
	void populate_student_selector(QComboBox* combo, const QList<int>& student_ids, int excluded_student_id = 0);//填充姓名、性别与学号搜索项
	void handle_student_search_text(QComboBox* combo, const QString& search_text);//按输入法状态安全触发筛选与关联刷新
	void refresh_after_student_search(QComboBox* combo);//刷新当前学生选择器对应的业务预览
	QComboBox* student_selector_for_editor(QObject* editor) const;//按输入框定位所属学生下拉框
	void filter_student_selector(QComboBox* combo, const QString& search_text);//实时筛选下拉内容并显示无匹配提示
	int selected_student_id(const QComboBox* combo) const;//仅返回由下拉结果明确选中的学生学号
	void select_student(QComboBox* combo, int student_id);//按稳定学号预选下拉项
	void update_assign_controls();//按安置方式启用目标宿舍与床位输入
	void refresh_assign_buildings();//按当前学生性别刷新可用目标楼栋
	void refresh_assign_dorms();//按目标楼栋刷新可用宿舍
	void refresh_assign_beds();//按目标宿舍刷新可用空床
	void update_assign_preview();//刷新入住学生与目标位置预览
	void submit_assignment();//提交学生入住业务
	void show_assignment_error(int result);//解释入住返回码
	void update_remove_preview();//刷新退宿学生与当前位置预览
	void submit_remove();//提交保留学籍的退宿业务
	void update_move_preview();//刷新调宿学生、原位置与目标位置预览
	void refresh_move_buildings();//按学生性别与可用宿舍刷新目标楼栋
	void refresh_move_dorms();//按目标楼栋刷新可用宿舍
	void refresh_move_beds();//按目标宿舍刷新可用床位
	void submit_move();//提交调宿或同宿舍换床业务
	void show_move_error(int result);//解释调宿返回码与恢复失败
	void update_swap_preview();//刷新两名学生当前位置预览
	void submit_swap();//提交两名学生床位互换业务
	void show_swap_error(int result);//解释学生互换返回码
	QString validate_swap_constraints(const student& student1, const student& student2, bool& data_error) const;//交换前只读检查性别与目标约束

	Ui::AccommodationPage* ui;
	QHash<QComboBox*, QList<int>> student_selector_ids;//保存各任务当前允许选择的完整学生目录
};

#endif // ACCOMMODATIONPAGE_H
