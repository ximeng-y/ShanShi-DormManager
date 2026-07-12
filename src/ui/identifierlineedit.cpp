#include "identifierlineedit.h"

#include <QIntValidator>

identifierlineedit::identifierlineedit(QWidget* parent)
	: QLineEdit(parent)
	, validator(new QIntValidator(minimum_value, maximum_value, this))
{
	setValidator(validator);
	setInputMethodHints(Qt::ImhDigitsOnly);
	connect(this, &QLineEdit::textChanged, this, [this]() {
		emit valueChanged(value());
	});
}

int identifierlineedit::minimum() const
{
	return minimum_value;
}

int identifierlineedit::maximum() const
{
	return maximum_value;
}

int identifierlineedit::value() const
{
	bool parsed = false;
	const int current_value = text().toInt(&parsed);
	return parsed && current_value >= minimum_value && current_value <= maximum_value
		? current_value
		: 0;
}

bool identifierlineedit::has_valid_value() const
{
	return !text().isEmpty() && hasAcceptableInput() && value() != 0;
}

void identifierlineedit::setMinimum(int minimum)
{
	minimum_value = minimum;
	if (maximum_value < minimum_value) {
		maximum_value = minimum_value;
	}
	update_validator();
}

void identifierlineedit::setMaximum(int maximum)
{
	maximum_value = maximum;
	if (minimum_value > maximum_value) {
		minimum_value = maximum_value;
	}
	update_validator();
}

void identifierlineedit::setValue(int value)
{
	if (value == 0) {
		clear();
		return;
	}
	setText(QString::number(value));
}

void identifierlineedit::update_validator()
{
	validator->setRange(minimum_value, maximum_value);
	if (!text().isEmpty() && !hasAcceptableInput()) {
		clear();
	}
}
