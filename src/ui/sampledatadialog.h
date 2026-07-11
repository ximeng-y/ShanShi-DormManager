#ifndef SAMPLEDATADIALOG_H
#define SAMPLEDATADIALOG_H

#include "sampledatagenerator.h"

#include <QDialog>

QT_BEGIN_NAMESPACE
namespace Ui {
class SampleDataDialog;
}
QT_END_NAMESPACE

//样例数据参数弹窗。负责收集追加生成参数并展示规模预览。
class SampleDataDialog : public QDialog
{
	Q_OBJECT

public:
	explicit SampleDataDialog(QWidget* parent = nullptr);
	~SampleDataDialog();
	sampledataresult generation_result() const;//返回本次成功生成的汇总结果

private:
	sampledataconfig current_config(bool* seed_valid = nullptr) const;//读取当前控件参数
	void refresh_preview_and_validation();//刷新规模预览、约束提示与提交状态
	void regenerate_seed();//生成新的32位随机种子
	void attempt_generate();//确认后执行追加生成

	Ui::SampleDataDialog* ui;
	sampledataresult generated_result;
};

#endif // SAMPLEDATADIALOG_H
