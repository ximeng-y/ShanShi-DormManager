#ifndef INFOBUTTON_H
#define INFOBUTTON_H

#include <QToolButton>

//规则说明按钮。以统一的圆形 i 图标展示简短提示，不承载业务操作。
class infobutton : public QToolButton
{
public:
	explicit infobutton(QWidget* parent = nullptr);
	explicit infobutton(const QString& information, QWidget* parent = nullptr);

	void set_information(const QString& information);//设置提示内容与辅助说明
};

#endif // INFOBUTTON_H
