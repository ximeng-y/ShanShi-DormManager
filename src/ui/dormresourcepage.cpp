#include "dormresourcepage.h"
#include "./ui_dormresourcepage.h"

#include <QHeaderView>

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
}

DormResourcePage::~DormResourcePage()
{
	delete ui;
}
