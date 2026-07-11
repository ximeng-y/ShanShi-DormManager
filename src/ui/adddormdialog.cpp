#include "adddormdialog.h"
#include "./ui_adddormdialog.h"

#include "core/building.h"
#include "core/dorm.h"
#include "core/school.h"
#include "system/check.h"
#include "uifeedback.h"

#include <QPushButton>
#include <QStyle>

namespace {
void set_dorm_field_error(QWidget* field, QLabel* error_label, bool has_error)
{
	field->setProperty("inputError", has_error);
	field->style()->unpolish(field);
	field->style()->polish(field);
	error_label->setVisible(has_error);
}
}

AddDormDialog::AddDormDialog(int building_id, QWidget* parent)
	: QDialog(parent)
	, ui(new Ui::AddDormDialog)
	, target_building_id(building_id)
{
	ui->setupUi(this);
	ui->buildingValueLabel->setText(QStringLiteral("%1号楼").arg(building_id));
	ui->genderLockCombo->addItem(QStringLiteral("暂不锁定"), 0);
	const building* current_building = school::instance().get_building(building_id);
	if (current_building != nullptr) {
		if (current_building->accepts_gender(1)) {
			ui->genderLockCombo->addItem(QStringLiteral("男舍"), 1);
		}
		if (current_building->accepts_gender(2)) {
			ui->genderLockCombo->addItem(QStringLiteral("女舍"), 2);
		}
	} else {
		ui->buildingValueLabel->setText(QStringLiteral("楼栋不存在"));
	}
	ui->buttonBox->button(QDialogButtonBox::Ok)->setText(QStringLiteral("添加"));
	ui->buttonBox->button(QDialogButtonBox::Cancel)->setText(QStringLiteral("取消"));
	ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(current_building != nullptr);
	connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &AddDormDialog::attempt_add);
	connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

AddDormDialog::~AddDormDialog()
{
	delete ui;
}

int AddDormDialog::added_dorm_id() const
{
	return added_id;
}

void AddDormDialog::attempt_add()
{
	clear_validation();
	const building* current_building = school::instance().get_building(target_building_id);
	if (current_building == nullptr) {
		uifeedback::show_error(this, QStringLiteral("无法新增宿舍"), QStringLiteral("所选楼栋已经不存在，请刷新后重试。"));
		return;
	}
	const int dorm_id = ui->dormIdSpin->value();
	const int max_num = ui->maxNumSpin->value();
	const int gender_lock = ui->genderLockCombo->currentData().toInt();
	if (!check::is_valid_dorm_id(dorm_id) || !check::is_valid_dorm_floor(dorm_id, current_building->get_max_floor())) {
		set_dorm_field_error(ui->dormIdSpin, ui->dormIdErrorLabel, true);
		ui->dormIdSpin->setFocus();
		uifeedback::show_error(this, QStringLiteral("无法新增宿舍"), QStringLiteral("宿舍号无效，或宿舍号对应楼层超过该楼最大楼层。"));
		return;
	}
	if (max_num < 1) {
		set_dorm_field_error(ui->maxNumSpin, ui->maxNumErrorLabel, true);
		ui->maxNumSpin->setFocus();
		uifeedback::show_error(this, QStringLiteral("无法新增宿舍"), ui->maxNumErrorLabel->text());
		return;
	}
	if (gender_lock != 0 && !current_building->accepts_gender(gender_lock)) {
		set_dorm_field_error(ui->genderLockCombo, ui->genderLockErrorLabel, true);
		ui->genderLockCombo->setFocus();
		uifeedback::show_error(this, QStringLiteral("无法新增宿舍"), ui->genderLockErrorLabel->text());
		return;
	}
	if (school::instance().get_dorm(target_building_id, dorm_id) != nullptr) {
		set_dorm_field_error(ui->dormIdSpin, ui->dormIdErrorLabel, true);
		uifeedback::show_error(this, QStringLiteral("无法新增宿舍"), QStringLiteral("该楼栋中已经存在相同宿舍号。"));
		return;
	}

	dorm dorm_to_add;
	const bool initialized = dorm_to_add.set_building_id(target_building_id)
		&& dorm_to_add.set_id(dorm_id)
		&& dorm_to_add.set_max_num(max_num);
	if (!initialized || !school::instance().add_dorm(dorm_to_add)) {
		uifeedback::show_error(this, QStringLiteral("无法新增宿舍"), QStringLiteral("宿舍资料与楼栋规则冲突，请检查后重试。"));
		return;
	}
	if (gender_lock != 0 && school::instance().set_dorm_gender(target_building_id, dorm_id, gender_lock) != 1) {
		const bool rolled_back = school::instance().remove_dorm(target_building_id, dorm_id);
		if (!rolled_back) {
			uifeedback::show_critical(this, QStringLiteral("新增宿舍回滚失败"), QStringLiteral("宿舍已添加，但性别锁设置失败且新宿舍未能删除。请暂停后续操作并核查数据。"));
		} else {
			uifeedback::show_error(this, QStringLiteral("无法新增宿舍"), QStringLiteral("房间性别锁与楼栋规则冲突，新宿舍已撤销。"));
		}
		return;
	}
	added_id = dorm_id;
	accept();
}

void AddDormDialog::clear_validation()
{
	set_dorm_field_error(ui->dormIdSpin, ui->dormIdErrorLabel, false);
	set_dorm_field_error(ui->maxNumSpin, ui->maxNumErrorLabel, false);
	set_dorm_field_error(ui->genderLockCombo, ui->genderLockErrorLabel, false);
}
