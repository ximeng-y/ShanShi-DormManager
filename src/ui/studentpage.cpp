#include "studentpage.h"
#include "./ui_studentpage.h"
#include "addstudentdialog.h"
#include "gendercorrectiondialog.h"
#include "studentdetaildialog.h"
#include "uifeedback.h"

#include "core/school.h"
#include "core/student.h"
#include "system/check.h"

#include <QHeaderView>
#include <QList>
#include <QMenu>
#include <QSet>
#include <QSettings>
#include <QShowEvent>
#include <QSignalBlocker>
#include <QStyle>
#include <QStringList>
#include <QTableWidgetItem>

#include <algorithm>

namespace {
void set_field_error(QWidget* field, QLabel* error_label, bool has_error)
{
	field->setProperty("inputError", has_error);
	field->style()->unpolish(field);
	field->style()->polish(field);
	error_label->setVisible(has_error);
}

QString student_gender_text(int gender)
{
	if (gender == 1) {
		return QStringLiteral("男");
	}
	if (gender == 2) {
		return QStringLiteral("女");
	}
	return QStringLiteral("未设置");
}

bool student_has_complete_position(const student& current_student)
{
	return current_student.get_building_id() > 0
		&& current_student.get_dorm_id() > 0
		&& current_student.get_bed_id() > 0
		&& current_student.get_floor() > 0;
}

QString student_accommodation_text(const student& current_student, bool assigned)
{
	const bool has_any_position = current_student.get_building_id() > 0
		|| current_student.get_dorm_id() > 0
		|| current_student.get_bed_id() > 0
		|| current_student.get_floor() > 0;
	if (!assigned && !has_any_position) {
		return QStringLiteral("未入住");
	}
	if (!assigned || !student_has_complete_position(current_student)) {
		return QStringLiteral("住宿记录异常");
	}
	return QStringLiteral("%1号楼 · %2室 · %3号床")
		.arg(current_student.get_building_id())
		.arg(current_student.get_dorm_id())
		.arg(current_student.get_bed_id());
}
}

StudentPage::StudentPage(QWidget* parent)
	: QWidget(parent)
	, ui(new Ui::StudentPage)
{
	ui->setupUi(this);
	ui->studentTable->verticalHeader()->setVisible(false);
	ui->studentTable->verticalHeader()->setDefaultSectionSize(42);
	ui->studentTable->horizontalHeader()->setStretchLastSection(true);
	ui->studentTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
	ui->studentTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
	ui->studentTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
	ui->studentTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
	ui->studentTable->horizontalHeader()->setSectionResizeMode(4, QHeaderView::ResizeToContents);
	ui->studentTable->horizontalHeader()->setSectionResizeMode(5, QHeaderView::ResizeToContents);

	auto* more_menu = new QMenu(ui->moreActionButton);
	QAction* gender_correction_action = more_menu->addAction(QStringLiteral("性别纠错"));
	more_menu->addSeparator();
	QAction* remove_student_action = more_menu->addAction(QStringLiteral("退学籍"));
	ui->moreActionButton->setMenu(more_menu);
	connect(gender_correction_action, &QAction::triggered, this, [this]() {
		GenderCorrectionDialog dialog(selected_student_id, this);
		if (dialog.exec() == QDialog::Accepted) {
			refresh_data();
			uifeedback::show_success(this, QStringLiteral("学生性别已纠正。"));
		}
	});
	connect(remove_student_action, &QAction::triggered, this, [this]() {
		const student* current_student = school::instance().get_student(selected_student_id);
		if (current_student == nullptr) {
			uifeedback::show_error(this, QStringLiteral("无法办理退学籍"), QStringLiteral("所选学生已经不存在，请刷新列表后重试。"));
			refresh_data();
			return;
		}
		const QString student_description = QStringLiteral("%1（%2）").arg(current_student->get_name()).arg(current_student->get_id());
		const bool has_position = current_student->get_building_id() > 0 || current_student->get_dorm_id() > 0
			|| current_student->get_floor() > 0 || current_student->get_bed_id() > 0;
		const QString impact = has_position
			? QStringLiteral("该学生存在住宿位置，系统将先办理退宿，再永久删除学生档案。")
			: QStringLiteral("该操作将永久删除学生档案。当前学生没有住宿位置。");
		if (!uifeedback::confirm_danger(this, QStringLiteral("确认办理退学籍"),
			QStringLiteral("确定删除 %1 吗？\n\n%2").arg(student_description, impact), QStringLiteral("确认删除"))) {
			return;
		}

		const int result = school::instance().remove_student(selected_student_id);
		if (result == 1) {
			selected_student_id = 0;
			refresh_data();
			uifeedback::show_information(this, QStringLiteral("退学籍完成"), has_position
				? QStringLiteral("学生档案已删除，原住宿位置已同步清退。")
				: QStringLiteral("学生档案已删除。"));
			return;
		}
		if (result == -8) {
			uifeedback::show_critical(this, QStringLiteral("住宿记录异常"), QStringLiteral("学生位置与宿舍床位记录不一致，系统未删除学生档案。请暂停相关操作并核查数据。"));
			return;
		}
		uifeedback::show_error(this, QStringLiteral("无法办理退学籍"), result == 0
			? QStringLiteral("该学生已经不存在，请刷新列表后重试。")
			: QStringLiteral("学生学号参数无效。"));
		refresh_data();
	});

	connect(ui->searchLineEdit, &QLineEdit::textChanged, this, [this]() { apply_filters(); });
	connect(ui->classFilterCombo, &QComboBox::currentIndexChanged, this, [this]() { apply_filters(); });
	connect(ui->statusFilterCombo, &QComboBox::currentIndexChanged, this, [this]() { apply_filters(); });
	connect(ui->resetFilterButton, &QPushButton::clicked, this, [this]() {
		const QSignalBlocker search_blocker(ui->searchLineEdit);
		const QSignalBlocker class_blocker(ui->classFilterCombo);
		const QSignalBlocker status_blocker(ui->statusFilterCombo);
		ui->searchLineEdit->clear();
		ui->classFilterCombo->setCurrentIndex(0);
		ui->statusFilterCombo->setCurrentIndex(0);
		apply_filters();
	});
	connect(ui->studentTable, &QTableWidget::cellClicked, this, [this](int row, int) {
		QTableWidgetItem* id_item = ui->studentTable->item(row, 0);
		if (id_item != nullptr) {
			show_student_summary(id_item->data(Qt::UserRole).toInt());
		}
	});
	connect(ui->studentTable, &QTableWidget::cellDoubleClicked, this, [this](int row, int) {
		QTableWidgetItem* id_item = ui->studentTable->item(row, 0);
		if (id_item != nullptr) {
			StudentDetailDialog dialog(id_item->data(Qt::UserRole).toInt(), this);
			dialog.exec();
		}
	});
	connect(ui->addStudentButton, &QPushButton::clicked, this, [this]() {
		AddStudentDialog dialog(this);
		if (dialog.exec() == QDialog::Accepted) {
			selected_student_id = dialog.added_student_id();
			refresh_data();
			uifeedback::show_success(this, QStringLiteral("学生档案已添加。"));
		}
	});
	connect(ui->editStudentButton, &QPushButton::clicked, this, &StudentPage::start_edit_student);
	connect(ui->cancelEditButton, &QPushButton::clicked, this, &StudentPage::cancel_edit_student);
	connect(ui->saveEditButton, &QPushButton::clicked, this, &StudentPage::save_student_changes);
	connect(ui->hideDetailButton, &QToolButton::clicked, this, [this]() {
		set_detail_panel_visible(false);
	});
	connect(ui->showDetailButton, &QToolButton::clicked, this, [this]() {
		set_detail_panel_visible(true);
	});

	QSettings settings(QStringLiteral("DormManager"), QStringLiteral("DormManager"));
	set_detail_panel_visible(settings.value(QStringLiteral("student/detailPanelVisible"), true).toBool());
}

StudentPage::~StudentPage()
{
	delete ui;
}

void StudentPage::refresh_data()
{
	rebuild_class_filter();
	apply_filters();
}

void StudentPage::showEvent(QShowEvent* event)
{
	QWidget::showEvent(event);
	if (!ui->editContent->isVisible()) {
		refresh_data();
	}
}

void StudentPage::rebuild_class_filter()
{
	const int selected_class = ui->classFilterCombo->currentData().toInt();
	QSet<int> class_numbers;
	const school& current_school = school::instance();
	for (int student_id : current_school.get_all_student_ids()) {
		const student* current_student = current_school.get_student(student_id);
		if (current_student != nullptr && current_student->get_class_num() > 0) {
			class_numbers.insert(current_student->get_class_num());
		}
	}
	QList<int> sorted_classes = class_numbers.values();
	std::sort(sorted_classes.begin(), sorted_classes.end());

	const QSignalBlocker blocker(ui->classFilterCombo);
	ui->classFilterCombo->clear();
	ui->classFilterCombo->addItem(QStringLiteral("全部班级"), 0);
	for (int class_num : sorted_classes) {
		ui->classFilterCombo->addItem(QStringLiteral("%1班").arg(class_num), class_num);
	}
	const int restored_index = ui->classFilterCombo->findData(selected_class);
	ui->classFilterCombo->setCurrentIndex(restored_index >= 0 ? restored_index : 0);
}

void StudentPage::apply_filters()
{
	const school& current_school = school::instance();
	const QString search_text = ui->searchLineEdit->text().trimmed();
	const int selected_class = ui->classFilterCombo->currentData().toInt();
	const int selected_status = ui->statusFilterCombo->currentIndex();
	QSet<int> assigned_ids;
	for (int assigned_id : current_school.get_assigned_student_ids()) {
		assigned_ids.insert(assigned_id);
	}
	QVector<int> matched_ids;

	for (int student_id : current_school.get_all_student_ids()) {
		const student* current_student = current_school.get_student(student_id);
		if (current_student == nullptr) {
			continue;
		}
		const bool assigned = assigned_ids.contains(student_id);
		const bool matches_search = search_text.isEmpty()
			|| QString::number(student_id).contains(search_text)
			|| current_student->get_name().contains(search_text, Qt::CaseInsensitive);
		const bool matches_class = selected_class == 0 || current_student->get_class_num() == selected_class;
		const bool matches_status = selected_status == 0
			|| (selected_status == 1 && assigned)
			|| (selected_status == 2 && !assigned);
		if (matches_search && matches_class && matches_status) {
			matched_ids.append(student_id);
		}
	}

	ui->studentTable->clearSelection();
	ui->studentTable->setCurrentCell(-1, -1);
	ui->studentTable->setRowCount(matched_ids.size());
	for (int row = 0; row < matched_ids.size(); ++row) {
		const student* current_student = current_school.get_student(matched_ids.at(row));
		if (current_student == nullptr) {
			continue;
		}
		const bool assigned = assigned_ids.contains(current_student->get_id());
		const QStringList values = {
			QString::number(current_student->get_id()),
			current_student->get_name(),
			student_gender_text(current_student->get_gender()),
			QStringLiteral("%1班").arg(current_student->get_class_num()),
			QString::number(current_student->get_grade()),
			assigned ? QStringLiteral("已入住") : QStringLiteral("未入住"),
			student_accommodation_text(*current_student, assigned)
		};
		for (int column = 0; column < values.size(); ++column) {
			auto* item = new QTableWidgetItem(values.at(column));
			item->setTextAlignment(column == 1 || column == 6 ? Qt::AlignVCenter | Qt::AlignLeft : Qt::AlignCenter);
			if (column == 0) {
				item->setData(Qt::UserRole, current_student->get_id());
			}
			ui->studentTable->setItem(row, column, item);
		}
	}

	ui->resultCountLabel->setText(matched_ids.isEmpty()
		? QStringLiteral("没有符合条件的学生")
		: QStringLiteral("已显示 %1 / %2 人").arg(matched_ids.size()).arg(current_school.get_student_count()));

	int selected_row = -1;
	for (int row = 0; row < ui->studentTable->rowCount(); ++row) {
		QTableWidgetItem* id_item = ui->studentTable->item(row, 0);
		if (id_item != nullptr && id_item->data(Qt::UserRole).toInt() == selected_student_id) {
			selected_row = row;
			break;
		}
	}
	if (selected_row >= 0) {
		ui->studentTable->selectRow(selected_row);
		show_student_summary(selected_student_id);
	} else {
		clear_student_summary();
	}
}

void StudentPage::show_student_summary(int student_id)
{
	const student* current_student = school::instance().get_student(student_id);
	if (current_student == nullptr) {
		clear_student_summary();
		return;
	}

	selected_student_id = student_id;
	const bool assigned = school::instance().get_assigned_student_ids().contains(student_id);
	ui->detailNameLabel->setText(current_student->get_name());
	ui->studentIdValueLabel->setText(QString::number(current_student->get_id()));
	ui->genderValueLabel->setText(student_gender_text(current_student->get_gender()));
	ui->gradeValueLabel->setText(QString::number(current_student->get_grade()));
	ui->classValueLabel->setText(QStringLiteral("%1班").arg(current_student->get_class_num()));
	ui->accommodationValueLabel->setText(student_accommodation_text(*current_student, assigned));
	ui->accommodationActionButton->setText(assigned
		? QStringLiteral("办理调宿")
		: QStringLiteral("办理入住"));
	ui->detailHintLabel->hide();
	ui->detailContent->show();
	ui->editContent->hide();
	ui->editStudentButton->setEnabled(true);
	ui->accommodationActionButton->setEnabled(true);
	ui->moreActionButton->setEnabled(true);
}

void StudentPage::clear_student_summary()
{
	selected_student_id = 0;
	ui->detailNameLabel->setText(QStringLiteral("学生详情"));
	ui->detailHintLabel->show();
	ui->detailContent->hide();
	ui->editContent->hide();
	ui->editStudentButton->setEnabled(false);
	ui->accommodationActionButton->setEnabled(false);
	ui->moreActionButton->setEnabled(false);
}

void StudentPage::set_detail_panel_visible(bool visible)
{
	ui->detailPanel->setVisible(visible);
	ui->showDetailButton->setVisible(!visible);
	if (visible) {
		ui->studentSplitter->setSizes({700, 360});
	}
	QSettings settings(QStringLiteral("DormManager"), QStringLiteral("DormManager"));
	settings.setValue(QStringLiteral("student/detailPanelVisible"), visible);
}

void StudentPage::start_edit_student()
{
	const student* current_student = school::instance().get_student(selected_student_id);
	if (current_student == nullptr) {
		uifeedback::show_error(this, QStringLiteral("无法修改资料"), QStringLiteral("所选学生已经不存在，请刷新列表后重试。"));
		set_student_directory_enabled(true);
		refresh_data();
		return;
	}

	ui->editStudentIdValueLabel->setText(QString::number(current_student->get_id()));
	ui->editNameLineEdit->setText(current_student->get_name());
	ui->editClassSpin->setValue(current_student->get_class_num());
	ui->editGradeSpin->setValue(current_student->get_grade());
	ui->detailContent->hide();
	ui->detailHintLabel->hide();
	ui->editContent->show();
	clear_edit_validation();
	set_student_directory_enabled(false);
	ui->editNameLineEdit->setFocus();
	ui->editNameLineEdit->selectAll();
}

void StudentPage::cancel_edit_student()
{
	set_student_directory_enabled(true);
	show_student_summary(selected_student_id);
}

void StudentPage::save_student_changes()
{
	const student* current_student = school::instance().get_student(selected_student_id);
	if (current_student == nullptr) {
		uifeedback::show_error(this, QStringLiteral("无法修改资料"), QStringLiteral("所选学生已经不存在，请刷新列表后重试。"));
		set_student_directory_enabled(true);
		refresh_data();
		return;
	}

	const QString new_name = ui->editNameLineEdit->text().trimmed();
	const int new_class_num = ui->editClassSpin->value();
	const int new_grade = ui->editGradeSpin->value();
	clear_edit_validation();
	if (!check::is_valid_student_name(new_name)) {
		set_field_error(ui->editNameLineEdit, ui->editNameErrorLabel, true);
		ui->editNameLineEdit->setFocus();
		uifeedback::show_error(this, QStringLiteral("无法修改资料"), ui->editNameErrorLabel->text());
		return;
	}
	if (!check::is_valid_class_num(new_class_num)) {
		set_field_error(ui->editClassSpin, ui->editClassErrorLabel, true);
		ui->editClassSpin->setFocus();
		uifeedback::show_error(this, QStringLiteral("无法修改资料"), ui->editClassErrorLabel->text());
		return;
	}
	if (!check::is_valid_grade(new_grade)) {
		set_field_error(ui->editGradeSpin, ui->editGradeErrorLabel, true);
		ui->editGradeSpin->setFocus();
		uifeedback::show_error(this, QStringLiteral("无法修改资料"), ui->editGradeErrorLabel->text());
		return;
	}

	const QString old_name = current_student->get_name();
	const int old_class_num = current_student->get_class_num();
	const int old_grade = current_student->get_grade();
	const bool change_name = new_name != old_name;
	const bool change_class = new_class_num != old_class_num;
	const bool change_grade = new_grade != old_grade;
	if (!change_name && !change_class && !change_grade) {
		cancel_edit_student();
		return;
	}

	school& current_school = school::instance();
	bool name_changed = false;
	bool class_changed = false;
	if (change_name) {
		name_changed = current_school.set_student_name(selected_student_id, new_name) == 1;
		if (!name_changed) {
			uifeedback::show_error(this, QStringLiteral("无法修改资料"), QStringLiteral("学生姓名未能保存，请刷新后重试。"));
			return;
		}
	}
	if (change_class) {
		class_changed = current_school.set_student_class_num(selected_student_id, new_class_num) == 1;
		if (!class_changed) {
			const bool restored = !name_changed || current_school.set_student_name(selected_student_id, old_name) == 1;
			if (!restored) {
				uifeedback::show_critical(this, QStringLiteral("资料恢复失败"), QStringLiteral("班级修改失败，且姓名未能恢复。请暂停后续操作并核查学生资料。"));
			} else {
				uifeedback::show_error(this, QStringLiteral("无法修改资料"), QStringLiteral("学生班级未能保存，已恢复原资料。"));
			}
			set_student_directory_enabled(true);
			refresh_data();
			return;
		}
	}
	if (change_grade && current_school.set_student_grade(selected_student_id, new_grade) != 1) {
		const bool class_restored = !class_changed || current_school.set_student_class_num(selected_student_id, old_class_num) == 1;
		const bool name_restored = !name_changed || current_school.set_student_name(selected_student_id, old_name) == 1;
		if (!class_restored || !name_restored) {
			uifeedback::show_critical(this, QStringLiteral("资料恢复失败"), QStringLiteral("年级修改失败，且原资料未能完整恢复。请暂停后续操作并核查学生资料。"));
		} else {
			uifeedback::show_error(this, QStringLiteral("无法修改资料"), QStringLiteral("学生年级未能保存，已恢复原资料。"));
		}
		set_student_directory_enabled(true);
		refresh_data();
		return;
	}

	set_student_directory_enabled(true);
	refresh_data();
	uifeedback::show_success(this, QStringLiteral("学生基础资料已更新。"));
}

void StudentPage::set_student_directory_enabled(bool enabled)
{
	ui->studentTable->setEnabled(enabled);
	ui->searchLineEdit->setEnabled(enabled);
	ui->classFilterCombo->setEnabled(enabled);
	ui->statusFilterCombo->setEnabled(enabled);
	ui->resetFilterButton->setEnabled(enabled);
	ui->addStudentButton->setEnabled(enabled);
	ui->hideDetailButton->setEnabled(enabled);
}

void StudentPage::clear_edit_validation()
{
	set_field_error(ui->editNameLineEdit, ui->editNameErrorLabel, false);
	set_field_error(ui->editClassSpin, ui->editClassErrorLabel, false);
	set_field_error(ui->editGradeSpin, ui->editGradeErrorLabel, false);
}
