#ifndef ADVANCEDADJUSTMENTPAGE_H
#define ADVANCEDADJUSTMENTPAGE_H

#include <QWidget>

class QShowEvent;

QT_BEGIN_NAMESPACE
namespace Ui {
class AdvancedAdjustmentPage;
}
QT_END_NAMESPACE

//高级调整中心。提供批量住宿、宿舍整体调整与批量清退向导入口。
class AdvancedAdjustmentPage : public QWidget
{
	Q_OBJECT

public:
	explicit AdvancedAdjustmentPage(QWidget* parent = nullptr);
	~AdvancedAdjustmentPage();

signals:
	void student_open_requested(int student_id);
	void dorm_open_requested(int building_id, int dorm_id);

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
