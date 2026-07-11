#include "overviewpage.h"
#include "./ui_overviewpage.h"

#include "core/building.h"
#include "core/dorm.h"
#include "core/school.h"
#include "core/student.h"
#include "sampledatadialog.h"
#include "uifeedback.h"

#include <QHeaderView>
#include <QProgressBar>
#include <QPushButton>
#include <QSet>
#include <QShowEvent>
#include <QStringList>
#include <QTableWidgetItem>

namespace {
QString building_gender_text(int gender)
{
	if (gender == 1) {
		return QStringLiteral("男生");
	}
	if (gender == 2) {
		return QStringLiteral("女生");
	}
	if (gender == 3) {
		return QStringLiteral("混宿");
	}
	return QStringLiteral("未知");
}
}

OverviewPage::OverviewPage(QWidget* parent)
	: QWidget(parent)
	, ui(new Ui::OverviewPage)
{
	ui->setupUi(this);
	ui->emptyBedInfoButton->set_information(ui->emptyBedInfoButton->toolTip());
	ui->sampleDataInfoButton->set_information(ui->sampleDataInfoButton->toolTip());
	ui->generateSampleDataButton->setAccessibleName(QStringLiteral("生成随机样例数据"));
	ui->buildingCapacityTable->setAccessibleName(QStringLiteral("宿舍楼容量概况"));
	ui->buildingCapacityTable->verticalHeader()->setVisible(false);
	ui->buildingCapacityTable->verticalHeader()->setDefaultSectionSize(42);
	ui->buildingCapacityTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
	ui->buildingCapacityTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
	ui->buildingCapacityTable->horizontalHeaderItem(4)->setText(QStringLiteral("空床位"));
	ui->buildingCapacityTable->horizontalHeaderItem(4)->setToolTip(QStringLiteral("统计未被占用的床位，不区分房间性别锁。"));
	connect(ui->generateSampleDataButton, &QPushButton::clicked, this, &OverviewPage::open_sample_data_dialog);
}

OverviewPage::~OverviewPage()
{
	delete ui;
}

void OverviewPage::refresh_data()
{
	refresh_summary();
	refresh_building_capacity();
}

void OverviewPage::showEvent(QShowEvent* event)
{
	QWidget::showEvent(event);
	refresh_data();
}

void OverviewPage::open_sample_data_dialog()
{
	SampleDataDialog dialog(this);
	if (dialog.exec() != QDialog::Accepted) {
		return;
	}
	const sampledataresult result = dialog.generation_result();
	if (!result.success) {
		return;
	}
	refresh_data();
	uifeedback::show_information(this, QStringLiteral("样例数据生成完成"),
		QStringLiteral("已新增 %1 栋楼、%2 间宿舍和 %3 名学生，其中 %4 人已入住。\n"
			"当前保留 %5 间未锁定空宿舍；随机种子为 %6。")
			.arg(result.added_building_count)
			.arg(result.added_dorm_count)
			.arg(result.added_student_count)
			.arg(result.assigned_student_count)
			.arg(result.remaining_unlocked_dorm_count)
			.arg(result.random_seed));
}

void OverviewPage::refresh_summary()
{
	school& current_school = school::instance();
	ui->studentCountValueLabel->setText(QString::number(current_school.get_student_count()));
	ui->assignedCountValueLabel->setText(QString::number(current_school.get_assigned_student_count()));
	ui->emptyBedValueLabel->setText(QString::number(current_school.get_empty_bed_count()));
	ui->buildingCountValueLabel->setText(QString::number(current_school.get_building_count()));
	ui->unassignedValueLabel->setText(QStringLiteral("%1 人").arg(current_school.get_unassigned_student_count()));

	int male_total = 0;
	int female_total = 0;
	int male_assigned = 0;
	int female_assigned = 0;
	int unset_gender_count = 0;
	QSet<int> assigned_ids;
	for (int assigned_id : current_school.get_assigned_student_ids()) {
		assigned_ids.insert(assigned_id);
	}
	for (int student_id : current_school.get_all_student_ids()) {
		const student* current_student = current_school.get_student(student_id);
		if (current_student == nullptr) {
			continue;
		}
		const bool assigned = assigned_ids.contains(student_id);
		if (current_student->get_gender() == 1) {
			++male_total;
			male_assigned += assigned ? 1 : 0;
		} else if (current_student->get_gender() == 2) {
			++female_total;
			female_assigned += assigned ? 1 : 0;
		} else {
			++unset_gender_count;
		}
	}
	ui->unsetGenderValueLabel->setText(QStringLiteral("%1 人").arg(unset_gender_count));

	const auto update_progress = [](QProgressBar* progress, int assigned, int total) {
		const int percentage = total > 0 ? assigned * 100 / total : 0;
		progress->setValue(percentage);
		progress->setFormat(total > 0
			? QStringLiteral("%1% · %2/%3 人").arg(percentage).arg(assigned).arg(total)
			: QStringLiteral("暂无数据"));
	};
	update_progress(ui->maleOccupancyProgress, male_assigned, male_total);
	update_progress(ui->femaleOccupancyProgress, female_assigned, female_total);
}

void OverviewPage::refresh_building_capacity()
{
	school& current_school = school::instance();
	const QVector<int> building_ids = current_school.get_all_building_ids();
	ui->buildingCapacityTable->setRowCount(building_ids.size());
	ui->buildingCapacityGroupBox->setTitle(building_ids.isEmpty()
		? QStringLiteral("宿舍楼容量概况（暂无楼栋）")
		: QStringLiteral("宿舍楼容量概况"));

	for (int row = 0; row < building_ids.size(); ++row) {
		const int building_id = building_ids.at(row);
		const building* current_building = current_school.get_building(building_id);
		if (current_building == nullptr) {
			continue;
		}

		int occupied_beds = 0;
		int empty_beds = 0;
		const QVector<QPair<int, int>> dorm_keys = current_school.get_dorm_keys_of_building(building_id);
		for (const QPair<int, int>& dorm_key : dorm_keys) {
			const dorm* current_dorm = current_school.get_dorm(dorm_key.first, dorm_key.second);
			if (current_dorm == nullptr) {
				continue;
			}
			occupied_beds += current_dorm->get_occupied_count();
			empty_beds += current_dorm->get_empty_count();
		}
		const int total_beds = occupied_beds + empty_beds;
		const int occupancy_rate = total_beds > 0 ? occupied_beds * 100 / total_beds : 0;

		const QStringList values = {
			QString::number(building_id),
			building_gender_text(current_building->get_for_gender()),
			QString::number(dorm_keys.size()),
			QString::number(occupied_beds),
			QString::number(empty_beds),
			QStringLiteral("%1%").arg(occupancy_rate)
		};
		for (int column = 0; column < values.size(); ++column) {
			auto* item = new QTableWidgetItem(values.at(column));
			item->setTextAlignment(Qt::AlignCenter);
			if (column == 0) {
				item->setData(Qt::UserRole, building_id);
			}
			ui->buildingCapacityTable->setItem(row, column, item);
		}
	}
}
