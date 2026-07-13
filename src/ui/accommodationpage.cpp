#include "accommodationpage.h"
#include "./ui_accommodationpage.h"

#include "core/building.h"
#include "core/dorm.h"
#include "core/school.h"
#include "core/student.h"
#include "system/check.h"
#include "uifeedback.h"

#include <QCheckBox>
#include <QComboBox>
#include <QEvent>
#include <QInputMethodEvent>
#include <QLineEdit>
#include <QPointer>
#include <QSet>
#include <QShowEvent>
#include <QSignalBlocker>
#include <QStandardItemModel>
#include <QTimer>

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

bool has_complete_accommodation(const student& current_student)
{
	return current_student.get_building_id() > 0 && current_student.get_dorm_id() > 0
		&& current_student.get_floor() > 0 && current_student.get_bed_id() > 0;
}

bool has_any_accommodation(const student& current_student)
{
	return current_student.get_building_id() > 0 || current_student.get_dorm_id() > 0
		|| current_student.get_floor() > 0 || current_student.get_bed_id() > 0;
}

bool has_consistent_accommodation(const student& current_student)
{
	if (!has_complete_accommodation(current_student)) {
		return false;
	}
	const dorm* source_dorm = school::instance().get_dorm(current_student.get_building_id(), current_student.get_dorm_id());
	return source_dorm != nullptr && source_dorm->get_student_id(current_student.get_bed_id()) == current_student.get_id();
}

QString student_selector_text(const student& current_student)
{
	const QString gender = current_student.get_gender() == 1 ? QStringLiteral("男")
		: (current_student.get_gender() == 2 ? QStringLiteral("女") : QStringLiteral("未设置"));
	return QStringLiteral("%1 · %2 · %3")
		.arg(current_student.get_name(), gender, QString::number(current_student.get_id()));
}

void configure_student_selector(QComboBox* combo)
{
	combo->setEditable(true);
	combo->setInsertPolicy(QComboBox::NoInsert);
	combo->setMaxVisibleItems(12);
	combo->lineEdit()->setPlaceholderText(QStringLiteral("输入姓名或学号"));
	combo->lineEdit()->setClearButtonEnabled(true);
	combo->setCompleter(nullptr);
}
}

AccommodationPage::AccommodationPage(QWidget* parent)
	: QWidget(parent)
	, ui(new Ui::AccommodationPage)
{
	ui->setupUi(this);
	ui->taskInfoButton->set_information(ui->taskInfoButton->toolTip());
	ui->taskTabs->setAccessibleName(QStringLiteral("住宿任务类型"));
	configure_student_selector(ui->assignStudentSpin);
	configure_student_selector(ui->removeStudentSpin);
	configure_student_selector(ui->moveStudentSpin);
	configure_student_selector(ui->swapStudent1Spin);
	configure_student_selector(ui->swapStudent2Spin);
	for (QComboBox* combo : {ui->assignStudentSpin, ui->removeStudentSpin, ui->moveStudentSpin,
		ui->swapStudent1Spin, ui->swapStudent2Spin})
		combo->lineEdit()->installEventFilter(this);
	ui->assignStudentSpin->setAccessibleName(QStringLiteral("入住学生姓名或学号"));
	ui->removeStudentSpin->setAccessibleName(QStringLiteral("退宿学生姓名或学号"));
	ui->moveStudentSpin->setAccessibleName(QStringLiteral("调宿学生姓名或学号"));
	ui->swapStudent1Spin->setAccessibleName(QStringLiteral("交换学生一姓名或学号"));
	ui->swapStudent2Spin->setAccessibleName(QStringLiteral("交换学生二姓名或学号"));
	ui->assignStrategyCombo->setAccessibleName(QStringLiteral("入住安置方式"));
	ui->assignBuildingSpin->setAccessibleName(QStringLiteral("入住目标楼栋"));
	ui->assignDormSpin->setAccessibleName(QStringLiteral("入住目标宿舍"));
	ui->assignBedSpin->setAccessibleName(QStringLiteral("入住目标床位"));
	ui->moveBuildingCombo->setAccessibleName(QStringLiteral("调宿目标楼栋"));
	ui->moveDormCombo->setAccessibleName(QStringLiteral("调宿目标宿舍"));
	ui->moveBedCombo->setAccessibleName(QStringLiteral("调宿目标床位"));
	ui->moveResetSourceGenderCheck->setAccessibleName(QStringLiteral("调离后解除原宿舍性别锁"));
	ui->moveBuildingCombo->setToolTip(QStringLiteral("仅显示接纳当前学生性别且存在可用宿舍的楼栋。"));
	ui->moveDormCombo->setToolTip(QStringLiteral("仅显示接纳当前学生性别且尚未住满的宿舍。"));
	ui->moveBedCombo->setToolTip(QStringLiteral("跨宿舍可自动选择最小空床位；同宿舍换床必须选择其他空床。"));
	connect(ui->assignStrategyCombo, &QComboBox::currentIndexChanged, this, [this]() {
		update_assign_controls();
		refresh_assign_buildings();
	});
	connect(ui->assignStudentSpin->lineEdit(), &QLineEdit::textEdited, this, [this](const QString& text) {
		handle_student_search_text(ui->assignStudentSpin, text);
	});
	connect(ui->assignStudentSpin, &QComboBox::currentIndexChanged, this, [this]() { refresh_assign_buildings(); });
	connect(ui->assignBuildingSpin, &QComboBox::currentIndexChanged, this, [this]() { refresh_assign_dorms(); });
	connect(ui->assignDormSpin, &QComboBox::currentIndexChanged, this, [this]() { refresh_assign_beds(); });
	connect(ui->assignBedSpin, &QComboBox::currentIndexChanged, this, [this]() { update_assign_preview(); });
	connect(ui->assignSubmitButton, &QPushButton::clicked, this, &AccommodationPage::submit_assignment);
	connect(ui->removeStudentSpin->lineEdit(), &QLineEdit::textEdited, this, [this](const QString& text) {
		handle_student_search_text(ui->removeStudentSpin, text);
	});
	connect(ui->removeStudentSpin, &QComboBox::currentIndexChanged, this, [this]() { update_remove_preview(); });
	connect(ui->removeSubmitButton, &QPushButton::clicked, this, &AccommodationPage::submit_remove);
	connect(ui->moveStudentSpin->lineEdit(), &QLineEdit::textEdited, this, [this](const QString& text) {
		handle_student_search_text(ui->moveStudentSpin, text);
	});
	connect(ui->moveStudentSpin, &QComboBox::currentIndexChanged, this, [this]() { refresh_move_buildings(); });
	connect(ui->moveBuildingCombo, &QComboBox::currentIndexChanged, this, [this]() { refresh_move_dorms(); });
	connect(ui->moveDormCombo, &QComboBox::currentIndexChanged, this, [this]() { refresh_move_beds(); });
	connect(ui->moveBedCombo, &QComboBox::currentIndexChanged, this, [this]() { update_move_preview(); });
	connect(ui->moveSubmitButton, &QPushButton::clicked, this, &AccommodationPage::submit_move);
	connect(ui->swapStudent1Spin->lineEdit(), &QLineEdit::textEdited, this, [this](const QString& text) {
		handle_student_search_text(ui->swapStudent1Spin, text);
	});
	connect(ui->swapStudent1Spin, &QComboBox::currentIndexChanged, this, [this]() {
		const int second_id = selected_student_id(ui->swapStudent2Spin);
		populate_student_selector(ui->swapStudent2Spin, school::instance().get_assigned_student_ids(),
			selected_student_id(ui->swapStudent1Spin));
		select_student(ui->swapStudent2Spin, second_id);
		update_swap_preview();
	});
	connect(ui->swapStudent2Spin->lineEdit(), &QLineEdit::textEdited, this, [this](const QString& text) {
		handle_student_search_text(ui->swapStudent2Spin, text);
	});
	connect(ui->swapStudent2Spin, &QComboBox::currentIndexChanged, this, [this]() { update_swap_preview(); });
	connect(ui->swapSubmitButton, &QPushButton::clicked, this, &AccommodationPage::submit_swap);
	refresh_student_selectors();
	update_assign_controls();
	refresh_assign_buildings();
	update_remove_preview();
	refresh_move_buildings();
	update_swap_preview();
	setTabOrder(ui->taskTabs, ui->assignStudentSpin);
	setTabOrder(ui->assignStudentSpin, ui->assignStrategyCombo);
	setTabOrder(ui->assignStrategyCombo, ui->assignBuildingSpin);
	setTabOrder(ui->assignBuildingSpin, ui->assignDormSpin);
	setTabOrder(ui->assignDormSpin, ui->assignBedSpin);
	setTabOrder(ui->assignBedSpin, ui->assignSubmitButton);
	setTabOrder(ui->moveStudentSpin, ui->moveBuildingCombo);
	setTabOrder(ui->moveBuildingCombo, ui->moveDormCombo);
	setTabOrder(ui->moveDormCombo, ui->moveBedCombo);
	setTabOrder(ui->moveBedCombo, ui->moveResetSourceGenderCheck);
	setTabOrder(ui->moveResetSourceGenderCheck, ui->moveSubmitButton);
}

bool AccommodationPage::eventFilter(QObject* watched, QEvent* event)
{
	if (event->type() == QEvent::InputMethod)
	{
		QComboBox* combo = student_selector_for_editor(watched);
		if (combo != nullptr)
		{
			auto* input_method_event = static_cast<QInputMethodEvent*>(event);
			QLineEdit* editor = combo->lineEdit();
			editor->setProperty("studentSearchImeEventActive", true);
			editor->setProperty("studentSearchImeComposing", !input_method_event->preeditString().isEmpty());
			const QPointer<QComboBox> guarded_combo(combo);
			QTimer::singleShot(0, this, [this, guarded_combo] {
				if (guarded_combo == nullptr || guarded_combo->lineEdit() == nullptr) return;
				QLineEdit* guarded_editor = guarded_combo->lineEdit();
				guarded_editor->setProperty("studentSearchImeEventActive", false);
				if (!guarded_editor->property("studentSearchImeComposing").toBool())
					handle_student_search_text(guarded_combo, guarded_editor->text());
			});
		}
	}
	return QWidget::eventFilter(watched, event);
}

QComboBox* AccommodationPage::student_selector_for_editor(QObject* editor) const
{
	for (QComboBox* combo : {ui->assignStudentSpin, ui->removeStudentSpin, ui->moveStudentSpin,
		ui->swapStudent1Spin, ui->swapStudent2Spin})
	{
		if (combo->lineEdit() == editor) return combo;
	}
	return nullptr;
}

void AccommodationPage::handle_student_search_text(QComboBox* combo, const QString& search_text)
{
	if (combo == nullptr || combo->lineEdit() == nullptr) return;
	QLineEdit* editor = combo->lineEdit();
	if (editor->property("studentSearchImeEventActive").toBool()
		|| editor->property("studentSearchImeComposing").toBool())
		return;
	filter_student_selector(combo, search_text);
	refresh_after_student_search(combo);
}

void AccommodationPage::refresh_after_student_search(QComboBox* combo)
{
	if (combo == ui->assignStudentSpin) refresh_assign_buildings();
	else if (combo == ui->removeStudentSpin) update_remove_preview();
	else if (combo == ui->moveStudentSpin) refresh_move_buildings();
	else if (combo == ui->swapStudent1Spin || combo == ui->swapStudent2Spin) update_swap_preview();
}

void AccommodationPage::refresh_move_buildings()//按学生性别与可用目标刷新楼栋下拉框
{
	{
		const QSignalBlocker unlock_blocker(ui->moveResetSourceGenderCheck);
		ui->moveResetSourceGenderCheck->setChecked(false);
		ui->moveResetSourceGenderCheck->setEnabled(false);
	}
	const QSignalBlocker blocker(ui->moveBuildingCombo);
	ui->moveBuildingCombo->clear();
	const student* current_student = school::instance().get_student(selected_student_id(ui->moveStudentSpin));
	if (current_student != nullptr && has_consistent_accommodation(*current_student)
		&& current_student->get_gender() >= 1 && current_student->get_gender() <= 2) {
		for (int building_id : school::instance().get_all_building_ids()) {
			const building* current_building = school::instance().get_building(building_id);
			if (current_building == nullptr || !current_building->accepts_gender(current_student->get_gender()))
				continue;
			bool has_available_dorm = false;
			for (const QPair<int, int>& dorm_key : school::instance().get_dorm_keys_of_building(building_id)) {
				const dorm* candidate = school::instance().get_dorm(dorm_key.first, dorm_key.second);
				if (candidate != nullptr && candidate->accepts_gender(current_student->get_gender()) && !candidate->is_full()) {
					has_available_dorm = true;
					break;
				}
			}
			if (has_available_dorm)
				ui->moveBuildingCombo->addItem(QStringLiteral("%1号楼").arg(building_id), building_id);
		}
	}
	ui->moveBuildingCombo->setEnabled(ui->moveBuildingCombo->count() > 0);
	refresh_move_dorms();
}

void AccommodationPage::refresh_move_dorms()//按目标楼栋刷新可用宿舍下拉框
{
	const QSignalBlocker blocker(ui->moveDormCombo);
	ui->moveDormCombo->clear();
	const student* current_student = school::instance().get_student(selected_student_id(ui->moveStudentSpin));
	const int building_id = ui->moveBuildingCombo->currentData().toInt();
	if (current_student != nullptr && building_id > 0) {
		for (const QPair<int, int>& dorm_key : school::instance().get_dorm_keys_of_building(building_id)) {
			const dorm* candidate = school::instance().get_dorm(dorm_key.first, dorm_key.second);
			if (candidate == nullptr || !candidate->accepts_gender(current_student->get_gender()) || candidate->is_full())
				continue;
			ui->moveDormCombo->addItem(QStringLiteral("%1室 · %2个空床")
				.arg(candidate->get_id()).arg(candidate->get_empty_count()), candidate->get_id());
		}
	}
	ui->moveDormCombo->setEnabled(ui->moveDormCombo->count() > 0);
	refresh_move_beds();
}

void AccommodationPage::refresh_move_beds()//按目标宿舍刷新床位下拉框
{
	const QSignalBlocker blocker(ui->moveBedCombo);
	ui->moveBedCombo->clear();
	const student* current_student = school::instance().get_student(selected_student_id(ui->moveStudentSpin));
	const int building_id = ui->moveBuildingCombo->currentData().toInt();
	const int dorm_id = ui->moveDormCombo->currentData().toInt();
	const dorm* target = school::instance().get_dorm(building_id, dorm_id);
	if (current_student != nullptr && target != nullptr) {
		const bool same_dorm = building_id == current_student->get_building_id() && dorm_id == current_student->get_dorm_id();
		if (!same_dorm)
			ui->moveBedCombo->addItem(QStringLiteral("自动选择最小空床位"), 0);
		for (int bed_id = 1; bed_id <= target->get_max_num(); ++bed_id) {
			if (target->is_bed_occupied(bed_id) == 0)
				ui->moveBedCombo->addItem(QStringLiteral("%1号床").arg(bed_id), bed_id);
		}
	}
	ui->moveBedCombo->setEnabled(ui->moveBedCombo->count() > 0);
	update_move_preview();
}

AccommodationPage::~AccommodationPage()
{
	delete ui;
}

void AccommodationPage::refresh_student_selectors()
{
	const int assign_id = selected_student_id(ui->assignStudentSpin);
	const int remove_id = selected_student_id(ui->removeStudentSpin);
	const int move_id = selected_student_id(ui->moveStudentSpin);
	const int swap1_id = selected_student_id(ui->swapStudent1Spin);
	const int swap2_id = selected_student_id(ui->swapStudent2Spin);
	const QList<int> assigned_ids = school::instance().get_assigned_student_ids();
	QSet<int> assigned_set;
	for (int student_id : assigned_ids) {
		assigned_set.insert(student_id);
	}
	QList<int> unassigned_ids;
	for (int student_id : school::instance().get_all_student_ids()) {
		if (!assigned_set.contains(student_id)) {
			unassigned_ids.append(student_id);
		}
	}

	populate_student_selector(ui->assignStudentSpin, unassigned_ids);
	populate_student_selector(ui->removeStudentSpin, assigned_ids);
	populate_student_selector(ui->moveStudentSpin, assigned_ids);
	populate_student_selector(ui->swapStudent1Spin, assigned_ids);
	select_student(ui->assignStudentSpin, assign_id);
	select_student(ui->removeStudentSpin, remove_id);
	select_student(ui->moveStudentSpin, move_id);
	select_student(ui->swapStudent1Spin, swap1_id);
	populate_student_selector(ui->swapStudent2Spin, assigned_ids, selected_student_id(ui->swapStudent1Spin));
	select_student(ui->swapStudent2Spin, swap2_id);
}

void AccommodationPage::populate_student_selector(QComboBox* combo, const QList<int>& student_ids, int excluded_student_id)
{
	QList<int> available_ids;
	for (int student_id : student_ids) {
		if (student_id != excluded_student_id && school::instance().get_student(student_id) != nullptr) {
			available_ids.append(student_id);
		}
	}
	student_selector_ids.insert(combo, available_ids);
	filter_student_selector(combo, QString());
}

void AccommodationPage::filter_student_selector(QComboBox* combo, const QString& search_text)
{
	const QString trimmed_search = search_text.trimmed();
	int match_count = 0;
	{
		const QSignalBlocker blocker(combo);
		combo->clear();
		for (int student_id : student_selector_ids.value(combo)) {
			const student* current_student = school::instance().get_student(student_id);
			if (current_student == nullptr) {
				continue;
			}
			const QString item_text = student_selector_text(*current_student);
			if (!trimmed_search.isEmpty()
				&& !current_student->get_name().contains(trimmed_search, Qt::CaseInsensitive)
				&& !QString::number(student_id).contains(trimmed_search)) {
				continue;
			}
			combo->addItem(item_text, student_id);
			++match_count;
		}
		if (!trimmed_search.isEmpty() && match_count == 0) {
			combo->addItem(QStringLiteral("没有匹配的学生"), 0);
			if (auto* model = qobject_cast<QStandardItemModel*>(combo->model())) {
				model->item(0)->setEnabled(false);
			}
		}
		combo->setCurrentIndex(-1);
		combo->lineEdit()->setText(search_text);
		combo->lineEdit()->setCursorPosition(search_text.size());
	}
	if (trimmed_search.isEmpty()) {
		combo->hidePopup();
	} else if (combo->isVisible() && combo->lineEdit()->hasFocus()) {
		combo->showPopup();
	}
}

int AccommodationPage::selected_student_id(const QComboBox* combo) const
{
	const int index = combo->currentIndex();
	if (index < 0 || combo->currentText() != combo->itemText(index)) {
		return 0;
	}
	return combo->itemData(index).toInt();
}

void AccommodationPage::select_student(QComboBox* combo, int student_id)
{
	const int index = combo->findData(student_id);
	combo->setCurrentIndex(index);
	if (index < 0) {
		combo->lineEdit()->clear();
	}
}

void AccommodationPage::refresh_data()
{
	refresh_student_selectors();
	refresh_assign_buildings();
	update_remove_preview();
	refresh_move_buildings();
	update_swap_preview();
}

void AccommodationPage::prepare_student_task(int student_id, bool assigned)
{
	if (assigned) {
		ui->taskTabs->setCurrentWidget(ui->moveTab);
		refresh_student_selectors();
		select_student(ui->moveStudentSpin, student_id);
		refresh_move_buildings();
	} else {
		ui->taskTabs->setCurrentWidget(ui->assignTab);
		refresh_student_selectors();
		select_student(ui->assignStudentSpin, student_id);
		refresh_assign_buildings();
	}
}

void AccommodationPage::showEvent(QShowEvent* event)
{
	QWidget::showEvent(event);
	refresh_data();
}

void AccommodationPage::update_assign_controls()
{
	const int strategy = ui->assignStrategyCombo->currentIndex();
	const bool specified_dorm = strategy == 2;
	ui->assignBuildingSpin->setEnabled(specified_dorm && ui->assignBuildingSpin->count() > 0);
	ui->assignDormSpin->setEnabled(specified_dorm && ui->assignDormSpin->count() > 0);
	ui->assignBedSpin->setEnabled(specified_dorm && ui->assignBedSpin->count() > 0);
}

void AccommodationPage::refresh_assign_buildings()
{
	const QSignalBlocker blocker(ui->assignBuildingSpin);
	ui->assignBuildingSpin->clear();
	const int strategy = ui->assignStrategyCombo->currentIndex();
	const student* current_student = school::instance().get_student(selected_student_id(ui->assignStudentSpin));
	if (strategy == 2 && current_student != nullptr
		&& current_student->get_gender() >= 1 && current_student->get_gender() <= 2) {
		for (int building_id : school::instance().get_all_building_ids()) {
			const building* current_building = school::instance().get_building(building_id);
			if (current_building == nullptr || !current_building->accepts_gender(current_student->get_gender())) {
				continue;
			}
			bool has_available_dorm = false;
			for (const QPair<int, int>& dorm_key : school::instance().get_dorm_keys_of_building(building_id)) {
				const dorm* candidate = school::instance().get_dorm(dorm_key.first, dorm_key.second);
				if (candidate != nullptr && candidate->accepts_gender(current_student->get_gender()) && !candidate->is_full()) {
					has_available_dorm = true;
					break;
				}
			}
			if (has_available_dorm) {
				ui->assignBuildingSpin->addItem(QStringLiteral("%1号楼").arg(building_id), building_id);
			}
		}
	}
	ui->assignBuildingSpin->setEnabled(strategy == 2 && ui->assignBuildingSpin->count() > 0);
	refresh_assign_dorms();
}

void AccommodationPage::refresh_assign_dorms()
{
	const QSignalBlocker blocker(ui->assignDormSpin);
	ui->assignDormSpin->clear();
	const int strategy = ui->assignStrategyCombo->currentIndex();
	const student* current_student = school::instance().get_student(selected_student_id(ui->assignStudentSpin));
	const int building_id = ui->assignBuildingSpin->currentData().toInt();
	if (strategy == 2 && current_student != nullptr && building_id > 0) {
		for (const QPair<int, int>& dorm_key : school::instance().get_dorm_keys_of_building(building_id)) {
			const dorm* candidate = school::instance().get_dorm(dorm_key.first, dorm_key.second);
			if (candidate == nullptr || !candidate->accepts_gender(current_student->get_gender()) || candidate->is_full()) {
				continue;
			}
			ui->assignDormSpin->addItem(QStringLiteral("%1室 · %2 · 空床 %3/%4")
				.arg(candidate->get_id())
				.arg(candidate->get_for_gender() == 1 ? QStringLiteral("男舍")
					: (candidate->get_for_gender() == 2 ? QStringLiteral("女舍") : QStringLiteral("性别锁未设置")))
				.arg(candidate->get_empty_count())
				.arg(candidate->get_max_num()), candidate->get_id());
		}
	}
	ui->assignDormSpin->setEnabled(strategy == 2 && ui->assignDormSpin->count() > 0);
	refresh_assign_beds();
}

void AccommodationPage::refresh_assign_beds()
{
	const QSignalBlocker blocker(ui->assignBedSpin);
	ui->assignBedSpin->clear();
	const int strategy = ui->assignStrategyCombo->currentIndex();
	const int building_id = ui->assignBuildingSpin->currentData().toInt();
	const int dorm_id = ui->assignDormSpin->currentData().toInt();
	const dorm* target = school::instance().get_dorm(building_id, dorm_id);
	if (strategy == 2 && target != nullptr) {
		ui->assignBedSpin->addItem(QStringLiteral("自动选择最小空床位"), 0);
		for (int bed_id = 1; bed_id <= target->get_max_num(); ++bed_id) {
			if (target->is_bed_occupied(bed_id) == 0) {
				ui->assignBedSpin->addItem(QStringLiteral("%1号床").arg(bed_id), bed_id);
			}
		}
	}
	ui->assignBedSpin->setEnabled(strategy == 2 && ui->assignBedSpin->count() > 0);
	update_assign_preview();
}

void AccommodationPage::update_assign_preview()
{
	ui->assignSubmitButton->setEnabled(false);
	const int student_id = selected_student_id(ui->assignStudentSpin);
	const student* current_student = school::instance().get_student(student_id);
	bool student_ready = false;
	if (!check::is_valid_student_id(student_id) || current_student == nullptr) {
		ui->assignStudentPreviewLabel->setText(QStringLiteral("请输入姓名或学号，并从匹配结果中选择一名未入住学生。"));
	} else if (school::instance().get_assigned_student_ids().contains(student_id) && has_consistent_accommodation(*current_student)) {
		ui->assignStudentPreviewLabel->setText(QStringLiteral("%1\n当前已入住：%2")
			.arg(accommodation_student_text(*current_student), accommodation_position_text(*current_student)));
	} else if (has_any_accommodation(*current_student)) {
		ui->assignStudentPreviewLabel->setText(QStringLiteral("%1\n住宿记录异常，不能办理入住。")
			.arg(accommodation_student_text(*current_student)));
		ui->assignSubmitButton->setEnabled(false);
	} else {
		ui->assignStudentPreviewLabel->setText(QStringLiteral("%1\n当前未入住，可办理入住。")
			.arg(accommodation_student_text(*current_student)));
		student_ready = current_student->get_gender() == 1 || current_student->get_gender() == 2;
	}

	const int strategy = ui->assignStrategyCombo->currentIndex();
	if (strategy == 0) {
		if (current_student == nullptr || current_student->get_gender() < 1 || current_student->get_gender() > 2) {
			ui->assignTargetPreviewLabel->setText(QStringLiteral("需要选择已设置有效性别的学生，系统才能推荐宿舍。"));
			return;
		}
		if (school::instance().get_assigned_student_ids().contains(student_id)) {
			ui->assignTargetPreviewLabel->setText(QStringLiteral("该学生已经入住，请使用调宿或换床流程。"));
			return;
		}
		if (has_any_accommodation(*current_student)) {
			ui->assignTargetPreviewLabel->setText(QStringLiteral("学生住宿记录异常，不能推荐新宿舍。"));
			return;
		}
		const dorm* target = school::instance().get_available_dorm(current_student->get_gender());
		ui->assignTargetPreviewLabel->setText(target == nullptr
			? QStringLiteral("当前没有接纳该性别的可用宿舍。")
			: QStringLiteral("系统推荐：%1号楼 · %2室 · 自动选择最小空床位。")
				.arg(target->get_building_id()).arg(target->get_id()));
		ui->assignSubmitButton->setEnabled(student_ready && target != nullptr);
		return;
	}
	if (strategy == 1) {
		const dorm* available = student_ready ? school::instance().get_available_dorm(current_student->get_gender()) : nullptr;
		ui->assignTargetPreviewLabel->setText(available == nullptr
			? QStringLiteral("当前没有接纳该学生性别的可用宿舍。")
			: QStringLiteral("系统将在所有接纳该学生性别的可用宿舍中随机选择。"));
		ui->assignSubmitButton->setEnabled(student_ready && available != nullptr);
		return;
	}
	if (strategy == 2) {
		const dorm* target = school::instance().get_dorm(
			ui->assignBuildingSpin->currentData().toInt(), ui->assignDormSpin->currentData().toInt());
		if (target == nullptr) {
			ui->assignTargetPreviewLabel->setText(QStringLiteral("当前没有可选择的目标宿舍。"));
			return;
		}
		const building* target_building = school::instance().get_building(target->get_building_id());
		const bool accepts_gender = student_ready && target_building != nullptr
			&& target_building->accepts_gender(current_student->get_gender())
			&& target->accepts_gender(current_student->get_gender());
		const int target_bed = ui->assignBedSpin->currentData().toInt();
		const bool bed_available = target_bed == 0
			? !target->is_full()
			: target->is_bed_occupied(target_bed) == 0;
		ui->assignTargetPreviewLabel->setText(target_bed > 0
			? QStringLiteral("目标：%1号楼 · %2室 · %3号床。").arg(target->get_building_id()).arg(target->get_id()).arg(target_bed)
			: QStringLiteral("目标：%1号楼 · %2室 · 自动选择最小空床位。").arg(target->get_building_id()).arg(target->get_id()));
		ui->assignSubmitButton->setEnabled(accepts_gender && bed_available);
	}
}

void AccommodationPage::submit_assignment()
{
	const int student_id = selected_student_id(ui->assignStudentSpin);
	const student* current_student = school::instance().get_student(student_id);
	if (!check::is_valid_student_id(student_id) || current_student == nullptr) {
		uifeedback::show_error(this, QStringLiteral("无法办理入住"), QStringLiteral("请通过姓名或学号搜索并选择一名学生。"));
		return;
	}
	if (school::instance().get_assigned_student_ids().contains(student_id)) {
		if (!has_consistent_accommodation(*current_student)) {
			uifeedback::show_critical(this, QStringLiteral("住宿记录异常"), QStringLiteral("学生位置与宿舍床位记录不一致，请暂停相关操作并核查数据。"));
		} else {
			uifeedback::show_error(this, QStringLiteral("无法办理入住"), QStringLiteral("该学生已经入住，请使用调宿或换床流程。"));
		}
		return;
	}
	if (has_any_accommodation(*current_student)) {
		uifeedback::show_critical(this, QStringLiteral("住宿记录异常"), QStringLiteral("学生存在部分住宿位置字段，系统不会建立新的床位记录。请暂停相关操作并核查数据。"));
		return;
	}
	if (current_student->get_gender() < 1 || current_student->get_gender() > 2) {
		uifeedback::show_error(this, QStringLiteral("无法办理入住"), QStringLiteral("该学生尚未设置有效性别，请先在学生管理中纠正。"));
		return;
	}
	const int strategy = ui->assignStrategyCombo->currentIndex();
	const int target_building_id = ui->assignBuildingSpin->currentData().toInt();
	const int target_dorm_id = ui->assignDormSpin->currentData().toInt();
	const int target_bed_id = ui->assignBedSpin->currentData().toInt();
	QString target_description;
	if (strategy == 2) {
		const dorm* target = school::instance().get_dorm(target_building_id, target_dorm_id);
		if (target == nullptr) {
			uifeedback::show_error(this, QStringLiteral("无法办理入住"), QStringLiteral("目标宿舍不存在。"));
			return;
		}
		const building* target_building = school::instance().get_building(target->get_building_id());
		if (target_building == nullptr || !target_building->accepts_gender(current_student->get_gender()) || !target->accepts_gender(current_student->get_gender())) {
			uifeedback::show_error(this, QStringLiteral("无法办理入住"), QStringLiteral("目标楼栋或宿舍不接纳该学生性别。"));
			return;
		}
		if (target->is_full()) {
			uifeedback::show_error(this, QStringLiteral("无法办理入住"), QStringLiteral("目标宿舍已经住满。"));
			return;
		}
		if (target_bed_id > 0) {
			const int occupied = target->is_bed_occupied(target_bed_id);
			if (occupied < 0) {
				uifeedback::show_error(this, QStringLiteral("无法办理入住"), QStringLiteral("目标床位号无效。"));
				return;
			}
			if (occupied == 1) {
				uifeedback::show_error(this, QStringLiteral("无法办理入住"), QStringLiteral("目标床位已经被占用。"));
				return;
			}
		}
		target_description = target_bed_id > 0
			? QStringLiteral("%1号楼 %2室 %3号床").arg(target->get_building_id()).arg(target->get_id()).arg(target_bed_id)
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
	} else if (target_bed_id == 0) {
		result = school::instance().assign_student_to_dorm(target_building_id, target_dorm_id, student_id);
	} else {
		result = school::instance().assign_student_to_dorm(target_building_id, target_dorm_id, student_id, target_bed_id);
	}
	if (result <= 0) {
		show_assignment_error(result);
		update_assign_preview();
		return;
	}
	const student* assigned_student = school::instance().get_student(student_id);
	const QString result_position = assigned_student == nullptr ? QStringLiteral("床位号 %1").arg(result) : accommodation_position_text(*assigned_student);
	uifeedback::show_success(this, QStringLiteral("入住办理成功：%1").arg(result_position));
	refresh_data();
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

void AccommodationPage::update_remove_preview()
{
	ui->removeSubmitButton->setEnabled(false);
	const int student_id = selected_student_id(ui->removeStudentSpin);
	const student* current_student = school::instance().get_student(student_id);
	if (!check::is_valid_student_id(student_id) || current_student == nullptr) {
		ui->removePreviewLabel->setText(QStringLiteral("请输入姓名或学号，并从匹配结果中选择一名已入住学生。退宿仅释放床位并保留学籍。"));
		return;
	}
	const bool assigned = school::instance().get_assigned_student_ids().contains(student_id);
	if (assigned && has_consistent_accommodation(*current_student)) {
		ui->removePreviewLabel->setText(QStringLiteral("%1\n\n当前住宿：%2\n退宿后学生档案将继续保留。")
			.arg(accommodation_student_text(*current_student), accommodation_position_text(*current_student)));
		ui->removeSubmitButton->setEnabled(true);
	} else if (has_any_accommodation(*current_student)) {
		ui->removePreviewLabel->setText(QStringLiteral("%1\n\n住宿位置字段不完整，不能办理退宿。")
			.arg(accommodation_student_text(*current_student)));
		ui->removeSubmitButton->setEnabled(false);
	} else {
		ui->removePreviewLabel->setText(QStringLiteral("%1\n\n当前未入住，无需办理退宿。")
			.arg(accommodation_student_text(*current_student)));
	}
}

void AccommodationPage::submit_remove()
{
	const int student_id = selected_student_id(ui->removeStudentSpin);
	const student* current_student = school::instance().get_student(student_id);
	if (!check::is_valid_student_id(student_id) || current_student == nullptr) {
		uifeedback::show_error(this, QStringLiteral("无法办理退宿"), QStringLiteral("请通过姓名或学号搜索并选择一名已入住学生。"));
		return;
	}
	const bool assigned = school::instance().get_assigned_student_ids().contains(student_id);
	const bool consistent_position = has_consistent_accommodation(*current_student);
	if (assigned != consistent_position || (!assigned && has_any_accommodation(*current_student))) {
		uifeedback::show_critical(this, QStringLiteral("住宿记录异常"), QStringLiteral("学生住宿位置字段不完整，请暂停相关操作并核查数据。"));
		return;
	}
	if (!assigned) {
		uifeedback::show_error(this, QStringLiteral("无需办理退宿"), QStringLiteral("该学生当前未入住。"));
		return;
	}
	const QString original_position = accommodation_position_text(*current_student);
	if (!uifeedback::confirm_action(this, QStringLiteral("确认办理退宿"),
		QStringLiteral("%1\n\n将释放：%2\n学生档案将继续保留。").arg(accommodation_student_text(*current_student), original_position),
		QStringLiteral("确认退宿"))) {
		return;
	}

	const int result = school::instance().remove_student_from_dorm(student_id);
	if (result > 0) {
		uifeedback::show_success(this, QStringLiteral("退宿办理成功：已释放 %1。").arg(original_position));
		refresh_data();
		return;
	}
	if (result == -8) {
		uifeedback::show_critical(this, QStringLiteral("住宿记录异常"), QStringLiteral("学生位置与宿舍床位记录不一致，请暂停相关操作并核查数据。"), QStringLiteral("业务返回值：-8"));
		return;
	}
	QString message;
	if (result == 0) message = QStringLiteral("该学生当前未入住。");
	else if (result == -6) message = QStringLiteral("学生已经不存在，请刷新后重试。");
	else message = QStringLiteral("学生学号参数无效。");
	uifeedback::show_error(this, QStringLiteral("无法办理退宿"), message, QStringLiteral("业务返回值：%1").arg(result));
	update_remove_preview();
}

void AccommodationPage::update_move_preview()
{
	ui->moveSubmitButton->setEnabled(false);
	const int student_id = selected_student_id(ui->moveStudentSpin);
	const student* current_student = school::instance().get_student(student_id);
	if (!check::is_valid_student_id(student_id) || current_student == nullptr) {
		ui->moveResetSourceGenderCheck->setChecked(false);
		ui->moveResetSourceGenderCheck->setEnabled(false);
		ui->movePreviewLabel->setText(QStringLiteral("请输入姓名或学号，并从匹配结果中选择一名已入住学生。"));
		return;
	}
	const bool assigned = school::instance().get_assigned_student_ids().contains(student_id);
	if (!assigned || !has_consistent_accommodation(*current_student)) {
		ui->moveResetSourceGenderCheck->setChecked(false);
		ui->moveResetSourceGenderCheck->setEnabled(false);
		ui->movePreviewLabel->setText(has_any_accommodation(*current_student)
			? QStringLiteral("%1\n\n住宿位置记录异常，不能办理调宿。").arg(accommodation_student_text(*current_student))
			: QStringLiteral("%1\n\n当前未入住，请先办理入住。").arg(accommodation_student_text(*current_student)));
		if (has_any_accommodation(*current_student)) {
			ui->moveSubmitButton->setEnabled(false);
		}
		return;
	}
	const int target_building_id = ui->moveBuildingCombo->currentData().toInt();
	const int target_dorm_id = ui->moveDormCombo->currentData().toInt();
	const dorm* target = school::instance().get_dorm(target_building_id, target_dorm_id);
	if (target == nullptr) {
		ui->moveResetSourceGenderCheck->setChecked(false);
		ui->moveResetSourceGenderCheck->setEnabled(false);
		ui->movePreviewLabel->setText(QStringLiteral("%1\n\n当前位置：%2\n当前没有符合该学生性别与空床条件的目标宿舍。")
			.arg(accommodation_student_text(*current_student), accommodation_position_text(*current_student)));
		return;
	}
	const int target_bed_id = ui->moveBedCombo->currentData().toInt();
	const QString target_text = target_bed_id > 0
		? QStringLiteral("%1号楼 · %2室 · %3号床").arg(target->get_building_id()).arg(target->get_id()).arg(target_bed_id)
		: QStringLiteral("%1号楼 · %2室 · 自动选择最小空床位").arg(target->get_building_id()).arg(target->get_id());
	ui->movePreviewLabel->setText(QStringLiteral("%1\n\n当前位置：%2\n目标位置：%3")
		.arg(accommodation_student_text(*current_student), accommodation_position_text(*current_student), target_text));
	const building* target_building = school::instance().get_building(target->get_building_id());
	const bool same_dorm = target->get_building_id() == current_student->get_building_id()
		&& target->get_id() == current_student->get_dorm_id();
	const bool can_reset_source_gender = can_reset_source_gender_after_move(*current_student,
		target->get_building_id(), target->get_id());
	if (!can_reset_source_gender) ui->moveResetSourceGenderCheck->setChecked(false);
	ui->moveResetSourceGenderCheck->setEnabled(can_reset_source_gender);
	const bool valid_same_dorm_target = !same_dorm
		|| (target_bed_id > 0 && target_bed_id != current_student->get_bed_id());
	const bool accepts_gender = target_building != nullptr
		&& target_building->accepts_gender(current_student->get_gender())
		&& target->accepts_gender(current_student->get_gender());
	const bool bed_available = target_bed_id > 0
		? target->is_bed_occupied(target_bed_id) == 0
		: !target->is_full();
	ui->moveSubmitButton->setEnabled(valid_same_dorm_target && accepts_gender && bed_available);
}

bool AccommodationPage::can_reset_source_gender_after_move(const student& current_student,
	int target_building_id, int target_dorm_id) const
{
	if (!has_consistent_accommodation(current_student)
		|| (current_student.get_building_id() == target_building_id && current_student.get_dorm_id() == target_dorm_id))
		return false;
	const building* source_building = school::instance().get_building(current_student.get_building_id());
	const dorm* source_dorm = school::instance().get_dorm(current_student.get_building_id(), current_student.get_dorm_id());
	return source_building != nullptr && source_building->get_for_gender() == 3
		&& source_dorm != nullptr && source_dorm->get_current_num() == 1
		&& (source_dorm->get_for_gender() == 1 || source_dorm->get_for_gender() == 2);
}

void AccommodationPage::submit_move()
{
	const int student_id = selected_student_id(ui->moveStudentSpin);
	const student* current_student = school::instance().get_student(student_id);
	if (!check::is_valid_student_id(student_id) || current_student == nullptr) {
		uifeedback::show_error(this, QStringLiteral("无法办理调宿"), QStringLiteral("请通过姓名或学号搜索并选择一名已入住学生。"));
		return;
	}
	const bool assigned = school::instance().get_assigned_student_ids().contains(student_id);
	const bool consistent_position = has_consistent_accommodation(*current_student);
	if (assigned != consistent_position || (!assigned && has_any_accommodation(*current_student))) {
		uifeedback::show_critical(this, QStringLiteral("住宿记录异常"), QStringLiteral("学生住宿位置字段不完整，请暂停相关操作并核查数据。"));
		return;
	}
	if (!assigned) {
		uifeedback::show_error(this, QStringLiteral("无法办理调宿"), QStringLiteral("该学生当前未入住，请先办理入住。"));
		return;
	}
	const int selected_building_id = ui->moveBuildingCombo->currentData().toInt();
	const int selected_dorm_id = ui->moveDormCombo->currentData().toInt();
	const dorm* target = school::instance().get_dorm(selected_building_id, selected_dorm_id);
	if (target == nullptr) {
		uifeedback::show_error(this, QStringLiteral("无法办理调宿"), QStringLiteral("目标宿舍不存在。"));
		return;
	}
	const int target_building_id = target->get_building_id();
	const int target_dorm_id = target->get_id();
	const int target_bed_id = ui->moveBedCombo->currentData().toInt();
	const bool reset_source_gender = ui->moveResetSourceGenderCheck->isChecked();
	if (reset_source_gender && !can_reset_source_gender_after_move(*current_student, target_building_id, target_dorm_id)) {
		uifeedback::show_error(this, QStringLiteral("无法解除原宿舍性别锁"),
			QStringLiteral("只有从混宿楼的单人宿舍调往其它宿舍时，才能在学生调离后解除原宿舍性别锁。"));
		return;
	}
	if (target_building_id == current_student->get_building_id() && target_dorm_id == current_student->get_dorm_id()) {
		if (target_bed_id == 0) {
			uifeedback::show_error(this, QStringLiteral("无法办理换床"), QStringLiteral("同宿舍换床必须指定一个不同的目标床位。"));
			return;
		}
		if (target_bed_id == current_student->get_bed_id()) {
			uifeedback::show_error(this, QStringLiteral("无需调整"), QStringLiteral("目标床位就是学生当前床位。"));
			return;
		}
	}
	const building* target_building = school::instance().get_building(target_building_id);
	if (target_building == nullptr || !target_building->accepts_gender(current_student->get_gender()) || !target->accepts_gender(current_student->get_gender())) {
		uifeedback::show_error(this, QStringLiteral("无法办理调宿"), QStringLiteral("目标楼栋或宿舍不接纳该学生性别。"));
		return;
	}
	if (target_bed_id == 0 && target->is_full()) {
		uifeedback::show_error(this, QStringLiteral("无法办理调宿"), QStringLiteral("目标宿舍已经住满。"));
		return;
	}
	if (target_bed_id > 0) {
		const int occupied = target->is_bed_occupied(target_bed_id);
		if (occupied < 0) {
			uifeedback::show_error(this, QStringLiteral("无法办理调宿"), QStringLiteral("目标床位号无效。"));
			return;
		}
		if (occupied == 1) {
			uifeedback::show_error(this, QStringLiteral("无法办理调宿"), QStringLiteral("目标床位已经被占用。"));
			return;
		}
	}
	const QString original_position = accommodation_position_text(*current_student);
	const QString target_position = target_bed_id > 0
		? QStringLiteral("%1号楼 %2室 %3号床").arg(target_building_id).arg(target_dorm_id).arg(target_bed_id)
		: QStringLiteral("%1号楼 %2室的最小空床位").arg(target_building_id).arg(target_dorm_id);
	const QString reset_notice = reset_source_gender
		? QStringLiteral("\n原宿舍性别锁：学生调离后解除") : QString();
	if (!uifeedback::confirm_action(this, QStringLiteral("确认调宿 / 换床"),
		QStringLiteral("%1\n\n原位置：%2\n目标位置：%3%4")
			.arg(accommodation_student_text(*current_student), original_position, target_position, reset_notice),
		QStringLiteral("确认调整"))) {
		return;
	}

	const int result = reset_source_gender
		? school::instance().move_student_to_dorm_reset_source_gender(target_building_id, target_dorm_id, student_id, target_bed_id)
		: (target_bed_id > 0
			? school::instance().move_student_to_dorm(target_building_id, target_dorm_id, student_id, target_bed_id)
			: school::instance().move_student_to_dorm(target_building_id, target_dorm_id, student_id));
	if (result <= 0) {
		show_move_error(result);
		refresh_move_buildings();
		return;
	}
	const student* moved_student = school::instance().get_student(student_id);
	uifeedback::show_success(this, moved_student == nullptr
		? QStringLiteral("调宿办理成功，新床位号为 %1。").arg(result)
		: QStringLiteral("调宿办理成功：%1").arg(accommodation_position_text(*moved_student)));
	refresh_data();
}

void AccommodationPage::show_move_error(int result)
{
	if (result == -10) {
		uifeedback::show_critical(this, QStringLiteral("调宿恢复失败"), QStringLiteral("调宿执行失败，且学生原床位未能完整恢复。请立即暂停后续操作并核查住宿数据。"), QStringLiteral("业务返回值：-10"));
		return;
	}
	if (result == -8) {
		uifeedback::show_critical(this, QStringLiteral("住宿记录异常"), QStringLiteral("源宿舍或目标宿舍不存在，或学生位置与床位记录不一致。请暂停相关操作并核查数据。"), QStringLiteral("业务返回值：-8"));
		return;
	}
	QString message;
	if (result == 0) message = QStringLiteral("该学生当前未入住。");
	else if (result == -2) message = QStringLiteral("目标床位已经被占用。");
	else if (result == -4) message = QStringLiteral("目标宿舍已经住满。");
	else if (result == -6) message = QStringLiteral("学生不存在或尚未设置有效性别。");
	else if (result == -7) message = QStringLiteral("目标楼栋或宿舍不接纳该学生性别。");
	else if (result == -11) message = QStringLiteral("原宿舍不满足解锁条件，或解除性别锁失败且调宿已恢复。");
	else message = QStringLiteral("输入参数无效，请检查目标楼栋、宿舍和床位。");
	uifeedback::show_error(this, QStringLiteral("无法办理调宿"), message, QStringLiteral("业务返回值：%1").arg(result));
}

void AccommodationPage::update_swap_preview()
{
	ui->swapSubmitButton->setEnabled(false);
	const int student_id1 = selected_student_id(ui->swapStudent1Spin);
	const int student_id2 = selected_student_id(ui->swapStudent2Spin);
	if (!check::is_valid_student_id(student_id1) || !check::is_valid_student_id(student_id2)) {
		ui->swapPreviewLabel->setText(QStringLiteral("请通过姓名或学号搜索并选择两名已入住学生。"));
		return;
	}
	if (student_id1 == student_id2) {
		ui->swapPreviewLabel->setText(QStringLiteral("两处学号相同，请选择两名不同学生。"));
		return;
	}
	const student* student1 = school::instance().get_student(student_id1);
	const student* student2 = school::instance().get_student(student_id2);
	if (student1 == nullptr || student2 == nullptr) {
		ui->swapPreviewLabel->setText(QStringLiteral("至少有一名学生不存在，请检查学号。"));
		return;
	}
	const bool assigned1 = school::instance().get_assigned_student_ids().contains(student_id1);
	const bool assigned2 = school::instance().get_assigned_student_ids().contains(student_id2);
	if (assigned1 != has_consistent_accommodation(*student1) || assigned2 != has_consistent_accommodation(*student2)
		|| (!assigned1 && has_any_accommodation(*student1)) || (!assigned2 && has_any_accommodation(*student2))) {
		ui->swapPreviewLabel->setText(QStringLiteral("至少一名学生的住宿位置记录异常，不能交换。"));
		ui->swapSubmitButton->setEnabled(false);
		return;
	}
	if (!assigned1 || !assigned2) {
		ui->swapPreviewLabel->setText(QStringLiteral("两名学生必须都已入住才能交换床位。"));
		return;
	}
	bool data_error = false;
	const QString constraint_error = validate_swap_constraints(*student1, *student2, data_error);
	if (!constraint_error.isEmpty()) {
		ui->swapPreviewLabel->setText(constraint_error);
		ui->swapSubmitButton->setEnabled(false);
		return;
	}
	ui->swapPreviewLabel->setText(QStringLiteral("学生一：%1\n当前位置：%2\n\n学生二：%3\n当前位置：%4")
		.arg(accommodation_student_text(*student1), accommodation_position_text(*student1),
			accommodation_student_text(*student2), accommodation_position_text(*student2)));
	ui->swapSubmitButton->setEnabled(true);
}

void AccommodationPage::submit_swap()
{
	const int student_id1 = selected_student_id(ui->swapStudent1Spin);
	const int student_id2 = selected_student_id(ui->swapStudent2Spin);
	if (!check::is_valid_student_id(student_id1) || !check::is_valid_student_id(student_id2) || student_id1 == student_id2) {
		uifeedback::show_error(this, QStringLiteral("无法交换床位"), QStringLiteral("请输入两名不同学生的有效学号。"));
		return;
	}
	const student* student1 = school::instance().get_student(student_id1);
	const student* student2 = school::instance().get_student(student_id2);
	if (student1 == nullptr || student2 == nullptr) {
		uifeedback::show_error(this, QStringLiteral("无法交换床位"), QStringLiteral("至少有一名学生不存在。"));
		return;
	}
	const bool assigned1 = school::instance().get_assigned_student_ids().contains(student_id1);
	const bool assigned2 = school::instance().get_assigned_student_ids().contains(student_id2);
	if (assigned1 != has_consistent_accommodation(*student1) || assigned2 != has_consistent_accommodation(*student2)
		|| (!assigned1 && has_any_accommodation(*student1)) || (!assigned2 && has_any_accommodation(*student2))) {
		uifeedback::show_critical(this, QStringLiteral("住宿记录异常"), QStringLiteral("至少一名学生的住宿位置字段不完整，请暂停相关操作并核查数据。"));
		return;
	}
	if (!assigned1 || !assigned2) {
		uifeedback::show_error(this, QStringLiteral("无法交换床位"), QStringLiteral("两名学生必须都已入住。"));
		return;
	}
	bool data_error = false;
	const QString constraint_error = validate_swap_constraints(*student1, *student2, data_error);
	if (!constraint_error.isEmpty()) {
		if (data_error) {
			uifeedback::show_critical(this, QStringLiteral("住宿记录异常"), constraint_error);
		} else {
			uifeedback::show_error(this, QStringLiteral("无法交换床位"), constraint_error);
		}
		return;
	}
	const QString student1_description = QStringLiteral("%1：%2").arg(accommodation_student_text(*student1), accommodation_position_text(*student1));
	const QString student2_description = QStringLiteral("%1：%2").arg(accommodation_student_text(*student2), accommodation_position_text(*student2));
	if (!uifeedback::confirm_action(this, QStringLiteral("确认交换床位"),
		QStringLiteral("%1\n\n%2\n\n确认交换两人的床位吗？").arg(student1_description, student2_description), QStringLiteral("确认交换"))) {
		return;
	}

	const int result = school::instance().swap_students(student_id1, student_id2);
	if (result != 1) {
		show_swap_error(result);
		update_swap_preview();
		return;
	}
	const student* swapped_student1 = school::instance().get_student(student_id1);
	const student* swapped_student2 = school::instance().get_student(student_id2);
	const QString result_text = swapped_student1 != nullptr && swapped_student2 != nullptr
		? QStringLiteral("床位交换成功：%1；%2。")
			.arg(accommodation_position_text(*swapped_student1), accommodation_position_text(*swapped_student2))
		: QStringLiteral("床位交换成功。");
	uifeedback::show_success(this, result_text);
	refresh_data();
}

void AccommodationPage::show_swap_error(int result)
{
	if (result == -6) {
		uifeedback::show_critical(this, QStringLiteral("交换恢复失败"), QStringLiteral("床位交换失败，且宿舍快照未能完整恢复。请立即暂停后续操作并核查住宿数据。"), QStringLiteral("业务返回值：-6"));
		return;
	}
	if (result == -4) {
		uifeedback::show_critical(this, QStringLiteral("住宿记录异常"), QStringLiteral("源宿舍不存在，或学生位置与床位记录不一致。请暂停相关操作并核查数据。"), QStringLiteral("业务返回值：-4"));
		return;
	}
	QString message;
	if (result == -1) message = QStringLiteral("学生学号无效，或选择了同一名学生。");
	else if (result == -2) message = QStringLiteral("至少一名学生不存在或尚未设置有效性别。");
	else if (result == -3) message = QStringLiteral("至少一名学生未入住，或住宿位置字段不完整。");
	else message = QStringLiteral("目标楼栋、宿舍或住客约束不允许本次交换，操作未生效。");
	uifeedback::show_error(this, QStringLiteral("无法交换床位"), message, QStringLiteral("业务返回值：%1").arg(result));
}

QString AccommodationPage::validate_swap_constraints(const student& student1, const student& student2, bool& data_error) const
{
	data_error = false;
	if (student1.get_gender() < 1 || student1.get_gender() > 2 || student2.get_gender() < 1 || student2.get_gender() > 2) {
		return QStringLiteral("至少一名学生尚未设置有效性别。请先完成性别纠错。");
	}
	const dorm* dorm1 = school::instance().get_dorm(student1.get_building_id(), student1.get_dorm_id());
	const dorm* dorm2 = school::instance().get_dorm(student2.get_building_id(), student2.get_dorm_id());
	const building* building1 = school::instance().get_building(student1.get_building_id());
	const building* building2 = school::instance().get_building(student2.get_building_id());
	if (dorm1 == nullptr || dorm2 == nullptr || building1 == nullptr || building2 == nullptr
		|| dorm1->get_student_id(student1.get_bed_id()) != student1.get_id()
		|| dorm2->get_student_id(student2.get_bed_id()) != student2.get_id()) {
		data_error = true;
		return QStringLiteral("学生位置与宿舍床位记录不一致，请暂停相关操作并核查数据。");
	}
	if (!building1->accepts_gender(student2.get_gender()) || !dorm1->accepts_gender(student2.get_gender())
		|| !building2->accepts_gender(student1.get_gender()) || !dorm2->accepts_gender(student1.get_gender())) {
		return QStringLiteral("交换后至少一名学生的性别不被目标楼栋或房间接纳。");
	}
	return QString();
}
