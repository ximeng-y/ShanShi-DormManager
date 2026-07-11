#include "sampledatadialog.h"
#include "./ui_sampledatadialog.h"

#include <QPushButton>

SampleDataDialog::SampleDataDialog(QWidget* parent)
	: QDialog(parent)
	, ui(new Ui::SampleDataDialog)
{
	ui->setupUi(this);
	ui->replaceInfoButton->set_information(ui->replaceInfoButton->toolTip());
	ui->buttonBox->button(QDialogButtonBox::Ok)->setText(QStringLiteral("开始生成"));
	ui->buttonBox->button(QDialogButtonBox::Cancel)->setText(QStringLiteral("取消"));
}

SampleDataDialog::~SampleDataDialog()
{
	delete ui;
}
