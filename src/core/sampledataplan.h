#ifndef SAMPLEDATAPLAN_H
#define SAMPLEDATAPLAN_H

#include <QString>
#include <QVector>

struct samplebuildingplan
{
	int id = 0;
	int gender = 0;
	int max_floor = 0;
};

struct sampledormplan
{
	int building_id = 0;
	int dorm_id = 0;
	int max_num = 0;
	int gender_lock = 0;
};

struct samplestudentplan
{
	int id = 0;
	QString name;
	int gender = 0;
	int class_num = 0;
	int grade = 0;
	int building_id = 0;
	int dorm_id = 0;
	int bed_id = 0;
};

struct sampledataplan
{
	QVector<samplebuildingplan> buildings;
	QVector<sampledormplan> dorms;
	QVector<samplestudentplan> students;
};

#endif // SAMPLEDATAPLAN_H
