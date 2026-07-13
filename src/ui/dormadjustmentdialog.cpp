#include "dormadjustmentdialog.h"
#include "./ui_dormadjustmentdialog.h"

#include "studentdetaildialog.h"
#include "uifeedback.h"
#include "core/building.h"
#include "core/dorm.h"
#include "core/student.h"

#include <QComboBox>
#include <QHeaderView>
#include <QClipboard>
#include <QGuiApplication>
#include <QItemSelectionModel>
#include <QLineEdit>
#include <QPushButton>
#include <QRadioButton>
#include <QScreen>
#include <QShowEvent>
#include <QSignalBlocker>
#include <QShortcut>
#include <QStandardItemModel>
#include <QVariant>

#include <algorithm>

namespace {
QString building_selector_gender_text(int gender)
{
	if (gender == 1) return QStringLiteral("男舍");
	if (gender == 2) return QStringLiteral("女舍");
	if (gender == 3) return QStringLiteral("混宿");
	return QStringLiteral("未知");
}

QString dorm_selector_gender_text(int gender)
{
	if (gender == 1) return QStringLiteral("男");
	if (gender == 2) return QStringLiteral("女");
	if (gender == 0) return QStringLiteral("未设置");
	return QStringLiteral("未知");
}
}

DormAdjustmentDialog::DormAdjustmentDialog(QWidget* parent)
	: QDialog(parent), ui(new Ui::DormAdjustmentDialog)
	, current_a_model(new bedpreviewmodel(this)), current_b_model(new bedpreviewmodel(this))
	, before_a_model(new bedpreviewmodel(this)), before_b_model(new bedpreviewmodel(this))
	, after_a_model(new bedpreviewmodel(this)), after_b_model(new bedpreviewmodel(this))
	, change_model(new QStandardItemModel(this))
{
	ui->setupUi(this);
	ui->fullSwapInfo->set_information(ui->fullSwapInfo->toolTip());
	ui->overlapInfo->set_information(ui->overlapInfo->toolTip());
	ui->evictInfo->set_information(ui->evictInfo->toolTip());
	ui->genderSwapInfo->set_information(ui->genderSwapInfo->toolTip());
	ui->currentAView->setModel(current_a_model); ui->currentBView->setModel(current_b_model);
	ui->beforeAView->setModel(before_a_model); ui->beforeBView->setModel(before_b_model);
	ui->afterAView->setModel(after_a_model); ui->afterBView->setModel(after_b_model);
	ui->changeTable->setModel(change_model);
	for (QTableView* view : findChildren<QTableView*>())
	{
		view->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
		view->verticalHeader()->setVisible(false);
		view->setSelectionBehavior(QAbstractItemView::SelectItems);
		view->setEditTriggers(QAbstractItemView::NoEditTriggers);
		QShortcut* copy_shortcut = new QShortcut(QKeySequence::Copy, view);
		connect(copy_shortcut, &QShortcut::activated, view, [view] {
			QModelIndexList indexes = view->selectionModel()->selectedIndexes();
			std::sort(indexes.begin(), indexes.end(), [](const QModelIndex& a, const QModelIndex& b) { return a.row() == b.row() ? a.column() < b.column() : a.row() < b.row(); });
			QString text;
			int previous_row = -1;
			for (const QModelIndex& index : indexes) {
				if (!text.isEmpty()) text += index.row() == previous_row ? QLatin1Char('\t') : QLatin1Char('\n');
				text += index.data().toString();
				previous_row = index.row();
			}
			if (!text.isEmpty()) QGuiApplication::clipboard()->setText(text);
		});
	}
	connect(ui->buildingACombo, &QComboBox::currentIndexChanged, this, [this] { refresh_dorms(true); });
	connect(ui->buildingBCombo, &QComboBox::currentIndexChanged, this, [this] { refresh_dorms(false); });
	connect(ui->dormACombo, &QComboBox::currentIndexChanged, this, [this] { refresh_current_views(); });
	connect(ui->dormBCombo, &QComboBox::currentIndexChanged, this, [this] { refresh_current_views(); });
	for (QComboBox* combo : {ui->dormACombo, ui->dormBCombo})
	{
		combo->setEditable(true);
		combo->setInsertPolicy(QComboBox::NoInsert);
		combo->lineEdit()->setPlaceholderText(QStringLiteral("输入宿舍号或性别"));
		combo->lineEdit()->setClearButtonEnabled(true);
	}
	connect(ui->dormACombo->lineEdit(), &QLineEdit::textEdited, this,
		[this](const QString& text) { filter_dorm_selector(true, text); });
	connect(ui->dormBCombo->lineEdit(), &QLineEdit::textEdited, this,
		[this](const QString& text) { filter_dorm_selector(false, text); });
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
		const building* current_building = school::instance().get_building(id);
		if (current_building == nullptr) continue;
		const QString text = QStringLiteral("%1号楼 · %2")
			.arg(id).arg(building_selector_gender_text(current_building->get_for_gender()));
		ui->buildingACombo->addItem(text, id);
		ui->buildingBCombo->addItem(text, id);
	}
	if (ui->buildingBCombo->count() > 1) ui->buildingBCombo->setCurrentIndex(1);
	refresh_dorms(true); refresh_dorms(false);
}

void DormAdjustmentDialog::refresh_dorms(bool first)
{
	QComboBox* building_combo = first ? ui->buildingACombo : ui->buildingBCombo;
	QComboBox* dorm_combo = first ? ui->dormACombo : ui->dormBCombo;
	QVariantList dorm_ids;
	for (const auto& key : school::instance().get_dorm_keys_of_building(building_combo->currentData().toInt()))
		dorm_ids.append(QVariant(key.second));
	dorm_combo->setProperty("availableDormIds", dorm_ids);
	filter_dorm_selector(first, QString(), true);
}

void DormAdjustmentDialog::filter_dorm_selector(bool first, const QString& search_text, bool select_first)
{
	QComboBox* combo = first ? ui->dormACombo : ui->dormBCombo;
	QComboBox* building_combo = first ? ui->buildingACombo : ui->buildingBCombo;
	const QVariantList dorm_ids = combo->property("availableDormIds").toList();
	const int building_id = building_combo->currentData().toInt();
	const QString trimmed_search = search_text.trimmed();
	int match_count = 0;
	{
		const QSignalBlocker blocker(combo);
		combo->clear();
		for (const QVariant& dorm_value : dorm_ids)
		{
			const int dorm_id = dorm_value.toInt();
			const dorm* current_dorm = school::instance().get_dorm(building_id, dorm_id);
			if (current_dorm == nullptr) continue;
			const QString gender_text = dorm_selector_gender_text(current_dorm->get_for_gender());
			const QString item_text = QStringLiteral("%1室 · %2").arg(dorm_id).arg(gender_text);
			if (!trimmed_search.isEmpty()
				&& !item_text.contains(trimmed_search, Qt::CaseInsensitive))
				continue;
			combo->addItem(item_text, dorm_id);
			++match_count;
		}
		if (!trimmed_search.isEmpty() && match_count == 0)
		{
			combo->addItem(QStringLiteral("没有匹配的宿舍"), 0);
			if (auto* model = qobject_cast<QStandardItemModel*>(combo->model()))
				model->item(0)->setEnabled(false);
		}
		if (select_first && match_count > 0)
		{
			combo->setCurrentIndex(0);
		}
		else
		{
			combo->setCurrentIndex(-1);
			combo->lineEdit()->setText(search_text);
			combo->lineEdit()->setCursorPosition(search_text.size());
		}
	}
	if (trimmed_search.isEmpty()) combo->hidePopup();
	else combo->showPopup();
	refresh_current_views();
}

int DormAdjustmentDialog::selected_dorm_id(bool first) const
{
	const QComboBox* combo = first ? ui->dormACombo : ui->dormBCombo;
	const int index = combo->currentIndex();
	if (index < 0 || combo->currentText() != combo->itemText(index)) return 0;
	return combo->itemData(index).toInt();
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
	const int ba = ui->buildingACombo->currentData().toInt(), da = selected_dorm_id(true);
	const int bb = ui->buildingBCombo->currentData().toInt(), db = selected_dorm_id(false);
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
	const int ba = ui->buildingACombo->currentData().toInt(), da = selected_dorm_id(true);
	const int bb = ui->buildingBCombo->currentData().toInt(), db = selected_dorm_id(false);
	struct mode_item { QRadioButton* radio; QLabel* reason; dorm_adjustment_mode mode; QString available_text; };
	const QList<mode_item> modes = {
		{ui->fullSwapRadio, ui->fullSwapReasonLabel, dorm_adjustment_mode::full_swap, QStringLiteral("两间宿舍的全部住客互换。")},
		{ui->overlapRadio, ui->overlapReasonLabel, dorm_adjustment_mode::overlap_swap, QStringLiteral("对应人数互换，多出的住客留在原宿舍。")},
		{ui->evictRadio, ui->evictReasonLabel, dorm_adjustment_mode::overlap_swap_and_evict, QStringLiteral("对应人数互换，多出的住客变为未入住。")},
		{ui->genderSwapRadio, ui->genderSwapReasonLabel, dorm_adjustment_mode::gender_dorm_swap, QStringLiteral("交换男生宿舍与女生宿舍住客，并同步调整宿舍性别锁。")}
	};
	for (const auto& item : modes)
	{
		const dorm_adjustment_preview test = school::instance().preview_dorm_adjustment(ba, da, bb, db, item.mode);
		item.radio->setEnabled(test.available);
		item.radio->setToolTip(test.available ? QStringLiteral("此方式适用于当前两间宿舍。") : test.unavailable_reason);
		item.reason->setText(test.available ? item.available_text : test.unavailable_reason);
		item.reason->setProperty("fieldError", !test.available);
		item.reason->style()->unpolish(item.reason);
		item.reason->style()->polish(item.reason);
		if (!test.available && item.radio->isChecked()) item.radio->setChecked(false);
	}
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
	current_preview = school::instance().preview_dorm_adjustment(ui->buildingACombo->currentData().toInt(), selected_dorm_id(true), ui->buildingBCombo->currentData().toInt(), selected_dorm_id(false), selected_mode());
	if (!current_preview.issues.isEmpty()) {
		QStringList details;
		for (const accommodation_data_issue& issue : current_preview.issues) details.append(issue.message);
		uifeedback::show_error(this, QStringLiteral("无法生成调整预览"), current_preview.issues.first().message, details.join(QLatin1Char('\n')));
		const accommodation_data_issue& issue = current_preview.issues.first();
		if ((issue.student_id > 0 || issue.building_id > 0)
			&& uifeedback::confirm_action(this, QStringLiteral("前往处理异常"), QStringLiteral("是否关闭当前向导并前往异常记录所在页面？"), QStringLiteral("前往处理"))) {
			reject();
			if (issue.student_id > 0) emit student_navigation_requested(issue.student_id);
			else emit dorm_navigation_requested(issue.building_id, issue.dorm_id);
		}
		return;
	}
	if (!current_preview.available) { uifeedback::show_error(this, QStringLiteral("当前方式不可用"), current_preview.unavailable_reason); return; }
	show_preview();
}

void DormAdjustmentDialog::show_preview()
{
	QVector<bedpreviewentry> before_a_entries = entries_for_state(current_preview.before_a, &current_preview.after_b, false);
	QVector<bedpreviewentry> before_b_entries = entries_for_state(current_preview.before_b, &current_preview.after_a, false);
	for (bedpreviewentry& entry : before_a_entries)
		if (entry.student_id > 0 && !current_preview.after_a.beds.contains(entry.student_id) && !current_preview.after_b.beds.contains(entry.student_id))
			entry.state = bedpreviewstate::evicted;
	for (bedpreviewentry& entry : before_b_entries)
		if (entry.student_id > 0 && !current_preview.after_a.beds.contains(entry.student_id) && !current_preview.after_b.beds.contains(entry.student_id))
			entry.state = bedpreviewstate::evicted;
	before_a_model->set_entries(before_a_entries);
	before_b_model->set_entries(before_b_entries);
	after_a_model->set_entries(entries_for_state(current_preview.after_a, &current_preview.before_b, true));
	after_b_model->set_entries(entries_for_state(current_preview.after_b, &current_preview.before_a, true));
	const auto gender_text = [](int gender) { return gender == 1 ? QStringLiteral("男生宿舍") : gender == 2 ? QStringLiteral("女生宿舍") : QStringLiteral("宿舍性别锁未设置"); };
	ui->beforeALabel->setText(QStringLiteral("宿舍 A：%1号楼 %2室 · %3").arg(current_preview.before_a.building_id).arg(current_preview.before_a.dorm_id).arg(gender_text(current_preview.before_a.gender)));
	ui->beforeBLabel->setText(QStringLiteral("宿舍 B：%1号楼 %2室 · %3").arg(current_preview.before_b.building_id).arg(current_preview.before_b.dorm_id).arg(gender_text(current_preview.before_b.gender)));
	ui->afterALabel->setText(QStringLiteral("宿舍 A：%1号楼 %2室 · %3").arg(current_preview.after_a.building_id).arg(current_preview.after_a.dorm_id).arg(gender_text(current_preview.after_a.gender)));
	ui->afterBLabel->setText(QStringLiteral("宿舍 B：%1号楼 %2室 · %3").arg(current_preview.after_b.building_id).arg(current_preview.after_b.dorm_id).arg(gender_text(current_preview.after_b.gender)));
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
