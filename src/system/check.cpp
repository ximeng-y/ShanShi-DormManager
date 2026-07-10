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

bool check::is_valid_dorm_id(int dorm_id)//检查宿舍号是否合法(101~9999 且末两位非00)
{
	//编码规则: 末两位=房间号(01~99), 百位及以上=楼层(1~99). floor=dorm_id/100.
	//3位(101~999)对应1~9楼, 4位(1001~9999)对应10~99楼. 末两位00(房间号缺失)非法.
	if (dorm_id < 101 || dorm_id > 9999)
		return false;
	if (dorm_id % 100 == 0)//末两位为00, 房间号缺失
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

bool check::is_valid_dorm_floor(int dorm_id, int max_floor)//检查宿舍号派生楼层是否在楼最大楼层内
{
	//floor 从 dorm_id 派生(dorm_id/100), 复用 is_valid_floor 校验 ∈[1,max_floor].
	//纯数值边界，不依赖聚合，与is_valid_bed_id(bed_id,max_num)同构；楼级校验由school::add_dorm调用。
	int floor = dorm_id / 100;
	return is_valid_floor(floor, max_floor);
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

bool check::is_valid_building_gender(int gender)//检查宿舍楼适用性别是否合法(1=男, 2=女, 3=男女混宿)
{
	if (gender < 1 || gender > 3)
		return false;
	return true;
}
