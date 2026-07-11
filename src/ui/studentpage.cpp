#include "studentpage.h"
#include "./ui_studentpage.h"

#include "core/school.h"
#include "core/student.h"

#include <QHeaderView>
#include <QList>
#include <QSet>
#include <QSettings>
#include <QShowEvent>
#include <QSignalBlocker>
#include <QStringList>
#include <QTableWidgetItem>

#include <algorithm>

namespace {
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
	refresh_data();
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
