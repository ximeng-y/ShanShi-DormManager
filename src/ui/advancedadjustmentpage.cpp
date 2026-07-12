#include "advancedadjustmentpage.h"
#include "./ui_advancedadjustmentpage.h"

#include "batchassignmentdialog.h"
#include "batchcleardialog.h"
#include "dormadjustmentdialog.h"
#include "core/school.h"

#include <QShowEvent>

AdvancedAdjustmentPage::AdvancedAdjustmentPage(QWidget* parent)
	: QWidget(parent)
	, ui(new Ui::AdvancedAdjustmentPage)
{
	ui->setupUi(this);
	ui->headingInfoButton->set_information(ui->headingInfoButton->toolTip());
	connect(ui->batchAssignmentButton, &QPushButton::clicked, this, &AdvancedAdjustmentPage::open_batch_assignment);
	connect(ui->dormAdjustmentButton, &QPushButton::clicked, this, &AdvancedAdjustmentPage::open_dorm_adjustment);
	connect(ui->batchClearButton, &QPushButton::clicked, this, &AdvancedAdjustmentPage::open_batch_clear);
	refresh_summary();
}

void AdvancedAdjustmentPage::showEvent(QShowEvent* event)
{
	QWidget::showEvent(event);
	refresh_summary();
}

void AdvancedAdjustmentPage::refresh_summary()
{
	school& current_school = school::instance();
	ui->batchAssignmentSummary->setText(QStringLiteral("未入住学生：%1 人").arg(current_school.get_unassigned_student_count()));
	ui->dormAdjustmentSummary->setText(QStringLiteral("当前宿舍：%1 间").arg(current_school.get_dorm_count()));
	ui->batchClearSummary->setText(QStringLiteral("当前已入住：%1 人").arg(current_school.get_assigned_student_count()));
	ui->dormAdjustmentButton->setEnabled(current_school.get_dorm_count() >= 2);
	ui->batchClearButton->setEnabled(current_school.get_dorm_count() > 0);
}

void AdvancedAdjustmentPage::open_batch_assignment()
{
	BatchAssignmentDialog dialog(this);
	dialog.exec();
	refresh_summary();
}

void AdvancedAdjustmentPage::open_dorm_adjustment()
{
	DormAdjustmentDialog dialog(this);
	dialog.exec();
	refresh_summary();
}

void AdvancedAdjustmentPage::open_batch_clear()
{
	BatchClearDialog dialog(this);
	dialog.exec();
	refresh_summary();
}

AdvancedAdjustmentPage::~AdvancedAdjustmentPage()
{
	delete ui;
}
