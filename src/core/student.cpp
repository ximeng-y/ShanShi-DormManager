#include "student.h"
#include <QString>
#include "system/check.h"

student::student()//构造函数
{
	name = "";
	class_num = 0;
	id = 10000000;
	bed_id = 0;
	dorm_id = 0;
	grade = 0;
}

//获取信息
QString student::get_name() const//获取学生姓名
{
	return name;
}
int student::get_class_num() const//获取学生班级
{
	return class_num;
}
int student::get_id() const//获取学生学号
{
	return id;
}
int student::get_grade() const//获取学生年级
{
	return grade;
}
int student::get_dorm_id() const//获取所在宿舍号
{
	return dorm_id;
}
int student::get_bed_id() const//获取所在床位号
{
	return bed_id;
}

//设置信息(使用bool返回是否成功)
bool student::set_name(const QString& name)//设置学生姓名
{
	if (!check::is_valid_student_name(name))
		return false;
	this->name = name;
	return true;
}
bool student::set_class_num(int class_num)//设置学生班级
{
	if (!check::is_valid_class_num(class_num))
		return false;
	this->class_num = class_num;
	return true;
}
bool student::set_id(int id)//设置学生学号
{
	if (!check::is_valid_student_id(id))
		return false;
	this->id = id;
	return true;
}
bool student::set_grade(int grade)//设置学生年级
{
	if (!check::is_valid_grade(grade))
		return false;
	this->grade = grade;
	return true;
}
bool student::set_dorm_id(int dorm_id)//设置所在宿舍号
{
	if (!check::is_valid_dorm_id(dorm_id))
		return false;
	this->dorm_id = dorm_id;
	return true;
}
bool student::set_bed_id(int bed_id)//设置所在床位号
{
	if (bed_id < 1)
		return false;
	this->bed_id = bed_id;
	return true;
}
