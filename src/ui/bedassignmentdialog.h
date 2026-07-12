#ifndef BEDASSIGNMENTDIALOG_H
#define BEDASSIGNMENTDIALOG_H

#include <QDialog>

QT_BEGIN_NAMESPACE
namespace Ui {
class BedAssignmentDialog;
}
QT_END_NAMESPACE

//指定空床入住弹窗。负责校验学生与当前床位是否可以建立住宿关系。
class BedAssignmentDialog : public QDialog
{
	Q_OBJECT

public:
	explicit BedAssignmentDialog(int building_id, int dorm_id, int bed_id, QWidget* parent = nullptr);
	~BedAssignmentDialog();
	int student_id() const;//返回已确认的学生学号，未确认时为0

private:
	void update_preview();//按当前学号展示学生与目标约束
	void attempt_accept();//复核学生及床位后确认

	Ui::BedAssignmentDialog* ui;
	int target_building_id = 0;
	int target_dorm_id = 0;
	int target_bed_id = 0;
	int confirmed_student_id = 0;
};

#endif // BEDASSIGNMENTDIALOG_H
