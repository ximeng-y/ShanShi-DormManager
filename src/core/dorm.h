#ifndef DORM_H
#define DORM_H

#include <QString>
#include <QVector>
#include "student.h"

class dorm
{
public:
	dorm();//构造函数

	//获取信息
	int get_id() const;//获取宿舍号
	int get_max_num() const;//获取最大人数
	int get_current_num() const;//获取实际人数
	int get_building_id() const;//获取所在宿舍楼号
	int get_floor() const;//获取所在楼层
	QString get_student_name(int bed_id) const;//获取宿舍内指定床位学生姓名
	int get_student_id(int bed_id) const;//获取宿舍内指定床位学生学号
	int get_student_class_num(int bed_id) const;//获取宿舍内指定床位学生班级号
	QVector<QString> get_student_name_list() const;//获取宿舍内所有学生的名字表
	QVector<int> get_student_id_list() const;//获取宿舍内所有学生的学号表
	QVector<int> get_student_class_num_list() const;//获取宿舍内所有学生的班级号表(不去重)
	

	//设置信息
	bool set_id(int id);//设置宿舍号
	bool set_max_num(int max_num);//设置最大人数
	bool set_building_id(int building_id);//设置所在宿舍楼号
	bool set_floor(int floor);//设置所在楼层

private:
	int id;//宿舍号
	int max_num;//最大人数
	int building_id;//所在宿舍楼号(1~99)
	int floor;//所在楼层(1~max_floor)
	QVector<student> students;//宿舍内学生
};

#endif // DORM_H
