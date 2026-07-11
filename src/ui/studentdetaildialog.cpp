#include "studentdetaildialog.h"
#include "./ui_studentdetaildialog.h"

#include "core/school.h"
#include "core/student.h"

namespace {
QString detail_gender_text(int gender)
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

StudentDetailDialog::StudentDetailDialog(int student_id, QWidget* parent)
	: QDialog(parent)
	, ui(new Ui::StudentDetailDialog)
{
	ui->setupUi(this);
	connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
	load_student(student_id);
}

StudentDetailDialog::~StudentDetailDialog()
{
	delete ui;
}

void StudentDetailDialog::load_student(int student_id)
{
	const student* current_student = school::instance().get_student(student_id);
	if (current_student == nullptr) {
		ui->nameLabel->setText(QStringLiteral("学生不存在"));
		ui->contentWidget->setEnabled(false);
		return;
	}

	ui->nameLabel->setText(current_student->get_name());
	ui->studentIdValueLabel->setText(QString::number(current_student->get_id()));
	ui->genderValueLabel->setText(detail_gender_text(current_student->get_gender()));
	ui->gradeValueLabel->setText(QString::number(current_student->get_grade()));
	ui->classValueLabel->setText(QStringLiteral("%1班").arg(current_student->get_class_num()));

	const bool assigned = school::instance().get_assigned_student_ids().contains(student_id);
	ui->statusValueLabel->setText(assigned ? QStringLiteral("已入住") : QStringLiteral("未入住"));
	if (assigned && current_student->get_building_id() > 0 && current_student->get_dorm_id() > 0
		&& current_student->get_floor() > 0 && current_student->get_bed_id() > 0) {
		ui->buildingValueLabel->setText(QStringLiteral("%1号楼").arg(current_student->get_building_id()));
		ui->dormValueLabel->setText(QStringLiteral("%1室").arg(current_student->get_dorm_id()));
		ui->floorValueLabel->setText(QStringLiteral("%1层").arg(current_student->get_floor()));
		ui->bedValueLabel->setText(QStringLiteral("%1号床").arg(current_student->get_bed_id()));
	} else if (assigned) {
		ui->buildingValueLabel->setText(QStringLiteral("住宿记录异常"));
		ui->dormValueLabel->setText(QStringLiteral("—"));
		ui->floorValueLabel->setText(QStringLiteral("—"));
		ui->bedValueLabel->setText(QStringLiteral("—"));
	} else {
		ui->buildingValueLabel->setText(QStringLiteral("—"));
		ui->dormValueLabel->setText(QStringLiteral("—"));
		ui->floorValueLabel->setText(QStringLiteral("—"));
		ui->bedValueLabel->setText(QStringLiteral("—"));
	}
}
