#ifndef BEDPREVIEWMODEL_H
#define BEDPREVIEWMODEL_H

#include <QAbstractTableModel>
#include <QString>
#include <QVector>

enum class bedpreviewstate
{
	empty,
	normal,
	move_out,
	move_in,
	evicted
};

struct bedpreviewentry
{
	int bed_id = 0;
	int student_id = 0;
	QString name;
	bedpreviewstate state = bedpreviewstate::empty;
};

class bedpreviewmodel : public QAbstractTableModel
{
	Q_OBJECT

public:
	enum role { student_id_role = Qt::UserRole + 1, bed_id_role, state_role };
	explicit bedpreviewmodel(QObject* parent = nullptr);
	int rowCount(const QModelIndex& parent = QModelIndex()) const override;
	int columnCount(const QModelIndex& parent = QModelIndex()) const override;
	QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
	void set_entries(const QVector<bedpreviewentry>& entries);

private:
	QVector<bedpreviewentry> items;
};

#endif // BEDPREVIEWMODEL_H
