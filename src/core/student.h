#ifndef STUDENT_H
#define STUDENT_H

#include <QString>

class student
{
public:
	student();//构造函数

	//获取信息
	QString get_name() const;//获取学生姓名
	int get_class_num() const;//获取学生班级
	int get_id() const;//获取学生学号
	int get_dorm_id() const;//获取所在宿舍号
	int get_bed_id() const;//获取所在床位号(自然数，从1开始)
	int get_grade() const;//获取学生年级
	int get_building_id() const;//获取所在宿舍楼号
	int get_floor() const;//获取所在楼层

	//设置信息
	bool set_name(const QString& name);//设置学生姓名
	bool set_class_num(int class_num);//设置学生班级
	bool set_id(int id);//设置学生学号
	bool set_dorm_id(int dorm_id);//设置所在宿舍号
	bool set_bed_id(int bed_id);//设置所在床位号(自然数，从1开始)
	bool set_grade(int grade);//设置学生年级
	bool set_building_id(int building_id);//设置所在宿舍楼号
	bool set_floor(int floor);//设置所在楼层

	void clear_dorm_info();//自动清零：将bed_id、dorm_id、building_id、floor重置为0（未分配状态），供移除操作调用

private:
	QString name;
	int class_num;//班级号(1~99)
	int id;//前2位年级(的后两位) 3~4位班级 5~8位序列号
	int grade;//年级（4位，例如2026）
	int dorm_id;//所在宿舍号
	int bed_id;//所在床位号(自然数，从1开始)
	int building_id;//所在宿舍楼号
	int floor;//所在楼层
};

#endif // STUDENT_H
