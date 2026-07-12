#include "mainwidget.h"
#include "./ui_mainwidget.h"
#include "uifeedback.h"

#include "core/school.h"

#include <QButtonGroup>
#include <QAbstractButton>
#include <QCloseEvent>
#include <QEvent>
#include <QGuiApplication>
#include <QList>
#include <QMessageBox>
#include <QPushButton>
#include <QResizeEvent>
#include <QScreen>
#include <QSettings>
#include <QShowEvent>
#include <QStyle>
#include <QStringList>
#include <QWindow>
#include <QTimer>

MainWidget::MainWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::MainWidget)
{
    ui->setupUi(this);
	restore_window_state();
	ui->sidebarToggleButton->setAccessibleName(QStringLiteral("展开或收起导航栏"));
	ui->overviewNavButton->setAccessibleName(QStringLiteral("数据概览"));
	ui->studentNavButton->setAccessibleName(QStringLiteral("学生管理"));
	ui->dormNavButton->setAccessibleName(QStringLiteral("宿舍资源"));
	ui->accommodationNavButton->setAccessibleName(QStringLiteral("住宿安排"));
	ui->advancedNavButton->setAccessibleName(QStringLiteral("高级调整"));

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
	connect(ui->studentPage, &StudentPage::accommodation_requested, this, [this](int student_id, bool assigned) {
		switch_page(3);
		ui->accommodationPage->prepare_student_task(student_id, assigned);
	});
	connect(ui->studentPage, &StudentPage::dorm_open_requested, this, [this](int building_id, int dorm_id) {
		ui->dormResourcePage->select_dorm(building_id, dorm_id);
		switch_page(2);
	});
	connect(ui->dormResourcePage, &DormResourcePage::student_open_requested, this, [this](int student_id) {
		ui->studentPage->select_student(student_id);
		switch_page(1);
	});
	connect(ui->overviewPage, &OverviewPage::building_open_requested, this, [this](int building_id) {
		ui->dormResourcePage->select_building(building_id);
		switch_page(2);
	});
	connect(ui->advancedAdjustmentPage, &AdvancedAdjustmentPage::student_open_requested, this, [this](int student_id) {
		ui->studentPage->select_student(student_id);
		switch_page(1);
	});
	connect(ui->advancedAdjustmentPage, &AdvancedAdjustmentPage::dorm_open_requested, this, [this](int building_id, int dorm_id) {
		if (dorm_id > 0) ui->dormResourcePage->select_dorm(building_id, dorm_id);
		else ui->dormResourcePage->select_building(building_id);
		switch_page(2);
	});
}

MainWidget::~MainWidget()
{
    delete ui;
}

void MainWidget::closeEvent(QCloseEvent* event)
{
	if (school::instance().ensure_persistence_current() != 1)
	{
		uifeedback::show_error(this, QStringLiteral("暂时无法退出"), QStringLiteral("数据尚未安全保存。"));
		event->ignore();
		return;
	}
	save_window_state();
	QWidget::closeEvent(event);
}

void MainWidget::resizeEvent(QResizeEvent* event)
{
	QWidget::resizeEvent(event);
	if (width() < 1180 && !sidebar_collapsed) {
		set_sidebar_collapsed(true);
	}
}

void MainWidget::showEvent(QShowEvent* event)
{
	QWidget::showEvent(event);
	if (persistence_notice_shown)
		return;
	persistence_notice_shown = true;
	QTimer::singleShot(0, this, &MainWidget::show_persistence_startup_notice);
}

void MainWidget::show_persistence_startup_notice()
{
	school& current_school = school::instance();
	bool conflict_resolved = false;
	if (current_school.persistence_status() == persistence_start_status::data_conflict)
	{
		QMessageBox dialog(this);
		dialog.setWindowTitle(QStringLiteral("选择宿舍数据"));
		dialog.setIcon(QMessageBox::Question);
		dialog.setText(QStringLiteral("程序目录和用户目录均存在有效宿舍数据，请选择本次使用的数据。"));
		dialog.setInformativeText(QStringLiteral("程序目录数据\n%1\n\n用户目录数据\n%2")
			.arg(current_school.persistence_candidate_summary(true), current_school.persistence_candidate_summary(false)));
		QAbstractButton* executable_button = dialog.addButton(QStringLiteral("使用程序目录数据"), QMessageBox::AcceptRole);
		QAbstractButton* fallback_button = dialog.addButton(QStringLiteral("使用用户目录数据"), QMessageBox::AcceptRole);
		dialog.exec();
		const bool use_executable = dialog.clickedButton() == executable_button;
		if (dialog.clickedButton() != executable_button && dialog.clickedButton() != fallback_button)
		{
			enter_read_only_mode();
			uifeedback::show_critical(this, QStringLiteral("尚未选择数据"),
				QStringLiteral("未选择本次使用的数据，系统将保持只读状态，不会执行任何修改。"));
			return;
		}
		if (!current_school.resolve_persistence_conflict(use_executable))
		{
			enter_read_only_mode();
			uifeedback::show_critical(this, QStringLiteral("无法加载所选数据"),
				QStringLiteral("所选数据未能完整恢复，系统不会执行后续修改。"), current_school.last_persistence_error());
			return;
		}
		conflict_resolved = true;
		refresh_all_pages();
	}

	switch (current_school.persistence_status())
	{
	case persistence_start_status::fallback_ready:
		if (!conflict_resolved)
			uifeedback::show_information(this, QStringLiteral("使用用户数据目录"),
				QStringLiteral("本次将使用以下用户数据目录自动保存：\n%1").arg(current_school.active_data_directory()));
		break;
	case persistence_start_status::backup_restored:
		uifeedback::show_information(this, QStringLiteral("已从备份恢复"),
			QStringLiteral("检测到正式数据文件无法使用，系统已恢复上一次成功保存的数据。损坏文件已保留在 recovery 文件夹中。"));
		break;
	case persistence_start_status::read_only_corrupt:
	case persistence_start_status::read_only_unwritable:
	case persistence_start_status::read_only_newer_version:
		lock_read_only_controls();
		uifeedback::show_critical(this, QStringLiteral("已进入只读安全模式"),
			QStringLiteral("当前数据无法安全保存，系统不会允许修改数据。"), current_school.last_persistence_error());
		break;
	default:
		break;
	}
}

void MainWidget::enter_read_only_mode()
{
	lock_read_only_controls();
}

bool MainWidget::eventFilter(QObject* watched, QEvent* event)
{
	if (event->type() == QEvent::EnabledChange && school::instance().is_read_only())
	{
		QWidget* widget = qobject_cast<QWidget*>(watched);
		if (widget != nullptr && widget->isEnabled())
			QTimer::singleShot(0, widget, [widget]() { widget->setEnabled(false); });
	}
	return QWidget::eventFilter(watched, event);
}

void MainWidget::lock_read_only_controls()
{
	static const QStringList write_control_names = {
		QStringLiteral("generateSampleDataButton"), QStringLiteral("addStudentButton"),
		QStringLiteral("editStudentButton"), QStringLiteral("saveEditButton"),
		QStringLiteral("accommodationActionButton"), QStringLiteral("moreActionButton"),
		QStringLiteral("addBuildingButton"), QStringLiteral("addDormButton"),
		QStringLiteral("buildingActionButton"), QStringLiteral("editDormButton"),
		QStringLiteral("removeDormButton"), QStringLiteral("clearDormButton"),
		QStringLiteral("assignSelectedBedButton"), QStringLiteral("removeSelectedBedButton"),
		QStringLiteral("moveWithinDormButton"), QStringLiteral("moveToOtherDormButton"),
		QStringLiteral("assignSubmitButton"), QStringLiteral("removeSubmitButton"),
		QStringLiteral("moveSubmitButton"), QStringLiteral("swapSubmitButton"),
		QStringLiteral("batchAssignmentButton"), QStringLiteral("dormAdjustmentButton"),
		QStringLiteral("batchClearButton")
	};
	for (const QString& name : write_control_names)
	{
		QWidget* control = findChild<QWidget*>(name);
		if (control == nullptr)
			continue;
		control->setEnabled(false);
		control->installEventFilter(this);
		control->setToolTip(QStringLiteral("当前处于只读安全模式，不能修改数据。"));
	}
}

void MainWidget::refresh_all_pages()
{
	ui->overviewPage->refresh_data();
	ui->studentPage->refresh_data();
	ui->dormResourcePage->refresh_data();
	ui->accommodationPage->refresh_data();
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
	const bool effective_collapsed = collapsed || width() < 1180;
	sidebar_collapsed = effective_collapsed;
	const int sidebar_width = effective_collapsed ? 64 : 184;
	ui->sidebar->setMinimumWidth(sidebar_width);
	ui->sidebar->setMaximumWidth(sidebar_width);
	ui->sidebarLayout->setContentsMargins(effective_collapsed ? 6 : 12, 18, effective_collapsed ? 6 : 12, 0);
	ui->sidebar->setProperty("collapsed", effective_collapsed);
	ui->sidebar->style()->unpolish(ui->sidebar);
	ui->sidebar->style()->polish(ui->sidebar);

	ui->overviewNavButton->setText(effective_collapsed ? QStringLiteral("概") : QStringLiteral("数据概览"));
	ui->studentNavButton->setText(effective_collapsed ? QStringLiteral("生") : QStringLiteral("学生管理"));
	ui->dormNavButton->setText(effective_collapsed ? QStringLiteral("舍") : QStringLiteral("宿舍资源"));
	ui->accommodationNavButton->setText(effective_collapsed ? QStringLiteral("住") : QStringLiteral("住宿安排"));
	ui->advancedNavButton->setText(effective_collapsed ? QStringLiteral("高") : QStringLiteral("高级调整"));
}

void MainWidget::restore_window_state()
{
	winId();//创建原生窗口，以便获取 Windows 标题栏和边框尺寸
	const QMargins frame_margins = windowHandle() == nullptr ? QMargins(8, 32, 8, 8) : windowHandle()->frameMargins();
	const auto fit_to_available = [this, frame_margins](const QRect& available, const QSize& preferred_client_size) {
		const QSize frame_size(frame_margins.left() + frame_margins.right(), frame_margins.top() + frame_margins.bottom());
		const QSize available_client_size = (available.size() - frame_size).expandedTo(minimumSize());
		const QSize client_size = preferred_client_size.boundedTo(available_client_size);
		resize(client_size);
		const QSize outer_size = client_size + frame_size;
		const QPoint frame_top_left = available.topLeft() + QPoint((available.width() - outer_size.width()) / 2, (available.height() - outer_size.height()) / 2);
		move(frame_top_left);
	};

	QSettings settings(QStringLiteral("DormManager"), QStringLiteral("DormManager"));
	const QByteArray saved_geometry = settings.value(QStringLiteral("window/geometry")).toByteArray();
	if (saved_geometry.isEmpty()) {
		QScreen* screen = QGuiApplication::primaryScreen();
		if (screen != nullptr) {
			fit_to_available(screen->availableGeometry(), QSize(1280, 720));
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
				fit_to_available(available, size());
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
