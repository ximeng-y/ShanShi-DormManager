#include "dormresourcepage.h"
#include "./ui_dormresourcepage.h"

#include "core/building.h"
#include "core/dorm.h"
#include "core/school.h"

#include <QHeaderView>
#include <QListWidgetItem>
#include <QShowEvent>
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
{
	ui->setupUi(this);
	ui->dormTable->verticalHeader()->setVisible(false);
	ui->dormTable->verticalHeader()->setDefaultSectionSize(42);
	ui->dormTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
	ui->dormTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
	ui->resourceSplitter->setSizes({190, 430, 360});

	connect(ui->buildingList, &QListWidget::currentItemChanged, this, [this](QListWidgetItem* current) {
		selected_building_id = current == nullptr ? 0 : current->data(Qt::UserRole).toInt();
		ui->addDormButton->setEnabled(selected_building_id > 0);
		refresh_dorm_list();
	});
	connect(ui->dormSearchLineEdit, &QLineEdit::textChanged, this, [this]() {
		refresh_dorm_list();
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
		if (building_id == selected_building_id) {
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
	ui->buildingList->setCurrentRow(restored_row >= 0 ? restored_row : 0);
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
	clear_dorm_detail();
}

void DormResourcePage::clear_dorm_detail()
{
	selected_dorm_id = 0;
	ui->dormDetailTitle->setText(QStringLiteral("宿舍详情"));
	ui->dormDetailHint->show();
	ui->dormDetailContent->hide();
	ui->editDormButton->setEnabled(false);
	ui->removeDormButton->setEnabled(false);
}
