#include "adddormdialog.h"
#include "./ui_adddormdialog.h"

#include "core/building.h"
#include "core/school.h"
#include "system/check.h"
#include "uifeedback.h"

#include <QPushButton>
#include <QRegularExpression>
#include <QRegularExpressionValidator>
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
	ui->floorCombo->setAccessibleName(QStringLiteral("宿舍楼层"));
	ui->roomNumEdit->setAccessibleName(QStringLiteral("宿舍号后两位"));
	ui->roomNumEdit->setValidator(new QRegularExpressionValidator(QRegularExpression(QStringLiteral("[0-9]{1,2}")), ui->roomNumEdit));
	ui->maxNumSpin->setAccessibleName(QStringLiteral("宿舍床位数量"));
	ui->genderLockCombo->setAccessibleName(QStringLiteral("房间性别锁"));
	ui->genderLockCombo->addItem(QStringLiteral("暂不锁定"), 0);
	const building* current_building = school::instance().get_building(building_id);
	if (current_building != nullptr) {
		original_max_floor = current_building->get_max_floor();
		for (int floor = 1; floor <= original_max_floor + 1 && floor <= 99; ++floor) {
			ui->floorCombo->addItem(QStringLiteral("%1层").arg(floor), floor);
		}
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
	connect(ui->floorCombo, &QComboBox::currentIndexChanged, this, [this]() { refresh_floor_selection(); });
	connect(ui->roomNumEdit, &QLineEdit::textChanged, this, [this]() { refresh_dorm_id_preview(); });
	connect(ui->roomNumEdit, &QLineEdit::editingFinished, this, &AddDormDialog::normalize_room_number);
	if (current_building != nullptr) {
		int default_floor_index = 0;
		for (int index = 0; index < ui->floorCombo->count(); ++index) {
			if (school::instance().suggest_dorm_id(building_id, ui->floorCombo->itemData(index).toInt()) > 0) {
				default_floor_index = index;
				break;
			}
		}
		ui->floorCombo->setCurrentIndex(default_floor_index);
		refresh_floor_selection();
	}
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
	const int floor = ui->floorCombo->currentData().toInt();
	const QString room_text = ui->roomNumEdit->text();
	const int room_num = room_text.toInt();
	const int max_num = ui->maxNumSpin->value();
	const int gender_lock = ui->genderLockCombo->currentData().toInt();
	if (!check::is_valid_floor(floor, 99) || floor > current_building->get_max_floor() + 1
		|| room_text.isEmpty() || !check::is_valid_dorm_room_num(room_num)) {
		set_dorm_field_error(ui->roomNumEdit, ui->dormIdErrorLabel, true);
		ui->roomNumEdit->setFocus();
		uifeedback::show_error(this, QStringLiteral("无法新增宿舍"), QStringLiteral("请选择有效楼层，并将宿舍号后两位设置为01～99。"));
		return;
	}
	if (max_num < 1 || max_num > 99) {
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
	const int result = school::instance().add_dorm(target_building_id, floor, room_num, max_num, gender_lock);
	if (result <= 0) {
		if (result == -2) {
			set_dorm_field_error(ui->roomNumEdit, ui->dormIdErrorLabel, true);
			ui->roomNumEdit->setFocus();
		}
		if (result == -6) {
			uifeedback::show_critical(this, QStringLiteral("新增宿舍恢复失败"), QStringLiteral("扩层或新增宿舍失败，且原楼层状态未能完整恢复。请暂停后续操作并核查楼栋与宿舍数据。"));
		} else {
			const QString detail = result == 0 ? QStringLiteral("所选楼栋已经不存在，请刷新后重试。")
				: result == -2 ? QStringLiteral("该宿舍号已被占用。系统没有静默更换编号，请确认新的建议值后再添加。")
				: result == -3 ? QStringLiteral("房间性别锁与当前楼栋规则冲突。")
				: result == -9 ? QStringLiteral("该楼层的01～99号宿舍已全部占用。")
				: result == -5 ? QStringLiteral("新增宿舍未能完成，本次操作造成的状态变化已恢复。")
				: QStringLiteral("宿舍资料不符合楼栋规则，请检查后重试。");
			uifeedback::show_error(this, QStringLiteral("无法新增宿舍"), detail);
		}
		if (result == -2 || result == -9)
			refresh_floor_selection();
		return;
	}
	added_id = result;
	accept();
}

void AddDormDialog::clear_validation()
{
	set_dorm_field_error(ui->roomNumEdit, ui->dormIdErrorLabel, false);
	set_dorm_field_error(ui->maxNumSpin, ui->maxNumErrorLabel, false);
	set_dorm_field_error(ui->genderLockCombo, ui->genderLockErrorLabel, false);
}

void AddDormDialog::refresh_floor_selection()//切换楼层时刷新扩层提示与最小缺号
{
	const int floor = ui->floorCombo->currentData().toInt();
	ui->floorPrefixLabel->setText(QString::number(floor));
	const bool expands_building = floor == original_max_floor + 1;
	ui->floorExpansionHintLabel->setVisible(expands_building);
	ui->floorExpansionHintLabel->setText(QStringLiteral("添加该宿舍后，%1号楼的最高楼层将由%2层调整为%3层。")
		.arg(target_building_id).arg(original_max_floor).arg(floor));
	const int suggested_id = school::instance().suggest_dorm_id(target_building_id, floor);
	const bool available = suggested_id > 0;
	set_dorm_field_error(ui->roomNumEdit, ui->dormIdErrorLabel, false);
	ui->roomNumEdit->setEnabled(available);
	ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(available);
	ui->dormIdErrorLabel->setText(available ? QStringLiteral("后两位必须为01～99，且当前宿舍号未被占用。")
		: suggested_id == -9 ? QStringLiteral("该楼层的宿舍号已全部占用。")
		: suggested_id == 0 ? QStringLiteral("所选楼栋已经不存在，请刷新后重试。")
		: QStringLiteral("楼栋或既有宿舍记录异常，暂时无法生成建议号。"));
	ui->dormIdErrorLabel->setVisible(!available);
	ui->roomNumEdit->setText(available
		? QStringLiteral("%1").arg(check::dorm_id_room_num(suggested_id), 2, 10, QLatin1Char('0')) : QString());
	refresh_dorm_id_preview();
}

void AddDormDialog::refresh_dorm_id_preview()//刷新完整宿舍号预览
{
	const int floor = ui->floorCombo->currentData().toInt();
	const int room_num = ui->roomNumEdit->text().toInt();
	const int dorm_id = check::make_dorm_id(floor, room_num);
	ui->dormIdPreviewLabel->setText(dorm_id > 0
		? QStringLiteral("完整：%1室").arg(dorm_id) : QStringLiteral("完整：待补全"));
}

void AddDormDialog::normalize_room_number()//将有效房间号补齐两位
{
	const int room_num = ui->roomNumEdit->text().toInt();
	if (check::is_valid_dorm_room_num(room_num))
		ui->roomNumEdit->setText(QStringLiteral("%1").arg(room_num, 2, 10, QLatin1Char('0')));
}
