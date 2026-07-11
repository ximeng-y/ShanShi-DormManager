#ifndef ADDBUILDINGDIALOG_H
#define ADDBUILDINGDIALOG_H

#include <QDialog>

QT_BEGIN_NAMESPACE
namespace Ui {
class AddBuildingDialog;
}
QT_END_NAMESPACE

//新增宿舍楼弹窗。负责收集楼号、适用性别与最大楼层。
class AddBuildingDialog : public QDialog
{
	Q_OBJECT

public:
	explicit AddBuildingDialog(QWidget* parent = nullptr);
	~AddBuildingDialog();
	int added_building_id() const;//返回成功添加的楼号，未成功时为0

private:
	void attempt_add();//校验输入并调用 school 新增楼栋
	void clear_validation();//清除字段错误状态

	Ui::AddBuildingDialog* ui;
	int added_id = 0;
};

#endif // ADDBUILDINGDIALOG_H
