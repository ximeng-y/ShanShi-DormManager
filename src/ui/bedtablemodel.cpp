#include "bedtablemodel.h"

#include "core/dorm.h"
#include "core/school.h"
#include "core/student.h"

#include <QBrush>
#include <QColor>

bedtablemodel::bedtablemodel(QObject* parent)
	: QAbstractTableModel(parent)
{
}

int bedtablemodel::rowCount(const QModelIndex& parent) const
{
	if (parent.isValid()) {
		return 0;
	}
	const dorm* current_dorm = school::instance().get_dorm(current_building_id, current_dorm_id);
	return current_dorm == nullptr ? 0 : (current_dorm->get_max_num() + 1) / 2;
}

int bedtablemodel::columnCount(const QModelIndex& parent) const
{
	return parent.isValid() ? 0 : 2;
}

QVariant bedtablemodel::data(const QModelIndex& index, int role) const
{
	if (!index.isValid()) {
		return QVariant();
	}
	const dorm* current_dorm = school::instance().get_dorm(current_building_id, current_dorm_id);
	if (current_dorm == nullptr) {
		return QVariant();
	}
	const int bed_id = index.row() * 2 + index.column() + 1;
	if (bed_id > current_dorm->get_max_num()) {
		return QVariant();
	}
	const int student_id = current_dorm->get_student_id(bed_id);
	const bool occupied = student_id > 0;
	if (role == Qt::DisplayRole) {
		if (!occupied) {
			return QStringLiteral("%1号床\n空闲").arg(bed_id);
		}
		const student* occupant = school::instance().get_student(student_id);
		return occupant == nullptr
			? QStringLiteral("%1号床\n记录异常：%2").arg(bed_id).arg(student_id)
			: QStringLiteral("%1号床\n%2 · %3").arg(bed_id).arg(occupant->get_name()).arg(student_id);
	}
	if (role == Qt::TextAlignmentRole) {
		return static_cast<int>(Qt::AlignCenter);
	}
	if (role == Qt::BackgroundRole) {
		return QBrush(occupied ? QColor(QStringLiteral("#eef4f8")) : QColor(QStringLiteral("#f8fafb")));
	}
	if (role == Qt::ForegroundRole) {
		return QBrush(QColor(QStringLiteral("#29455f")));
	}
	if (role == Qt::ToolTipRole) {
		return data(index, Qt::DisplayRole).toString().replace(QLatin1Char('\n'), QStringLiteral("："));
	}
	if (role == Qt::UserRole) {
		return bed_id;
	}
	return QVariant();
}

void bedtablemodel::set_dorm(int building_id, int dorm_id)
{
	beginResetModel();
	current_building_id = building_id;
	current_dorm_id = dorm_id;
	endResetModel();
}
