#ifndef STUDENTDETAILDIALOG_H
#define STUDENTDETAILDIALOG_H

#include <QDialog>

QT_BEGIN_NAMESPACE
namespace Ui {
class StudentDetailDialog;
}
QT_END_NAMESPACE

//学生完整详情窗口。只读展示学生资料，不在双击行为中隐式进入编辑。
class StudentDetailDialog : public QDialog
{
	Q_OBJECT

public:
	explicit StudentDetailDialog(int student_id, QWidget* parent = nullptr);
	~StudentDetailDialog();

private:
	void load_student(int student_id);//加载指定学生当前资料

	Ui::StudentDetailDialog* ui;
};

#endif // STUDENTDETAILDIALOG_H
