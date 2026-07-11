#ifndef BEDTABLEMODEL_H
#define BEDTABLEMODEL_H

#include <QAbstractTableModel>

//床位轻量表格模型。按可见索引读取宿舍床位，避免为大量床位创建独立控件。
class bedtablemodel : public QAbstractTableModel
{
public:
	explicit bedtablemodel(QObject* parent = nullptr);

	int rowCount(const QModelIndex& parent = QModelIndex()) const override;
	int columnCount(const QModelIndex& parent = QModelIndex()) const override;
	QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
	void set_dorm(int building_id, int dorm_id);//切换当前展示的宿舍

private:
	int current_building_id = 0;
	int current_dorm_id = 0;
};

#endif // BEDTABLEMODEL_H
