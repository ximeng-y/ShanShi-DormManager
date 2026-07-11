#include "editbuildingdialog.h"
#include "./ui_editbuildingdialog.h"

#include "core/building.h"
#include "core/school.h"
#include "uifeedback.h"

#include <QPushButton>

EditBuildingDialog::EditBuildingDialog(int building_id, QWidget* parent)
	: QDialog(parent)
	, ui(new Ui::EditBuildingDialog)
	, target_building_id(building_id)
{
	ui->setupUi(this);
	ui->genderCombo->addItem(QStringLiteral("男生楼"), 1);
	ui->genderCombo->addItem(QStringLiteral("女生楼"), 2);
	ui->genderCombo->addItem(QStringLiteral("男女混宿楼"), 3);
	ui->buttonBox->button(QDialogButtonBox::Ok)->setText(QStringLiteral("保存修改"));
	ui->buttonBox->button(QDialogButtonBox::Cancel)->setText(QStringLiteral("取消"));
	connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &EditBuildingDialog::attempt_save);
	connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

	const building* current_building = school::instance().get_building(building_id);
	if (current_building == nullptr) {
		ui->buildingIdValueLabel->setText(QStringLiteral("楼栋不存在"));
		ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
		return;
	}
	original_gender = current_building->get_for_gender();
	original_max_floor = current_building->get_max_floor();
	ui->buildingIdValueLabel->setText(QStringLiteral("%1号楼").arg(building_id));
	ui->genderCombo->setCurrentIndex(ui->genderCombo->findData(original_gender));
	ui->maxFloorSpin->setValue(original_max_floor);
}

EditBuildingDialog::~EditBuildingDialog()
{
	delete ui;
}

void EditBuildingDialog::attempt_save()
{
	const int new_gender = ui->genderCombo->currentData().toInt();
	const int new_max_floor = ui->maxFloorSpin->value();
	const bool change_gender = new_gender != original_gender;
	const bool change_floor = new_max_floor != original_max_floor;
	if (!change_gender && !change_floor) {
		uifeedback::show_error(this, QStringLiteral("无需保存"), QStringLiteral("楼栋属性没有发生变化。"));
		return;
	}

	school& current_school = school::instance();
	bool gender_changed = false;
	if (change_gender) {
		const int result = current_school.set_building_gender(target_building_id, new_gender);
		if (result != 1) {
			uifeedback::show_error(this, QStringLiteral("无法修改楼栋"), result == -2
				? QStringLiteral("楼内已有宿舍或住客与目标性别冲突。")
				: (result == 0 ? QStringLiteral("楼栋已经不存在，请刷新后重试。") : QStringLiteral("目标性别参数无效。")));
			return;
		}
		gender_changed = true;
	}
	if (change_floor) {
		const int result = current_school.set_building_max_floor(target_building_id, new_max_floor);
		if (result != 1) {
			const bool restored = !gender_changed || current_school.set_building_gender(target_building_id, original_gender) == 1;
			if (!restored) {
				uifeedback::show_critical(this, QStringLiteral("楼栋属性恢复失败"), QStringLiteral("最大楼层修改失败，且楼栋性别未能恢复。请暂停后续操作并核查数据。"));
			} else {
				uifeedback::show_error(this, QStringLiteral("无法修改楼栋"), result == -2
					? QStringLiteral("现有宿舍楼层超过新的最大楼层，已恢复原属性。")
					: (result == 0 ? QStringLiteral("楼栋已经不存在，已撤销前置修改。") : QStringLiteral("最大楼层参数无效，已恢复原属性。")));
			}
			return;
		}
	}
	accept();
}
