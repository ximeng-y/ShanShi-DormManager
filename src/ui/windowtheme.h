#ifndef WINDOWTHEME_H
#define WINDOWTHEME_H

#include <QObject>

class QApplication;
class QEvent;
class QWidget;

//顶层窗口主题适配器。确保 Windows 深色模式下仍使用浅色标题栏。
class windowtheme : public QObject
{
public:
	static void install(QApplication& application);//安装全局顶层窗口事件过滤器

protected:
	bool eventFilter(QObject* watched, QEvent* event) override;

private:
	explicit windowtheme(QObject* parent = nullptr);
	static void apply_light_title_bar(QWidget* widget);//向已创建原生窗口应用浅色标题栏
};

#endif // WINDOWTHEME_H
