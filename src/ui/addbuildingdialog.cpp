#include "addbuildingdialog.h"
#include "./ui_addbuildingdialog.h"

#include "core/school.h"
#include "system/check.h"
#include "uifeedback.h"

#include <QPushButton>
#include <QStyle>

namespace {
void set_building_field_error(QWidget* field, QLabel* error_label, bool has_error)
{
	field->setProperty("inputError", has_error);
	field->style()->unpolish(field);
	field->style()->polish(field);
	error_label->setVisible(has_error);
}
}

AddBuildingDialog::AddBuildingDialog(QWidget* parent)
	: QDialog(parent)
	, ui(new Ui::AddBuildingDialog)
{
	ui->setupUi(this);
	ui->genderCombo->addItem(QStringLiteral("男生楼"), 1);
	ui->genderCombo->addItem(QStringLiteral("女生楼"), 2);
	ui->genderCombo->addItem(QStringLiteral("男女混宿楼"), 3);
	ui->buildingIdSpin->setAccessibleName(QStringLiteral("宿舍楼号"));
	ui->genderCombo->setAccessibleName(QStringLiteral("楼栋适用性别"));
	ui->maxFloorSpin->setAccessibleName(QStringLiteral("楼栋最大楼层"));
	ui->buttonBox->button(QDialogButtonBox::Ok)->setText(QStringLiteral("添加"));
	ui->buttonBox->button(QDialogButtonBox::Cancel)->setText(QStringLiteral("取消"));
	connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &AddBuildingDialog::attempt_add);
	connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

AddBuildingDialog::~AddBuildingDialog()
{
	delete ui;
}

int AddBuildingDialog::added_building_id() const
{
	return added_id;
}

void AddBuildingDialog::attempt_add()
{
	clear_validation();
	const int building_id = ui->buildingIdSpin->value();
	const int gender = ui->genderCombo->currentData().toInt();
	const int max_floor = ui->maxFloorSpin->value();
	if (!check::is_valid_building_id(building_id)) {
		set_building_field_error(ui->buildingIdSpin, ui->buildingIdErrorLabel, true);
		ui->buildingIdSpin->setFocus();
		uifeedback::show_error(this, QStringLiteral("无法新增楼栋"), ui->buildingIdErrorLabel->text());
		return;
	}
	if (!check::is_valid_building_gender(gender)) {
		set_building_field_error(ui->genderCombo, ui->genderErrorLabel, true);
		ui->genderCombo->setFocus();
		uifeedback::show_error(this, QStringLiteral("无法新增楼栋"), ui->genderErrorLabel->text());
		return;
	}
	if (!check::is_valid_max_floor(max_floor)) {
		set_building_field_error(ui->maxFloorSpin, ui->maxFloorErrorLabel, true);
		ui->maxFloorSpin->setFocus();
		uifeedback::show_error(this, QStringLiteral("无法新增楼栋"), ui->maxFloorErrorLabel->text());
		return;
	}

	const int result = school::instance().add_building(building_id, gender, max_floor);
	if (result == 1) {
		added_id = building_id;
		accept();
		return;
	}
	uifeedback::show_error(this, QStringLiteral("无法新增楼栋"), result == 0
		? QStringLiteral("该楼号已经存在，请检查后重新输入。")
		: QStringLiteral("楼栋资料不符合数据规则。"));
}

void AddBuildingDialog::clear_validation()
{
	set_building_field_error(ui->buildingIdSpin, ui->buildingIdErrorLabel, false);
	set_building_field_error(ui->genderCombo, ui->genderErrorLabel, false);
	set_building_field_error(ui->maxFloorSpin, ui->maxFloorErrorLabel, false);
}
