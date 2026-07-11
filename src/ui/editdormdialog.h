#ifndef EDITDORMDIALOG_H
#define EDITDORMDIALOG_H

#include <QDialog>

QT_BEGIN_NAMESPACE
namespace Ui {
class EditDormDialog;
}
QT_END_NAMESPACE

//宿舍属性编辑弹窗。修改容量和性别锁，后置操作失败时恢复原容量。
class EditDormDialog : public QDialog
{
	Q_OBJECT

public:
	explicit EditDormDialog(int building_id, int dorm_id, QWidget* parent = nullptr);
	~EditDormDialog();

private:
	void attempt_save();//保存宿舍容量与性别锁

	Ui::EditDormDialog* ui;
	int target_building_id = 0;
	int target_dorm_id = 0;
	int original_max_num = 0;
	int original_gender = 0;
};

#endif // EDITDORMDIALOG_H
