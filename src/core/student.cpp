#include "student.h"
#include <QString>
#include "system/check.h"

student::student()//构造函数
{
	name = "";
	class_num = 0;
	id = 10000000;
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