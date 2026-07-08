#include "check.h"
#include <QString>

bool check::is_valid_student_id(int id)//检查学号是否合法(10000000~99999999)
{
	if(id < 10000000 || id > 99999999)
		return false;

	return true;
}

bool check::is_valid_class_num(int class_num)//检查班级号是否合法(1~99)
{
	if (class_num < 1 || class_num > 99)
		return false;
	return true;
}

bool check::is_valid_grade(int grade)//检查年级是否合法(2000~2999)
{
	if (grade < 2000 || grade > 2999)
		return false;
	return true;
}

bool check::is_valid_dorm_id(int dorm_id)//检查宿舍号是否合法(1001~9999)
{
	if (dorm_id < 1001 || dorm_id > 9999)
		return false;
	return true;
}

bool check::is_valid_building_id(int building_id)//检查宿舍楼号是否合法(1~99)
{
	if (building_id < 1 || building_id > 99)
		return false;
	return true;
}

bool check::is_valid_bed_id(int bed_id, int max_num)//检查自然数床位号是否合法(1~max_num)
{
	if (bed_id < 1 || bed_id > max_num)
		return false;
	return true;
}

bool check::is_valid_floor(int floor, int max_floor)//检查楼层是否合法(1~max_floor)
{
	if (floor < 1 || floor > max_floor)
		return false;
	return true;
}

bool check::is_valid_student_name(const QString& name)//检查学生姓名是否合法(1~20个字符,禁止error)
{
	if (name.length() < 1 || name.length() > 20)
		return false;
	if (name.contains("error"))
		return false;
	return true;
}

bool check::is_valid_max_floor(int max_floor)//检查最大楼层数是否合法(1~99)
{
	if (max_floor < 1 || max_floor > 99)
		return false;
	return true;
}

bool check::is_valid_gender(int gender)//检查性别是否合法(0=未设置, 1=男, 2=女)
{
	if (gender < 0 || gender > 2)
		return false;
	return true;
}