#ifndef ACCOMMODATIONPAGE_H
#define ACCOMMODATIONPAGE_H

#include <QWidget>

QT_BEGIN_NAMESPACE
namespace Ui {
class AccommodationPage;
}
QT_END_NAMESPACE

//住宿安排页面。按任务组织入住、退宿、调宿换床与学生互换流程。
class AccommodationPage : public QWidget
{
	Q_OBJECT

public:
	explicit AccommodationPage(QWidget* parent = nullptr);
	~AccommodationPage();

private:
	void update_assign_controls();//按安置方式启用目标宿舍与床位输入
	void update_assign_preview();//刷新入住学生与目标位置预览
	void submit_assignment();//提交学生入住业务
	void show_assignment_error(int result);//解释入住返回码
	void update_remove_preview();//刷新退宿学生与当前位置预览
	void submit_remove();//提交保留学籍的退宿业务
	void update_move_preview();//刷新调宿学生、原位置与目标位置预览
	void submit_move();//提交调宿或同宿舍换床业务
	void show_move_error(int result);//解释调宿返回码与恢复失败

	Ui::AccommodationPage* ui;
};

#endif // ACCOMMODATIONPAGE_H
