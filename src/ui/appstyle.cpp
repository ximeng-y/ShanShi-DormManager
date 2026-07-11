#include "appstyle.h"

QString appstyle::stylesheet()
{
	return QStringLiteral(R"(
QWidget {
	color: #243447;
	font-family: "Microsoft YaHei UI", "Microsoft YaHei", sans-serif;
	font-size: 14px;
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

QFrame#sidebar QPushButton:checked {
	background: #e8f0f6;
	color: #17324d;
	font-weight: 600;
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

QPushButton:disabled {
	border-color: #dfe5ea;
	background: #f2f4f6;
	color: #9ba8b4;
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

QToolTip {
	padding: 6px 8px;
	border: 1px solid #9dafbe;
	border-radius: 4px;
	background: #ffffff;
	color: #243447;
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
)");
}
