#include "accommodationpage.h"
#include "./ui_accommodationpage.h"

#include "core/dorm.h"
#include "core/school.h"
#include "core/student.h"
#include "system/check.h"
#include "uifeedback.h"

namespace {
QString accommodation_student_text(const student& current_student)
{
	return QStringLiteral("%1（%2）· %3级 · %4班")
		.arg(current_student.get_name())
		.arg(current_student.get_id())
		.arg(current_student.get_grade())
		.arg(current_student.get_class_num());
}

QString accommodation_position_text(const student& current_student)
{
	return QStringLiteral("%1号楼 · %2室 · %3号床")
		.arg(current_student.get_building_id())
		.arg(current_student.get_dorm_id())
		.arg(current_student.get_bed_id());
}
}

AccommodationPage::AccommodationPage(QWidget* parent)
	: QWidget(parent)
	, ui(new Ui::AccommodationPage)
{
	ui->setupUi(this);
	ui->assignSubmitButton->setEnabled(true);
	connect(ui->assignStrategyCombo, &QComboBox::currentIndexChanged, this, [this]() {
		update_assign_controls();
		update_assign_preview();
	});
	connect(ui->assignStudentSpin, &QSpinBox::valueChanged, this, [this]() { update_assign_preview(); });
	connect(ui->assignBuildingSpin, &QSpinBox::valueChanged, this, [this]() { update_assign_preview(); });
	connect(ui->assignDormSpin, &QSpinBox::valueChanged, this, [this]() { update_assign_preview(); });
	connect(ui->assignBedSpin, &QSpinBox::valueChanged, this, [this]() { update_assign_preview(); });
	connect(ui->assignSubmitButton, &QPushButton::clicked, this, &AccommodationPage::submit_assignment);
	update_assign_controls();
	update_assign_preview();
}

AccommodationPage::~AccommodationPage()
{
	delete ui;
}

void AccommodationPage::update_assign_controls()
{
	const int strategy = ui->assignStrategyCombo->currentIndex();
	const bool specified_dorm = strategy >= 2;
	const bool specified_bed = strategy == 3;
	ui->assignBuildingSpin->setEnabled(specified_dorm);
	ui->assignDormSpin->setEnabled(specified_dorm);
	ui->assignBedSpin->setEnabled(specified_bed);
	if (!specified_dorm) {
		ui->assignBuildingSpin->setValue(0);
		ui->assignDormSpin->setValue(100);
	}
	if (!specified_bed) {
		ui->assignBedSpin->setValue(0);
	}
}

void AccommodationPage::update_assign_preview()
{
	const int student_id = ui->assignStudentSpin->value();
	const student* current_student = school::instance().get_student(student_id);
	if (!check::is_valid_student_id(student_id) || current_student == nullptr) {
		ui->assignStudentPreviewLabel->setText(QStringLiteral("请输入已有学生的8位学号。"));
	} else if (school::instance().get_assigned_student_ids().contains(student_id)) {
		ui->assignStudentPreviewLabel->setText(QStringLiteral("%1\n当前已入住：%2")
			.arg(accommodation_student_text(*current_student), accommodation_position_text(*current_student)));
	} else {
		ui->assignStudentPreviewLabel->setText(QStringLiteral("%1\n当前未入住，可办理入住。")
			.arg(accommodation_student_text(*current_student)));
	}

	const int strategy = ui->assignStrategyCombo->currentIndex();
	if (strategy == 0 && current_student != nullptr && current_student->get_gender() >= 1 && current_student->get_gender() <= 2) {
		const dorm* target = school::instance().get_available_dorm(current_student->get_gender());
		ui->assignTargetPreviewLabel->setText(target == nullptr
			? QStringLiteral("当前没有接纳该性别的可用宿舍。")
			: QStringLiteral("系统推荐：%1号楼 · %2室 · 自动选择最小空床位。")
				.arg(target->get_building_id()).arg(target->get_id()));
		return;
	}
	if (strategy == 1) {
		ui->assignTargetPreviewLabel->setText(QStringLiteral("系统将在所有接纳该学生性别的可用宿舍中随机选择。"));
		return;
	}
	if (strategy >= 2) {
		const dorm* target = school::instance().get_dorm(ui->assignBuildingSpin->value(), ui->assignDormSpin->value());
		if (target == nullptr) {
			ui->assignTargetPreviewLabel->setText(QStringLiteral("目标宿舍不存在，请检查楼号和宿舍号。"));
			ui->assignBedSpin->setMaximum(2147483647);
			return;
		}
		ui->assignBedSpin->setMaximum(qMax(1, target->get_max_num()));
		ui->assignTargetPreviewLabel->setText(strategy == 3
			? QStringLiteral("目标：%1号楼 · %2室 · %3号床。").arg(target->get_building_id()).arg(target->get_id()).arg(ui->assignBedSpin->value())
			: QStringLiteral("目标：%1号楼 · %2室 · 自动选择最小空床位。").arg(target->get_building_id()).arg(target->get_id()));
	}
}

void AccommodationPage::submit_assignment()
{
	const int student_id = ui->assignStudentSpin->value();
	const student* current_student = school::instance().get_student(student_id);
	if (!check::is_valid_student_id(student_id) || current_student == nullptr) {
		uifeedback::show_error(this, QStringLiteral("无法办理入住"), QStringLiteral("学生学号无效或学生不存在。"));
		return;
	}
	if (school::instance().get_assigned_student_ids().contains(student_id)) {
		uifeedback::show_error(this, QStringLiteral("无法办理入住"), QStringLiteral("该学生已经入住，请使用调宿或换床流程。"));
		return;
	}
	if (current_student->get_gender() < 1 || current_student->get_gender() > 2) {
		uifeedback::show_error(this, QStringLiteral("无法办理入住"), QStringLiteral("该学生尚未设置有效性别，请先在学生管理中纠正。"));
		return;
	}
	const int strategy = ui->assignStrategyCombo->currentIndex();
	QString target_description;
	if (strategy >= 2) {
		const dorm* target = school::instance().get_dorm(ui->assignBuildingSpin->value(), ui->assignDormSpin->value());
		if (target == nullptr) {
			uifeedback::show_error(this, QStringLiteral("无法办理入住"), QStringLiteral("目标宿舍不存在。"));
			return;
		}
		target_description = strategy == 3
			? QStringLiteral("%1号楼 %2室 %3号床").arg(target->get_building_id()).arg(target->get_id()).arg(ui->assignBedSpin->value())
			: QStringLiteral("%1号楼 %2室的最小空床位").arg(target->get_building_id()).arg(target->get_id());
	} else {
		target_description = strategy == 0 ? QStringLiteral("系统推荐的最小顺位可用宿舍") : QStringLiteral("系统随机选择的可用宿舍");
	}
	if (!uifeedback::confirm_action(this, QStringLiteral("确认办理入住"),
		QStringLiteral("%1\n\n将入住：%2").arg(accommodation_student_text(*current_student), target_description), QStringLiteral("确认入住"))) {
		return;
	}

	int result = 0;
	if (strategy == 0) {
		result = school::instance().assign_student_to_available_dorm(student_id);
	} else if (strategy == 1) {
		result = school::instance().assign_student_to_available_dorm_random(student_id);
	} else if (strategy == 2) {
		result = school::instance().assign_student_to_dorm(ui->assignBuildingSpin->value(), ui->assignDormSpin->value(), student_id);
	} else {
		result = school::instance().assign_student_to_dorm(ui->assignBuildingSpin->value(), ui->assignDormSpin->value(), student_id, ui->assignBedSpin->value());
	}
	if (result <= 0) {
		show_assignment_error(result);
		update_assign_preview();
		return;
	}
	const student* assigned_student = school::instance().get_student(student_id);
	const QString result_position = assigned_student == nullptr ? QStringLiteral("床位号 %1").arg(result) : accommodation_position_text(*assigned_student);
	uifeedback::show_success(this, QStringLiteral("入住办理成功：%1").arg(result_position));
	update_assign_preview();
}

void AccommodationPage::show_assignment_error(int result)
{
	QString message;
	if (result == -2) message = QStringLiteral("目标床位已经被占用。");
	else if (result == -3) message = QStringLiteral("该学生已经入住。");
	else if (result == -4) message = QStringLiteral("目标宿舍已经住满。");
	else if (result == -5) message = QStringLiteral("目标宿舍尚未完成有效配置。");
	else if (result == -6) message = QStringLiteral("学生不存在或尚未设置有效性别。");
	else if (result == -7) message = QStringLiteral("目标楼栋或宿舍不接纳该学生性别。");
	else if (result == -8) message = QStringLiteral("目标宿舍不存在。");
	else if (result == -9) message = QStringLiteral("当前没有可用宿舍。");
	else message = QStringLiteral("输入参数无效，请检查学生、楼栋、宿舍和床位。");
	uifeedback::show_error(this, QStringLiteral("无法办理入住"), message, QStringLiteral("业务返回值：%1").arg(result));
}
