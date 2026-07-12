#include "addstudentdialog.h"
#include "./ui_addstudentdialog.h"

#include "core/school.h"
#include "system/check.h"
#include "uifeedback.h"

#include <QPushButton>
#include <QRegularExpression>
#include <QRegularExpressionValidator>
#include <QStyle>

namespace {
void set_field_error(QWidget* field, QLabel* error_label, bool has_error)
{
	field->setProperty("inputError", has_error);
	field->style()->unpolish(field);
	field->style()->polish(field);
	error_label->setVisible(has_error);
}
}

AddStudentDialog::AddStudentDialog(QWidget* parent)
	: QDialog(parent)
	, ui(new Ui::AddStudentDialog)
{
	ui->setupUi(this);
	ui->genderCombo->clear();
	ui->genderCombo->addItem(QStringLiteral("男"), 1);
	ui->genderCombo->addItem(QStringLiteral("女"), 2);
	ui->genderCombo->setCurrentIndex(-1);
	ui->studentSequenceEdit->setValidator(new QRegularExpressionValidator(QRegularExpression(QStringLiteral("[0-9]{4}")), ui->studentSequenceEdit));
	ui->studentIdPrefixLabel->setAccessibleName(QStringLiteral("学号前四位"));
	ui->studentSequenceEdit->setAccessibleName(QStringLiteral("学号后四位"));
	ui->nameLineEdit->setAccessibleName(QStringLiteral("学生姓名"));
	ui->genderCombo->setAccessibleName(QStringLiteral("学生性别"));
	ui->genderCombo->setAccessibleDescription(QStringLiteral("新增学生必须主动选择男或女。"));
	ui->classSpin->setAccessibleName(QStringLiteral("学生班级"));
	ui->gradeSpin->setAccessibleName(QStringLiteral("学生年级"));
	ui->buttonBox->button(QDialogButtonBox::Ok)->setText(QStringLiteral("添加"));
	ui->buttonBox->button(QDialogButtonBox::Cancel)->setText(QStringLiteral("取消"));
	connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &AddStudentDialog::attempt_add);
	connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
	connect(ui->gradeSpin, &QSpinBox::valueChanged, this, [this]() { refresh_student_id(true); });
	connect(ui->classSpin, &QSpinBox::valueChanged, this, [this]() { refresh_student_id(false); });
	connect(ui->studentSequenceEdit, &QLineEdit::textChanged, this, [this]() { refresh_student_id(false); });
	refresh_student_id(true);
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
	clear_validation();
	const QString name = ui->nameLineEdit->text().trimmed();
	const int gender = ui->genderCombo->currentData().toInt();
	const int class_num = ui->classSpin->value();
	const int grade = ui->gradeSpin->value();

	const QString sequence_text = ui->studentSequenceEdit->text();
	const int sequence = sequence_text.toInt();
	if (sequence_text.size() != 4 || !check::is_valid_student_sequence(sequence)) {
		set_field_error(ui->studentSequenceEdit, ui->studentIdErrorLabel, true);
		ui->studentSequenceEdit->setFocus();
		uifeedback::show_error(this, QStringLiteral("无法新增学生"), QStringLiteral("学号后四位必须为0001～9999。"));
		return;
	}
	if (!check::is_valid_student_name(name)) {
		set_field_error(ui->nameLineEdit, ui->nameErrorLabel, true);
		ui->nameLineEdit->setFocus();
		uifeedback::show_error(this, QStringLiteral("无法新增学生"), QStringLiteral("姓名应为1～20个字符，且不能包含禁止内容。"));
		return;
	}
	if ((gender != 1 && gender != 2) || !check::is_valid_class_num(class_num) || !check::is_valid_grade(grade)) {
		if (gender != 1 && gender != 2) {
			set_field_error(ui->genderCombo, ui->genderErrorLabel, true);
			ui->genderCombo->setFocus();
		} else if (!check::is_valid_class_num(class_num)) {
			set_field_error(ui->classSpin, ui->classErrorLabel, true);
			ui->classSpin->setFocus();
		} else {
			set_field_error(ui->gradeSpin, ui->gradeErrorLabel, true);
			ui->gradeSpin->setFocus();
		}
		uifeedback::show_error(this, QStringLiteral("无法新增学生"), QStringLiteral("性别、班级或年级不符合数据规则。"));
		return;
	}
	const int result = school::instance().add_student(name, gender, grade, class_num, sequence);
	if (result <= 0) {
		if (result == -2 || result == -3) {
			set_field_error(ui->studentSequenceEdit, ui->studentIdErrorLabel, true);
			ui->studentSequenceEdit->setFocus();
		}
		const QString detail = result == -2 ? QStringLiteral("该后四位序号已被同年级学生使用。")
			: result == -3 ? QStringLiteral("生成的完整学号已经存在。")
			: result == -9 ? QStringLiteral("该年级的学号序号已经耗尽。")
			: QStringLiteral("学生资料不符合统一学号规则，请检查后重试。");
		uifeedback::show_error(this, QStringLiteral("无法新增学生"), detail);
		return;
	}

	added_id = result;
	accept();
}

void AddStudentDialog::clear_validation()
{
	set_field_error(ui->studentSequenceEdit, ui->studentIdErrorLabel, false);
	set_field_error(ui->nameLineEdit, ui->nameErrorLabel, false);
	set_field_error(ui->genderCombo, ui->genderErrorLabel, false);
	set_field_error(ui->classSpin, ui->classErrorLabel, false);
	set_field_error(ui->gradeSpin, ui->gradeErrorLabel, false);
}

void AddStudentDialog::refresh_student_id(bool refresh_sequence)//刷新学号建议与预览
{
	const int grade = ui->gradeSpin->value();
	const int class_num = ui->classSpin->value();
	ui->studentIdPrefixLabel->setText(QStringLiteral("%1%2").arg(grade % 100, 2, 10, QLatin1Char('0')).arg(class_num, 2, 10, QLatin1Char('0')));
	if (refresh_sequence)
	{
		const int suggested_id = school::instance().suggest_student_id(grade, class_num);
		ui->studentSequenceEdit->setText(suggested_id > 0
			? QStringLiteral("%1").arg(check::student_id_sequence(suggested_id), 4, 10, QLatin1Char('0')) : QString());
	}
	const QString sequence = ui->studentSequenceEdit->text();
	ui->studentIdPreviewLabel->setText(sequence.size() == 4
		? QStringLiteral("完整：%1%2").arg(ui->studentIdPrefixLabel->text(), sequence)
		: QStringLiteral("完整：待补全"));
}
