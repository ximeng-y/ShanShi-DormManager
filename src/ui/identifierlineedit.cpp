#include "identifierlineedit.h"

#include <QValidator>

namespace {
class ascii_identifier_validator : public QValidator
{
public:
	explicit ascii_identifier_validator(QObject* parent = nullptr)
		: QValidator(parent)
	{
	}

	void set_range(int minimum, int maximum)
	{
		minimum_value = minimum;
		maximum_value = maximum;
	}

	State validate(QString& input, int&) const override
	{
		if (input.isEmpty()) {
			return Intermediate;
		}
		if (input.size() > QString::number(maximum_value).size()
			|| (input.size() > 1 && input.startsWith(QLatin1Char('0')))) {
			return Invalid;
		}
		for (QChar character : input) {
			if (character < QLatin1Char('0') || character > QLatin1Char('9')) {
				return Invalid;
			}
		}
		bool parsed = false;
		const qlonglong value = input.toLongLong(&parsed);
		if (!parsed || value > maximum_value) {
			return Invalid;
		}
		return value >= minimum_value ? Acceptable : Intermediate;
	}

private:
	int minimum_value = 0;
	int maximum_value = 99999999;
};
}

identifierlineedit::identifierlineedit(QWidget* parent)
	: QLineEdit(parent)
	, validator(new ascii_identifier_validator(this))
{
	setValidator(validator);
	setInputMethodHints(Qt::ImhDigitsOnly);
	update_validator();
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
	if (text().size() > 1 && text().startsWith(QLatin1Char('0'))) {
		return 0;
	}
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
	if (value == 0 || value < minimum_value || value > maximum_value) {
		clear();
		return;
	}
	setText(QString::number(value));
}

void identifierlineedit::update_validator()
{
	static_cast<ascii_identifier_validator*>(validator)->set_range(minimum_value, maximum_value);
	setMaxLength(QString::number(maximum_value).size());
	if (!text().isEmpty() && !hasAcceptableInput()) {
		clear();
	}
}
