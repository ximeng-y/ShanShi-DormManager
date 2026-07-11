#include "overviewpage.h"
#include "./ui_overviewpage.h"

#include "core/school.h"
#include "core/student.h"

#include <QProgressBar>
#include <QShowEvent>

OverviewPage::OverviewPage(QWidget* parent)
	: QWidget(parent)
	, ui(new Ui::OverviewPage)
{
	ui->setupUi(this);
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
	for (int student_id : current_school.get_all_student_ids()) {
		const student* current_student = current_school.get_student(student_id);
		if (current_student == nullptr) {
			continue;
		}
		const bool assigned = current_student->get_dorm_id() > 0;
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
	//楼栋容量表格在独立提交中接入，当前先保持结构与刷新入口稳定。
	ui->buildingCapacityTable->setRowCount(0);
}
