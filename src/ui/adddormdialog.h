#ifndef ADDDORMDIALOG_H
#define ADDDORMDIALOG_H

#include <QDialog>

QT_BEGIN_NAMESPACE
namespace Ui {
class AddDormDialog;
}
QT_END_NAMESPACE

//新增宿舍弹窗。按所选楼栋约束宿舍号、容量与预设性别锁。
class AddDormDialog : public QDialog
{
	Q_OBJECT

public:
	explicit AddDormDialog(int building_id, QWidget* parent = nullptr);
	~AddDormDialog();
	int added_dorm_id() const;//返回成功添加的宿舍号，未成功时为0

private:
	void attempt_add();//校验输入并调用 school 统一扩层新增事务
	void clear_validation();//清除字段错误状态
	void refresh_floor_selection();//刷新楼层提示、最小缺号建议和添加状态
	void refresh_dorm_id_preview();//按楼层与后两位刷新完整宿舍号预览
	void normalize_room_number();//将有效房间号补齐为两位

	Ui::AddDormDialog* ui;
	int target_building_id = 0;
	int original_max_floor = 0;
	int added_id = 0;
};

#endif // ADDDORMDIALOG_H
