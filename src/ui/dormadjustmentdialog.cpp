#include "dormadjustmentdialog.h"
#include "./ui_dormadjustmentdialog.h"

#include "studentdetaildialog.h"
#include "uifeedback.h"

#include <QHeaderView>
#include <QPushButton>
#include <QRadioButton>
#include <QScreen>
#include <QShowEvent>
#include <QSignalBlocker>
#include <QStandardItemModel>

DormAdjustmentDialog::DormAdjustmentDialog(QWidget* parent)
	: QDialog(parent), ui(new Ui::DormAdjustmentDialog)
	, current_a_model(new bedpreviewmodel(this)), current_b_model(new bedpreviewmodel(this))
	, before_a_model(new bedpreviewmodel(this)), before_b_model(new bedpreviewmodel(this))
	, after_a_model(new bedpreviewmodel(this)), after_b_model(new bedpreviewmodel(this))
	, change_model(new QStandardItemModel(this))
{
	ui->setupUi(this);
	ui->currentAView->setModel(current_a_model); ui->currentBView->setModel(current_b_model);
	ui->beforeAView->setModel(before_a_model); ui->beforeBView->setModel(before_b_model);
	ui->afterAView->setModel(after_a_model); ui->afterBView->setModel(after_b_model);
	ui->changeTable->setModel(change_model);
	for (QTableView* view : findChildren<QTableView*>())
	{
		view->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
		view->verticalHeader()->setVisible(false);
		view->setSelectionBehavior(QAbstractItemView::SelectItems);
	}
	connect(ui->buildingACombo, &QComboBox::currentIndexChanged, this, [this] { refresh_dorms(true); });
	connect(ui->buildingBCombo, &QComboBox::currentIndexChanged, this, [this] { refresh_dorms(false); });
	connect(ui->dormACombo, &QComboBox::currentIndexChanged, this, [this] { refresh_current_views(); });
	connect(ui->dormBCombo, &QComboBox::currentIndexChanged, this, [this] { refresh_current_views(); });
	for (QRadioButton* radio : {ui->fullSwapRadio, ui->overlapRadio, ui->evictRadio, ui->genderSwapRadio})
		connect(radio, &QRadioButton::toggled, this, [this] { invalidate_preview(); });
	connect(ui->previewButton, &QPushButton::clicked, this, &DormAdjustmentDialog::generate_preview);
	connect(ui->backButton, &QPushButton::clicked, this, [this] { ui->stepStack->setCurrentIndex(0); ui->backButton->hide(); ui->applyButton->hide(); ui->previewButton->show(); });
	connect(ui->riskCheckBox, &QCheckBox::toggled, ui->applyButton, &QPushButton::setEnabled);
	connect(ui->applyButton, &QPushButton::clicked, this, &DormAdjustmentDialog::apply_preview);
	connect(ui->cancelButton, &QPushButton::clicked, this, &QDialog::reject);
	for (QTableView* view : {ui->currentAView, ui->currentBView, ui->beforeAView, ui->beforeBView, ui->afterAView, ui->afterBView})
		connect(view, &QTableView::doubleClicked, this, &DormAdjustmentDialog::open_student_from_view);
	refresh_buildings();
}

DormAdjustmentDialog::~DormAdjustmentDialog() { delete ui; }

void DormAdjustmentDialog::showEvent(QShowEvent* event)
{
	QDialog::showEvent(event);
	if (fitted_to_screen) return;
	fitted_to_screen = true;
	if (QScreen* current = screen())
	{
		const QRect available = current->availableGeometry();
		resize(qMin(1100, available.width() - 32), qMin(680, available.height() - 32));
		move(available.center() - rect().center());
	}
}

void DormAdjustmentDialog::refresh_buildings()
{
	const QSignalBlocker block_a(ui->buildingACombo), block_b(ui->buildingBCombo);
	ui->buildingACombo->clear(); ui->buildingBCombo->clear();
	for (int id : school::instance().get_all_building_ids())
	{
		ui->buildingACombo->addItem(QStringLiteral("%1号楼").arg(id), id);
		ui->buildingBCombo->addItem(QStringLiteral("%1号楼").arg(id), id);
	}
	if (ui->buildingBCombo->count() > 1) ui->buildingBCombo->setCurrentIndex(1);
	refresh_dorms(true); refresh_dorms(false);
}

void DormAdjustmentDialog::refresh_dorms(bool first)
{
	QComboBox* building_combo = first ? ui->buildingACombo : ui->buildingBCombo;
	QComboBox* dorm_combo = first ? ui->dormACombo : ui->dormBCombo;
	const QSignalBlocker blocker(dorm_combo);
	dorm_combo->clear();
	for (const auto& key : school::instance().get_dorm_keys_of_building(building_combo->currentData().toInt()))
		dorm_combo->addItem(QStringLiteral("%1室").arg(key.second), key.second);
	refresh_current_views();
}

QVector<bedpreviewentry> DormAdjustmentDialog::entries_for_state(const dorm_preview_state& state, const dorm_preview_state* other, bool after) const
{
	QVector<bedpreviewentry> entries;
	for (int i = 0; i < state.beds.size(); ++i)
	{
		const int id = state.beds[i];
		bedpreviewstate display_state = id > 0 ? bedpreviewstate::normal : bedpreviewstate::empty;
		if (id > 0 && other != nullptr)
		{
			const bool in_other = other->beds.contains(id);
			display_state = after && in_other ? bedpreviewstate::move_in : !after && in_other ? bedpreviewstate::move_out : display_state;
		}
		const student* s = id > 0 ? school::instance().get_student(id) : nullptr;
		entries.append({i + 1, id, s == nullptr ? QString() : s->get_name(), display_state});
	}
	return entries;
}

void DormAdjustmentDialog::refresh_current_views()
{
	const int ba = ui->buildingACombo->currentData().toInt(), da = ui->dormACombo->currentData().toInt();
	const int bb = ui->buildingBCombo->currentData().toInt(), db = ui->dormBCombo->currentData().toInt();
	const dorm* a = school::instance().get_dorm(ba, da); const dorm* b = school::instance().get_dorm(bb, db);
	dorm_preview_state sa{ba, da, a == nullptr ? 0 : a->get_for_gender(), {}};
	dorm_preview_state sb{bb, db, b == nullptr ? 0 : b->get_for_gender(), {}};
	if (a) for (int i = 1; i <= a->get_max_num(); ++i) sa.beds.append(qMax(0, a->get_student_id(i)));
	if (b) for (int i = 1; i <= b->get_max_num(); ++i) sb.beds.append(qMax(0, b->get_student_id(i)));
	current_a_model->set_entries(entries_for_state(sa, nullptr, false));
	current_b_model->set_entries(entries_for_state(sb, nullptr, false));
	refresh_modes(); invalidate_preview();
}

void DormAdjustmentDialog::refresh_modes()
{
	const int ba = ui->buildingACombo->currentData().toInt(), da = ui->dormACombo->currentData().toInt();
	const int bb = ui->buildingBCombo->currentData().toInt(), db = ui->dormBCombo->currentData().toInt();
	const QList<QPair<QRadioButton*, dorm_adjustment_mode>> modes = {{ui->fullSwapRadio,dorm_adjustment_mode::full_swap},{ui->overlapRadio,dorm_adjustment_mode::overlap_swap},{ui->evictRadio,dorm_adjustment_mode::overlap_swap_and_evict},{ui->genderSwapRadio,dorm_adjustment_mode::gender_dorm_swap}};
	QStringList unavailable;
	for (const auto& item : modes)
	{
		const dorm_adjustment_preview test = school::instance().preview_dorm_adjustment(ba, da, bb, db, item.second);
		item.first->setEnabled(test.available);
		item.first->setToolTip(test.available ? QStringLiteral("此方式适用于当前两间宿舍。") : test.unavailable_reason);
		if (!test.available && item.first->isChecked()) item.first->setChecked(false);
		if (!test.available && !test.unavailable_reason.isEmpty()) unavailable.append(item.first->text() + QStringLiteral("：") + test.unavailable_reason);
	}
	ui->modeDescriptionLabel->setText(unavailable.isEmpty() ? QStringLiteral("请选择调整方式，然后生成调整前后预览。") : unavailable.join(QLatin1Char('\n')));
}

void DormAdjustmentDialog::invalidate_preview()
{
	current_preview = {};
	ui->riskCheckBox->setChecked(false);
	ui->previewButton->setEnabled(ui->fullSwapRadio->isChecked() || ui->overlapRadio->isChecked() || ui->evictRadio->isChecked() || ui->genderSwapRadio->isChecked());
}

dorm_adjustment_mode DormAdjustmentDialog::selected_mode() const
{
	if (ui->overlapRadio->isChecked()) return dorm_adjustment_mode::overlap_swap;
	if (ui->evictRadio->isChecked()) return dorm_adjustment_mode::overlap_swap_and_evict;
	if (ui->genderSwapRadio->isChecked()) return dorm_adjustment_mode::gender_dorm_swap;
	return dorm_adjustment_mode::full_swap;
}

void DormAdjustmentDialog::generate_preview()
{
	current_preview = school::instance().preview_dorm_adjustment(ui->buildingACombo->currentData().toInt(), ui->dormACombo->currentData().toInt(), ui->buildingBCombo->currentData().toInt(), ui->dormBCombo->currentData().toInt(), selected_mode());
	if (!current_preview.issues.isEmpty()) { uifeedback::show_error(this, QStringLiteral("无法生成调整预览"), current_preview.issues.first().message); return; }
	if (!current_preview.available) { uifeedback::show_error(this, QStringLiteral("当前方式不可用"), current_preview.unavailable_reason); return; }
	show_preview();
}

void DormAdjustmentDialog::show_preview()
{
	before_a_model->set_entries(entries_for_state(current_preview.before_a, &current_preview.after_b, false));
	before_b_model->set_entries(entries_for_state(current_preview.before_b, &current_preview.after_a, false));
	after_a_model->set_entries(entries_for_state(current_preview.after_a, &current_preview.before_b, true));
	after_b_model->set_entries(entries_for_state(current_preview.after_b, &current_preview.before_a, true));
	change_model->clear(); change_model->setHorizontalHeaderLabels({QStringLiteral("学号"),QStringLiteral("姓名"),QStringLiteral("原位置"),QStringLiteral("新位置"),QStringLiteral("结果")});
	for (const accommodation_change& c : current_preview.changes)
	{
		const student* s = school::instance().get_student(c.student_id);
		const QString old_pos = QStringLiteral("%1-%2-%3床").arg(c.old_building_id).arg(c.old_dorm_id).arg(c.old_bed_id);
		const QString new_pos = c.new_building_id == 0 ? QStringLiteral("未入住") : QStringLiteral("%1-%2-%3床").arg(c.new_building_id).arg(c.new_dorm_id).arg(c.new_bed_id);
		change_model->appendRow({new QStandardItem(QString::number(c.student_id)),new QStandardItem(s ? s->get_name() : QStringLiteral("未知学生")),new QStandardItem(old_pos),new QStandardItem(new_pos),new QStandardItem(c.new_building_id == 0 ? QStringLiteral("离宿") : old_pos == new_pos ? QStringLiteral("位置不变") : QStringLiteral("调整"))});
	}
	ui->stepStack->setCurrentIndex(1); ui->previewButton->hide(); ui->backButton->show(); ui->applyButton->show(); ui->applyButton->setEnabled(false);
}

void DormAdjustmentDialog::apply_preview()
{
	const int result = school::instance().apply_dorm_adjustment(current_preview);
	if (result == 1) { uifeedback::show_information(this, QStringLiteral("宿舍调整完成"), QStringLiteral("两间宿舍及相关学生的住宿位置已同步更新。")); accept(); }
	else if (result == -6) uifeedback::show_critical(this, QStringLiteral("调整恢复不完整"), QStringLiteral("调整失败，且未能完整恢复两间宿舍原状态。请暂停后续操作并核查住宿数据。"));
	else uifeedback::show_error(this, QStringLiteral("无法完成宿舍调整"), QStringLiteral("宿舍或住客状态已经变化，请返回并重新生成预览。"), QStringLiteral("业务返回值：%1").arg(result));
}

void DormAdjustmentDialog::open_student_from_view(const QModelIndex& index)
{
	const int student_id = index.data(bedpreviewmodel::student_id_role).toInt();
	if (student_id > 0) { StudentDetailDialog dialog(student_id, this); dialog.exec(); }
}
