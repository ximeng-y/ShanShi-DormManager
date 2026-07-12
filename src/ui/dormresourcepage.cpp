#include "dormresourcepage.h"
#include "./ui_dormresourcepage.h"
#include "addbuildingdialog.h"
#include "bedassignmentdialog.h"
#include "bedmovedialog.h"
#include "adddormdialog.h"
#include "bedtablemodel.h"
#include "editbuildingdialog.h"
#include "editdormdialog.h"
#include "uifeedback.h"

#include "core/building.h"
#include "core/dorm.h"
#include "core/school.h"
#include "core/student.h"

#include <QHeaderView>
#include <QItemSelectionModel>
#include <QListWidgetItem>
#include <QMenu>
#include <QShowEvent>
#include <QSignalBlocker>
#include <QStringList>
#include <QTableWidgetItem>

namespace {
QString resource_building_gender_text(int gender)
{
	if (gender == 1) {
		return QStringLiteral("男生");
	}
	if (gender == 2) {
		return QStringLiteral("女生");
	}
	return gender == 3 ? QStringLiteral("混宿") : QStringLiteral("未知");
}

QString resource_dorm_gender_text(int gender)
{
	if (gender == 1) {
		return QStringLiteral("男舍");
	}
	if (gender == 2) {
		return QStringLiteral("女舍");
	}
	return QStringLiteral("未锁定");
}
}

DormResourcePage::DormResourcePage(QWidget* parent)
	: QWidget(parent)
	, ui(new Ui::DormResourcePage)
	, bed_model(new bedtablemodel(this))
{
	ui->setupUi(this);
	ui->dormTable->verticalHeader()->setVisible(false);
	ui->dormTable->verticalHeader()->setDefaultSectionSize(42);
	ui->dormTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
	ui->dormTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
	ui->bedTableView->setModel(bed_model);
	ui->bedTableView->horizontalHeader()->setVisible(false);
	ui->bedTableView->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
	ui->bedTableView->verticalHeader()->setVisible(false);
	ui->bedTableView->verticalHeader()->setDefaultSectionSize(70);
	connect(ui->bedTableView->selectionModel(), &QItemSelectionModel::currentChanged,
		this, &DormResourcePage::update_bed_action_state);
	ui->buildingList->setAccessibleName(QStringLiteral("楼栋目录"));
	ui->dormTable->setAccessibleName(QStringLiteral("宿舍目录"));
	ui->bedTableView->setAccessibleName(QStringLiteral("床位与住客"));
	ui->bedAreaInfoButton->set_information(ui->bedAreaInfoButton->toolTip());
	ui->bedSelectionLabel->setAccessibleName(QStringLiteral("当前选中床位"));
	ui->assignSelectedBedButton->setAccessibleName(QStringLiteral("安排学生入住当前空床"));
	ui->removeSelectedBedButton->setAccessibleName(QStringLiteral("办理当前床位住客退宿"));
	ui->moveWithinDormButton->setAccessibleName(QStringLiteral("将当前住客换到本宿舍空床"));
	ui->moveToOtherDormButton->setAccessibleName(QStringLiteral("将当前住客调往其他宿舍"));
	ui->cancelBedActionButton->setAccessibleName(QStringLiteral("取消选择目标空床"));
	update_bed_action_state(QModelIndex());
	connect(ui->assignSelectedBedButton, &QPushButton::clicked, this, &DormResourcePage::assign_selected_bed);
	connect(ui->removeSelectedBedButton, &QPushButton::clicked, this, &DormResourcePage::remove_selected_occupant);
	connect(ui->moveWithinDormButton, &QPushButton::clicked, this, &DormResourcePage::begin_within_dorm_move);
	connect(ui->moveToOtherDormButton, &QPushButton::clicked, this, &DormResourcePage::move_selected_occupant_to_other_dorm);
	connect(ui->cancelBedActionButton, &QPushButton::clicked, this, &DormResourcePage::cancel_pending_bed_action);
	ui->resourceSplitter->setStretchFactor(0, 0);
	ui->resourceSplitter->setStretchFactor(1, 1);
	ui->resourceSplitter->setStretchFactor(2, 0);
	ui->resourceSplitter->setSizes({190, 430, 360});
	auto* building_menu = new QMenu(ui->buildingActionButton);
	QAction* edit_building_action = building_menu->addAction(QStringLiteral("修改楼栋属性"));
	building_menu->addSeparator();
	QAction* remove_building_action = building_menu->addAction(QStringLiteral("删除楼栋"));
	ui->buildingActionButton->setMenu(building_menu);
	connect(edit_building_action, &QAction::triggered, this, [this]() {
		EditBuildingDialog dialog(selected_building_id, this);
		if (dialog.exec() == QDialog::Accepted) {
			refresh_data();
			uifeedback::show_success(this, QStringLiteral("楼栋属性已更新。"));
		}
	});
	connect(remove_building_action, &QAction::triggered, this, [this]() {
		const building* current_building = school::instance().get_building(selected_building_id);
		if (current_building == nullptr) {
			uifeedback::show_error(this, QStringLiteral("无法删除楼栋"), QStringLiteral("所选楼栋已经不存在，请刷新后重试。"));
			refresh_data();
			return;
		}
		const QVector<QPair<int, int>> dorm_keys = school::instance().get_dorm_keys_of_building(selected_building_id);
		int occupant_count = 0;
		for (const QPair<int, int>& dorm_key : dorm_keys) {
			const dorm* current_dorm = school::instance().get_dorm(dorm_key.first, dorm_key.second);
			if (current_dorm != nullptr) {
				occupant_count += current_dorm->get_current_num();
			}
		}
		const QString impact = QStringLiteral("将删除 %1号楼及其 %2 间宿舍，并清退 %3 名住客。此操作无法撤销。")
			.arg(selected_building_id)
			.arg(dorm_keys.size())
			.arg(occupant_count);
		if (!uifeedback::confirm_danger(this, QStringLiteral("确认删除楼栋"), impact, QStringLiteral("确认删除"))) {
			return;
		}
		if (!school::instance().remove_building(selected_building_id)) {
			uifeedback::show_critical(this, QStringLiteral("楼栋删除失败"), QStringLiteral("楼内住宿记录可能存在不一致，系统未能完成级联删除。请暂停相关操作并核查数据。"));
			return;
		}
		selected_building_id = 0;
		selected_dorm_id = 0;
		refresh_data();
		uifeedback::show_information(this, QStringLiteral("楼栋删除完成"),
			QStringLiteral("楼栋及 %1 间宿舍已删除，%2 名住客已同步清退。").arg(dorm_keys.size()).arg(occupant_count));
	});

	connect(ui->buildingList, &QListWidget::currentItemChanged, this, [this](QListWidgetItem* current) {
		const int new_building_id = current == nullptr ? 0 : current->data(Qt::UserRole).toInt();
		if (new_building_id != selected_building_id) {
			selected_dorm_id = 0;
		}
		selected_building_id = new_building_id;
		ui->addDormButton->setEnabled(selected_building_id > 0);
		ui->buildingActionButton->setEnabled(selected_building_id > 0);
		refresh_dorm_list();
	});
	connect(ui->dormSearchLineEdit, &QLineEdit::textChanged, this, [this]() {
		refresh_dorm_list();
	});
	connect(ui->dormTable, &QTableWidget::currentCellChanged, this, [this](int row, int, int, int) {
		QTableWidgetItem* id_item = ui->dormTable->item(row, 0);
		if (id_item != nullptr) {
			show_dorm_detail(id_item->data(Qt::UserRole).toInt());
		}
	});
	connect(ui->addBuildingButton, &QPushButton::clicked, this, [this]() {
		AddBuildingDialog dialog(this);
		if (dialog.exec() == QDialog::Accepted) {
			selected_building_id = dialog.added_building_id();
			refresh_data();
			uifeedback::show_success(this, QStringLiteral("宿舍楼已添加。"));
		}
	});
	connect(ui->addDormButton, &QPushButton::clicked, this, [this]() {
		AddDormDialog dialog(selected_building_id, this);
		if (dialog.exec() == QDialog::Accepted) {
			selected_dorm_id = dialog.added_dorm_id();
			const QSignalBlocker search_blocker(ui->dormSearchLineEdit);
			ui->dormSearchLineEdit->clear();
			refresh_data();
			uifeedback::show_success(this, QStringLiteral("宿舍已添加。"));
		}
	});
	setTabOrder(ui->addBuildingButton, ui->addDormButton);
	setTabOrder(ui->addDormButton, ui->buildingActionButton);
	setTabOrder(ui->buildingActionButton, ui->buildingList);
	setTabOrder(ui->buildingList, ui->dormSearchLineEdit);
	setTabOrder(ui->dormSearchLineEdit, ui->dormTable);
	setTabOrder(ui->dormTable, ui->bedTableView);
	setTabOrder(ui->bedTableView, ui->assignSelectedBedButton);
	setTabOrder(ui->assignSelectedBedButton, ui->removeSelectedBedButton);
	setTabOrder(ui->removeSelectedBedButton, ui->moveWithinDormButton);
	setTabOrder(ui->moveWithinDormButton, ui->moveToOtherDormButton);
	setTabOrder(ui->moveToOtherDormButton, ui->cancelBedActionButton);
	setTabOrder(ui->cancelBedActionButton, ui->editDormButton);
	setTabOrder(ui->editDormButton, ui->removeDormButton);
	connect(ui->editDormButton, &QPushButton::clicked, this, [this]() {
		EditDormDialog dialog(selected_building_id, selected_dorm_id, this);
		if (dialog.exec() == QDialog::Accepted) {
			refresh_data();
			uifeedback::show_success(this, QStringLiteral("宿舍属性已更新。"));
		}
	});
	connect(ui->removeDormButton, &QPushButton::clicked, this, [this]() {
		const dorm* current_dorm = school::instance().get_dorm(selected_building_id, selected_dorm_id);
		if (current_dorm == nullptr) {
			uifeedback::show_error(this, QStringLiteral("无法删除宿舍"), QStringLiteral("所选宿舍已经不存在，请刷新后重试。"));
			refresh_data();
			return;
		}
		const int occupant_count = current_dorm->get_current_num();
		const QString impact = occupant_count > 0
			? QStringLiteral("将删除 %1号楼 %2室，并清退其中 %3 名住客。此操作无法撤销。")
				.arg(selected_building_id).arg(selected_dorm_id).arg(occupant_count)
			: QStringLiteral("将删除 %1号楼 %2室。当前宿舍没有住客，此操作无法撤销。")
				.arg(selected_building_id).arg(selected_dorm_id);
		if (!uifeedback::confirm_danger(this, QStringLiteral("确认删除宿舍"), impact, QStringLiteral("确认删除"))) {
			return;
		}
		if (!school::instance().remove_dorm(selected_building_id, selected_dorm_id)) {
			uifeedback::show_critical(this, QStringLiteral("宿舍删除失败"), QStringLiteral("宿舍床位与学生位置记录可能不一致，系统未执行删除。请暂停相关操作并核查数据。"));
			return;
		}
		selected_dorm_id = 0;
		refresh_data();
		uifeedback::show_information(this, QStringLiteral("宿舍删除完成"), occupant_count > 0
			? QStringLiteral("宿舍已删除，%1 名住客已同步清退。").arg(occupant_count)
			: QStringLiteral("宿舍已删除。"));
	});
}

DormResourcePage::~DormResourcePage()
{
	delete ui;
}

void DormResourcePage::refresh_data()
{
	pending_move_student_id = 0;
	pending_source_building_id = 0;
	pending_source_dorm_id = 0;
	pending_source_bed_id = 0;
	refresh_building_list();
}

void DormResourcePage::select_building(int building_id)
{
	if (school::instance().get_building(building_id) == nullptr) {
		return;
	}
	selected_building_id = building_id;
	selected_dorm_id = 0;
	const QSignalBlocker search_blocker(ui->dormSearchLineEdit);
	ui->dormSearchLineEdit->clear();
	if (isVisible()) {
		refresh_data();
	}
}

void DormResourcePage::showEvent(QShowEvent* event)
{
	QWidget::showEvent(event);
	refresh_data();
}

void DormResourcePage::refresh_building_list()
{
	const school& current_school = school::instance();
	const QVector<int> building_ids = current_school.get_all_building_ids();
	const int requested_building_id = selected_building_id;
	const QSignalBlocker blocker(ui->buildingList);
	ui->buildingList->clear();
	int restored_row = -1;
	for (int row = 0; row < building_ids.size(); ++row) {
		const int building_id = building_ids.at(row);
		const building* current_building = current_school.get_building(building_id);
		if (current_building == nullptr) {
			continue;
		}
		const int dorm_count = current_school.get_dorm_keys_of_building(building_id).size();
		auto* item = new QListWidgetItem(QStringLiteral("%1号楼\n%2 · %3间宿舍")
			.arg(building_id)
			.arg(resource_building_gender_text(current_building->get_for_gender()))
			.arg(dorm_count));
		item->setData(Qt::UserRole, building_id);
		item->setSizeHint(QSize(0, 54));
		ui->buildingList->addItem(item);
		if (building_id == requested_building_id) {
			restored_row = ui->buildingList->count() - 1;
		}
	}
	ui->buildingCountLabel->setText(building_ids.isEmpty()
		? QStringLiteral("暂无楼栋")
		: QStringLiteral("共 %1 栋").arg(building_ids.size()));
	if (ui->buildingList->count() == 0) {
		selected_building_id = 0;
		ui->addDormButton->setEnabled(false);
		ui->buildingActionButton->setEnabled(false);
		refresh_dorm_list();
		return;
	}
	const int target_row = restored_row >= 0 ? restored_row : 0;
	ui->buildingList->setCurrentRow(target_row);
	selected_building_id = ui->buildingList->item(target_row)->data(Qt::UserRole).toInt();
	if (selected_building_id != requested_building_id) {
		selected_dorm_id = 0;
	}
	ui->addDormButton->setEnabled(true);
	ui->buildingActionButton->setEnabled(true);
	refresh_dorm_list();
}

void DormResourcePage::refresh_dorm_list()
{
	const school& current_school = school::instance();
	const QString search_text = ui->dormSearchLineEdit->text().trimmed();
	QVector<QPair<int, int>> matched_keys;
	if (selected_building_id > 0) {
		for (const QPair<int, int>& dorm_key : current_school.get_dorm_keys_of_building(selected_building_id)) {
			if (search_text.isEmpty() || QString::number(dorm_key.second).contains(search_text)) {
				matched_keys.append(dorm_key);
			}
		}
	}

	ui->dormTable->clearSelection();
	ui->dormTable->setCurrentCell(-1, -1);
	ui->dormTable->setRowCount(matched_keys.size());
	for (int row = 0; row < matched_keys.size(); ++row) {
		const dorm* current_dorm = current_school.get_dorm(matched_keys.at(row).first, matched_keys.at(row).second);
		if (current_dorm == nullptr) {
			continue;
		}
		const QString status = current_dorm->is_full()
			? QStringLiteral("已满")
			: (current_dorm->is_empty() ? QStringLiteral("空闲") : QStringLiteral("可用"));
		const QStringList values = {
			QString::number(current_dorm->get_id()),
			resource_dorm_gender_text(current_dorm->get_for_gender()),
			QStringLiteral("%1/%2").arg(current_dorm->get_current_num()).arg(current_dorm->get_max_num()),
			status
		};
		for (int column = 0; column < values.size(); ++column) {
			auto* item = new QTableWidgetItem(values.at(column));
			item->setTextAlignment(Qt::AlignCenter);
			if (column == 0) {
				item->setData(Qt::UserRole, current_dorm->get_id());
			}
			ui->dormTable->setItem(row, column, item);
		}
	}
	ui->dormPanelTitle->setText(selected_building_id > 0
		? QStringLiteral("%1号楼宿舍").arg(selected_building_id)
		: QStringLiteral("宿舍目录"));
	ui->dormCountLabel->setText(selected_building_id == 0
		? QStringLiteral("请先选择楼栋")
		: (matched_keys.isEmpty() ? QStringLiteral("没有符合条件的宿舍") : QStringLiteral("已显示 %1 间宿舍").arg(matched_keys.size())));
	int restored_row = -1;
	for (int row = 0; row < ui->dormTable->rowCount(); ++row) {
		QTableWidgetItem* id_item = ui->dormTable->item(row, 0);
		if (id_item != nullptr && id_item->data(Qt::UserRole).toInt() == selected_dorm_id) {
			restored_row = row;
			break;
		}
	}
	if (restored_row >= 0) {
		ui->dormTable->selectRow(restored_row);
		show_dorm_detail(selected_dorm_id);
	} else {
		clear_dorm_detail();
	}
}

void DormResourcePage::clear_dorm_detail()
{
	pending_move_student_id = 0;
	pending_source_building_id = 0;
	pending_source_dorm_id = 0;
	pending_source_bed_id = 0;
	selected_dorm_id = 0;
	ui->dormDetailTitle->setText(QStringLiteral("宿舍详情"));
	ui->dormDetailHint->show();
	ui->dormDetailContent->hide();
	ui->editDormButton->setEnabled(false);
	ui->removeDormButton->setEnabled(false);
	bed_model->set_dorm(0, 0);
	update_bed_action_state(QModelIndex());
}

void DormResourcePage::show_dorm_detail(int dorm_id)
{
	if (selected_dorm_id != dorm_id) {
		pending_move_student_id = 0;
		pending_source_building_id = 0;
		pending_source_dorm_id = 0;
		pending_source_bed_id = 0;
	}
	const dorm* current_dorm = school::instance().get_dorm(selected_building_id, dorm_id);
	if (current_dorm == nullptr) {
		clear_dorm_detail();
		return;
	}

	selected_dorm_id = dorm_id;
	ui->dormDetailTitle->setText(QStringLiteral("%1室").arg(current_dorm->get_id()));
	ui->buildingValueLabel->setText(QStringLiteral("%1号楼").arg(current_dorm->get_building_id()));
	ui->floorValueLabel->setText(QStringLiteral("%1层").arg(current_dorm->get_floor()));
	ui->genderLockValueLabel->setText(resource_dorm_gender_text(current_dorm->get_for_gender()));
	ui->capacityValueLabel->setText(QStringLiteral("%1/%2 已入住").arg(current_dorm->get_current_num()).arg(current_dorm->get_max_num()));
	ui->dormDetailHint->hide();
	ui->dormDetailContent->show();
	ui->editDormButton->setEnabled(true);
	ui->removeDormButton->setEnabled(true);

	bed_model->set_dorm(selected_building_id, selected_dorm_id);
	ui->bedTableView->clearSelection();
	ui->bedTableView->setCurrentIndex(QModelIndex());
	update_bed_action_state(QModelIndex());
}

void DormResourcePage::update_bed_action_state(const QModelIndex& index)
{
	selected_bed_id = index.data(bedtablemodel::bed_id_role).toInt();
	selected_student_id = index.data(bedtablemodel::student_id_role).toInt();
	const bool occupied = index.data(bedtablemodel::occupied_role).toBool();
	const bool record_valid = index.data(bedtablemodel::record_valid_role).toBool();
	if (pending_move_student_id > 0) {
		ui->assignSelectedBedButton->hide();
		ui->removeSelectedBedButton->hide();
		ui->moveWithinDormButton->hide();
		ui->moveToOtherDormButton->hide();
		ui->cancelBedActionButton->show();
		if (selected_bed_id <= 0 || selected_bed_id == pending_source_bed_id) {
			ui->bedSelectionLabel->setText(QStringLiteral("请选择本宿舍中的一个空床作为目标。"));
			return;
		}
		if (!record_valid || occupied) {
			ui->bedSelectionLabel->setText(QStringLiteral("%1号床不是可用空床，请选择其他床位。")
				.arg(selected_bed_id));
			return;
		}
		complete_within_dorm_move(selected_bed_id);
		return;
	}
	ui->cancelBedActionButton->hide();
	if (selected_bed_id <= 0) {
		ui->bedSelectionLabel->setText(QStringLiteral("选择一个床位后显示可用操作。"));
	} else if (!record_valid) {
		ui->bedSelectionLabel->setText(QStringLiteral("%1号床记录异常，已暂停床位操作。").arg(selected_bed_id));
	} else if (occupied) {
		ui->bedSelectionLabel->setText(QStringLiteral("已选择 %1号床 · 学号 %2").arg(selected_bed_id).arg(selected_student_id));
	} else {
		ui->bedSelectionLabel->setText(QStringLiteral("已选择 %1号床 · 当前空闲").arg(selected_bed_id));
	}

	const bool available_empty_bed = selected_bed_id > 0 && record_valid && !occupied;
	const bool available_occupant = selected_bed_id > 0 && record_valid && occupied;
	const dorm* current_dorm = school::instance().get_dorm(selected_building_id, selected_dorm_id);
	const bool has_empty_bed = current_dorm != nullptr && current_dorm->get_empty_count() > 0;
	ui->assignSelectedBedButton->setVisible(available_empty_bed);
	ui->assignSelectedBedButton->setEnabled(available_empty_bed);
	ui->removeSelectedBedButton->setVisible(available_occupant);
	ui->removeSelectedBedButton->setEnabled(available_occupant);
	ui->moveWithinDormButton->setVisible(available_occupant);
	ui->moveWithinDormButton->setEnabled(available_occupant && has_empty_bed);
	ui->moveWithinDormButton->setToolTip(has_empty_bed
		? QStringLiteral("在当前宿舍的可视化床位中选择一个空床。")
		: QStringLiteral("当前宿舍没有可用于换床的空床。"));
	ui->moveToOtherDormButton->setVisible(available_occupant);
	ui->moveToOtherDormButton->setEnabled(available_occupant);
}

void DormResourcePage::assign_selected_bed()
{
	const int building_id = selected_building_id;
	const int dorm_id = selected_dorm_id;
	const int bed_id = selected_bed_id;
	const dorm* target = school::instance().get_dorm(building_id, dorm_id);
	if (target == nullptr || target->is_bed_occupied(bed_id) != 0) {
		uifeedback::show_error(this, QStringLiteral("无法办理入住"), QStringLiteral("当前所选床位已经失效或不再空闲，请刷新后重试。"));
		refresh_data();
		return;
	}
	BedAssignmentDialog dialog(building_id, dorm_id, bed_id, this);
	if (dialog.exec() != QDialog::Accepted) {
		return;
	}
	const int result = school::instance().assign_student_to_dorm(building_id, dorm_id, dialog.student_id(), bed_id);
	if (result > 0) {
		refresh_data();
		uifeedback::show_success(this, QStringLiteral("入住办理成功：已安排至%1号楼%2室%3号床。")
			.arg(building_id).arg(dorm_id).arg(bed_id));
		return;
	}
	QString message = QStringLiteral("学生或目标床位状态已经变化，请刷新后重试。");
	if (result == -2) message = QStringLiteral("目标床位已经被占用。");
	else if (result == -3) message = QStringLiteral("该学生已经入住其他床位。");
	else if (result == -6) message = QStringLiteral("学生不存在或尚未设置有效性别。");
	else if (result == -7) message = QStringLiteral("目标楼栋或宿舍不接纳该学生性别。");
	uifeedback::show_error(this, QStringLiteral("无法办理入住"), message, QStringLiteral("业务返回值：%1").arg(result));
	refresh_data();
}

void DormResourcePage::remove_selected_occupant()
{
	const int student_id = selected_student_id;
	const student* current_student = school::instance().get_student(student_id);
	const dorm* current_dorm = school::instance().get_dorm(selected_building_id, selected_dorm_id);
	if (current_student == nullptr || current_dorm == nullptr
		|| current_dorm->get_student_id(selected_bed_id) != student_id
		|| current_student->get_building_id() != selected_building_id
		|| current_student->get_dorm_id() != selected_dorm_id
		|| current_student->get_bed_id() != selected_bed_id
		|| current_student->get_floor() != selected_dorm_id / 100) {
		uifeedback::show_critical(this, QStringLiteral("住宿记录异常"), QStringLiteral("当前床位与学生位置记录不一致，请暂停相关操作并核查数据。"));
		refresh_data();
		return;
	}
	if (!uifeedback::confirm_action(this, QStringLiteral("确认办理退宿"),
		QStringLiteral("将为 %1（%2）办理退宿并释放%3号床，学生档案继续保留。")
			.arg(current_student->get_name()).arg(student_id).arg(selected_bed_id), QStringLiteral("确认退宿"))) {
		return;
	}
	const int result = school::instance().remove_student_from_dorm(student_id);
	if (result > 0) {
		refresh_data();
		uifeedback::show_success(this, QStringLiteral("退宿办理成功，%1号床已释放。").arg(result));
		return;
	}
	if (result == -8) {
		uifeedback::show_critical(this, QStringLiteral("住宿记录异常"), QStringLiteral("学生位置与宿舍床位记录不一致，请暂停相关操作并核查数据。"));
	} else {
		uifeedback::show_error(this, QStringLiteral("无法办理退宿"), QStringLiteral("学生或床位状态已经变化，请刷新后重试。"),
			QStringLiteral("业务返回值：%1").arg(result));
	}
	refresh_data();
}

void DormResourcePage::begin_within_dorm_move()
{
	const dorm* current_dorm = school::instance().get_dorm(selected_building_id, selected_dorm_id);
	if (selected_student_id <= 0 || selected_bed_id <= 0 || current_dorm == nullptr || current_dorm->get_empty_count() <= 0) {
		return;
	}
	pending_move_student_id = selected_student_id;
	pending_source_building_id = selected_building_id;
	pending_source_dorm_id = selected_dorm_id;
	pending_source_bed_id = selected_bed_id;
	ui->bedTableView->clearSelection();
	ui->bedTableView->setCurrentIndex(QModelIndex());
	update_bed_action_state(QModelIndex());
}

void DormResourcePage::move_selected_occupant_to_other_dorm()
{
	const int student_id = selected_student_id;
	const student* current_student = school::instance().get_student(student_id);
	const dorm* current_dorm = school::instance().get_dorm(selected_building_id, selected_dorm_id);
	if (current_student == nullptr || current_dorm == nullptr
		|| current_dorm->get_student_id(selected_bed_id) != student_id
		|| current_student->get_building_id() != selected_building_id
		|| current_student->get_dorm_id() != selected_dorm_id
		|| current_student->get_bed_id() != selected_bed_id
		|| current_student->get_floor() != selected_dorm_id / 100) {
		uifeedback::show_critical(this, QStringLiteral("住宿记录异常"), QStringLiteral("当前床位与学生位置记录不一致，请暂停相关操作并核查数据。"));
		refresh_data();
		return;
	}

	const QString student_name = current_student->get_name();
	BedMoveDialog dialog(student_id, this);
	if (dialog.exec() != QDialog::Accepted) {
		return;
	}
	const int target_building_id = dialog.target_building_id();
	const int target_dorm_id = dialog.target_dorm_id();
	const int target_bed_id = dialog.target_bed_id();
	const int result = target_bed_id > 0
		? school::instance().move_student_to_dorm(target_building_id, target_dorm_id, student_id, target_bed_id)
		: school::instance().move_student_to_dorm(target_building_id, target_dorm_id, student_id);
	if (result > 0) {
		selected_building_id = target_building_id;
		selected_dorm_id = target_dorm_id;
		const QSignalBlocker search_blocker(ui->dormSearchLineEdit);
		ui->dormSearchLineEdit->clear();
		refresh_data();
		uifeedback::show_success(this, QStringLiteral("调宿成功：%1（%2）已安排至%3号楼%4室%5号床。")
			.arg(student_name).arg(student_id)
			.arg(target_building_id).arg(target_dorm_id).arg(result));
		return;
	}
	if (result == -10) {
		uifeedback::show_critical(this, QStringLiteral("调宿恢复失败"), QStringLiteral("调宿执行失败且原床位未能完整恢复，请暂停后续操作并核查数据。"));
	} else {
		QString message = QStringLiteral("学生或目标住宿状态已经变化，请刷新后重试。");
		if (result == -2) message = QStringLiteral("目标床位已经被占用。");
		else if (result == -4) message = QStringLiteral("目标宿舍已经没有空床。");
		else if (result == -6) message = QStringLiteral("学生不存在或尚未设置有效性别。");
		else if (result == -7) message = QStringLiteral("目标楼栋或宿舍不接纳该学生性别。");
		if (result == -8) {
			uifeedback::show_critical(this, QStringLiteral("住宿记录异常"), QStringLiteral("学生位置或源、目标宿舍床位记录不一致，请暂停相关操作并核查数据。"));
		} else {
			uifeedback::show_error(this, QStringLiteral("无法完成调宿"), message,
				QStringLiteral("业务返回值：%1").arg(result));
		}
	}
	refresh_data();
}

void DormResourcePage::cancel_pending_bed_action()
{
	pending_move_student_id = 0;
	pending_source_building_id = 0;
	pending_source_dorm_id = 0;
	pending_source_bed_id = 0;
	ui->bedTableView->clearSelection();
	ui->bedTableView->setCurrentIndex(QModelIndex());
	update_bed_action_state(QModelIndex());
}

void DormResourcePage::complete_within_dorm_move(int target_bed_id)
{
	const int student_id = pending_move_student_id;
	if (selected_building_id != pending_source_building_id || selected_dorm_id != pending_source_dorm_id) {
		uifeedback::show_error(this, QStringLiteral("无法完成换床"), QStringLiteral("当前宿舍已经变化，请重新选择住客和目标空床。"));
		cancel_pending_bed_action();
		return;
	}
	if (!uifeedback::confirm_action(this, QStringLiteral("确认换床"),
		QStringLiteral("将学号 %1 从%2号床移动到%3号床。")
			.arg(student_id).arg(pending_source_bed_id).arg(target_bed_id), QStringLiteral("确认换床"))) {
		ui->bedSelectionLabel->setText(QStringLiteral("换床尚未执行，可继续选择其他空床或取消。"));
		ui->bedTableView->clearSelection();
		ui->bedTableView->setCurrentIndex(QModelIndex());
		return;
	}
	pending_move_student_id = 0;
	pending_source_building_id = 0;
	pending_source_dorm_id = 0;
	pending_source_bed_id = 0;
	const int result = school::instance().move_student_to_dorm(
		selected_building_id, selected_dorm_id, student_id, target_bed_id);
	if (result > 0) {
		refresh_data();
		uifeedback::show_success(this, QStringLiteral("换床成功，学生已移动到%1号床。").arg(result));
		return;
	}
	if (result == -10) {
		uifeedback::show_critical(this, QStringLiteral("换床恢复失败"), QStringLiteral("换床执行失败且原床位未能完整恢复，请暂停后续操作并核查数据。"));
	} else if (result == -8) {
		uifeedback::show_critical(this, QStringLiteral("住宿记录异常"), QStringLiteral("学生位置或宿舍床位记录不一致，请暂停相关操作并核查数据。"));
	} else {
		uifeedback::show_error(this, QStringLiteral("无法完成换床"), QStringLiteral("目标床位或学生状态已经变化，请刷新后重试。"),
			QStringLiteral("业务返回值：%1").arg(result));
	}
	refresh_data();
}
