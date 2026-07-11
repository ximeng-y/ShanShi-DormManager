#include "mainwidget.h"
#include "./ui_mainwidget.h"

#include <QButtonGroup>
#include <QList>
#include <QPushButton>
#include <QStyle>
#include <QStringList>

MainWidget::MainWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::MainWidget)
{
    ui->setupUi(this);
	resize(1280, 720);

	auto* navigation_group = new QButtonGroup(this);
	navigation_group->setExclusive(true);
	navigation_group->addButton(ui->overviewNavButton, 0);
	navigation_group->addButton(ui->studentNavButton, 1);
	navigation_group->addButton(ui->dormNavButton, 2);
	navigation_group->addButton(ui->accommodationNavButton, 3);
	navigation_group->addButton(ui->advancedNavButton, 4);

	connect(navigation_group, &QButtonGroup::idClicked, this, [this](int index) {
		switch_page(index);
	});
	connect(ui->sidebarToggleButton, &QToolButton::clicked, this, [this]() {
		set_sidebar_collapsed(!sidebar_collapsed);
	});
}

MainWidget::~MainWidget()
{
    delete ui;
}

void MainWidget::switch_page(int index)
{
	static const QStringList page_titles = {
		QStringLiteral("数据概览"),
		QStringLiteral("学生管理"),
		QStringLiteral("宿舍资源"),
		QStringLiteral("住宿安排"),
		QStringLiteral("高级调整")
	};
	const QList<QPushButton*> navigation_buttons = {
		ui->overviewNavButton,
		ui->studentNavButton,
		ui->dormNavButton,
		ui->accommodationNavButton,
		ui->advancedNavButton
	};
	if (index < 0 || index >= navigation_buttons.size()) {
		return;
	}

	ui->pageStack->setCurrentIndex(index);
	ui->pageTitleLabel->setText(page_titles.at(index));
	navigation_buttons.at(index)->setChecked(true);
}

void MainWidget::set_sidebar_collapsed(bool collapsed)
{
	sidebar_collapsed = collapsed;
	const int sidebar_width = collapsed ? 64 : 184;
	ui->sidebar->setMinimumWidth(sidebar_width);
	ui->sidebar->setMaximumWidth(sidebar_width);
	ui->sidebarLayout->setContentsMargins(collapsed ? 6 : 12, 18, collapsed ? 6 : 12, 0);
	ui->sidebar->setProperty("collapsed", collapsed);
	ui->sidebar->style()->unpolish(ui->sidebar);
	ui->sidebar->style()->polish(ui->sidebar);

	ui->overviewNavButton->setText(collapsed ? QStringLiteral("概") : QStringLiteral("数据概览"));
	ui->studentNavButton->setText(collapsed ? QStringLiteral("生") : QStringLiteral("学生管理"));
	ui->dormNavButton->setText(collapsed ? QStringLiteral("舍") : QStringLiteral("宿舍资源"));
	ui->accommodationNavButton->setText(collapsed ? QStringLiteral("住") : QStringLiteral("住宿安排"));
	ui->advancedNavButton->setText(collapsed ? QStringLiteral("高") : QStringLiteral("高级调整"));
}
