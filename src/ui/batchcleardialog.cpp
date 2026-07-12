#include "batchcleardialog.h"
#include "./ui_batchcleardialog.h"

#include "core/building.h"
#include "core/dorm.h"
#include "core/student.h"
#include "uifeedback.h"

#include <QApplication>
#include <QButtonGroup>
#include <QCheckBox>
#include <QClipboard>
#include <QComboBox>
#include <QHeaderView>
#include <QPushButton>
#include <QRadioButton>
#include <QScreen>
#include <QShowEvent>
#include <QStyle>
#include <QTableWidget>

#include <QStringList>

BatchClearDialog::BatchClearDialog(QWidget* parent)
	: QDialog(parent)
	, ui(new Ui::BatchClearDialog)
{
	ui->setupUi(this);
	auto* scope_group = new QButtonGroup(this);
	scope_group->addButton(ui->dormScopeRadio);
	scope_group->addButton(ui->buildingScopeRadio);
	scope_group->addButton(ui->allScopeRadio);
	auto* gender_group = new QButtonGroup(this);
	gender_group->addButton(ui->keepGenderRadio);
	gender_group->addButton(ui->resetGenderRadio);
	//这些单选按钮在 .ui 中共享父控件，建立独立分组后重新明确各组默认项。
	ui->dormScopeRadio->setChecked(true);
	ui->keepGenderRadio->setChecked(true);
	ui->genderLockInfoButton->set_information(ui->genderLockInfoButton->toolTip());
	ui->scopeInfoButton->set_information(ui->scopeInfoButton->toolTip());
	ui->studentTable->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
	ui->studentTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
	ui->dormTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
	ui->issueTable->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
	ui->issueTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Stretch);

	ui->buildingCombo->setAccessibleName(QStringLiteral("清退范围楼栋"));
	ui->dormCombo->setAccessibleName(QStringLiteral("清退范围宿舍"));
	ui->keepGenderRadio->setAccessibleName(QStringLiteral("保留宿舍性别锁"));
	ui->resetGenderRadio->setAccessibleName(QStringLiteral("解除宿舍性别锁"));
	ui->riskCheckBox->setAccessibleDescription(QStringLiteral("指定楼栋或全部宿舍清退前必须勾选。"));

	connect(ui->dormScopeRadio, &QRadioButton::toggled, this, &BatchClearDialog::refresh_scope_controls);
	connect(ui->buildingScopeRadio, &QRadioButton::toggled, this, &BatchClearDialog::refresh_scope_controls);
	connect(ui->allScopeRadio, &QRadioButton::toggled, this, &BatchClearDialog::refresh_scope_controls);
	connect(ui->buildingCombo, &QComboBox::currentIndexChanged, this, &BatchClearDialog::populate_dorms);
	connect(ui->dormCombo, &QComboBox::currentIndexChanged, this, &BatchClearDialog::refresh_summary);
	connect(ui->keepGenderRadio, &QRadioButton::toggled, this, &BatchClearDialog::refresh_summary);
	connect(ui->previewButton, &QPushButton::clicked, this, &BatchClearDialog::generate_preview);
	connect(ui->cancelButton, &QPushButton::clicked, this, &QDialog::reject);
	connect(ui->backButton, &QPushButton::clicked, this, &BatchClearDialog::return_to_selection);
	connect(ui->executeButton, &QPushButton::clicked, this, &BatchClearDialog::apply_preview);
	connect(ui->closeButton, &QPushButton::clicked, this, &QDialog::reject);
	connect(ui->riskCheckBox, &QCheckBox::toggled, this, &BatchClearDialog::update_execute_state);
	connect(ui->copyIssuesButton, &QPushButton::clicked, this, &BatchClearDialog::copy_issues);

	populate_buildings();
	refresh_scope_controls();
}

BatchClearDialog::~BatchClearDialog()
{
	delete ui;
}

void BatchClearDialog::set_initial_dorm(int building_id, int dorm_id)
{
	ui->dormScopeRadio->setChecked(true);
	const int building_index = ui->buildingCombo->findData(building_id);
	if (building_index >= 0) {
		ui->buildingCombo->setCurrentIndex(building_index);
	}
	const int dorm_index = ui->dormCombo->findData(dorm_id);
	if (dorm_index >= 0) {
		ui->dormCombo->setCurrentIndex(dorm_index);
	}
	refresh_scope_controls();
}

void BatchClearDialog::showEvent(QShowEvent* event)
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
		qMin(900, qMax(680, available.width() - 32)),
		qMin(650, qMax(500, available.height() - 32)));
	resize(target_size);
	move(available.center().x() - width() / 2, available.center().y() - height() / 2);
}

clear_scope BatchClearDialog::selected_scope() const
{
	if (ui->buildingScopeRadio->isChecked()) {
		return clear_scope::building;
	}
	if (ui->allScopeRadio->isChecked()) {
		return clear_scope::all;
	}
	return clear_scope::dorm;
}

bool BatchClearDialog::reset_gender() const
{
	return ui->resetGenderRadio->isChecked();
}

void BatchClearDialog::populate_buildings()
{
	const int previous_id = ui->buildingCombo->currentData().toInt();
	ui->buildingCombo->blockSignals(true);
	ui->buildingCombo->clear();
	for (int building_id : school::instance().get_all_building_ids()) {
		ui->buildingCombo->addItem(QStringLiteral("%1号楼").arg(building_id), building_id);
	}
	const int previous_index = ui->buildingCombo->findData(previous_id);
	if (previous_index >= 0) {
		ui->buildingCombo->setCurrentIndex(previous_index);
	}
	ui->buildingCombo->blockSignals(false);
	populate_dorms();
}

void BatchClearDialog::populate_dorms()
{
	const int previous_id = ui->dormCombo->currentData().toInt();
	const int building_id = ui->buildingCombo->currentData().toInt();
	ui->dormCombo->blockSignals(true);
	ui->dormCombo->clear();
	for (const auto& key : school::instance().get_dorm_keys_of_building(building_id)) {
		ui->dormCombo->addItem(QStringLiteral("%1室").arg(key.second), key.second);
	}
	const int previous_index = ui->dormCombo->findData(previous_id);
	if (previous_index >= 0) {
		ui->dormCombo->setCurrentIndex(previous_index);
	}
	ui->dormCombo->blockSignals(false);
	refresh_summary();
}

void BatchClearDialog::refresh_scope_controls()
{
	preview_ready = false;
	const clear_scope scope = selected_scope();
	ui->buildingLabel->setVisible(scope != clear_scope::all);
	ui->buildingCombo->setVisible(scope != clear_scope::all);
	ui->dormLabel->setVisible(scope == clear_scope::dorm);
	ui->dormCombo->setVisible(scope == clear_scope::dorm);
	refresh_summary();
}

void BatchClearDialog::refresh_summary()
{
	const clear_scope scope = selected_scope();
	const int building_id = ui->buildingCombo->currentData().toInt();
	const int dorm_id = ui->dormCombo->currentData().toInt();
	const batch_clear_preview preview = school::instance().preview_clear_dorms(scope, building_id, dorm_id, reset_gender());

	ui->summaryScopeValue->setText(scope_text());
	ui->summaryDormValue->setText(QStringLiteral("%1 间").arg(preview.dorm_changes.size()));
	ui->summaryStudentValue->setText(QStringLiteral("%1 人").arg(preview.changes.size()));
	ui->summaryGenderValue->setText(reset_gender()
		? QStringLiteral("清退后解除宿舍性别锁")
		: QStringLiteral("保留当前宿舍性别锁"));
	ui->summaryIssueLabel->setVisible(!preview.issues.isEmpty());
	ui->summaryIssueLabel->setText(preview.issues.isEmpty()
		? QString()
		: QStringLiteral("发现 %1 条住宿信息异常，生成预览后可查看详情。").arg(preview.issues.size()));
	ui->emptyHintLabel->setVisible(preview.issues.isEmpty() && preview.changes.isEmpty());

	const bool target_available = scope == clear_scope::all
		|| (scope == clear_scope::building && ui->buildingCombo->currentIndex() >= 0)
		|| (scope == clear_scope::dorm && ui->buildingCombo->currentIndex() >= 0 && ui->dormCombo->currentIndex() >= 0);
	ui->previewButton->setEnabled(target_available);
}

void BatchClearDialog::generate_preview()
{
	current_preview = school::instance().preview_clear_dorms(
		selected_scope(), ui->buildingCombo->currentData().toInt(), ui->dormCombo->currentData().toInt(), reset_gender());
	preview_ready = true;
	show_preview();
	ui->pages->setCurrentWidget(ui->previewPage);
}

void BatchClearDialog::show_preview()
{
	ui->previewScopeValue->setText(scope_text());
	ui->previewStudentValue->setText(QStringLiteral("%1 人").arg(current_preview.changes.size()));
	ui->previewDormValue->setText(QStringLiteral("%1 间").arg(current_preview.dorm_changes.size()));
	ui->previewGenderValue->setText(current_preview.reset_gender
		? QStringLiteral("清退后解除宿舍性别锁")
		: QStringLiteral("保留当前宿舍性别锁"));

	ui->studentTable->setRowCount(current_preview.changes.size());
	for (int row = 0; row < current_preview.changes.size(); ++row) {
		const accommodation_change& change = current_preview.changes.at(row);
		const student* s = school::instance().get_student(change.student_id);
		const QString name = s == nullptr ? QStringLiteral("学生不存在") : s->get_name();
		const QString position = QStringLiteral("%1号楼 %2室 %3床")
			.arg(change.old_building_id).arg(change.old_dorm_id).arg(change.old_bed_id);
		ui->studentTable->setItem(row, 0, new QTableWidgetItem(QString::number(change.student_id)));
		ui->studentTable->setItem(row, 1, new QTableWidgetItem(name));
		ui->studentTable->setItem(row, 2, new QTableWidgetItem(position));
		ui->studentTable->setItem(row, 3, new QTableWidgetItem(QStringLiteral("未入住（保留学生档案）")));
	}

	ui->dormTable->setRowCount(current_preview.dorm_changes.size());
	for (int row = 0; row < current_preview.dorm_changes.size(); ++row) {
		const dorm_gender_change& change = current_preview.dorm_changes.at(row);
		ui->dormTable->setItem(row, 0, new QTableWidgetItem(QStringLiteral("%1号楼").arg(change.building_id)));
		ui->dormTable->setItem(row, 1, new QTableWidgetItem(QStringLiteral("%1室").arg(change.dorm_id)));
		ui->dormTable->setItem(row, 2, new QTableWidgetItem(gender_text(change.old_gender)));
		ui->dormTable->setItem(row, 3, new QTableWidgetItem(gender_text(change.new_gender)));
	}

	ui->issueTable->setRowCount(current_preview.issues.size());
	for (int row = 0; row < current_preview.issues.size(); ++row) {
		const accommodation_data_issue& issue = current_preview.issues.at(row);
		ui->issueTable->setItem(row, 0, new QTableWidgetItem(issue.student_id > 0 ? QString::number(issue.student_id) : QStringLiteral("—")));
		ui->issueTable->setItem(row, 1, new QTableWidgetItem(issue.building_id > 0 ? QStringLiteral("%1号楼").arg(issue.building_id) : QStringLiteral("—")));
		ui->issueTable->setItem(row, 2, new QTableWidgetItem(issue.dorm_id > 0 ? QStringLiteral("%1室").arg(issue.dorm_id) : QStringLiteral("—")));
		ui->issueTable->setItem(row, 3, new QTableWidgetItem(issue.message));
	}

	const bool has_issues = !current_preview.issues.isEmpty();
	ui->previewStatusLabel->setProperty("fieldError", has_issues);
	ui->previewStatusLabel->style()->unpolish(ui->previewStatusLabel);
	ui->previewStatusLabel->style()->polish(ui->previewStatusLabel);
	if (has_issues) {
		ui->previewStatusLabel->setText(QStringLiteral("无法执行：发现 %1 条住宿信息异常，请先处理后重新预览。").arg(current_preview.issues.size()));
		ui->previewTabs->setCurrentWidget(ui->issueTab);
	} else if (current_preview.changes.isEmpty()) {
		ui->previewStatusLabel->setText(QStringLiteral("当前范围没有住客，无需执行清退。"));
		ui->previewTabs->setCurrentWidget(ui->studentTab);
	} else {
		ui->previewStatusLabel->setText(QStringLiteral("预检通过：本次只清除住宿关系，不会删除学生档案。"));
		ui->previewTabs->setCurrentWidget(ui->studentTab);
	}
	ui->issueTab->setEnabled(has_issues);
	ui->copyIssuesButton->setVisible(has_issues);

	const bool needs_risk_confirmation = current_preview.scope != clear_scope::dorm;
	ui->riskCheckBox->setVisible(needs_risk_confirmation);
	ui->riskCheckBox->setChecked(false);
	ui->riskCheckBox->setText(current_preview.scope == clear_scope::all
		? QStringLiteral("我已了解全部住客将变为未入住，学生档案仍会保留")
		: QStringLiteral("我已了解该宿舍楼内全部住客将变为未入住，学生档案仍会保留"));
	ui->executeButton->setProperty("dangerButton", current_preview.scope == clear_scope::all);
	ui->executeButton->setProperty("primaryButton", current_preview.scope != clear_scope::all);
	ui->executeButton->style()->unpolish(ui->executeButton);
	ui->executeButton->style()->polish(ui->executeButton);
	update_execute_state();
}

void BatchClearDialog::update_execute_state()
{
	const bool needs_risk_confirmation = current_preview.scope != clear_scope::dorm;
	ui->executeButton->setEnabled(preview_ready && current_preview.can_apply()
		&& (!needs_risk_confirmation || ui->riskCheckBox->isChecked()));
}

void BatchClearDialog::return_to_selection()
{
	preview_ready = false;
	ui->pages->setCurrentWidget(ui->selectionPage);
	refresh_summary();
}

void BatchClearDialog::apply_preview()
{
	if (!preview_ready || !current_preview.can_apply()) {
		return;
	}
	const int result = school::instance().apply_batch_clear(current_preview);
	if (result >= 0) {
		uifeedback::show_information(this, QStringLiteral("批量清退完成"),
			QStringLiteral("已清退 %1 名住客，学生档案继续保留。\n%2")
				.arg(result)
				.arg(current_preview.reset_gender
					? QStringLiteral("受影响宿舍的宿舍性别锁已解除。")
					: QStringLiteral("受影响宿舍的宿舍性别锁已保留。")));
		accept();
		return;
	}

	preview_ready = false;
	update_execute_state();
	if (result == -6) {
		uifeedback::show_critical(this, QStringLiteral("清退恢复不完整"),
			QStringLiteral("批量清退失败，且未能完整恢复操作前的住宿数据。请暂停后续操作并核对学生与床位信息。"));
	} else if (result == -5) {
		uifeedback::show_error(this, QStringLiteral("批量清退失败"),
			QStringLiteral("执行过程中发生错误，操作前的住宿数据已恢复。"));
	} else if (result == -7) {
		uifeedback::show_error(this, QStringLiteral("清退预览已失效"),
			QStringLiteral("学生、床位或宿舍性别锁已发生变化，请返回后重新生成预览。"));
	} else if (result == -8) {
		uifeedback::show_critical(this, QStringLiteral("住宿信息异常"),
			QStringLiteral("检测到学生住宿信息与实际床位不一致，无法执行批量清退。请先核对异常记录。"));
	} else {
		uifeedback::show_error(this, QStringLiteral("无法完成批量清退"),
			QStringLiteral("本次操作未完成，请返回后重新生成预览。"), QStringLiteral("业务返回值：%1").arg(result));
	}
}

void BatchClearDialog::copy_issues()
{
	QStringList lines;
	for (const accommodation_data_issue& issue : current_preview.issues) {
		QString target;
		if (issue.student_id > 0) {
			target += QStringLiteral("学生 %1").arg(issue.student_id);
		}
		if (issue.building_id > 0) {
			if (!target.isEmpty()) {
				target += QStringLiteral("，");
			}
			target += issue.dorm_id > 0
				? QStringLiteral("%1号楼 %2室").arg(issue.building_id).arg(issue.dorm_id)
				: QStringLiteral("%1号楼").arg(issue.building_id);
		}
		lines.append(target.isEmpty() ? issue.message : target + QStringLiteral("：") + issue.message);
	}
	QApplication::clipboard()->setText(lines.join(QLatin1Char('\n')));
	uifeedback::show_success(this, QStringLiteral("异常信息已复制"));
}

QString BatchClearDialog::gender_text(int gender) const
{
	switch (gender) {
	case 1:
		return QStringLiteral("男生宿舍");
	case 2:
		return QStringLiteral("女生宿舍");
	default:
		return QStringLiteral("未设置宿舍性别锁");
	}
}

QString BatchClearDialog::scope_text() const
{
	const clear_scope scope = selected_scope();
	if (scope == clear_scope::all) {
		return QStringLiteral("全部宿舍");
	}
	const int building_id = ui->buildingCombo->currentData().toInt();
	if (scope == clear_scope::building) {
		return ui->buildingCombo->currentIndex() >= 0
			? QStringLiteral("%1号楼全部宿舍").arg(building_id)
			: QStringLiteral("尚未选择宿舍楼");
	}
	return ui->buildingCombo->currentIndex() >= 0 && ui->dormCombo->currentIndex() >= 0
		? QStringLiteral("%1号楼 %2室").arg(building_id).arg(ui->dormCombo->currentData().toInt())
		: QStringLiteral("尚未选择宿舍");
}
