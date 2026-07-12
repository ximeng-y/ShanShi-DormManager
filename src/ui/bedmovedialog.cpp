#include "bedmovedialog.h"
#include "./ui_bedmovedialog.h"

#include "core/building.h"
#include "core/dorm.h"
#include "core/school.h"
#include "core/student.h"
#include "uifeedback.h"

#include <QPushButton>
#include <QSignalBlocker>

BedMoveDialog::BedMoveDialog(int student_id, QWidget* parent)
	: QDialog(parent)
	, ui(new Ui::BedMoveDialog)
	, current_student_id(student_id)
{
	ui->setupUi(this);
	const student* current_student = school::instance().get_student(student_id);
	ui->studentValueLabel->setText(current_student == nullptr
		? QStringLiteral("学生记录已不存在")
		: QStringLiteral("%1（%2）· 当前 %3号楼%4室%5号床")
			.arg(current_student->get_name()).arg(student_id)
			.arg(current_student->get_building_id()).arg(current_student->get_dorm_id()).arg(current_student->get_bed_id()));
	ui->buildingCombo->setAccessibleName(QStringLiteral("调宿目标楼栋"));
	ui->dormCombo->setAccessibleName(QStringLiteral("调宿目标宿舍"));
	ui->bedCombo->setAccessibleName(QStringLiteral("调宿目标床位"));
	ui->buildingLabel->setBuddy(ui->buildingCombo);
	ui->dormLabel->setBuddy(ui->dormCombo);
	ui->bedLabel->setBuddy(ui->bedCombo);
	ui->buildingCombo->setToolTip(QStringLiteral("仅显示接纳当前学生性别且存在可用目标宿舍的楼栋。"));
	ui->dormCombo->setToolTip(QStringLiteral("仅显示接纳当前学生性别且至少有一个空床的其他宿舍。"));
	ui->bedCombo->setToolTip(QStringLiteral("可让系统选择最小空床位，也可明确指定一个空床。"));
	ui->buttonBox->button(QDialogButtonBox::Ok)->setText(QStringLiteral("确认调宿"));
	ui->buttonBox->button(QDialogButtonBox::Cancel)->setText(QStringLiteral("取消"));
	connect(ui->buildingCombo, &QComboBox::currentIndexChanged, this, &BedMoveDialog::refresh_dorms);
	connect(ui->dormCombo, &QComboBox::currentIndexChanged, this, &BedMoveDialog::refresh_beds);
	connect(ui->bedCombo, &QComboBox::currentIndexChanged, this, &BedMoveDialog::refresh_beds);
	connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &BedMoveDialog::attempt_accept);
	connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
	setTabOrder(ui->buildingCombo, ui->dormCombo);
	setTabOrder(ui->dormCombo, ui->bedCombo);
	setTabOrder(ui->bedCombo, ui->buttonBox->button(QDialogButtonBox::Ok));
	setTabOrder(ui->buttonBox->button(QDialogButtonBox::Ok), ui->buttonBox->button(QDialogButtonBox::Cancel));
	refresh_buildings();
}

BedMoveDialog::~BedMoveDialog()
{
	delete ui;
}

int BedMoveDialog::target_building_id() const
{
	return confirmed_building_id;
}

int BedMoveDialog::target_dorm_id() const
{
	return confirmed_dorm_id;
}

int BedMoveDialog::target_bed_id() const
{
	return confirmed_bed_id;
}

void BedMoveDialog::refresh_buildings()
{
	const QSignalBlocker blocker(ui->buildingCombo);
	ui->buildingCombo->clear();
	const student* current_student = school::instance().get_student(current_student_id);
	if (current_student == nullptr || current_student->get_gender() < 1 || current_student->get_gender() > 2) {
		refresh_dorms();
		return;
	}
	for (int building_id : school::instance().get_all_building_ids()) {
		const building* current_building = school::instance().get_building(building_id);
		bool has_available_target = false;
		for (const QPair<int, int>& dorm_key : school::instance().get_dorm_keys_of_building(building_id)) {
			const dorm* candidate = school::instance().get_dorm(dorm_key.first, dorm_key.second);
			if (candidate != nullptr && candidate->accepts_gender(current_student->get_gender()) && !candidate->is_full()
				&& (dorm_key.first != current_student->get_building_id() || dorm_key.second != current_student->get_dorm_id())) {
				has_available_target = true;
				break;
			}
		}
		if (current_building != nullptr && current_building->accepts_gender(current_student->get_gender()) && has_available_target) {
			ui->buildingCombo->addItem(QStringLiteral("%1号楼").arg(building_id), building_id);
		}
	}
	ui->buildingCombo->setEnabled(ui->buildingCombo->count() > 0);
	refresh_dorms();
}

void BedMoveDialog::refresh_dorms()
{
	const QSignalBlocker blocker(ui->dormCombo);
	ui->dormCombo->clear();
	const student* current_student = school::instance().get_student(current_student_id);
	const int building_id = ui->buildingCombo->currentData().toInt();
	if (current_student == nullptr || building_id <= 0) {
		refresh_beds();
		return;
	}
	for (const QPair<int, int>& dorm_key : school::instance().get_dorm_keys_of_building(building_id)) {
		const dorm* current_dorm = school::instance().get_dorm(dorm_key.first, dorm_key.second);
		if (current_dorm == nullptr || !current_dorm->accepts_gender(current_student->get_gender())
			|| current_dorm->is_full()
			|| (dorm_key.first == current_student->get_building_id() && dorm_key.second == current_student->get_dorm_id())) {
			continue;
		}
		ui->dormCombo->addItem(QStringLiteral("%1室 · %2个空床")
			.arg(current_dorm->get_id()).arg(current_dorm->get_empty_count()), current_dorm->get_id());
	}
	ui->dormCombo->setEnabled(ui->dormCombo->count() > 0);
	refresh_beds();
}

void BedMoveDialog::refresh_beds()
{
	const int building_id = ui->buildingCombo->currentData().toInt();
	const int dorm_id = ui->dormCombo->currentData().toInt();
	const dorm* target_dorm = school::instance().get_dorm(building_id, dorm_id);
	if (sender() != ui->bedCombo) {
		const QSignalBlocker blocker(ui->bedCombo);
		ui->bedCombo->clear();
		if (target_dorm != nullptr) {
			ui->bedCombo->addItem(QStringLiteral("自动选择最小空床位"), 0);
			for (int bed_id = 1; bed_id <= target_dorm->get_max_num(); ++bed_id) {
				if (target_dorm->is_bed_occupied(bed_id) == 0) {
					ui->bedCombo->addItem(QStringLiteral("%1号床").arg(bed_id), bed_id);
				}
			}
		}
	}
	const bool valid_target = target_dorm != nullptr && !target_dorm->is_full();
	ui->bedCombo->setEnabled(valid_target);
	ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(valid_target);
	ui->previewLabel->setText(valid_target
		? QStringLiteral("目标：%1号楼 · %2室 · %3")
			.arg(building_id).arg(dorm_id).arg(ui->bedCombo->currentText())
		: QStringLiteral("当前没有可用于调宿的目标宿舍。"));
}

void BedMoveDialog::attempt_accept()
{
	const student* current_student = school::instance().get_student(current_student_id);
	const int building_id = ui->buildingCombo->currentData().toInt();
	const int dorm_id = ui->dormCombo->currentData().toInt();
	const int bed_id = ui->bedCombo->currentData().toInt();
	const building* target_building = school::instance().get_building(building_id);
	const dorm* target_dorm = school::instance().get_dorm(building_id, dorm_id);
	const bool source_complete = current_student != nullptr
		&& current_student->get_building_id() > 0 && current_student->get_dorm_id() > 0
		&& current_student->get_bed_id() > 0 && current_student->get_floor() > 0;
	if (!source_complete || target_building == nullptr || target_dorm == nullptr
		|| (building_id == current_student->get_building_id() && dorm_id == current_student->get_dorm_id())
		|| !target_building->accepts_gender(current_student->get_gender())
		|| !target_dorm->accepts_gender(current_student->get_gender()) || target_dorm->is_full()
		|| (bed_id > 0 && target_dorm->is_bed_occupied(bed_id) != 0)) {
		uifeedback::show_error(this, QStringLiteral("无法确认调宿"), QStringLiteral("目标宿舍或床位状态已经变化，请重新选择。"));
		refresh_buildings();
		return;
	}
	confirmed_building_id = building_id;
	confirmed_dorm_id = dorm_id;
	confirmed_bed_id = bed_id;
	accept();
}
