#include "mainwidget.h"
#include "./ui_mainwidget.h"

#include <QButtonGroup>
#include <QCloseEvent>
#include <QGuiApplication>
#include <QList>
#include <QPushButton>
#include <QScreen>
#include <QSettings>
#include <QStyle>
#include <QStringList>

MainWidget::MainWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::MainWidget)
{
    ui->setupUi(this);
	restore_window_state();

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

void MainWidget::closeEvent(QCloseEvent* event)
{
	save_window_state();
	QWidget::closeEvent(event);
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

void MainWidget::restore_window_state()
{
	QSettings settings(QStringLiteral("DormManager"), QStringLiteral("DormManager"));
	const QByteArray saved_geometry = settings.value(QStringLiteral("window/geometry")).toByteArray();
	if (saved_geometry.isEmpty()) {
		QScreen* screen = QGuiApplication::primaryScreen();
		if (screen != nullptr) {
			const QRect available = screen->availableGeometry();
			resize(QSize(1280, 720).boundedTo(available.size()));
			move(available.center() - rect().center());
		} else {
			resize(1280, 720);
		}
	} else {
		restoreGeometry(saved_geometry);
		QScreen* target_screen = nullptr;
		int largest_visible_area = 0;
		for (QScreen* screen : QGuiApplication::screens()) {
			const QRect visible = screen->availableGeometry().intersected(frameGeometry());
			const int visible_area = visible.width() * visible.height();
			if (visible_area > largest_visible_area) {
				largest_visible_area = visible_area;
				target_screen = screen;
			}
		}

		if (target_screen == nullptr) {
			target_screen = QGuiApplication::primaryScreen();
		}
		if (target_screen != nullptr && !isMaximized() && !isFullScreen()) {
			const QRect available = target_screen->availableGeometry();
			const QRect visible = available.intersected(frameGeometry());
			const bool sufficiently_visible = visible.width() >= qMin(320, frameGeometry().width())
				&& visible.height() >= qMin(180, frameGeometry().height());
			if (!sufficiently_visible || frameGeometry().width() > available.width() || frameGeometry().height() > available.height()) {
				resize(size().boundedTo(available.size()));
				move(available.center() - rect().center());
			}
		}
	}

	set_sidebar_collapsed(settings.value(QStringLiteral("navigation/collapsed"), false).toBool());
}

void MainWidget::save_window_state() const
{
	QSettings settings(QStringLiteral("DormManager"), QStringLiteral("DormManager"));
	settings.setValue(QStringLiteral("window/geometry"), saveGeometry());
	settings.setValue(QStringLiteral("navigation/collapsed"), sidebar_collapsed);
}
