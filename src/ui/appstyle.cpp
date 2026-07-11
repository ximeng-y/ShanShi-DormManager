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
)");
}
