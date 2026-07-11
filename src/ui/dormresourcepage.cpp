#include "dormresourcepage.h"
#include "./ui_dormresourcepage.h"
#include "addbuildingdialog.h"
#include "bedtablemodel.h"
#include "uifeedback.h"

#include "core/building.h"
#include "core/dorm.h"
#include "core/school.h"

#include <QHeaderView>
#include <QListWidgetItem>
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
	ui->resourceSplitter->setSizes({190, 430, 360});

	connect(ui->buildingList, &QListWidget::currentItemChanged, this, [this](QListWidgetItem* current) {
		const int new_building_id = current == nullptr ? 0 : current->data(Qt::UserRole).toInt();
		if (new_building_id != selected_building_id) {
			selected_dorm_id = 0;
		}
		selected_building_id = new_building_id;
		ui->addDormButton->setEnabled(selected_building_id > 0);
		refresh_dorm_list();
	});
	connect(ui->dormSearchLineEdit, &QLineEdit::textChanged, this, [this]() {
		refresh_dorm_list();
	});
	connect(ui->dormTable, &QTableWidget::cellClicked, this, [this](int row, int) {
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
}

DormResourcePage::~DormResourcePage()
{
	delete ui;
}

void DormResourcePage::refresh_data()
{
	refresh_building_list();
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
	selected_dorm_id = 0;
	ui->dormDetailTitle->setText(QStringLiteral("宿舍详情"));
	ui->dormDetailHint->show();
	ui->dormDetailContent->hide();
	ui->editDormButton->setEnabled(false);
	ui->removeDormButton->setEnabled(false);
	bed_model->set_dorm(0, 0);
}

void DormResourcePage::show_dorm_detail(int dorm_id)
{
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
}
