#include "bedpreviewmodel.h"

#include <QBrush>
#include <QColor>

bedpreviewmodel::bedpreviewmodel(QObject* parent)
	: QAbstractTableModel(parent)
{
}

int bedpreviewmodel::rowCount(const QModelIndex& parent) const
{
	return parent.isValid() ? 0 : (items.size() + 1) / 2;
}

int bedpreviewmodel::columnCount(const QModelIndex& parent) const
{
	return parent.isValid() ? 0 : 2;
}

QVariant bedpreviewmodel::data(const QModelIndex& index, int role) const
{
	if (!index.isValid()) return {};
	const int position = index.row() * 2 + index.column();
	if (position < 0 || position >= items.size()) return {};
	const bedpreviewentry& entry = items[position];
	if (role == student_id_role) return entry.student_id;
	if (role == bed_id_role) return entry.bed_id;
	if (role == state_role) return static_cast<int>(entry.state);
	if (role == Qt::TextAlignmentRole) return Qt::AlignCenter;
	if (role == Qt::DisplayRole)
	{
		if (entry.student_id <= 0) return QStringLiteral("%1床 · 空").arg(entry.bed_id);
		const QString marker = entry.state == bedpreviewstate::move_out ? QStringLiteral(" →迁出")
			: entry.state == bedpreviewstate::move_in ? QStringLiteral(" ←迁入")
			: entry.state == bedpreviewstate::evicted ? QStringLiteral(" · 离宿") : QString();
		return QStringLiteral("%1床 · %2%3").arg(entry.bed_id).arg(entry.name, marker);
	}
	if (role == Qt::ForegroundRole)
	{
		if (entry.state == bedpreviewstate::empty) return QBrush(QColor(QStringLiteral("#8290a3")));
		if (entry.state == bedpreviewstate::evicted) return QBrush(QColor(QStringLiteral("#d85b65")));
		if (entry.state == bedpreviewstate::move_in) return QBrush(QColor(QStringLiteral("#2f8f6b")));
	}
	return {};
}

void bedpreviewmodel::set_entries(const QVector<bedpreviewentry>& entries)
{
	beginResetModel();
	items = entries;
	endResetModel();
}
