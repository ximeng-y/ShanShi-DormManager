#include "uifeedback.h"

#include <QAbstractButton>
#include <QEvent>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QStyle>
#include <QTimer>
#include <QWidget>

namespace {
void set_button_role(QPushButton* button, const char* property_name)
{
	if (button == nullptr) {
		return;
	}
	button->setProperty(property_name, true);
	button->style()->unpolish(button);
	button->style()->polish(button);
}

class success_toast : public QFrame
{
public:
	explicit success_toast(QWidget* host)
		: QFrame(host)
		, host_widget(host)
		, message_label(new QLabel(this))
		, hide_timer(new QTimer(this))
	{
		setObjectName(QStringLiteral("successToast"));
		setAttribute(Qt::WA_ShowWithoutActivating);
		setAttribute(Qt::WA_TransparentForMouseEvents);
		setMinimumWidth(280);
		setMaximumWidth(420);

		auto* layout = new QHBoxLayout(this);
		layout->setContentsMargins(14, 10, 14, 10);
		layout->setSpacing(10);
		auto* status_label = new QLabel(QStringLiteral("✓"), this);
		status_label->setObjectName(QStringLiteral("successToastIcon"));
		message_label->setObjectName(QStringLiteral("successToastLabel"));
		message_label->setWordWrap(true);
		layout->addWidget(status_label, 0, Qt::AlignTop);
		layout->addWidget(message_label, 1);

		hide_timer->setSingleShot(true);
		connect(hide_timer, &QTimer::timeout, this, &QWidget::hide);
		host_widget->installEventFilter(this);
		hide();
	}

	void show_message(const QString& message)
	{
		message_label->setText(message);
		adjustSize();
		reposition();
		raise();
		show();
		hide_timer->start(2600);
	}

protected:
	bool eventFilter(QObject* watched, QEvent* event) override
	{
		if (watched == host_widget && event->type() == QEvent::Resize) {
			reposition();
		}
		return QFrame::eventFilter(watched, event);
	}

private:
	void reposition()
	{
		const int margin = 20;
		const int top_bar_offset = 68;
		move(qMax(margin, host_widget->width() - width() - margin), top_bar_offset);
	}

	QWidget* host_widget;
	QLabel* message_label;
	QTimer* hide_timer;
};
}

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

void uifeedback::show_information(QWidget* parent, const QString& title, const QString& message)
{
	QMessageBox box(QMessageBox::Information, title, message, QMessageBox::Ok, parent);
	box.button(QMessageBox::Ok)->setText(QStringLiteral("确定"));
	set_button_role(qobject_cast<QPushButton*>(box.button(QMessageBox::Ok)), "primaryButton");
	box.exec();
}

bool uifeedback::confirm_danger(QWidget* parent, const QString& title, const QString& message, const QString& confirm_text)
{
	QMessageBox box(QMessageBox::Warning, title, message, QMessageBox::Cancel, parent);
	box.button(QMessageBox::Cancel)->setText(QStringLiteral("取消"));
	QPushButton* confirm_button = box.addButton(confirm_text, QMessageBox::AcceptRole);
	set_button_role(confirm_button, "dangerButton");
	box.setDefaultButton(QMessageBox::Cancel);
	box.exec();
	return box.clickedButton() == confirm_button;
}

bool uifeedback::confirm_action(QWidget* parent, const QString& title, const QString& message, const QString& confirm_text)
{
	QMessageBox box(QMessageBox::Question, title, message, QMessageBox::Cancel, parent);
	box.button(QMessageBox::Cancel)->setText(QStringLiteral("取消"));
	QPushButton* confirm_button = box.addButton(confirm_text, QMessageBox::AcceptRole);
	set_button_role(confirm_button, "primaryButton");
	box.setDefaultButton(confirm_button);
	box.exec();
	return box.clickedButton() == confirm_button;
}

void uifeedback::show_success(QWidget* parent, const QString& message)
{
	if (parent == nullptr) {
		return;
	}

	QWidget* host = parent->window();
	QFrame* existing_frame = host->findChild<QFrame*>(QStringLiteral("successToast"), Qt::FindDirectChildrenOnly);
	auto* toast = existing_frame == nullptr
		? new success_toast(host)
		: static_cast<success_toast*>(existing_frame);
	toast->show_message(message);
}
