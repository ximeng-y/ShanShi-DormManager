#include "batchassignmentdialog.h"
#include "./ui_batchassignmentdialog.h"

#include "core/student.h"
#include "studentdetaildialog.h"
#include "uifeedback.h"

#include <QApplication>
#include <QButtonGroup>
#include <QCheckBox>
#include <QClipboard>
#include <QHeaderView>
#include <QKeySequence>
#include <QLineEdit>
#include <QPushButton>
#include <QRandomGenerator>
#include <QRadioButton>
#include <QRegularExpression>
#include <QRegularExpressionValidator>
#include <QScreen>
#include <QShortcut>
#include <QShowEvent>
#include <QStringList>
#include <QStyle>
#include <QTableWidget>

#include <algorithm>
#include <limits>

namespace {
QString position_text(int building_id, int dorm_id, int bed_id)
{
	if (building_id <= 0 || dorm_id <= 0 || bed_id <= 0) {
		return QStringLiteral("未入住");
	}
	return QStringLiteral("%1号楼 %2室 %3床").arg(building_id).arg(dorm_id).arg(bed_id);
}

QString student_name(int student_id)
{
	const student* current_student = school::instance().get_student(student_id);
	return current_student == nullptr ? QStringLiteral("学生不存在") : current_student->get_name();
}

QString selected_table_text(QTableWidget* table)
{
	if (table == nullptr || table->selectedItems().isEmpty()) {
		return {};
	}
	QList<QTableWidgetItem*> items = table->selectedItems();
	std::sort(items.begin(), items.end(), [](const QTableWidgetItem* left, const QTableWidgetItem* right)
	{
		return left->row() != right->row() ? left->row() < right->row() : left->column() < right->column();
	});
	QString text;
	int previous_row = -1;
	for (QTableWidgetItem* item : items) {
		if (previous_row >= 0) {
			text += item->row() == previous_row ? QLatin1Char('\t') : QLatin1Char('\n');
		}
		text += item->text();
		previous_row = item->row();
	}
	return text;
}

void enable_table_copy(QTableWidget* table)
{
	auto* shortcut = new QShortcut(QKeySequence::Copy, table);
	QObject::connect(shortcut, &QShortcut::activated, table, [table]
	{
		const QString text = selected_table_text(table);
		if (!text.isEmpty()) {
			QApplication::clipboard()->setText(text);
		}
	});
}

void set_student_item(QTableWidget* table, int row, int column, int student_id, const QString& text)
{
	auto* item = new QTableWidgetItem(text);
	item->setData(Qt::UserRole, student_id);
	table->setItem(row, column, item);
}
}

BatchAssignmentDialog::BatchAssignmentDialog(QWidget* parent)
	: QDialog(parent)
	, ui(new Ui::BatchAssignmentDialog)
{
	ui->setupUi(this);
	ui->assignmentRadio->setAccessibleName(QStringLiteral("为未入住学生补分"));
	ui->reassignmentRadio->setAccessibleName(QStringLiteral("全校重新安排"));
	ui->fillRadio->setAccessibleName(QStringLiteral("优先填满宿舍"));
	ui->randomRadio->setAccessibleName(QStringLiteral("随机安排"));
	ui->preserveBuildingRadio->setAccessibleName(QStringLiteral("尽量保持原宿舍楼"));
	ui->seedLineEdit->setAccessibleName(QStringLiteral("随机种子"));
	ui->seedLineEdit->setAccessibleDescription(QStringLiteral("相同数据、策略和随机种子会生成相同的住宿安排预览。"));
	ui->seedLineEdit->setValidator(new QRegularExpressionValidator(
		QRegularExpression(QStringLiteral("[0-9]{0,10}")), ui->seedLineEdit));
	auto* task_group = new QButtonGroup(this);
	task_group->setExclusive(true);
	task_group->addButton(ui->assignmentRadio);
	task_group->addButton(ui->reassignmentRadio);
	for (QTableWidget* table : {ui->changesTable, ui->unassignedTable, ui->issuesTable}) {
		table->horizontalHeader()->setStretchLastSection(true);
		table->verticalHeader()->setVisible(false);
		table->setTextElideMode(Qt::ElideRight);
		enable_table_copy(table);
	}
	ui->changesTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
	ui->changesTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
	ui->changesTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
	ui->changesTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Stretch);
	ui->changesTable->horizontalHeader()->setSectionResizeMode(4, QHeaderView::ResizeToContents);
	ui->unassignedTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
	ui->unassignedTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
	ui->unassignedTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
	ui->issuesTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
	ui->issuesTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);

	connect(ui->assignmentRadio, &QRadioButton::toggled, this, [this](bool checked)
	{
		if (checked) {
			preview_ready = false;
			refresh_task_summary();
		}
	});
	connect(ui->reassignmentRadio, &QRadioButton::toggled, this, [this](bool checked)
	{
		if (checked) {
			preview_ready = false;
			refresh_task_summary();
		}
	});
	connect(ui->cancelButton, &QPushButton::clicked, this, &QDialog::reject);
	connect(ui->nextButton, &QPushButton::clicked, this, [this]
	{
		refresh_strategy_page();
		ui->pageStack->setCurrentWidget(ui->strategyPage);
		ui->stepLabel->setText(QStringLiteral("2 / 3"));
		ui->backButton->show();
		ui->nextButton->hide();
		ui->previewButton->show();
	});
	connect(ui->backButton, &QPushButton::clicked, this, [this]
	{
		if (ui->pageStack->currentWidget() == ui->previewPage) {
			ui->pageStack->setCurrentWidget(ui->strategyPage);
			ui->stepLabel->setText(QStringLiteral("2 / 3"));
			ui->previewButton->show();
			ui->regeneratePreviewButton->hide();
			ui->applyButton->hide();
			return;
		}
		ui->pageStack->setCurrentWidget(ui->taskPage);
		ui->stepLabel->setText(QStringLiteral("1 / 3"));
		ui->backButton->hide();
		ui->previewButton->hide();
		ui->nextButton->show();
	});
	connect(ui->regenerateSeedButton, &QPushButton::clicked, this, &BatchAssignmentDialog::regenerate_seed);
	connect(ui->previewButton, &QPushButton::clicked, this, &BatchAssignmentDialog::generate_preview);
	connect(ui->regeneratePreviewButton, &QPushButton::clicked, this, &BatchAssignmentDialog::generate_preview);
	connect(ui->applyButton, &QPushButton::clicked, this, &BatchAssignmentDialog::apply_preview);
	connect(ui->riskCheckBox, &QCheckBox::toggled, this, &BatchAssignmentDialog::update_apply_state);
	connect(ui->copyIssuesButton, &QPushButton::clicked, this, &BatchAssignmentDialog::copy_issues);
	connect(ui->changesTable, &QTableWidget::cellDoubleClicked, this, &BatchAssignmentDialog::open_student_detail);
	connect(ui->unassignedTable, &QTableWidget::cellDoubleClicked, this, &BatchAssignmentDialog::open_student_detail);
	connect(ui->issuesTable, &QTableWidget::cellDoubleClicked, this, &BatchAssignmentDialog::open_student_detail);

	regenerate_seed();
	refresh_task_summary();
}

BatchAssignmentDialog::~BatchAssignmentDialog()
{
	delete ui;
}

void BatchAssignmentDialog::showEvent(QShowEvent* event)
{
	QDialog::showEvent(event);
	refresh_task_summary();
	if (fitted_to_screen || screen() == nullptr) {
		return;
	}
	fitted_to_screen = true;
	const QSize available = screen()->availableGeometry().size() - QSize(40, 40);
	resize(qMin(width(), available.width()), qMin(height(), available.height()));
}

bool BatchAssignmentDialog::is_reassignment() const
{
	return ui->reassignmentRadio->isChecked();
}

quint32 BatchAssignmentDialog::current_seed(bool* valid) const
{
	bool conversion_ok = false;
	const quint64 value = ui->seedLineEdit->text().toULongLong(&conversion_ok);
	const bool seed_valid = conversion_ok && value <= std::numeric_limits<quint32>::max();
	if (valid != nullptr) {
		*valid = seed_valid;
	}
	return seed_valid ? static_cast<quint32>(value) : 0;
}

void BatchAssignmentDialog::refresh_task_summary()
{
	school& current_school = school::instance();
	if (is_reassignment()) {
		ui->currentSummaryLabel->setText(QStringLiteral("学生总数：%1 人　当前已入住：%2 人　未入住：%3 人\n总床位：%4 个　当前空床：%5 个")
			.arg(current_school.get_student_count())
			.arg(current_school.get_assigned_student_count())
			.arg(current_school.get_unassigned_student_count())
			.arg(current_school.get_total_bed_count())
			.arg(current_school.get_empty_bed_count()));
		return;
	}
	const int candidates = current_school.get_unassigned_student_count();
	const int available = current_school.get_empty_bed_count();
	ui->currentSummaryLabel->setText(QStringLiteral("未入住学生：%1 人　当前空床：%2 个　理论最多安排：%3 人\n实际可安排人数还会受到学生性别、楼栋用途和宿舍性别锁限制。")
		.arg(candidates).arg(available).arg(qMin(candidates, available)));
}

void BatchAssignmentDialog::refresh_strategy_page()
{
	const bool reassign = is_reassignment();
	ui->preserveBuildingRadio->setVisible(reassign);
	ui->preserveBuildingDescriptionLabel->setVisible(reassign);
	ui->fillRadio->setText(reassign ? QStringLiteral("优先填满宿舍") : QStringLiteral("优先填满已有宿舍"));
	ui->fillDescriptionLabel->setText(reassign
		? QStringLiteral("重新安排后优先提高已使用宿舍的入住率，减少零散占用。")
		: QStringLiteral("优先使用已有住客且尚有空床的宿舍，减少零散占用。"));
	if (!reassign && ui->preserveBuildingRadio->isChecked()) {
		ui->fillRadio->setChecked(true);
	}
	ui->applyButton->setProperty("primaryButton", !reassign);
	ui->applyButton->setProperty("dangerButton", reassign);
	ui->applyButton->style()->unpolish(ui->applyButton);
	ui->applyButton->style()->polish(ui->applyButton);
	ui->riskCheckBox->setChecked(false);
}

void BatchAssignmentDialog::refresh_preview_page()
{
	ui->changesTable->setRowCount(0);
	ui->unassignedTable->setRowCount(0);
	ui->issuesTable->setRowCount(0);

	const QVector<accommodation_change>& changes = is_reassignment() ? all_students_preview.changes : assignment_preview.changes;
	const QVector<int>& unassigned_ids = is_reassignment() ? all_students_preview.unassigned_student_ids : assignment_preview.unassigned_student_ids;
	const QVector<accommodation_data_issue>& issues = is_reassignment() ? all_students_preview.issues : assignment_preview.issues;
	const int candidate_count = is_reassignment() ? all_students_preview.candidate_count : assignment_preview.candidate_count;
	const int assigned_count = is_reassignment()
		? all_students_preview.changes.size() - all_students_preview.unassigned_student_ids.size()
		: assignment_preview.changes.size();

	for (const accommodation_change& change : changes) {
		const int row = ui->changesTable->rowCount();
		ui->changesTable->insertRow(row);
		set_student_item(ui->changesTable, row, 0, change.student_id, QString::number(change.student_id));
		set_student_item(ui->changesTable, row, 1, change.student_id, student_name(change.student_id));
		set_student_item(ui->changesTable, row, 2, change.student_id,
			position_text(change.old_building_id, change.old_dorm_id, change.old_bed_id));
		set_student_item(ui->changesTable, row, 3, change.student_id,
			position_text(change.new_building_id, change.new_dorm_id, change.new_bed_id));
		QString result_text;
		if (change.new_building_id <= 0) {
			result_text = QStringLiteral("无法安排");
		} else if (change.old_building_id <= 0) {
			result_text = QStringLiteral("将入住");
		} else if (change.old_building_id == change.new_building_id
			&& change.old_dorm_id == change.new_dorm_id && change.old_bed_id == change.new_bed_id) {
			result_text = QStringLiteral("位置不变");
		} else {
			result_text = QStringLiteral("将调整");
		}
		set_student_item(ui->changesTable, row, 4, change.student_id, result_text);
	}

	for (int student_id : unassigned_ids) {
		const int row = ui->unassignedTable->rowCount();
		ui->unassignedTable->insertRow(row);
		set_student_item(ui->unassignedTable, row, 0, student_id, QString::number(student_id));
		set_student_item(ui->unassignedTable, row, 1, student_id, student_name(student_id));
		set_student_item(ui->unassignedTable, row, 2, student_id, QStringLiteral("没有符合学生性别与宿舍限制的可用床位"));
	}

	for (const accommodation_data_issue& issue : issues) {
		const int row = ui->issuesTable->rowCount();
		ui->issuesTable->insertRow(row);
		QString object_text;
		if (issue.student_id > 0) {
			object_text = QStringLiteral("学生 %1").arg(issue.student_id);
		} else if (issue.building_id > 0 && issue.dorm_id > 0) {
			object_text = QStringLiteral("%1号楼 %2室").arg(issue.building_id).arg(issue.dorm_id);
		} else if (issue.building_id > 0) {
			object_text = QStringLiteral("%1号楼").arg(issue.building_id);
		} else {
			object_text = QStringLiteral("住宿数据");
		}
		set_student_item(ui->issuesTable, row, 0, issue.student_id, object_text);
		set_student_item(ui->issuesTable, row, 1, issue.student_id, issue.message);
	}

	ui->changesTable->resizeRowsToContents();
	ui->unassignedTable->resizeRowsToContents();
	ui->issuesTable->resizeRowsToContents();
	ui->previewTabs->setTabText(0, QStringLiteral("安排明细（%1）").arg(changes.size()));
	ui->previewTabs->setTabText(1, QStringLiteral("无法安排（%1）").arg(unassigned_ids.size()));
	ui->previewTabs->setTabText(2, QStringLiteral("住宿异常（%1）").arg(issues.size()));
	ui->copyIssuesButton->setVisible(!issues.isEmpty());
	ui->riskCheckBox->setVisible(is_reassignment() && issues.isEmpty() && !changes.isEmpty());
	ui->riskCheckBox->setChecked(false);

	if (!issues.isEmpty()) {
		ui->previewStatusLabel->setText(QStringLiteral("预检结果：不能执行"));
		ui->previewSummaryLabel->setText(QStringLiteral("发现 %1 条住宿记录异常。请先核查异常对象，再重新生成预览；系统不会跳过异常数据继续执行。").arg(issues.size()));
		ui->previewTabs->setCurrentWidget(ui->issuesTab);
	} else if (changes.isEmpty()) {
		ui->previewStatusLabel->setText(QStringLiteral("预检结果：无需执行"));
		ui->previewSummaryLabel->setText(is_reassignment()
			? QStringLiteral("当前没有可生成的重新安排结果，请检查学生和宿舍资源。")
			: QStringLiteral("当前没有可安排的未入住学生，或没有符合条件的可用床位。"));
		ui->previewTabs->setCurrentWidget(unassigned_ids.isEmpty() ? ui->changesTab : ui->unassignedTab);
	} else {
		ui->previewStatusLabel->setText(QStringLiteral("预检结果：可以执行"));
		const int remaining_beds = qMax(0, is_reassignment()
			? school::instance().get_total_bed_count() - assigned_count
			: school::instance().get_empty_bed_count() - assigned_count);
		const QString strategy_text = ui->randomRadio->isChecked() ? QStringLiteral("随机安排")
			: ui->preserveBuildingRadio->isChecked() ? QStringLiteral("尽量保持原宿舍楼")
			: is_reassignment() ? QStringLiteral("优先填满宿舍") : QStringLiteral("优先填满已有宿舍");
		ui->previewSummaryLabel->setText(QStringLiteral("待处理：%1 人　可安排：%2 人　无法安排：%3 人　安排后空床：%4 个\n策略：%5　随机种子：%6。确认执行时将严格使用下方具体安排，不会重新随机。")
			.arg(candidate_count).arg(assigned_count).arg(unassigned_ids.size()).arg(remaining_beds)
			.arg(strategy_text).arg(is_reassignment() ? all_students_preview.random_seed : assignment_preview.random_seed));
		ui->previewTabs->setCurrentWidget(ui->changesTab);
	}
	update_apply_state();
}

void BatchAssignmentDialog::update_apply_state()
{
	const bool can_apply = preview_ready && (is_reassignment() ? all_students_preview.can_apply() : assignment_preview.can_apply());
	ui->applyButton->setEnabled(can_apply && (!is_reassignment() || ui->riskCheckBox->isChecked()));
}

void BatchAssignmentDialog::regenerate_seed()
{
	ui->seedLineEdit->setText(QString::number(QRandomGenerator::global()->generate()));
	ui->seedLineEdit->setProperty("inputError", false);
	ui->seedLineEdit->style()->unpolish(ui->seedLineEdit);
	ui->seedLineEdit->style()->polish(ui->seedLineEdit);
	ui->seedErrorLabel->hide();
}

void BatchAssignmentDialog::generate_preview()
{
	bool seed_valid = false;
	const quint32 seed = current_seed(&seed_valid);
	ui->seedLineEdit->setProperty("inputError", !seed_valid);
	ui->seedLineEdit->style()->unpolish(ui->seedLineEdit);
	ui->seedLineEdit->style()->polish(ui->seedLineEdit);
	ui->seedErrorLabel->setVisible(!seed_valid);
	if (!seed_valid) {
		uifeedback::show_error(this, QStringLiteral("随机种子无效"), QStringLiteral("请输入 0～4294967295 范围内的整数。"));
		return;
	}

	if (is_reassignment()) {
		reassignment_strategy strategy = reassignment_strategy::fill_dorms_first;
		if (ui->randomRadio->isChecked()) {
			strategy = reassignment_strategy::random;
		} else if (ui->preserveBuildingRadio->isChecked()) {
			strategy = reassignment_strategy::preserve_building_first;
		}
		all_students_preview = school::instance().preview_reassign_all_students(strategy, seed);
	} else {
		const assignment_strategy strategy = ui->randomRadio->isChecked()
			? assignment_strategy::random : assignment_strategy::fill_occupied_first;
		assignment_preview = school::instance().preview_assign_unassigned_students(strategy, seed);
	}
	preview_ready = true;
	refresh_preview_page();
	ui->pageStack->setCurrentWidget(ui->previewPage);
	ui->stepLabel->setText(QStringLiteral("3 / 3"));
	ui->previewButton->hide();
	ui->regeneratePreviewButton->show();
	ui->applyButton->show();
}

void BatchAssignmentDialog::apply_preview()
{
	if (!preview_ready) {
		return;
	}
	const int result = is_reassignment()
		? school::instance().apply_reassignment(all_students_preview)
		: school::instance().apply_batch_assignment(assignment_preview);
	if (result >= 0) {
		const int unassigned_count = is_reassignment()
			? all_students_preview.unassigned_student_ids.size()
			: assignment_preview.unassigned_student_ids.size();
		const QString strategy_text = ui->randomRadio->isChecked() ? QStringLiteral("随机安排")
			: ui->preserveBuildingRadio->isChecked() ? QStringLiteral("尽量保持原宿舍楼")
			: is_reassignment() ? QStringLiteral("优先填满宿舍") : QStringLiteral("优先填满已有宿舍");
		uifeedback::show_information(this,
			is_reassignment() ? QStringLiteral("全校重新安排完成") : QStringLiteral("批量补分完成"),
			QStringLiteral("已成功安排 %1 人，仍有 %2 人未入住。\n使用策略：%3。实际结果与确认前的预览一致。")
				.arg(result).arg(unassigned_count).arg(strategy_text));
		accept();
		return;
	}
	if (result == -6) {
		uifeedback::show_critical(this, QStringLiteral("住宿数据恢复不完整"),
			QStringLiteral("执行失败后未能完整恢复操作前的住宿安排，请暂停后续住宿操作并核查学生位置、宿舍床位和宿舍性别锁。"),
			QStringLiteral("业务返回值：-6"));
		return;
	}
	if (result == -8) {
		uifeedback::show_critical(this, QStringLiteral("住宿记录异常"),
			QStringLiteral("检测到学生住宿位置与实际床位不一致，本次操作已停止，请核查数据后重新预览。"),
			QStringLiteral("业务返回值：-8"));
		preview_ready = false;
		update_apply_state();
		return;
	}
	if (result == -7) {
		uifeedback::show_error(this, QStringLiteral("预览已经失效"),
			QStringLiteral("生成预览后，相关学生、宿舍或床位数据已经发生变化。请重新生成预览后再执行。"));
		preview_ready = false;
		update_apply_state();
		return;
	}
	if (result == -5) {
		uifeedback::show_error(this, QStringLiteral("批量住宿分配失败"),
			QStringLiteral("操作未能完成，但系统已恢复操作前的住宿安排。请刷新数据后重试。"),
			QStringLiteral("业务返回值：-5"));
		preview_ready = false;
		update_apply_state();
		return;
	}
	uifeedback::show_error(this, QStringLiteral("无法执行批量住宿分配"),
		QStringLiteral("当前预览不再满足执行条件，请重新生成预览。"), QStringLiteral("业务返回值：%1").arg(result));
	preview_ready = false;
	update_apply_state();
}

void BatchAssignmentDialog::copy_issues()
{
	const QString details = issue_details();
	if (!details.isEmpty()) {
		QApplication::clipboard()->setText(details);
		uifeedback::show_success(this, QStringLiteral("住宿异常信息已复制。"));
	}
}

void BatchAssignmentDialog::open_student_detail(int row, int column)
{
	Q_UNUSED(column)
	auto* table = qobject_cast<QTableWidget*>(sender());
	if (table == nullptr || row < 0 || row >= table->rowCount() || table->item(row, 0) == nullptr) {
		return;
	}
	if (table == ui->issuesTable) {
		const QVector<accommodation_data_issue>& issues = is_reassignment()
			? all_students_preview.issues : assignment_preview.issues;
		if (row >= issues.size()) {
			return;
		}
		const accommodation_data_issue& issue = issues.at(row);
		if (issue.student_id > 0) {
			emit student_navigation_requested(issue.student_id);
			accept();
		} else if (issue.building_id > 0) {
			emit dorm_navigation_requested(issue.building_id, issue.dorm_id);
			accept();
		}
		return;
	}
	const int student_id = table->item(row, 0)->data(Qt::UserRole).toInt();
	if (student_id <= 0 || school::instance().get_student(student_id) == nullptr) {
		return;
	}
	StudentDetailDialog dialog(student_id, this);
	dialog.exec();
}

QString BatchAssignmentDialog::issue_details() const
{
	const QVector<accommodation_data_issue>& issues = is_reassignment()
		? all_students_preview.issues : assignment_preview.issues;
	QStringList lines;
	for (const accommodation_data_issue& issue : issues) {
		QString object_text;
		if (issue.student_id > 0) {
			object_text = QStringLiteral("学生 %1").arg(issue.student_id);
		} else if (issue.building_id > 0 && issue.dorm_id > 0) {
			object_text = QStringLiteral("%1号楼 %2室").arg(issue.building_id).arg(issue.dorm_id);
		} else if (issue.building_id > 0) {
			object_text = QStringLiteral("%1号楼").arg(issue.building_id);
		} else {
			object_text = QStringLiteral("住宿数据");
		}
		lines.append(QStringLiteral("%1：%2").arg(object_text, issue.message));
	}
	return lines.join(QLatin1Char('\n'));
}
