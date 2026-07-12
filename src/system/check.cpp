#include "check.h"
#include <QString>

bool check::is_valid_student_id(int id)//检查学号是否符合YYCCSSSS基本结构
{
	if(id < 10000000 || id > 99999999)
		return false;
	return is_valid_class_num((id / 10000) % 100) && is_valid_student_sequence(id % 10000);
}

bool check::is_valid_student_sequence(int sequence)//检查同年级学生序号是否合法(1~9999)
{
	return sequence >= 1 && sequence <= 9999;
}

int check::make_student_id(int grade, int class_num, int sequence)//按YYCCSSSS生成学号
{
	if (!is_valid_grade(grade) || !is_valid_class_num(class_num) || !is_valid_student_sequence(sequence))
		return 0;
	return (grade % 100) * 1000000 + class_num * 10000 + sequence;
}

int check::student_id_year_suffix(int student_id)//解析学号年级后两位
{
	if (!is_valid_student_id(student_id))
		return -1;
	return student_id / 1000000;
}

int check::student_id_class_num(int student_id)//解析学号班级号
{
	if (!is_valid_student_id(student_id))
		return -1;
	return (student_id / 10000) % 100;
}

int check::student_id_sequence(int student_id)//解析学号同年级序号
{
	if (!is_valid_student_id(student_id))
		return -1;
	return student_id % 10000;
}

bool check::is_student_id_consistent(int student_id, int grade, int class_num)//检查学号与完整年级、班级是否一致
{
	if (!is_valid_grade(grade) || !is_valid_class_num(class_num) || !is_valid_student_id(student_id))
		return false;
	return student_id_year_suffix(student_id) == grade % 100 && student_id_class_num(student_id) == class_num;
}

bool check::is_valid_class_num(int class_num)//检查班级号是否合法(1~99)
{
	if (class_num < 1 || class_num > 99)
		return false;
	return true;
}

bool check::is_valid_grade(int grade)//检查年级是否合法(2010~2099)
{
	if (grade < 2010 || grade > 2099)
		return false;
	return true;
}

bool check::is_valid_dorm_id(int dorm_id)//检查宿舍号是否合法(101~9999 且末两位非00)
{
	if (dorm_id < 101 || dorm_id > 9999)
		return false;
	return is_valid_floor(dorm_id / 100, 99) && is_valid_dorm_room_num(dorm_id % 100);
}

bool check::is_valid_dorm_room_num(int room_num)//检查宿舍房间号是否合法(1~99)
{
	return room_num >= 1 && room_num <= 99;
}

int check::make_dorm_id(int floor, int room_num)//按楼层和房间号生成宿舍号
{
	if (!is_valid_floor(floor, 99) || !is_valid_dorm_room_num(room_num))
		return 0;
	return floor * 100 + room_num;
}

int check::dorm_id_floor(int dorm_id)//解析宿舍号楼层
{
	return is_valid_dorm_id(dorm_id) ? dorm_id / 100 : -1;
}

int check::dorm_id_room_num(int dorm_id)//解析宿舍号后两位房间号
{
	return is_valid_dorm_id(dorm_id) ? dorm_id % 100 : -1;
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
	return is_valid_dorm_id(dorm_id) && is_valid_floor(dorm_id_floor(dorm_id), max_floor);
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
