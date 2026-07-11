#ifndef GENDERCORRECTIONDIALOG_H
#define GENDERCORRECTIONDIALOG_H

#include <QDialog>

QT_BEGIN_NAMESPACE
namespace Ui {
class GenderCorrectionDialog;
}
QT_END_NAMESPACE

//性别纠错弹窗。说明住宿约束并调用 school 完成一致性校验。
class GenderCorrectionDialog : public QDialog
{
	Q_OBJECT

public:
	explicit GenderCorrectionDialog(int student_id, QWidget* parent = nullptr);
	~GenderCorrectionDialog();

private:
	void attempt_correction();//提交目标性别并解释业务返回值

	Ui::GenderCorrectionDialog* ui;
	int target_student_id = 0;
};

#endif // GENDERCORRECTIONDIALOG_H
