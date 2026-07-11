#ifndef STUDENTPAGE_H
#define STUDENTPAGE_H

#include <QWidget>

QT_BEGIN_NAMESPACE
namespace Ui {
class StudentPage;
}
QT_END_NAMESPACE

//学生管理页面。负责学生目录、资料摘要与学生相关业务入口。
class StudentPage : public QWidget
{
	Q_OBJECT

public:
	explicit StudentPage(QWidget* parent = nullptr);
	~StudentPage();

private:
	Ui::StudentPage* ui;
};

#endif // STUDENTPAGE_H
