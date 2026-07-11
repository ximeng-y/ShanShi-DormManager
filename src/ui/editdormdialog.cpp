#include "editdormdialog.h"
#include "./ui_editdormdialog.h"

#include "core/building.h"
#include "core/dorm.h"
#include "core/school.h"
#include "uifeedback.h"

#include <QPushButton>

EditDormDialog::EditDormDialog(int building_id, int dorm_id, QWidget* parent)
	: QDialog(parent)
	, ui(new Ui::EditDormDialog)
	, target_building_id(building_id)
	, target_dorm_id(dorm_id)
{
	ui->setupUi(this);
	ui->genderCombo->addItem(QStringLiteral("未锁定"), 0);
	const building* current_building = school::instance().get_building(building_id);
	if (current_building != nullptr && current_building->accepts_gender(1)) {
		ui->genderCombo->addItem(QStringLiteral("男舍"), 1);
	}
	if (current_building != nullptr && current_building->accepts_gender(2)) {
		ui->genderCombo->addItem(QStringLiteral("女舍"), 2);
	}
	ui->buttonBox->button(QDialogButtonBox::Ok)->setText(QStringLiteral("保存修改"));
	ui->buttonBox->button(QDialogButtonBox::Cancel)->setText(QStringLiteral("取消"));
	connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &EditDormDialog::attempt_save);
	connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

	const dorm* current_dorm = school::instance().get_dorm(building_id, dorm_id);
	if (current_dorm == nullptr || current_building == nullptr) {
		ui->dormValueLabel->setText(QStringLiteral("宿舍不存在"));
		ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
		return;
	}
	original_max_num = current_dorm->get_max_num();
	original_gender = current_dorm->get_for_gender();
	ui->dormValueLabel->setText(QStringLiteral("%1号楼 · %2室").arg(building_id).arg(dorm_id));
	ui->maxNumSpin->setMaximum(qMax(99, original_max_num));
	ui->maxNumSpin->setValue(original_max_num);
	ui->genderCombo->setCurrentIndex(ui->genderCombo->findData(original_gender));
}

EditDormDialog::~EditDormDialog()
{
	delete ui;
}

void EditDormDialog::attempt_save()
{
	const int new_max_num = ui->maxNumSpin->value();
	const int new_gender = ui->genderCombo->currentData().toInt();
	const bool change_capacity = new_max_num != original_max_num;
	const bool change_gender = new_gender != original_gender;
	if (!change_capacity && !change_gender) {
		uifeedback::show_error(this, QStringLiteral("无需保存"), QStringLiteral("宿舍属性没有发生变化。"));
		return;
	}
	if (new_max_num < 1 || (new_max_num > 99 && new_max_num != original_max_num)) {
		uifeedback::show_error(this, QStringLiteral("无法修改宿舍"), QStringLiteral("床位数量必须在1～99之间。"));
		return;
	}

	school& current_school = school::instance();
	bool capacity_changed = false;
	if (change_capacity) {
		const int result = current_school.set_dorm_max_num(target_building_id, target_dorm_id, new_max_num);
		if (result != 1) {
			uifeedback::show_error(this, QStringLiteral("无法修改宿舍"), result == -2
				? QStringLiteral("缩减床位会丢弃已有住客，请先调整相关学生床位。")
				: (result == 0 ? QStringLiteral("宿舍已经不存在，请刷新后重试。") : QStringLiteral("床位数量参数无效。")));
			return;
		}
		capacity_changed = true;
	}
	if (change_gender) {
		const int result = current_school.set_dorm_gender(target_building_id, target_dorm_id, new_gender);
		if (result != 1) {
			const bool restored = !capacity_changed || current_school.set_dorm_max_num(target_building_id, target_dorm_id, original_max_num) == 1;
			if (!restored) {
				uifeedback::show_critical(this, QStringLiteral("宿舍属性恢复失败"), QStringLiteral("性别锁修改失败，且原床位容量未能恢复。请暂停后续操作并核查数据。"));
			} else {
				const QString restore_suffix = capacity_changed ? QStringLiteral("，已恢复原容量") : QString();
				QString message;
				if (result == -2) {
					message = QStringLiteral("所在楼栋不接纳目标性别%1。").arg(restore_suffix);
				} else if (result == -3) {
					message = QStringLiteral("现有住客与目标性别锁冲突，或有住客时不能解除性别锁%1。").arg(restore_suffix);
				} else {
					message = result == 0 ? QStringLiteral("宿舍已经不存在。") : QStringLiteral("性别锁参数无效%1。").arg(restore_suffix);
				}
				uifeedback::show_error(this, QStringLiteral("无法修改宿舍"), message);
			}
			return;
		}
	}
	accept();
}
