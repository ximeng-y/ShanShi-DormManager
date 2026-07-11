#ifndef EDITBUILDINGDIALOG_H
#define EDITBUILDINGDIALOG_H

#include <QDialog>

QT_BEGIN_NAMESPACE
namespace Ui {
class EditBuildingDialog;
}
QT_END_NAMESPACE

//宿舍楼属性编辑弹窗。按顺序修改性别与楼层，失败时恢复已完成的前置修改。
class EditBuildingDialog : public QDialog
{
	Q_OBJECT

public:
	explicit EditBuildingDialog(int building_id, QWidget* parent = nullptr);
	~EditBuildingDialog();

private:
	void attempt_save();//保存楼栋属性并处理失败恢复

	Ui::EditBuildingDialog* ui;
	int target_building_id = 0;
	int original_gender = 0;
	int original_max_floor = 0;
};

#endif // EDITBUILDINGDIALOG_H
