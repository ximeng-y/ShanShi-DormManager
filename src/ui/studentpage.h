#ifndef STUDENTPAGE_H
#define STUDENTPAGE_H

#include <QWidget>

QT_BEGIN_NAMESPACE
namespace Ui {
class StudentPage;
}
QT_END_NAMESPACE

class QShowEvent;

//学生管理页面。负责学生目录、资料摘要与学生相关业务入口。
class StudentPage : public QWidget
{
	Q_OBJECT

public:
	explicit StudentPage(QWidget* parent = nullptr);
	~StudentPage();
	void refresh_data();//刷新学生目录与筛选结果

protected:
	void showEvent(QShowEvent* event) override;//页面显示时刷新学生目录

private:
	void rebuild_class_filter();//根据当前学生目录刷新班级筛选项
	void apply_filters();//按搜索、班级和入住状态刷新表格

	Ui::StudentPage* ui;
};

#endif // STUDENTPAGE_H
