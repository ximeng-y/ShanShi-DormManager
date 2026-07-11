#include "mainwidget.h"
#include "./ui_mainwidget.h"

#include <QButtonGroup>
#include <QPushButton>
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
		static const QStringList page_titles = {
			QStringLiteral("数据概览"),
			QStringLiteral("学生管理"),
			QStringLiteral("宿舍资源"),
			QStringLiteral("住宿安排"),
			QStringLiteral("高级调整")
		};
		switch_page(index, page_titles.at(index));
	});
	connect(ui->sidebarToggleButton, &QToolButton::clicked, this, [this]() {
		set_sidebar_collapsed(!sidebar_collapsed);
	});
}

MainWidget::~MainWidget()
{
    delete ui;
}

void MainWidget::switch_page(int index, const QString& title)
{
	ui->pageStack->setCurrentIndex(index);
	ui->pageTitleLabel->setText(title);
}

void MainWidget::set_sidebar_collapsed(bool collapsed)
{
	sidebar_collapsed = collapsed;
	const int sidebar_width = collapsed ? 64 : 184;
	ui->sidebar->setMinimumWidth(sidebar_width);
	ui->sidebar->setMaximumWidth(sidebar_width);

	ui->overviewNavButton->setText(collapsed ? QStringLiteral("概览") : QStringLiteral("数据概览"));
	ui->studentNavButton->setText(collapsed ? QStringLiteral("学生") : QStringLiteral("学生管理"));
	ui->dormNavButton->setText(collapsed ? QStringLiteral("宿舍") : QStringLiteral("宿舍资源"));
	ui->accommodationNavButton->setText(collapsed ? QStringLiteral("住宿") : QStringLiteral("住宿安排"));
	ui->advancedNavButton->setText(collapsed ? QStringLiteral("高级") : QStringLiteral("高级调整"));
}
