#ifndef SCHOOLSNAPSHOT_H
#define SCHOOLSNAPSHOT_H

#include <QDateTime>
#include <QString>
#include <QVector>

struct building_snapshot
{
	int id = 0;
	int max_floor = 0;
	int gender = 0;

	bool operator==(const building_snapshot& other) const
	{
		return id == other.id && max_floor == other.max_floor && gender == other.gender;
	}
};

struct dorm_snapshot
{
	int building_id = 0;
	int dorm_id = 0;
	int max_beds = 0;
	int gender_lock = 0;
	QVector<int> beds;

	bool operator==(const dorm_snapshot& other) const
	{
		return building_id == other.building_id && dorm_id == other.dorm_id
			&& max_beds == other.max_beds && gender_lock == other.gender_lock && beds == other.beds;
	}
};

struct student_snapshot
{
	int id = 0;
	QString name;
	int gender = 0;
	int grade = 0;
	int class_number = 0;
	int building_id = 0;
	int dorm_id = 0;
	int floor = 0;
	int bed_id = 0;

	bool operator==(const student_snapshot& other) const
	{
		return id == other.id && name == other.name && gender == other.gender
			&& grade == other.grade && class_number == other.class_number
			&& building_id == other.building_id && dorm_id == other.dorm_id
			&& floor == other.floor && bed_id == other.bed_id;
	}
};

//学校完整持久化快照。只保存领域事实，不保存统计、筛选和批量预览等派生状态。
struct schoolsnapshot
{
	static constexpr int current_version = 1;
	QString format = QStringLiteral("DormManagerData");
	int version = current_version;
	QDateTime saved_at;
	QVector<building_snapshot> buildings;
	QVector<dorm_snapshot> dorms;
	QVector<student_snapshot> students;

	bool data_equals(const schoolsnapshot& other) const;//比较领域数据，忽略保存时间
};

#endif // SCHOOLSNAPSHOT_H
