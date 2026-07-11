#include "uifeedback.h"

#include <QAbstractButton>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QTimer>
#include <QWidget>

void uifeedback::show_error(QWidget* parent, const QString& title, const QString& message, const QString& details)
{
	QMessageBox box(QMessageBox::Warning, title, message, QMessageBox::Ok, parent);
	box.button(QMessageBox::Ok)->setText(QStringLiteral("关闭"));
	if (!details.isEmpty()) {
		box.setDetailedText(details);
	}
	box.exec();
}

void uifeedback::show_critical(QWidget* parent, const QString& title, const QString& message, const QString& details)
{
	QMessageBox box(QMessageBox::Critical, title, message, QMessageBox::Ok, parent);
	box.button(QMessageBox::Ok)->setText(QStringLiteral("关闭"));
	if (!details.isEmpty()) {
		box.setDetailedText(details);
	}
	box.exec();
}

bool uifeedback::confirm_danger(QWidget* parent, const QString& title, const QString& message, const QString& confirm_text)
{
	QMessageBox box(QMessageBox::Warning, title, message, QMessageBox::Cancel, parent);
	box.button(QMessageBox::Cancel)->setText(QStringLiteral("取消"));
	QPushButton* confirm_button = box.addButton(confirm_text, QMessageBox::AcceptRole);
	box.setDefaultButton(QMessageBox::Cancel);
	box.exec();
	return box.clickedButton() == confirm_button;
}

void uifeedback::show_success(QWidget* parent, const QString& message)
{
	if (parent == nullptr) {
		return;
	}

	QWidget* host = parent->window();
	auto* toast = new QFrame(host);
	toast->setObjectName(QStringLiteral("successToast"));
	toast->setAttribute(Qt::WA_ShowWithoutActivating);
	toast->setMinimumWidth(280);
	toast->setMaximumWidth(420);

	auto* layout = new QHBoxLayout(toast);
	layout->setContentsMargins(14, 10, 14, 10);
	layout->setSpacing(10);

	auto* status_label = new QLabel(QStringLiteral("✓"), toast);
	status_label->setObjectName(QStringLiteral("successToastIcon"));
	auto* message_label = new QLabel(message, toast);
	message_label->setObjectName(QStringLiteral("successToastLabel"));
	message_label->setWordWrap(true);
	layout->addWidget(status_label, 0, Qt::AlignTop);
	layout->addWidget(message_label, 1);

	toast->adjustSize();
	const int margin = 20;
	toast->move(qMax(margin, host->width() - toast->width() - margin), margin);
	toast->raise();
	toast->show();
	QTimer::singleShot(2600, toast, &QObject::deleteLater);
}
