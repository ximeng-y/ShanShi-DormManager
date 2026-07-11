#include "addstudentdialog.h"
#include "./ui_addstudentdialog.h"

#include "core/school.h"
#include "core/student.h"
#include "system/check.h"
#include "uifeedback.h"

#include <QPushButton>

AddStudentDialog::AddStudentDialog(QWidget* parent)
	: QDialog(parent)
	, ui(new Ui::AddStudentDialog)
{
	ui->setupUi(this);
	ui->genderCombo->clear();
	ui->genderCombo->addItem(QStringLiteral("未设置"), 0);
	ui->genderCombo->addItem(QStringLiteral("男"), 1);
	ui->genderCombo->addItem(QStringLiteral("女"), 2);
	ui->buttonBox->button(QDialogButtonBox::Ok)->setText(QStringLiteral("添加"));
	ui->buttonBox->button(QDialogButtonBox::Cancel)->setText(QStringLiteral("取消"));
	connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &AddStudentDialog::attempt_add);
	connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

AddStudentDialog::~AddStudentDialog()
{
	delete ui;
}

int AddStudentDialog::added_student_id() const
{
	return added_id;
}

void AddStudentDialog::attempt_add()
{
	const int student_id = ui->studentIdSpin->value();
	const QString name = ui->nameLineEdit->text().trimmed();
	const int gender = ui->genderCombo->currentData().toInt();
	const int class_num = ui->classSpin->value();
	const int grade = ui->gradeSpin->value();

	if (!check::is_valid_student_id(student_id)) {
		uifeedback::show_error(this, QStringLiteral("无法新增学生"), QStringLiteral("学号必须为8位有效数字。"));
		return;
	}
	if (!check::is_valid_student_name(name)) {
		uifeedback::show_error(this, QStringLiteral("无法新增学生"), QStringLiteral("姓名应为1～20个字符，且不能包含禁止内容。"));
		return;
	}
	if (!check::is_valid_gender(gender) || !check::is_valid_class_num(class_num) || !check::is_valid_grade(grade)) {
		uifeedback::show_error(this, QStringLiteral("无法新增学生"), QStringLiteral("性别、班级或年级不符合数据规则。"));
		return;
	}
	if (school::instance().get_student(student_id) != nullptr) {
		uifeedback::show_error(this, QStringLiteral("无法新增学生"), QStringLiteral("该学号已经存在，请检查后重新输入。"));
		return;
	}

	student student_to_add;
	const bool initialized = student_to_add.set_id(student_id)
		&& student_to_add.set_name(name)
		&& student_to_add.set_gender(gender)
		&& student_to_add.set_class_num(class_num)
		&& student_to_add.set_grade(grade);
	if (!initialized || !school::instance().add_student(student_to_add)) {
		uifeedback::show_error(this, QStringLiteral("无法新增学生"), QStringLiteral("学生资料未能写入，请检查输入后重试。"));
		return;
	}

	added_id = student_id;
	accept();
}
