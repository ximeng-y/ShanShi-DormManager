#ifndef IDENTIFIERLINEEDIT_H
#define IDENTIFIERLINEEDIT_H

#include <QLineEdit>

class QIntValidator;

//标识符输入控件。使用真实占位提示并将空输入统一解释为0。
class identifierlineedit : public QLineEdit
{
	Q_OBJECT
	Q_PROPERTY(int minimum READ minimum WRITE setMinimum)
	Q_PROPERTY(int maximum READ maximum WRITE setMaximum)
	Q_PROPERTY(int value READ value WRITE setValue NOTIFY valueChanged)

public:
	explicit identifierlineedit(QWidget* parent = nullptr);
	int minimum() const;//返回允许的最小标识符
	int maximum() const;//返回允许的最大标识符
	int value() const;//返回当前标识符，空值或非法输入返回0
	bool has_valid_value() const;//判断当前内容是否为范围内的完整标识符

public slots:
	void setMinimum(int minimum);//设置允许的最小标识符
	void setMaximum(int maximum);//设置允许的最大标识符
	void setValue(int value);//设置标识符，0清空输入

signals:
	void valueChanged(int value);//输入内容变化后发送当前值，空值或非法输入为0

private:
	void update_validator();//同步数值范围校验器

	int minimum_value = 0;
	int maximum_value = 99999999;
	QIntValidator* validator = nullptr;
};

#endif // IDENTIFIERLINEEDIT_H
