#include "sampledatadialog.h"
#include "./ui_sampledatadialog.h"

#include "core/school.h"
#include "uifeedback.h"

#include <QApplication>
#include <QCheckBox>
#include <QLineEdit>
#include <QPushButton>
#include <QRadioButton>
#include <QRandomGenerator>
#include <QRegularExpression>
#include <QRegularExpressionValidator>
#include <QScreen>
#include <QSet>
#include <QShowEvent>
#include <QSpinBox>
#include <QStyle>

#include <limits>

SampleDataDialog::SampleDataDialog(QWidget* parent)
	: QDialog(parent)
	, ui(new Ui::SampleDataDialog)
{
	ui->setupUi(this);
	ui->replaceInfoButton->set_information(ui->replaceInfoButton->toolTip());
	ui->validationInfoButton->set_information(QStringLiteral("查看全部参数问题。"));
	ui->appendRadio->setAccessibleName(QStringLiteral("追加到当前数据"));
	ui->replaceRadio->setAccessibleName(QStringLiteral("清空现有数据后生成样例数据"));
	ui->maleBuildingSpin->setAccessibleName(QStringLiteral("男生楼数量"));
	ui->femaleBuildingSpin->setAccessibleName(QStringLiteral("女生楼数量"));
	ui->mixedBuildingSpin->setAccessibleName(QStringLiteral("混合宿舍楼数量"));
	ui->floorSpin->setAccessibleName(QStringLiteral("每栋楼层数"));
	ui->dormPerFloorSpin->setAccessibleName(QStringLiteral("每层宿舍数量"));
	ui->fourBedDormSpin->setAccessibleName(QStringLiteral("每栋四人间数量"));
	ui->sixBedDormSpin->setAccessibleName(QStringLiteral("每栋六人间数量"));
	ui->mixedMaleDormSpin->setAccessibleName(QStringLiteral("每栋混合楼男舍数量"));
	ui->mixedFemaleDormSpin->setAccessibleName(QStringLiteral("每栋混合楼女舍数量"));
	ui->mixedUnlockedDormSpin->setAccessibleName(QStringLiteral("每栋混合楼宿舍性别锁未设置的宿舍数量"));
	ui->reservedUnlockedSpin->setAccessibleName(QStringLiteral("全部混合楼合计保留的宿舍性别锁未设置空宿舍数量"));
	ui->maleStudentSpin->setAccessibleName(QStringLiteral("生成男生总数"));
	ui->maleAssignedSpin->setAccessibleName(QStringLiteral("生成男生入住人数"));
	ui->femaleStudentSpin->setAccessibleName(QStringLiteral("生成女生总数"));
	ui->femaleAssignedSpin->setAccessibleName(QStringLiteral("生成女生入住人数"));
	ui->minimumClassSpin->setAccessibleName(QStringLiteral("最小班级号"));
	ui->maximumClassSpin->setAccessibleName(QStringLiteral("最大班级号"));
	ui->minimumGradeSpin->setAccessibleName(QStringLiteral("最小年级"));
	ui->maximumGradeSpin->setAccessibleName(QStringLiteral("最大年级"));
	ui->seedLineEdit->setAccessibleName(QStringLiteral("随机种子"));
	ui->seedLineEdit->setAccessibleDescription(QStringLiteral("相同参数和种子可生成相同的随机规划结果。"));
	ui->regenerateSeedButton->setAccessibleName(QStringLiteral("重新生成随机种子"));
	ui->parameterTabs->setAccessibleName(QStringLiteral("样例数据参数分类"));
	ui->buttonBox->button(QDialogButtonBox::Ok)->setText(QStringLiteral("开始生成"));
	ui->buttonBox->button(QDialogButtonBox::Cancel)->setText(QStringLiteral("取消"));
	ui->seedLineEdit->setValidator(new QRegularExpressionValidator(
		QRegularExpression(QStringLiteral("[0-9]{0,10}")), ui->seedLineEdit));
	const QList<QSpinBox*> parameter_spins = findChildren<QSpinBox*>();
	for (QSpinBox* spin : parameter_spins) {
		connect(spin, &QSpinBox::valueChanged, this, &SampleDataDialog::refresh_preview_and_validation);
	}
	connect(ui->mixedBuildingSpin, &QSpinBox::valueChanged, this, &SampleDataDialog::update_mixed_controls);
	connect(ui->appendRadio, &QRadioButton::toggled, this, &SampleDataDialog::refresh_preview_and_validation);
	connect(ui->replaceRadio, &QRadioButton::toggled, this, &SampleDataDialog::refresh_preview_and_validation);
	connect(ui->replaceRiskCheckBox, &QCheckBox::toggled, this, &SampleDataDialog::refresh_preview_and_validation);
	connect(ui->seedLineEdit, &QLineEdit::textChanged, this, &SampleDataDialog::refresh_preview_and_validation);
	connect(ui->regenerateSeedButton, &QPushButton::clicked, this, &SampleDataDialog::regenerate_seed);
	connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &SampleDataDialog::attempt_generate);
	connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
	QWidget::setTabOrder(ui->appendRadio, ui->maleBuildingSpin);
	QWidget::setTabOrder(ui->maleBuildingSpin, ui->femaleBuildingSpin);
	QWidget::setTabOrder(ui->femaleBuildingSpin, ui->mixedBuildingSpin);
	QWidget::setTabOrder(ui->mixedBuildingSpin, ui->floorSpin);
	QWidget::setTabOrder(ui->floorSpin, ui->dormPerFloorSpin);
	QWidget::setTabOrder(ui->dormPerFloorSpin, ui->fourBedDormSpin);
	QWidget::setTabOrder(ui->fourBedDormSpin, ui->sixBedDormSpin);
	QWidget::setTabOrder(ui->sixBedDormSpin, ui->mixedMaleDormSpin);
	QWidget::setTabOrder(ui->mixedMaleDormSpin, ui->mixedFemaleDormSpin);
	QWidget::setTabOrder(ui->mixedFemaleDormSpin, ui->mixedUnlockedDormSpin);
	QWidget::setTabOrder(ui->mixedUnlockedDormSpin, ui->reservedUnlockedSpin);
	QWidget::setTabOrder(ui->reservedUnlockedSpin, ui->maleStudentSpin);
	QWidget::setTabOrder(ui->maleStudentSpin, ui->maleAssignedSpin);
	QWidget::setTabOrder(ui->maleAssignedSpin, ui->femaleStudentSpin);
	QWidget::setTabOrder(ui->femaleStudentSpin, ui->femaleAssignedSpin);
	QWidget::setTabOrder(ui->femaleAssignedSpin, ui->minimumClassSpin);
	QWidget::setTabOrder(ui->minimumClassSpin, ui->maximumClassSpin);
	QWidget::setTabOrder(ui->maximumClassSpin, ui->minimumGradeSpin);
	QWidget::setTabOrder(ui->minimumGradeSpin, ui->maximumGradeSpin);
	QWidget::setTabOrder(ui->maximumGradeSpin, ui->seedLineEdit);
	QWidget::setTabOrder(ui->seedLineEdit, ui->regenerateSeedButton);
	update_mixed_controls();
	regenerate_seed();
}

SampleDataDialog::~SampleDataDialog()
{
	delete ui;
}

sampledataresult SampleDataDialog::generation_result() const
{
	return generated_result;
}

void SampleDataDialog::showEvent(QShowEvent* event)
{
	QDialog::showEvent(event);
	if (fitted_to_screen) {
		return;
	}
	fitted_to_screen = true;
	QScreen* current_screen = screen();
	if (current_screen == nullptr) {
		return;
	}
	const QRect available = current_screen->availableGeometry();
	const QSize target_size(
		qMin(820, qMax(360, available.width() - 32)),
		qMin(500, qMax(360, available.height() - 32)));
	resize(target_size);
	move(available.center().x() - width() / 2, available.center().y() - height() / 2);
}

sampledataconfig SampleDataDialog::current_config(bool* seed_valid) const
{
	sampledataconfig config;
	config.mode = ui->replaceRadio->isChecked() ? sampledatamode::replace_reserved : sampledatamode::append;
	config.male_building_count = ui->maleBuildingSpin->value();
	config.female_building_count = ui->femaleBuildingSpin->value();
	config.mixed_building_count = ui->mixedBuildingSpin->value();
	config.floors_per_building = ui->floorSpin->value();
	config.dorms_per_floor = ui->dormPerFloorSpin->value();
	config.four_bed_dorm_count = ui->fourBedDormSpin->value();
	config.six_bed_dorm_count = ui->sixBedDormSpin->value();
	config.mixed_male_dorm_count = ui->mixedMaleDormSpin->value();
	config.mixed_female_dorm_count = ui->mixedFemaleDormSpin->value();
	config.mixed_unlocked_dorm_count = ui->mixedUnlockedDormSpin->value();
	config.minimum_unlocked_empty_dorm_count = ui->reservedUnlockedSpin->value();
	if (config.mixed_building_count == 0) {
		config.mixed_male_dorm_count = 0;
		config.mixed_female_dorm_count = 0;
		config.mixed_unlocked_dorm_count = 0;
		config.minimum_unlocked_empty_dorm_count = 0;
	}
	config.male_student_count = ui->maleStudentSpin->value();
	config.female_student_count = ui->femaleStudentSpin->value();
	config.male_assigned_count = ui->maleAssignedSpin->value();
	config.female_assigned_count = ui->femaleAssignedSpin->value();
	config.minimum_class_num = ui->minimumClassSpin->value();
	config.maximum_class_num = ui->maximumClassSpin->value();
	config.minimum_grade = ui->minimumGradeSpin->value();
	config.maximum_grade = ui->maximumGradeSpin->value();

	bool parsed = false;
	const qulonglong seed_value = ui->seedLineEdit->text().toULongLong(&parsed);
	parsed = parsed && seed_value <= std::numeric_limits<quint32>::max();
	config.random_seed = parsed ? static_cast<quint32>(seed_value) : 0;
	if (seed_valid != nullptr) {
		*seed_valid = parsed;
	}
	return config;
}

void SampleDataDialog::update_mixed_controls()
{
	ui->mixedDormGroupBox->setEnabled(ui->mixedBuildingSpin->value() > 0);
}

void SampleDataDialog::refresh_preview_and_validation()
{
	bool seed_valid = false;
	const sampledataconfig config = current_config(&seed_valid);
	const sampledatapreview scale = sampledatagenerator::preview(config);
	ui->capacityHintLabel->setText(QStringLiteral("两类房间之和应等于每栋宿舍总数（当前 %1 间）。")
		.arg(config.floors_per_building * config.dorms_per_floor));
	ui->previewValueLabel->setText(QStringLiteral("%1 栋楼 / %2 间宿舍 / %3 张床 / %4 名学生")
		.arg(scale.building_count).arg(scale.dorm_count).arg(scale.bed_count).arg(scale.student_count));
	ui->previewDetailLabel->setText(QStringLiteral("其中 %1 人入住，保留 %2 名未入住学生和至少 %3 间宿舍性别锁未设置的空宿舍。")
		.arg(scale.assigned_student_count).arg(scale.unassigned_student_count)
		.arg(scale.reserved_unlocked_dorm_count));

	QStringList errors;
	if (!seed_valid) {
		errors.append(QStringLiteral("随机种子必须是0～4294967295之间的整数。"));
	} else {
		errors = sampledatagenerator::validate_config(config, school::instance());
	}
	ui->validationLabel->setVisible(!errors.isEmpty());
	ui->validationLabel->setText(errors.isEmpty()
		? QString()
		: errors.first() + (errors.size() > 1
			? QStringLiteral("（另有%1项，点击说明按钮查看）").arg(errors.size() - 1)
			: QString()));
	ui->validationLabel->setToolTip(errors.join(QLatin1Char('\n')));
	ui->validationInfoButton->setVisible(errors.size() > 1);
	if (errors.size() > 1) {
		ui->validationInfoButton->set_information(errors.join(QLatin1Char('\n')));
	}
	const bool replace_mode = config.mode == sampledatamode::replace_reserved;
	ui->replaceImpactLabel->setVisible(replace_mode);
	ui->replaceRiskCheckBox->setVisible(replace_mode);
	ui->replaceImpactLabel->setText(QStringLiteral("将删除当前 %1 名学生、%2 间宿舍和 %3 栋宿舍楼，再生成新的样例数据。")
		.arg(school::instance().get_student_count()).arg(school::instance().get_dorm_count()).arg(school::instance().get_building_count()));
	ui->buttonBox->button(QDialogButtonBox::Ok)->setText(replace_mode ? QStringLiteral("清空并生成") : QStringLiteral("开始生成"));
	ui->buttonBox->button(QDialogButtonBox::Ok)->setProperty("dangerButton", replace_mode);
	ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(errors.isEmpty() && (!replace_mode || ui->replaceRiskCheckBox->isChecked()));
	ui->buttonBox->button(QDialogButtonBox::Ok)->style()->unpolish(ui->buttonBox->button(QDialogButtonBox::Ok));
	ui->buttonBox->button(QDialogButtonBox::Ok)->style()->polish(ui->buttonBox->button(QDialogButtonBox::Ok));
}

void SampleDataDialog::regenerate_seed()
{
	ui->seedLineEdit->setText(QString::number(QRandomGenerator::global()->generate()));
}

void SampleDataDialog::attempt_generate()
{
	bool seed_valid = false;
	const sampledataconfig config = current_config(&seed_valid);
	const QStringList errors = seed_valid
		? sampledatagenerator::validate_config(config, school::instance())
		: QStringList{QStringLiteral("随机种子必须是0～4294967295之间的整数。")};
	if (!errors.isEmpty()) {
		refresh_preview_and_validation();
		uifeedback::show_error(this, QStringLiteral("无法生成样例数据"), errors.first(), errors.join(QLatin1Char('\n')));
		return;
	}

	const sampledatapreview scale = sampledatagenerator::preview(config);
	if (config.mode == sampledatamode::replace_reserved) {
		if (!ui->replaceRiskCheckBox->isChecked()) {
			uifeedback::show_error(this, QStringLiteral("请确认数据替换风险"), QStringLiteral("请先勾选“我已了解现有数据将被全部删除并替换”。"));
			return;
		}
		QString plan_error;
		const sampledataplan plan = sampledatagenerator::create_replace_plan(config, school::instance(), &plan_error);
		if (!plan_error.isEmpty()) {
			uifeedback::show_error(this, QStringLiteral("无法生成样例数据计划"), plan_error);
			return;
		}
		QApplication::setOverrideCursor(Qt::WaitCursor);
		const int result = school::instance().replace_all_with_sample_data(plan);
		QApplication::restoreOverrideCursor();
		if (result == 1) {
			generated_result.success = true;
			generated_result.replaced_existing_data = true;
			generated_result.random_seed = config.random_seed;
			generated_result.added_building_count = scale.building_count;
			generated_result.added_dorm_count = scale.dorm_count;
			generated_result.added_student_count = scale.student_count;
			generated_result.assigned_student_count = scale.assigned_student_count;
			QSet<QString> occupied_dorms;
			for (const samplestudentplan& student_plan : plan.students)
				if (student_plan.building_id > 0) occupied_dorms.insert(QStringLiteral("%1/%2").arg(student_plan.building_id).arg(student_plan.dorm_id));
			for (const sampledormplan& dorm_plan : plan.dorms)
				if (dorm_plan.gender_lock == 0 && !occupied_dorms.contains(QStringLiteral("%1/%2").arg(dorm_plan.building_id).arg(dorm_plan.dorm_id)))
					++generated_result.remaining_unlocked_dorm_count;
			accept();
			return;
		}
		if (result == -6) uifeedback::show_critical(this, QStringLiteral("原数据恢复不完整"), QStringLiteral("样例数据生成失败，且未能完整恢复原数据。请暂停后续操作并核查学生、宿舍和床位。"));
		else uifeedback::show_error(this, QStringLiteral("样例数据替换失败"), result == -8 ? QStringLiteral("现有住宿数据存在不一致，请先修复后再重试。") : QStringLiteral("新数据未能完整写入，原数据已恢复。"), QStringLiteral("业务返回值：%1").arg(result));
		refresh_preview_and_validation();
		return;
	}
	const QString confirmation = QStringLiteral(
		"将向当前系统追加 %1 栋楼、%2 间宿舍和 %3 名学生，其中 %4 人将被安排入住。\n\n"
		"现有数据不会被删除，随机种子为 %5。")
		.arg(scale.building_count).arg(scale.dorm_count).arg(scale.student_count)
		.arg(scale.assigned_student_count).arg(config.random_seed);
	if (!uifeedback::confirm_action(this, QStringLiteral("确认生成样例数据"), confirmation, QStringLiteral("开始生成"))) {
		return;
	}

	QPushButton* generate_button = ui->buttonBox->button(QDialogButtonBox::Ok);
	generate_button->setEnabled(false);
	QApplication::setOverrideCursor(Qt::WaitCursor);
	generated_result = sampledatagenerator::generate(config, school::instance());
	QApplication::restoreOverrideCursor();
	if (!generated_result.success) {
		const QString rollback_summary = generated_result.rollback_complete
			? QStringLiteral("本次已写入的数据已全部撤销，原有数据未被修改。")
			: QStringLiteral("补偿恢复不完整，仍残留 %1 栋楼、%2 间宿舍、%3 名学生，请暂停继续操作并核对数据。")
				.arg(generated_result.residual_building_count)
				.arg(generated_result.residual_dorm_count)
				.arg(generated_result.residual_student_count);
		if (generated_result.rollback_complete) {
			uifeedback::show_error(this, QStringLiteral("样例数据生成失败"), generated_result.error_message, rollback_summary);
		} else {
			uifeedback::show_critical(this, QStringLiteral("样例数据恢复不完整"), rollback_summary, generated_result.error_message);
		}
		refresh_preview_and_validation();
		return;
	}
	accept();
}
