#include "infobutton.h"

#include <QCursor>
#include <QFont>

infobutton::infobutton(QWidget* parent)
	: QToolButton(parent)
{
	setProperty("infoButton", true);
	setText(QStringLiteral("i"));
	setFixedSize(20, 20);
	setAutoRaise(true);
	setCursor(Qt::WhatsThisCursor);
	setFocusPolicy(Qt::StrongFocus);
	setAccessibleName(QStringLiteral("说明"));

	QFont info_font = font();
	info_font.setBold(true);
	setFont(info_font);
}

infobutton::infobutton(const QString& information, QWidget* parent)
	: infobutton(parent)
{
	set_information(information);
}

void infobutton::set_information(const QString& information)
{
	setToolTip(information);
	setAccessibleDescription(information);
}
