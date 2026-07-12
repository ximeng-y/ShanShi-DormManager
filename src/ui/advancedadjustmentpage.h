#ifndef ADVANCEDADJUSTMENTPAGE_H
#define ADVANCEDADJUSTMENTPAGE_H

#include <QWidget>

class QShowEvent;

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

protected:
	void showEvent(QShowEvent* event) override;

private:
	void refresh_summary();
	void open_batch_assignment();
	void open_dorm_adjustment();
	void open_batch_clear();
	Ui::AdvancedAdjustmentPage* ui;
};

#endif // ADVANCEDADJUSTMENTPAGE_H
