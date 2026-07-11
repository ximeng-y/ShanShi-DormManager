#include "advancedadjustmentpage.h"
#include "./ui_advancedadjustmentpage.h"

AdvancedAdjustmentPage::AdvancedAdjustmentPage(QWidget* parent)
	: QWidget(parent)
	, ui(new Ui::AdvancedAdjustmentPage)
{
	ui->setupUi(this);
}

AdvancedAdjustmentPage::~AdvancedAdjustmentPage()
{
	delete ui;
}
