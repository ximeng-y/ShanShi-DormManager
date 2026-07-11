#include "studentpage.h"
#include "./ui_studentpage.h"

#include <QHeaderView>

StudentPage::StudentPage(QWidget* parent)
	: QWidget(parent)
	, ui(new Ui::StudentPage)
{
	ui->setupUi(this);
	ui->studentTable->verticalHeader()->setVisible(false);
	ui->studentTable->verticalHeader()->setDefaultSectionSize(42);
	ui->studentTable->horizontalHeader()->setStretchLastSection(true);
	ui->studentTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
	ui->studentTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
	ui->studentTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
	ui->studentTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
	ui->studentTable->horizontalHeader()->setSectionResizeMode(4, QHeaderView::ResizeToContents);
	ui->studentTable->horizontalHeader()->setSectionResizeMode(5, QHeaderView::ResizeToContents);
}

StudentPage::~StudentPage()
{
	delete ui;
}
