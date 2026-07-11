#ifndef ADDSTUDENTDIALOG_H
#define ADDSTUDENTDIALOG_H

#include <QDialog>

QT_BEGIN_NAMESPACE
namespace Ui {
class AddStudentDialog;
}
QT_END_NAMESPACE

//新增学生弹窗。负责一次性收集并校验学生基础资料。
class AddStudentDialog : public QDialog
{
	Q_OBJECT

public:
	explicit AddStudentDialog(QWidget* parent = nullptr);
	~AddStudentDialog();
	int added_student_id() const;//返回本次成功添加的学号，未成功时为0

private:
	void attempt_add();//校验输入并调用 school 新增学生

	Ui::AddStudentDialog* ui;
	int added_id = 0;
};

#endif // ADDSTUDENTDIALOG_H
