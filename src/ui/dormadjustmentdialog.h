#ifndef DORMADJUSTMENTDIALOG_H
#define DORMADJUSTMENTDIALOG_H

#include <QDialog>
#include "core/school.h"
#include "bedpreviewmodel.h"

QT_BEGIN_NAMESPACE
namespace Ui { class DormAdjustmentDialog; }
QT_END_NAMESPACE

class QStandardItemModel;
class QShowEvent;

class DormAdjustmentDialog : public QDialog
{
	Q_OBJECT
public:
	explicit DormAdjustmentDialog(QWidget* parent = nullptr);
	~DormAdjustmentDialog();

signals:
	void student_navigation_requested(int student_id);
	void dorm_navigation_requested(int building_id, int dorm_id);

protected:
	void showEvent(QShowEvent* event) override;

private:
	void refresh_buildings();
	void refresh_dorms(bool first);
	void filter_dorm_selector(bool first, const QString& search_text, bool select_first = false);//按宿舍号与性别文字实时筛选
	int selected_dorm_id(bool first) const;//仅返回由下拉结果明确选中的宿舍号
	void refresh_current_views();
	void refresh_modes();
	void invalidate_preview();
	void generate_preview();
	void show_preview();
	void apply_preview();
	void open_student_from_view(const QModelIndex& index);
	QVector<bedpreviewentry> entries_for_state(const dorm_preview_state& state, const dorm_preview_state* other, bool after) const;
	dorm_adjustment_mode selected_mode() const;

	Ui::DormAdjustmentDialog* ui;
	bedpreviewmodel* current_a_model;
	bedpreviewmodel* current_b_model;
	bedpreviewmodel* before_a_model;
	bedpreviewmodel* before_b_model;
	bedpreviewmodel* after_a_model;
	bedpreviewmodel* after_b_model;
	QStandardItemModel* change_model;
	dorm_adjustment_preview current_preview;
	bool fitted_to_screen = false;
};

#endif // DORMADJUSTMENTDIALOG_H
