#ifndef BEDMOVEDIALOG_H
#define BEDMOVEDIALOG_H

#include <QDialog>

QT_BEGIN_NAMESPACE
namespace Ui {
class BedMoveDialog;
}
QT_END_NAMESPACE

//住客跨宿舍调宿弹窗。使用现有楼栋、宿舍和空床目录选择目标位置。
class BedMoveDialog : public QDialog
{
	Q_OBJECT

public:
	explicit BedMoveDialog(int student_id, QWidget* parent = nullptr);
	~BedMoveDialog();
	int target_building_id() const;//返回确认的目标楼号
	int target_dorm_id() const;//返回确认的目标宿舍号
	int target_bed_id() const;//返回确认的目标床位，0表示自动选择

private:
	void refresh_buildings();//刷新接纳当前学生的楼栋目录
	void refresh_dorms();//刷新当前楼栋可用宿舍目录
	void refresh_beds();//刷新当前宿舍空床目录与确认状态
	void attempt_accept();//复核选择后确认

	Ui::BedMoveDialog* ui;
	int current_student_id = 0;
	int confirmed_building_id = 0;
	int confirmed_dorm_id = 0;
	int confirmed_bed_id = 0;
};

#endif // BEDMOVEDIALOG_H
