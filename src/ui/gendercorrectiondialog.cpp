#include "gendercorrectiondialog.h"
#include "./ui_gendercorrectiondialog.h"

#include "core/school.h"
#include "core/student.h"
#include "uifeedback.h"

#include <QPushButton>

namespace {
QString correction_gender_text(int gender)
{
	if (gender == 1) {
		return QStringLiteral("男");
	}
	if (gender == 2) {
		return QStringLiteral("女");
	}
	return QStringLiteral("未设置");
}
}

GenderCorrectionDialog::GenderCorrectionDialog(int student_id, QWidget* parent)
	: QDialog(parent)
	, ui(new Ui::GenderCorrectionDialog)
	, target_student_id(student_id)
{
	ui->setupUi(this);
	ui->targetGenderCombo->addItem(QStringLiteral("男"), 1);
	ui->targetGenderCombo->addItem(QStringLiteral("女"), 2);
	ui->buttonBox->button(QDialogButtonBox::Ok)->setText(QStringLiteral("确认纠错"));
	ui->buttonBox->button(QDialogButtonBox::Cancel)->setText(QStringLiteral("取消"));
	connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &GenderCorrectionDialog::attempt_correction);
	connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

	const student* current_student = school::instance().get_student(student_id);
	if (current_student == nullptr) {
		ui->studentValueLabel->setText(QStringLiteral("学生不存在"));
		ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
		return;
	}
	ui->studentValueLabel->setText(QStringLiteral("%1（%2）").arg(current_student->get_name()).arg(current_student->get_id()));
	current_gender = current_student->get_gender();
	ui->currentGenderValueLabel->setText(correction_gender_text(current_gender));
	const int target_index = ui->targetGenderCombo->findData(current_student->get_gender() == 1 ? 2 : 1);
	ui->targetGenderCombo->setCurrentIndex(target_index >= 0 ? target_index : 0);
}

GenderCorrectionDialog::~GenderCorrectionDialog()
{
	delete ui;
}

void GenderCorrectionDialog::attempt_correction()
{
	const int target_gender = ui->targetGenderCombo->currentData().toInt();
	if (target_gender == current_gender) {
		uifeedback::show_error(this, QStringLiteral("无需纠正性别"), QStringLiteral("目标性别与当前性别相同，请选择实际需要纠正的性别。"));
		return;
	}
	const int result = school::instance().correct_student_gender(target_student_id, target_gender);
	if (result == 1) {
		accept();
		return;
	}
	if (result == -7) {
		uifeedback::show_error(this, QStringLiteral("无法纠正性别"), QStringLiteral("所在楼栋或宿舍不接纳目标性别，多人宿舍也不能只修改一名住客的性别。"));
		return;
	}
	if (result == -8) {
		uifeedback::show_critical(this, QStringLiteral("住宿记录异常"), QStringLiteral("学生位置与宿舍床位记录不一致，请暂停相关操作并核查数据。"));
		return;
	}
	uifeedback::show_error(this, QStringLiteral("无法纠正性别"), result == 0
		? QStringLiteral("该学生已经不存在，请刷新列表后重试。")
		: QStringLiteral("目标性别参数无效，请重新选择。"));
}
