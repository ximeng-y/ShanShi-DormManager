#ifndef ADVANCEDADJUSTMENTPAGE_H
#define ADVANCEDADJUSTMENTPAGE_H

#include <QWidget>

QT_BEGIN_NAMESPACE
namespace Ui {
class AdvancedAdjustmentPage;
}
QT_END_NAMESPACE

//高级调整说明页面。只预告第二阶段能力，不提供尚未接入的操作按钮。
class AdvancedAdjustmentPage : public QWidget
{
	Q_OBJECT

public:
	explicit AdvancedAdjustmentPage(QWidget* parent = nullptr);
	~AdvancedAdjustmentPage();

private:
	Ui::AdvancedAdjustmentPage* ui;
};

#endif // ADVANCEDADJUSTMENTPAGE_H
