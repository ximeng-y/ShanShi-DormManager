#include "bedassignmentdialog.h"
#include "./ui_bedassignmentdialog.h"

#include "core/building.h"
#include "core/dorm.h"
#include "core/school.h"
#include "core/student.h"
#include "system/check.h"
#include "uifeedback.h"

#include <QPushButton>

namespace {
bool has_any_position(const student& current_student)
{
	return current_student.get_bed_id() != 0 || current_student.get_dorm_id() != 0
		|| current_student.get_building_id() != 0 || current_student.get_floor() != 0;
}
}

BedAssignmentDialog::BedAssignmentDialog(int building_id, int dorm_id, int bed_id, QWidget* parent)
	: QDialog(parent)
	, ui(new Ui::BedAssignmentDialog)
	, target_building_id(building_id)
	, target_dorm_id(dorm_id)
	, target_bed_id(bed_id)
{
	ui->setupUi(this);
	ui->targetValueLabel->setText(QStringLiteral("%1号楼 · %2室 · %3号床")
		.arg(building_id).arg(dorm_id).arg(bed_id));
	ui->studentIdInput->setAccessibleName(QStringLiteral("待入住学生学号"));
	ui->buttonBox->button(QDialogButtonBox::Ok)->setText(QStringLiteral("确认入住"));
	ui->buttonBox->button(QDialogButtonBox::Cancel)->setText(QStringLiteral("取消"));
	connect(ui->studentIdInput, &identifierlineedit::valueChanged, this, &BedAssignmentDialog::update_preview);
	connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &BedAssignmentDialog::attempt_accept);
	connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
	update_preview();
}

BedAssignmentDialog::~BedAssignmentDialog()
{
	delete ui;
}

int BedAssignmentDialog::student_id() const
{
	return confirmed_student_id;
}

void BedAssignmentDialog::update_preview()
{
	ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
	const int student_id = ui->studentIdInput->value();
	const student* current_student = school::instance().get_student(student_id);
	if (!check::is_valid_student_id(student_id) || current_student == nullptr) {
		ui->previewLabel->setText(QStringLiteral("输入已有学生的8位学号。"));
		return;
	}
	if (has_any_position(*current_student)
		|| school::instance().get_assigned_student_ids().contains(student_id)) {
		ui->previewLabel->setText(QStringLiteral("%1（%2）当前已有住宿位置，请使用调宿流程。")
			.arg(current_student->get_name()).arg(student_id));
		return;
	}
	const building* target_building = school::instance().get_building(target_building_id);
	const dorm* target_dorm = school::instance().get_dorm(target_building_id, target_dorm_id);
	if (current_student->get_gender() < 1 || current_student->get_gender() > 2
		|| target_building == nullptr || target_dorm == nullptr
		|| !target_building->accepts_gender(current_student->get_gender())
		|| !target_dorm->accepts_gender(current_student->get_gender())
		|| target_dorm->is_bed_occupied(target_bed_id) != 0) {
		ui->previewLabel->setText(QStringLiteral("学生性别、目标房间性别锁或床位状态不允许本次入住。"));
		return;
	}
	ui->previewLabel->setText(QStringLiteral("%1（%2）· %3级 · %4班\n确认后入住当前所选空床。")
		.arg(current_student->get_name()).arg(student_id)
		.arg(current_student->get_grade()).arg(current_student->get_class_num()));
	ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
}

void BedAssignmentDialog::attempt_accept()
{
	update_preview();
	if (!ui->buttonBox->button(QDialogButtonBox::Ok)->isEnabled()) {
		uifeedback::show_error(this, QStringLiteral("无法办理入住"), QStringLiteral("学生或目标床位不符合入住条件，请检查后重试。"));
		return;
	}
	confirmed_student_id = ui->studentIdInput->value();
	accept();
}
