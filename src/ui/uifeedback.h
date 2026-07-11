#ifndef UIFEEBACK_H
#define UIFEEBACK_H

#include <QString>

class QWidget;

//界面反馈统一入口。负责错误弹窗、危险操作确认和非模态成功提示。
class uifeedback
{
public:
	static void show_error(QWidget* parent, const QString& title, const QString& message, const QString& details = QString());//显示普通业务错误
	static void show_critical(QWidget* parent, const QString& title, const QString& message, const QString& details = QString());//显示数据一致性等严重错误
	static void show_information(QWidget* parent, const QString& title, const QString& message);//显示高影响操作完成结果
	static bool confirm_danger(QWidget* parent, const QString& title, const QString& message, const QString& confirm_text = QStringLiteral("确认"));//确认高影响操作
	static void show_success(QWidget* parent, const QString& message);//在窗口右上角显示短暂成功提示
};

#endif // UIFEEBACK_H
