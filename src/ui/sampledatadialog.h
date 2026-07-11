#ifndef SAMPLEDATADIALOG_H
#define SAMPLEDATADIALOG_H

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

private:
	Ui::SampleDataDialog* ui;
};

#endif // SAMPLEDATADIALOG_H
