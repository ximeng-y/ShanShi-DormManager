#include "appstyle.h"

#include <QColor>

QPalette appstyle::light_palette()
{
	QPalette palette;
	const QColor text(QStringLiteral("#243447"));
	const QColor muted_text(QStringLiteral("#6d7f90"));
	const QColor disabled_text(QStringLiteral("#9ba8b4"));
	const QColor window(QStringLiteral("#f3f6f9"));
	const QColor base(QStringLiteral("#ffffff"));
	const QColor alternate(QStringLiteral("#f7f9fb"));
	const QColor highlight(QStringLiteral("#dceaf5"));
	for (QPalette::ColorGroup group : {QPalette::Active, QPalette::Inactive}) {
		palette.setColor(group, QPalette::Window, window);
		palette.setColor(group, QPalette::WindowText, text);
		palette.setColor(group, QPalette::Base, base);
		palette.setColor(group, QPalette::AlternateBase, alternate);
		palette.setColor(group, QPalette::ToolTipBase, base);
		palette.setColor(group, QPalette::ToolTipText, text);
		palette.setColor(group, QPalette::Text, text);
		palette.setColor(group, QPalette::Button, base);
		palette.setColor(group, QPalette::ButtonText, text);
		palette.setColor(group, QPalette::BrightText, QColor(QStringLiteral("#ffffff")));
		palette.setColor(group, QPalette::Link, QColor(QStringLiteral("#315f86")));
		palette.setColor(group, QPalette::LinkVisited, QColor(QStringLiteral("#557f9f")));
		palette.setColor(group, QPalette::Highlight, highlight);
		palette.setColor(group, QPalette::HighlightedText, QColor(QStringLiteral("#17324d")));
		palette.setColor(group, QPalette::PlaceholderText, muted_text);
		palette.setColor(group, QPalette::Light, QColor(QStringLiteral("#ffffff")));
		palette.setColor(group, QPalette::Midlight, QColor(QStringLiteral("#eef2f5")));
		palette.setColor(group, QPalette::Mid, QColor(QStringLiteral("#bdc9d4")));
		palette.setColor(group, QPalette::Dark, QColor(QStringLiteral("#7890a4")));
		palette.setColor(group, QPalette::Shadow, QColor(QStringLiteral("#526579")));
	}
	palette.setColor(QPalette::Disabled, QPalette::Window, window);
	palette.setColor(QPalette::Disabled, QPalette::WindowText, disabled_text);
	palette.setColor(QPalette::Disabled, QPalette::Base, QColor(QStringLiteral("#f2f4f6")));
	palette.setColor(QPalette::Disabled, QPalette::AlternateBase, QColor(QStringLiteral("#f2f4f6")));
	palette.setColor(QPalette::Disabled, QPalette::ToolTipBase, base);
	palette.setColor(QPalette::Disabled, QPalette::ToolTipText, disabled_text);
	palette.setColor(QPalette::Disabled, QPalette::Text, disabled_text);
	palette.setColor(QPalette::Disabled, QPalette::Button, QColor(QStringLiteral("#f2f4f6")));
	palette.setColor(QPalette::Disabled, QPalette::ButtonText, disabled_text);
	palette.setColor(QPalette::Disabled, QPalette::BrightText, QColor(QStringLiteral("#ffffff")));
	palette.setColor(QPalette::Disabled, QPalette::Link, disabled_text);
	palette.setColor(QPalette::Disabled, QPalette::LinkVisited, disabled_text);
	palette.setColor(QPalette::Disabled, QPalette::Highlight, QColor(QStringLiteral("#e5e9ed")));
	palette.setColor(QPalette::Disabled, QPalette::HighlightedText, disabled_text);
	palette.setColor(QPalette::Disabled, QPalette::PlaceholderText, QColor(QStringLiteral("#b3bdc6")));
	palette.setColor(QPalette::Disabled, QPalette::Light, QColor(QStringLiteral("#f8f9fa")));
	palette.setColor(QPalette::Disabled, QPalette::Midlight, QColor(QStringLiteral("#eef1f3")));
	palette.setColor(QPalette::Disabled, QPalette::Mid, QColor(QStringLiteral("#d6dce1")));
	palette.setColor(QPalette::Disabled, QPalette::Dark, QColor(QStringLiteral("#aeb8c1")));
	palette.setColor(QPalette::Disabled, QPalette::Shadow, QColor(QStringLiteral("#9ba8b4")));
	return palette;
}

QString appstyle::stylesheet()
{
	return QStringLiteral(R"(
QWidget {
	color: #243447;
	font-family: "Microsoft YaHei UI", "Microsoft YaHei", sans-serif;
	font-size: 14px;
}

QDialog,
QMessageBox {
	background: #f3f6f9;
	color: #243447;
}

QWidget#MainWidget,
QStackedWidget#pageStack,
QStackedWidget#pageStack > QWidget {
	background: #f3f6f9;
}

QFrame#topBar {
	background: #ffffff;
	border-bottom: 1px solid #dce3ea;
}

QLabel#applicationTitleLabel {
	color: #17324d;
	font-size: 18px;
	font-weight: 600;
}

QLabel#pageTitleLabel {
	color: #526579;
	font-size: 14px;
}

QLabel[pageHeading="true"] {
	color: #17324d;
	font-size: 22px;
	font-weight: 600;
}

QLabel[pageDescription="true"] {
	color: #6d7f90;
}

QFrame[statCard="true"] {
	border: 1px solid #d8e0e7;
	border-radius: 8px;
	background: #ffffff;
}

QLabel[statValue="true"] {
	color: #17324d;
	font-size: 26px;
	font-weight: 600;
}

QProgressBar {
	min-height: 18px;
	border: 0;
	border-radius: 5px;
	background: #e6edf3;
	color: #315066;
	text-align: center;
}

QProgressBar::chunk {
	border-radius: 5px;
	background: #557f9f;
}

QToolButton#sidebarToggleButton {
	min-width: 34px;
	min-height: 34px;
	border: 0;
	border-radius: 6px;
	background: transparent;
	color: #35546f;
	font-size: 18px;
}

QToolButton#sidebarToggleButton:hover {
	background: #e9f0f6;
}

QToolButton#sidebarToggleButton:focus {
	border: 2px solid #315f86;
}

QToolButton[infoButton="true"] {
	padding: 0;
	border: 1px solid #7890a4;
	border-radius: 10px;
	background: #ffffff;
	color: #46657f;
	font-size: 12px;
}

QToolButton[infoButton="true"]:hover,
QToolButton[infoButton="true"]:focus {
	border-color: #315f86;
	background: #e9f1f7;
	color: #17324d;
}

QFrame#sidebar {
	background: #18324a;
	border: 0;
}

QFrame#sidebar QPushButton {
	min-height: 40px;
	padding: 0 12px;
	border: 0;
	border-radius: 6px;
	background: transparent;
	color: #dbe7f0;
	text-align: left;
}

QFrame#sidebar QPushButton:hover {
	background: #274861;
	color: #ffffff;
}

QFrame#sidebar QPushButton:focus {
	border: 2px solid #8fb2cc;
}

QFrame#sidebar QPushButton:checked {
	background: #e8f0f6;
	color: #17324d;
	font-weight: 600;
}

QFrame#sidebar[collapsed="true"] QPushButton {
	padding: 0;
	text-align: center;
}

QPushButton {
	min-height: 36px;
	padding: 0 16px;
	border: 1px solid #bdc9d4;
	border-radius: 6px;
	background: #ffffff;
	color: #29455f;
}

QPushButton:hover {
	border-color: #6f8da6;
	background: #f7fafc;
}

QPushButton:pressed {
	background: #e8eff5;
}

QPushButton:focus,
QToolButton:focus {
	border: 2px solid #315f86;
}

QPushButton:disabled {
	border-color: #dfe5ea;
	background: #f2f4f6;
	color: #9ba8b4;
}

QPushButton[primaryButton="true"] {
	border-color: #315f86;
	background: #315f86;
	color: #ffffff;
}

QPushButton[primaryButton="true"]:hover {
	border-color: #274f70;
	background: #274f70;
}

QPushButton[dangerButton="true"] {
	border-color: #b94343;
	background: #b94343;
	color: #ffffff;
}

QPushButton[dangerButton="true"]:hover,
QPushButton[dangerButton="true"]:focus {
	border-color: #983535;
	background: #983535;
}

QPushButton[primaryButton="true"]:disabled,
QPushButton[dangerButton="true"]:disabled {
	border-color: #dfe5ea;
	background: #f2f4f6;
	color: #9ba8b4;
}

QFrame[detailPanel="true"] {
	border: 1px solid #d8e0e7;
	border-radius: 8px;
	background: #ffffff;
}

QFrame[resourcePanel="true"] {
	border: 1px solid #d8e0e7;
	border-radius: 8px;
	background: #ffffff;
}

QFrame[advancedCard="true"] {
	border: 1px solid #d8e0e7;
	border-radius: 8px;
	background: #ffffff;
}

QLabel[phaseBadge="true"] {
	border: 1px solid #9dafbe;
	border-radius: 12px;
	background: #edf2f6;
	color: #526579;
	font-size: 12px;
}

QLabel[detailHeading="true"] {
	color: #17324d;
	font-size: 18px;
	font-weight: 600;
}

QLabel[detailSubheading="true"] {
	color: #35546f;
	font-weight: 600;
}

QTableView#bedTableView {
	border: 1px solid #d8e0e7;
	border-radius: 6px;
	gridline-color: #d8e0e7;
}

QLineEdit,
QSpinBox,
QComboBox {
	min-height: 36px;
	padding: 0 10px;
	border: 1px solid #c5d0da;
	border-radius: 6px;
	background: #ffffff;
	selection-background-color: #315f86;
}

QLineEdit:focus,
QSpinBox:focus,
QComboBox:focus {
	border: 1px solid #315f86;
}

QLineEdit:disabled,
QSpinBox:disabled,
QComboBox:disabled {
	border-color: #dfe5ea;
	background: #f2f4f6;
	color: #9ba8b4;
}

QRadioButton,
QCheckBox {
	color: #243447;
}

QRadioButton:disabled,
QCheckBox:disabled,
QLabel:disabled,
QGroupBox:disabled {
	color: #9ba8b4;
}

QWidget:disabled {
	color: #9ba8b4;
}

QLineEdit[inputError="true"],
QSpinBox[inputError="true"],
QComboBox[inputError="true"] {
	border: 1px solid #b94343;
	background: #fff7f7;
}

QLabel[fieldError="true"] {
	color: #a23333;
	font-size: 12px;
}

QDialog#SampleDataDialog QGroupBox {
	margin-top: 10px;
	padding-top: 10px;
}

QDialog#SampleDataDialog QFrame#previewFrame {
	background: #f8fbfd;
}

QTableView,
QTreeView,
QListView {
	border: 1px solid #d8e0e7;
	border-radius: 6px;
	background: #ffffff;
	alternate-background-color: #f7f9fb;
	gridline-color: #e7ecf0;
	selection-background-color: #dceaf5;
	selection-color: #17324d;
}

QComboBox QAbstractItemView {
	border: 1px solid #bdc9d4;
	background: #ffffff;
	color: #243447;
	selection-background-color: #dceaf5;
	selection-color: #17324d;
}

QTableView:disabled,
QTreeView:disabled,
QListView:disabled {
	border-color: #e1e6eb;
	background: #f2f4f6;
	alternate-background-color: #eef1f4;
	color: #9ba8b4;
	selection-background-color: #e1e6eb;
	selection-color: #8795a2;
}

QHeaderView::section {
	min-height: 38px;
	padding: 0 10px;
	border: 0;
	border-bottom: 1px solid #d8e0e7;
	background: #edf2f6;
	color: #425b70;
	font-weight: 600;
}

QGroupBox {
	margin-top: 12px;
	padding-top: 14px;
	border: 1px solid #d8e0e7;
	border-radius: 8px;
	background: #ffffff;
	font-weight: 600;
}

QGroupBox::title {
	subcontrol-origin: margin;
	left: 12px;
	padding: 0 4px;
}

QTabWidget::pane {
	border: 1px solid #d8e0e7;
	border-radius: 8px;
	background: #f8fafb;
}

QTabBar::tab {
	min-width: 110px;
	min-height: 38px;
	padding: 0 14px;
	border: 1px solid transparent;
	border-bottom: 0;
	background: transparent;
	color: #526579;
}

QTabBar::tab:selected {
	border-color: #d8e0e7;
	border-radius: 6px 6px 0 0;
	background: #ffffff;
	color: #17324d;
	font-weight: 600;
}

QTabBar::tab:focus {
	border-color: #315f86;
}

QTabBar::tab:disabled {
	color: #9ba8b4;
	background: #f2f4f6;
}

QMenu {
	padding: 6px;
	border: 1px solid #bdc9d4;
	border-radius: 6px;
	background: #ffffff;
	color: #243447;
}

QMenu::item {
	min-width: 150px;
	padding: 8px 22px 8px 12px;
	border-radius: 4px;
	background: transparent;
}

QMenu::item:selected {
	background: #dceaf5;
	color: #17324d;
}

QMenu::item:disabled {
	color: #9ba8b4;
}

QMenu::separator {
	height: 1px;
	margin: 5px 8px;
	background: #d8e0e7;
}

QMessageBox QLabel {
	color: #243447;
}

QMessageBox QPushButton {
	min-width: 88px;
}

QToolTip {
	padding: 6px 8px;
	border: 1px solid #9dafbe;
	border-radius: 4px;
	background: #ffffff;
	color: #243447;
}

QFrame#successToast {
	border: 1px solid #8db49d;
	border-radius: 8px;
	background: #eef8f1;
}

QLabel#successToastIcon {
	color: #277349;
	font-size: 16px;
	font-weight: 700;
}

QLabel#successToastLabel {
	color: #28543a;
}

QScrollBar:vertical {
	width: 10px;
	margin: 0;
	background: transparent;
}

QScrollBar::handle:vertical {
	min-height: 28px;
	border-radius: 5px;
	background: #bac7d2;
}

QScrollBar::add-line:vertical,
QScrollBar::sub-line:vertical {
	height: 0;
}

QScrollBar:horizontal {
	height: 10px;
	margin: 0;
	background: transparent;
}

QScrollBar::handle:horizontal {
	min-width: 28px;
	border-radius: 5px;
	background: #bac7d2;
}

QScrollBar::add-line:horizontal,
QScrollBar::sub-line:horizontal {
	width: 0;
}

QSplitter::handle {
	background: transparent;
}

QSplitter::handle:horizontal {
	width: 8px;
}

QSplitter::handle:horizontal:hover {
	background: #e2e9ef;
}
)");
}
